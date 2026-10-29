#include "nosfix/keysound_dispatch.hpp"

#include <cmath>
#include <limits>

namespace nosfix::audio {
namespace {

void RaiseMaximum(std::atomic<std::uint32_t>& target, std::uint32_t value) noexcept {
    std::uint32_t current = target.load(std::memory_order_relaxed);
    while (value > current &&
           !target.compare_exchange_weak(current, value,
                                         std::memory_order_relaxed,
                                         std::memory_order_relaxed)) {
    }
}

// high > low. The mathematical difference of two int64 values always fits in
// uint64; unsigned subtraction avoids signed overflow for a positive block
// start and an INT64_MIN due sample.
std::uint32_t ClampDifferenceToU32(std::int64_t high, std::int64_t low) noexcept {
    const std::uint64_t difference =
        static_cast<std::uint64_t>(high) - static_cast<std::uint64_t>(low);
    if (difference > std::numeric_limits<std::uint32_t>::max()) {
        return std::numeric_limits<std::uint32_t>::max();
    }
    return static_cast<std::uint32_t>(difference);
}

} // namespace

KeysoundQueue::KeysoundQueue(std::uint32_t capacity) noexcept {
    if (capacity < kMinCapacity || capacity > kMaxCapacity) return;
    if ((capacity & (capacity - 1U)) != 0) return;
    capacity_ = capacity;
    mask_ = capacity - 1U;
    slots_.reset(new (std::nothrow) Slot[capacity]);
    if (!slots_) {
        capacity_ = 0;
        mask_ = 0;
    }
}

KeysoundPushResult KeysoundQueue::Push(const KeysoundRequest& request) noexcept {
    if (!Valid()) return {KeysoundPushStatus::InvalidArgument};
    if (request.sequence == 0 || request.generation == 0 ||
        request.capture_qpc <= 0 || request.due_qpc < 0 ||
        !std::isfinite(request.volume_db) || !std::isfinite(request.pan) ||
        request.pan < -1.0F || request.pan > 1.0F) {
        rejected_invalid_.fetch_add(1, std::memory_order_relaxed);
        return {KeysoundPushStatus::InvalidArgument};
    }

    const std::uint64_t highest = highest_sequence_.load(std::memory_order_relaxed);
    if (request.sequence <= highest) {
        rejected_duplicate_.fetch_add(1, std::memory_order_relaxed);
        return {KeysoundPushStatus::Duplicate};
    }

    const std::uint32_t head = head_.load(std::memory_order_acquire);
    const std::uint32_t tail = tail_.load(std::memory_order_relaxed);
    const std::uint32_t used = tail - head; // unsigned wrap is the ring distance
    if (used >= capacity_) {
        rejected_overflow_.fetch_add(1, std::memory_order_relaxed);
        return {KeysoundPushStatus::Overflow};
    }

    Slot& slot = slots_[tail & mask_];
    slot.request = request;
    slot.dispatched = false;
    // Publish the pending count before the tail. The consumer acquires tail
    // before decrementing pending_, so it can never observe a zero-to-underflow
    // window for this event.
    const std::uint32_t pending =
        pending_.fetch_add(1, std::memory_order_relaxed) + 1U;
    RaiseMaximum(high_watermark_, pending);
    tail_.store(tail + 1U, std::memory_order_release);
    highest_sequence_.store(request.sequence, std::memory_order_relaxed);
    pushed_.fetch_add(1, std::memory_order_relaxed);
    return {KeysoundPushStatus::Accepted};
}

KeysoundDispatchOutcome KeysoundQueue::DispatchDue(
    std::int64_t block_start_sample,
    std::uint32_t block_frames,
    std::uint64_t generation,
    KeysoundSink sink,
    void* sink_context) noexcept {
    KeysoundDispatchOutcome outcome;
    if (!Valid() || sink == nullptr || block_frames == 0) return outcome;
    if (block_start_sample >
        std::numeric_limits<std::int64_t>::max() - static_cast<std::int64_t>(block_frames)) {
        return outcome;
    }
    const std::int64_t block_end =
        block_start_sample + static_cast<std::int64_t>(block_frames);

    const std::uint32_t head = head_.load(std::memory_order_relaxed);
    const std::uint32_t tail = tail_.load(std::memory_order_acquire);
    std::uint32_t count = tail - head;
    if (count > capacity_) count = capacity_;

    for (std::uint32_t i = 0; i < count; ++i) {
        Slot& slot = slots_[(head + i) & mask_];
        if (slot.dispatched) continue;

        if (slot.request.generation != generation) {
            slot.dispatched = true;
            pending_.fetch_sub(1, std::memory_order_relaxed);
            ++outcome.dropped_stale;
            dropped_stale_.fetch_add(1, std::memory_order_relaxed);
            continue;
        }

        QpcToSampleMapper mapper = mapper_;
        if (mapper == nullptr) break; // retried when a mapper is installed
        void* mapper_context = mapper_context_;

        const std::int64_t due_qpc =
            slot.request.due_qpc != 0 ? slot.request.due_qpc : slot.request.capture_qpc;
        std::int64_t due_sample = 0;
        if (!mapper(mapper_context, due_qpc, due_sample)) {
            continue; // timeline not mapped yet; leave queued
        }
        if (due_sample >= block_end) continue; // future block

        KeysoundDispatch dispatch;
        dispatch.sequence = slot.request.sequence;
        dispatch.generation = slot.request.generation;
        dispatch.capture_qpc = slot.request.capture_qpc;
        dispatch.due_device_sample = due_sample;
        dispatch.block_start_sample = block_start_sample;
        if (due_sample < block_start_sample) {
            dispatch.offset_in_block = 0;
            dispatch.late_frames =
                ClampDifferenceToU32(block_start_sample, due_sample);
            ++outcome.late;
            late_dispatches_.fetch_add(1, std::memory_order_relaxed);
            total_late_frames_.fetch_add(static_cast<std::uint64_t>(dispatch.late_frames),
                                         std::memory_order_relaxed);
            RaiseMaximum(max_late_frames_, dispatch.late_frames);
        } else {
            const std::int64_t offset = due_sample - block_start_sample;
            dispatch.offset_in_block =
                offset >= static_cast<std::int64_t>(block_frames)
                    ? block_frames - 1U
                    : static_cast<std::uint32_t>(offset);
            dispatch.late_frames = 0;
        }
        dispatch.cue_id = slot.request.cue_id;
        dispatch.variation = slot.request.variation;
        dispatch.priority = slot.request.priority;
        dispatch.flags = slot.request.flags;
        dispatch.volume_db = slot.request.volume_db;
        dispatch.pan = slot.request.pan;

        slot.dispatched = true;
        pending_.fetch_sub(1, std::memory_order_relaxed);
        ++outcome.dispatched;
        dispatched_.fetch_add(1, std::memory_order_relaxed);
        if (!sink(sink_context, dispatch)) {
            ++outcome.sink_rejected;
            sink_rejected_.fetch_add(1, std::memory_order_relaxed);
        }
    }

    std::uint32_t retire = head;
    while (retire != tail && slots_[retire & mask_].dispatched) {
        slots_[retire & mask_].dispatched = false;
        ++retire;
    }
    head_.store(retire, std::memory_order_release);
    return outcome;
}

std::uint32_t KeysoundQueue::Pending() const noexcept {
    if (!Valid()) return 0;
    return pending_.load(std::memory_order_acquire);
}

KeysoundQueueStats KeysoundQueue::SnapshotStats() const noexcept {
    KeysoundQueueStats stats;
    stats.pushed = pushed_.load(std::memory_order_relaxed);
    stats.dispatched = dispatched_.load(std::memory_order_relaxed);
    stats.rejected_overflow = rejected_overflow_.load(std::memory_order_relaxed);
    stats.rejected_duplicate = rejected_duplicate_.load(std::memory_order_relaxed);
    stats.rejected_invalid = rejected_invalid_.load(std::memory_order_relaxed);
    stats.dropped_stale_generation = dropped_stale_.load(std::memory_order_relaxed);
    stats.sink_rejected = sink_rejected_.load(std::memory_order_relaxed);
    stats.late_dispatches = late_dispatches_.load(std::memory_order_relaxed);
    stats.total_late_frames = total_late_frames_.load(std::memory_order_relaxed);
    stats.max_late_frames = max_late_frames_.load(std::memory_order_relaxed);
    stats.pending = Pending();
    stats.high_watermark = high_watermark_.load(std::memory_order_relaxed);
    return stats;
}

} // namespace nosfix::audio
