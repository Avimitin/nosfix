#pragma once

#include <atomic>
#include <cstddef>
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

// ===========================================================================
// nostalgia frame-gated cue array (round 2) - model of sub_180304F30
//
// Evidence: analyze/evidence/nos202502_0x180304f30.c (exported pseudocode) and
// the "displaced cue array" note in analyze/impl-20261030/ABI_COMPLETION.md E17
// (`0x1813E4AD8`, 0xC00 bytes, end marker `0x1813E56D8`, handle array
// `0x1813E4AE8` = base + 16). 0xC00 / 24 = 128 records, which agrees with the
// pseudocode's own dedupe bound (`v23 >= 384` with a 3-quadword stride = 128).
// audio-frame-gate-proposal.md quotes 2048 from the same address range; that is
// an arithmetic slip and the pass is not repeated here.
//
// What the original does, per call:
//   1. returns immediately unless word_1813E472C is set;
//   2. due = a6 + HIDWORD(qword_181366908)          (int32 add, may wrap)
//   3. if sound_id >= 0 and a record with that id exists: fade the old sound out
//      and clear the record (id -1, bytes +4..+6 zero, +8..+15 = -1, handle 0);
//   4. if due > 0 the request is *stored* with start = current + a6 (see the
//      unresolved note below) and, when a5 != -1, end = a5 + start; it is played
//      later by a per-frame consumer that starts everything with start <= current;
//   5. if due <= 0 the request is played immediately, after a dedupe scan for a
//      stored pending record with the same a3 byte and the same handle (param
//      range 1..0x58 only), and is then stored with start = -1 as an
//      "already started" marker - which is exactly what the dedupe scan skips.
//
// Unresolved (fail-closed)
// ------------------------
// * The stored start is rendered as `dword_1813E56D8 + v14`, but
//   `dword_1813E56D8` is the *end address* of the array, so one operand of that
//   store is mis-rendered. The task brief states the rule as `current + a6`, and
//   this model uses exactly that; whether the binary adds the frame counter a
//   second time cannot be settled without the instruction bytes. A wrong answer
//   shifts the start by `current` frames, which is worse than the gate, so
//   nothing here is live-wired.
// * The per-frame consumer (retirement write) has no proven RVA. RetireDue() is
//   a model of its documented behaviour, not a claim about a call site.
// * Whether 0x180004870 may be called from the producer thread is open, so this
//   model never calls libxact: it publishes records and ready items only.
namespace nosfix::audio {

inline constexpr std::uint32_t kFrameGateCapacity = 128;
inline constexpr std::uint32_t kFrameGateRecordBytes = 24;
inline constexpr std::uint32_t kFrameGateNoFrame = 0xFFFFFFFFu; // the "-1" marker
inline constexpr std::uint32_t kFrameGateInvalidSlot = 0xFFFFFFFFu;
// The dedupe scan in the original is gated on (a3 - 1) <= 0x57, i.e. 1..0x58.
inline constexpr std::uint8_t kFrameGateDedupeMinParam = 1;
inline constexpr std::uint8_t kFrameGateDedupeMaxParam = 0x58;

#pragma pack(push, 1)
struct FrameGateRecord {
    std::int32_t sound_id = -1;                    // +0  -1 = free slot
    std::uint8_t flags = 0;                        // +4
    std::uint8_t param = 0;                        // +5  a3
    std::uint8_t param2 = 0;                       // +6  a4
    std::uint8_t reserved = 0;                     // +7
    std::uint32_t start_frame = kFrameGateNoFrame; // +8  -1 = immediate marker
    std::uint32_t end_frame = kFrameGateNoFrame;   // +12 a5-derived fade end
    std::uint64_t handle = 0;                      // +16 CLibXact sound handle
};
#pragma pack(pop)

static_assert(sizeof(FrameGateRecord) == kFrameGateRecordBytes, "24-byte cue record");
static_assert(offsetof(FrameGateRecord, sound_id) == 0, "cue id offset");
static_assert(offsetof(FrameGateRecord, param) == 5, "cue a3 offset");
static_assert(offsetof(FrameGateRecord, param2) == 6, "cue a4 offset");
static_assert(offsetof(FrameGateRecord, start_frame) == 8, "cue start frame offset");
static_assert(offsetof(FrameGateRecord, end_frame) == 12, "cue end frame offset");
static_assert(offsetof(FrameGateRecord, handle) == 16, "cue handle offset");

struct FrameGateRequest {
    std::int32_t sound_id = -1;               // a1
    std::uint64_t handle = 0;                 // a2
    std::uint8_t param = 0;                   // a3
    std::uint8_t param2 = 0;                  // a4
    std::uint32_t extra = kFrameGateNoFrame;  // a5, added to the start frame
    std::int32_t defer_frames = 0;            // a6
};

enum class FrameGateDecision : std::uint8_t {
    Deferred = 0,        // stored, started when start_frame <= current
    ImmediateRecorded,   // started now and the marker was stored
    ImmediateUnrecorded, // started now, array full: the marker is lost
    TableFull,           // deferred request dropped: no free slot
    Refused,             // the gate flag is clear (word_1813E472C == 0)
    Invalid,             // id -1 (the free marker) or a null handle
};

struct FrameGateOutcome {
    FrameGateDecision decision = FrameGateDecision::Refused;
    std::uint32_t slot = kFrameGateInvalidSlot;
    std::uint32_t start_frame = kFrameGateNoFrame;
    std::uint32_t end_frame = kFrameGateNoFrame;
    bool replaced_existing = false; // same id found: faded out and cleared
    bool dedupe_stopped = false;    // immediate path stopped a pending duplicate
};

struct FrameGateReady {
    std::uint32_t slot = 0;
    std::int32_t sound_id = -1;
    std::uint64_t handle = 0;
    std::uint8_t param = 0;
    std::uint8_t param2 = 0;
    std::uint32_t start_frame = 0;
    std::uint32_t end_frame = kFrameGateNoFrame;
};

struct FrameGateStats {
    std::uint64_t requested = 0;
    std::uint64_t deferred = 0;
    std::uint64_t immediate = 0;
    std::uint64_t refused_disabled = 0;
    std::uint64_t invalid = 0;
    std::uint64_t replaced_same_id = 0;
    std::uint64_t dedupe_stopped = 0;
    std::uint64_t table_full = 0;
    std::uint64_t retired = 0;
    std::uint64_t retire_unreported = 0; // ready items that did not fit the out array
    std::uint64_t abandoned_generation = 0;
    std::uint64_t generation_changes = 0;
    std::uint32_t live = 0;
    std::uint32_t high_watermark = 0;
    std::uint32_t max_scan_steps = 0; // bounded: at most 3 * kFrameGateCapacity
};

// Frame-thread-only model of the 0x180304F30 cue array. Not thread safe: the
// game frame that produces and the consumer that retires it are the same
// thread. No allocation, no lock, no logging; every scan is bounded by
// kFrameGateCapacity.
class FrameGateSchedule final {
public:
    FrameGateSchedule() = default;

