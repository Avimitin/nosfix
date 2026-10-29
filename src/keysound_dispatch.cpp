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

// ---------------------------------------------------------------------------
// Frame-gated cue array (model of nostalgia sub_180304F30)
// ---------------------------------------------------------------------------

void FrameGateSchedule::ClearRecord(FrameGateRecord& record) noexcept {
    // The original's clear sequence at 0x180304FE6..0x180305016: id = -1, the
    // bytes at +4..+6 zeroed, then +8..+15 = -1, then the handle slot cleared.
    record.sound_id = -1;
    record.flags = 0;
    record.param = 0;
    record.param2 = 0;
    record.start_frame = kFrameGateNoFrame;
    record.end_frame = kFrameGateNoFrame;
    record.handle = 0;
}

std::uint32_t FrameGateSchedule::FindFree() noexcept {
    // The original scans from index 0 every time and stops at the end address.
    for (std::uint32_t index = 0; index < kFrameGateCapacity; ++index) {
        if (records_[index].sound_id == -1) return index;
    }
    return kFrameGateInvalidSlot;
}

void FrameGateSchedule::RaiseScanSteps(std::uint32_t steps) noexcept {
    if (steps > max_scan_steps_) max_scan_steps_ = steps;
}

void FrameGateSchedule::SetGeneration(std::uint64_t generation) noexcept {
    if (generation == generation_) return;
    for (auto& record : records_) {
        if (record.sound_id != -1) {
            ClearRecord(record);
            ++abandoned_generation_;
        }
    }
    live_ = 0;
    generation_ = generation;
    ++generation_changes_;
}

FrameGateOutcome FrameGateSchedule::Schedule(const FrameGateRequest& request,
                                             std::uint32_t current_frame) noexcept {
    FrameGateOutcome outcome;
    ++requested_;
    if (!enabled_) {
        ++refused_disabled_;
        outcome.decision = FrameGateDecision::Refused;
        return outcome;
    }
    if (request.sound_id < 0 || request.handle == 0) {
        // -1 is the free marker: storing it would make the slot look empty.
        ++invalid_;
        outcome.decision = FrameGateDecision::Invalid;
        return outcome;
    }

    // v14 = a6 + HIDWORD(qword_181366908), a signed add that may wrap.
    const std::int32_t scheduled =
        request.defer_frames + static_cast<std::int32_t>(current_frame);
    const std::uint32_t due = static_cast<std::uint32_t>(scheduled);
    outcome.start_frame = due;
    outcome.end_frame =
        request.extra == kFrameGateNoFrame ? kFrameGateNoFrame : request.extra + due;

    // Step 3: an existing record with this id is faded out and cleared first.
    for (std::uint32_t index = 0; index < kFrameGateCapacity; ++index) {
        FrameGateRecord& record = records_[index];
        if (record.sound_id != request.sound_id) continue;
        ClearRecord(record);
        --live_;
        ++replaced_same_id_;
        outcome.replaced_existing = true;
        break;
    }

    const bool deferred = scheduled > 0;
    if (!deferred) {
        // Step 5: immediate. The dedupe scan only runs for param in 1..0x58 and
        // looks for a *stored pending* record (start != -1) with the same a3 byte
        // and the same handle; the original stops that sound first.
        ++immediate_;
        outcome.decision = FrameGateDecision::ImmediateRecorded;
        if (request.param >= kFrameGateDedupeMinParam &&
            request.param <= kFrameGateDedupeMaxParam) {
            std::uint32_t steps = 0;
            for (std::uint32_t index = 0; index < kFrameGateCapacity; ++index) {
                ++steps;
                const FrameGateRecord& record = records_[index];
                if (record.param != request.param) continue;
                if (record.start_frame == kFrameGateNoFrame) continue;
                if (record.handle != request.handle) continue;
                if (record.sound_id != request.sound_id && record.sound_id >= 0) {
                    ++dedupe_stopped_;
                    outcome.dedupe_stopped = true;
                }
                break;
            }
            RaiseScanSteps(steps);
        }
    } else {
        ++deferred_;
        outcome.decision = FrameGateDecision::Deferred;
    }

    const std::uint32_t slot = FindFree();
    if (slot == kFrameGateInvalidSlot) {
        // The original returns without storing. An immediate request has already
        // been started at this point, so the marker is what is lost.
        ++table_full_;
        if (!deferred) outcome.decision = FrameGateDecision::ImmediateUnrecorded;
        else outcome.decision = FrameGateDecision::TableFull;
        return outcome;
    }

    FrameGateRecord& record = records_[slot];
    record.sound_id = request.sound_id;
    record.flags = 0;
    record.param = request.param;
    record.param2 = request.param2;
    record.start_frame = deferred ? due : kFrameGateNoFrame;
    record.end_frame = outcome.end_frame;
    record.handle = request.handle;
    ++live_;
    if (live_ > high_watermark_) high_watermark_ = live_;
    outcome.slot = slot;
    outcome.start_frame = record.start_frame;
    outcome.end_frame = record.end_frame;
    return outcome;
}

std::uint32_t FrameGateSchedule::RetireDue(std::uint32_t current_frame,
                                           FrameGateReady* out,
                                           std::uint32_t capacity) noexcept {
    if (out == nullptr || capacity == 0) return 0;
    std::uint32_t written = 0;
    for (std::uint32_t index = 0; index < kFrameGateCapacity; ++index) {
        FrameGateRecord& record = records_[index];
        if (record.sound_id == -1) continue;
        if (record.start_frame == kFrameGateNoFrame) continue; // immediate marker
        if (record.start_frame > current_frame) continue;
        if (written == capacity) {
            ++retire_unreported_;
            continue;
        }
        FrameGateReady& ready = out[written++];
        ready.slot = index;
        ready.sound_id = record.sound_id;
        ready.handle = record.handle;
        ready.param = record.param;
        ready.param2 = record.param2;
        ready.start_frame = record.start_frame;
        ready.end_frame = record.end_frame;
        ClearRecord(record);
        --live_;
    }
    retired_ += written;
    return written;
}

const FrameGateRecord& FrameGateSchedule::RecordAt(std::uint32_t index) const noexcept {
    static const FrameGateRecord kEmpty{};
    if (index >= kFrameGateCapacity) return kEmpty;
    return records_[index];
}

FrameGateStats FrameGateSchedule::Stats() const noexcept {
    FrameGateStats stats;
    stats.requested = requested_;
    stats.deferred = deferred_;
    stats.immediate = immediate_;
    stats.refused_disabled = refused_disabled_;
    stats.invalid = invalid_;
    stats.replaced_same_id = replaced_same_id_;
    stats.dedupe_stopped = dedupe_stopped_;
    stats.table_full = table_full_;
    stats.retired = retired_;
    stats.retire_unreported = retire_unreported_;
    stats.abandoned_generation = abandoned_generation_;
    stats.generation_changes = generation_changes_;
    stats.live = live_;
    stats.high_watermark = high_watermark_;
    stats.max_scan_steps = max_scan_steps_;
    return stats;
}

} // namespace nosfix::audio
