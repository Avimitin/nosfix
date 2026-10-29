#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <new>

// Bounded, allocation-free keysound event queue.
//
// One producer (input/command thread) and one consumer (the ASIO callback
// thread). Push() and DispatchDue() never allocate, lock, log, wait or call
// game code. DispatchDue() calls a plain function-pointer sink inside the
// callback; the sink must obey the same rules.
//
// The queue is not a mixer. It delivers a cue with a sample-accurate offset
// inside the current output block; starting/looping/stopping the voice is the
// mixer's job. Due timestamps are mapped through the AudioTimeline's QPC to
// render-sample mapping, so an event can never be dispatched twice: once the
// sink has seen it, the slot is retired.
namespace nosfix::audio {

struct KeysoundRequest {
    // Producer-assigned, strictly increasing for the life of the process.
    // Across generations it must not restart at zero, otherwise the once-only
    // dedupe window would reject fresh events.
    std::uint64_t sequence = 0;
    std::uint64_t generation = 0;

    // Host QPC captured when the physical/logical event arrived.
    std::int64_t capture_qpc = 0;

    // Optional scheduled start (0 = capture_qpc). A future due time keeps the
    // event queued until the block that contains it.
    std::int64_t due_qpc = 0;

    std::uint32_t cue_id = 0;
    std::uint16_t variation = 0;
    std::uint8_t priority = 0;
    std::uint8_t flags = 0;

    float volume_db = 0.0F;
    float pan = 0.0F;
};

struct KeysoundDispatch {
    std::uint64_t sequence = 0;
    std::uint64_t generation = 0;
    std::int64_t capture_qpc = 0;
    std::int64_t due_device_sample = 0;
    std::int64_t block_start_sample = 0;

    // 0 <= offset_in_block < block_frames. Late events start at offset 0.
    std::uint32_t offset_in_block = 0;
    std::uint32_t late_frames = 0;

    std::uint32_t cue_id = 0;
    std::uint16_t variation = 0;
    std::uint8_t priority = 0;
    std::uint8_t flags = 0;

    float volume_db = 0.0F;
    float pan = 0.0F;
};

enum class KeysoundPushStatus {
    Accepted,
    InvalidArgument,
    Duplicate,
    Overflow,
};

struct KeysoundPushResult {
    KeysoundPushStatus status = KeysoundPushStatus::InvalidArgument;
    [[nodiscard]] bool accepted() const noexcept {
        return status == KeysoundPushStatus::Accepted;
    }
};

struct KeysoundQueueStats {
    std::uint64_t pushed = 0;
    std::uint64_t dispatched = 0;
    std::uint64_t rejected_overflow = 0;
    std::uint64_t rejected_duplicate = 0;
    std::uint64_t rejected_invalid = 0;
    std::uint64_t dropped_stale_generation = 0;
    std::uint64_t sink_rejected = 0;
    std::uint64_t late_dispatches = 0;
    std::uint64_t total_late_frames = 0;
    std::uint32_t max_late_frames = 0;
    std::uint32_t pending = 0;
    std::uint32_t high_watermark = 0;
};

struct KeysoundDispatchOutcome {
    std::uint32_t dispatched = 0;
    std::uint32_t late = 0;
    std::uint32_t sink_rejected = 0;
    std::uint32_t dropped_stale = 0;
};

// Maps a QPC value to the render-sample domain. Returns false when no valid
// mapping exists (timeline not started/paused); the event stays queued and the
// mapping is retried next block.
using QpcToSampleMapper = bool (*)(void* context,
                                   std::int64_t qpc_ticks,
                                   std::int64_t& sample_out) noexcept;

// Called from the audio callback. Return false if the mixer could not start
// the voice; the event is still retired (never retried) and the failure is
// counted.
using KeysoundSink = bool (*)(void* context,
                              const KeysoundDispatch& dispatch) noexcept;

class KeysoundQueue final {
public:
    static constexpr std::uint32_t kMinCapacity = 64;
    static constexpr std::uint32_t kMaxCapacity = 65536;

    explicit KeysoundQueue(std::uint32_t capacity) noexcept;
    ~KeysoundQueue() = default;

    KeysoundQueue(const KeysoundQueue&) = delete;
    KeysoundQueue& operator=(const KeysoundQueue&) = delete;

    [[nodiscard]] bool Valid() const noexcept { return slots_ != nullptr; }
    [[nodiscard]] std::uint32_t Capacity() const noexcept { return capacity_; }

    [[nodiscard]] KeysoundPushResult Push(const KeysoundRequest& request) noexcept;

    // Must be installed before the consumer starts, or while both producer and
    // consumer are quiesced. It is not a runtime reconfiguration: a concurrent
    // SetMapper could pair a new mapper with an old context.
    void SetMapper(QpcToSampleMapper mapper, void* context) noexcept {
        mapper_context_ = context;
        mapper_ = mapper;
    }
    [[nodiscard]] KeysoundDispatchOutcome DispatchDue(
        std::int64_t block_start_sample,
        std::uint32_t block_frames,
        std::uint64_t generation,
        KeysoundSink sink,
        void* sink_context) noexcept;

    [[nodiscard]] KeysoundQueueStats SnapshotStats() const noexcept;
    // Not-yet-dispatched events. Ring occupancy can additionally include
    // retired holes behind a future head; a full ring is always reported as
    // rejected_overflow, never as a silent drop.
    [[nodiscard]] std::uint32_t Pending() const noexcept;

private:
    struct Slot {
        KeysoundRequest request{};
        bool dispatched = false;
    };

    std::uint32_t capacity_ = 0;
    std::uint32_t mask_ = 0;
    std::unique_ptr<Slot[]> slots_;

    std::atomic<std::uint32_t> head_{0};
    std::atomic<std::uint32_t> tail_{0};
    std::atomic<std::uint32_t> pending_{0};

    // Producer-owned dedupe window. std::atomic because SnapshotStats reads it
    // from other threads; only the producer advances it.
    std::atomic<std::uint64_t> highest_sequence_{0};

    QpcToSampleMapper mapper_ = nullptr;
    void* mapper_context_ = nullptr;
    std::atomic<std::uint64_t> pushed_{0};
    std::atomic<std::uint64_t> dispatched_{0};
    std::atomic<std::uint64_t> rejected_overflow_{0};
    std::atomic<std::uint64_t> rejected_duplicate_{0};
    std::atomic<std::uint64_t> rejected_invalid_{0};
    std::atomic<std::uint64_t> dropped_stale_{0};
    std::atomic<std::uint64_t> sink_rejected_{0};
    std::atomic<std::uint64_t> late_dispatches_{0};
    std::atomic<std::uint64_t> total_late_frames_{0};
    std::atomic<std::uint32_t> max_late_frames_{0};
    std::atomic<std::uint32_t> high_watermark_{0};
};

} // namespace nosfix::audio