    FrameGateSchedule(const FrameGateSchedule&) = delete;
    FrameGateSchedule& operator=(const FrameGateSchedule&) = delete;

    void SetEnabled(bool enabled) noexcept { enabled_ = enabled; }
    [[nodiscard]] bool Enabled() const noexcept { return enabled_; }

    // A new generation drops every stored cue without starting it (seek, chart
    // reload, device reset). A repeated generation is a no-op.
    void SetGeneration(std::uint64_t generation) noexcept;
    [[nodiscard]] std::uint64_t Generation() const noexcept { return generation_; }

    // current_frame is HIDWORD(qword_181366908).
    [[nodiscard]] FrameGateOutcome Schedule(const FrameGateRequest& request,
                                            std::uint32_t current_frame) noexcept;

    // Walks the array in index order and reports (then clears) every record with
    // start_frame != -1 && start_frame <= current_frame. Returns the number
    // written to `out`; records that do not fit stay stored and are counted.
    [[nodiscard]] std::uint32_t RetireDue(std::uint32_t current_frame,
                                          FrameGateReady* out,
                                          std::uint32_t capacity) noexcept;

    [[nodiscard]] const FrameGateRecord& RecordAt(std::uint32_t index) const noexcept;
    [[nodiscard]] std::uint32_t LiveCount() const noexcept { return live_; }
    [[nodiscard]] FrameGateStats Stats() const noexcept;

private:
    void ClearRecord(FrameGateRecord& record) noexcept;
    [[nodiscard]] std::uint32_t FindFree() noexcept;
    void RaiseScanSteps(std::uint32_t steps) noexcept;

    FrameGateRecord records_[kFrameGateCapacity]{};
    bool enabled_ = false;
    std::uint64_t generation_ = 0;
    std::uint32_t live_ = 0;

    std::uint64_t requested_ = 0;
    std::uint64_t deferred_ = 0;
    std::uint64_t immediate_ = 0;
    std::uint64_t refused_disabled_ = 0;
    std::uint64_t invalid_ = 0;
    std::uint64_t replaced_same_id_ = 0;
    std::uint64_t dedupe_stopped_ = 0;
    std::uint64_t table_full_ = 0;
    std::uint64_t retired_ = 0;
    std::uint64_t retire_unreported_ = 0;
    std::uint64_t abandoned_generation_ = 0;
    std::uint64_t generation_changes_ = 0;
    std::uint32_t high_watermark_ = 0;
    std::uint32_t max_scan_steps_ = 0;
};

} // namespace nosfix::audio
