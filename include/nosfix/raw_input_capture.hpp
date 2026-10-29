#pragma once

#include "nosfix/song_clock.hpp"
#include "nosfix/song_clock_publication.hpp"

#include <Windows.h>

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>

namespace nosfix {

// Eight seconds at 1 kHz, or one second at 8 kHz, before accounting for
// other devices/reset records. Covers observed scene-loading stalls; bounded
// overflow still produces a reset, never an apparently complete event stream.
inline constexpr std::size_t kRawInputCaptureCapacity = 8192;
inline constexpr std::size_t kRawInputCapturePayloadBytes = 2048;
inline constexpr std::size_t kRawInputCaptureMaxInputPackets =
    kRawInputCaptureCapacity - 1; // one slot remains available for a reset record

enum class RawInputPacketKind : std::uint8_t {
    Input,
    Fence,       // producer heartbeat: exact QPC + clock, no payload
    StreamReset,
};

enum class RawInputResetReason : std::uint8_t {
    None,
    DeviceChange,
    QueueOverflow,
    ReadFailure,
    Oversize,
    Malformed,
    ClockReversal, // QPC went backwards; timing invalidated, no clamping
    Reentrant,     // a nested producer callback; the nested record was dropped
};

enum class RawInputDeviceChangeKind : std::uint8_t {
    None,    // stream loss without a per-device identity (DBT_DEVICECHANGE, etc.)
    Arrival, // WM_INPUT_DEVICE_CHANGE / GIDC_ARRIVAL
    Removal, // WM_INPUT_DEVICE_CHANGE / GIDC_REMOVAL
};

// Immutable clock snapshot taken by the producer *before* it reads the packet
// QPC. The packet retains it, so a queued old QPC is never mapped with a clock
// segment that was published after the capture. `valid` is false when no
// snapshot had been published yet or the seqlock read could not complete.
struct RawInputCaptureClock final {
    SongClockSegment segment{};
    std::uint64_t version = 0;
    SongClockContinuity continuity = SongClockContinuity::Monotonic;
    bool valid = false;
};

// Optional identity of the device whose hotplug produced a reset record.
struct RawInputBoundary final {
    RawInputResetReason reason = RawInputResetReason::None;
    RawInputDeviceChangeKind device_change = RawInputDeviceChangeKind::None;
    std::uint64_t device_handle = 0; // WM_INPUT_DEVICE_CHANGE lParam, 0 if unknown
};

// Coherent producer-ordering certificate. `qpc` is the QPC of the most recently
// *published exact* record (input, fence or reset) and never decreases, so no
// future exact record can carry an earlier QPC. `qpc_exact` is false when the
// last record's QPC could not be trusted; the qpc then remains the last exact
// observed value. All fields come from one seqlock snapshot, never a mix.
struct RawInputProducerCursor final {
    bool valid = false;
    bool qpc_exact = false;
    std::int64_t qpc = 0;
    std::uint64_t sequence = 0;
    std::uint64_t stream_epoch = 0;
};

struct RawInputPacket final {
    RawInputPacketKind kind = RawInputPacketKind::StreamReset;
    RawInputResetReason reset_reason = RawInputResetReason::None;
    std::uint64_t sequence = 0;
    std::uint64_t stream_epoch = 0;
    std::int64_t qpc = 0;
    // False when the record's time could not be established exactly (QPC read
    // failure or reversal). `observed_qpc` preserves the rejected raw value for
    // diagnosis; it is never used as an ordering stamp.
    bool qpc_exact = true;
    std::int64_t observed_qpc = 0;
    std::uint32_t raw_type = 0;
    std::uint32_t payload_size = 0;
    RawInputCaptureClock clock{};
    RawInputDeviceChangeKind change_kind = RawInputDeviceChangeKind::None;
    std::uint64_t change_device = 0;
    std::array<std::byte, kRawInputCapturePayloadBytes> payload{};
};

struct RawInputCaptureStats final {
    bool active = false;
    DWORD input_thread_id = 0;
    std::int64_t qpc_frequency = 0;
    std::uint64_t stream_epoch = 0;
    std::uint64_t packets_captured = 0;
    std::uint64_t fences_captured = 0;
    std::uint64_t fences_dropped = 0;
    std::uint64_t ignored_mouse_packets = 0;
    std::uint64_t dropped_packets = 0;
    std::uint64_t oversize_packets = 0;
    std::uint64_t read_failures = 0;
    std::uint64_t malformed_packets = 0;
    std::uint64_t device_change_boundaries = 0;
    std::uint64_t clock_reversals = 0;
    std::uint64_t reentrant_boundaries = 0;
    std::uint64_t capture_clock_snapshots = 0; // records carrying a valid segment
};

// Bounded single-producer/single-consumer storage. At most 8191 input packets
// are queued at once; the final slot is reserved for a reset record. The
// producer is the verified Spice input-window thread (and its heartbeat, which
// only posts a message to that thread); the consumer is the game/plugin
// thread. No queue operation allocates or takes a lock.
class RawInputPacketQueue final {
public:
    RawInputPacketQueue() noexcept;
    ~RawInputPacketQueue();
    RawInputPacketQueue(const RawInputPacketQueue&) = delete;
    RawInputPacketQueue& operator=(const RawInputPacketQueue&) = delete;

    [[nodiscard]] bool Ready() const noexcept;
    [[nodiscard]] bool PushRawInput(std::int64_t qpc,
                                    std::span<const std::byte> payload,
                                    const RawInputCaptureClock& clock = {},
                                    bool qpc_exact = true) noexcept;
    // Producer heartbeat: no payload, just an exact ordering stamp and clock.
    // Dropped (counted) when the queue is full; never invents a reset.
    [[nodiscard]] bool PushFence(std::int64_t qpc,
                                 const RawInputCaptureClock& clock,
                                 bool qpc_exact = true) noexcept;
    void NoteIgnoredMouse() noexcept;
    void MarkGap(std::int64_t qpc, RawInputResetReason reason,
                 const RawInputCaptureClock& clock = {},
                 RawInputDeviceChangeKind change = RawInputDeviceChangeKind::None,
                 std::uint64_t change_device = 0,
                 bool qpc_exact = true, std::int64_t observed_qpc = 0) noexcept;
    void MarkGap(std::int64_t qpc, const RawInputBoundary& boundary,
                 const RawInputCaptureClock& clock = {}, bool qpc_exact = true,
                 std::int64_t observed_qpc = 0) noexcept;
    [[nodiscard]] bool TryPop(RawInputPacket& packet) noexcept;
    [[nodiscard]] RawInputCaptureStats Stats() const noexcept;

    // Coherent snapshot; `valid == false` if nothing was produced or the reader
    // hit kCursorReadAttempts consecutive writer updates.
    [[nodiscard]] RawInputProducerCursor ProducerCursor() const noexcept;

    static constexpr unsigned kCursorReadAttempts = 64;

private:
    void PublishProducerCursor(std::int64_t qpc, bool qpc_exact,
                               std::uint64_t sequence,
                               std::uint64_t stream_epoch) noexcept;
    void NoteReversal(std::int64_t observed_qpc,
                      const RawInputCaptureClock& clock) noexcept;

    std::unique_ptr<RawInputPacket[]> slots_;
    alignas(64) std::atomic<std::uint64_t> write_index_{0};
    alignas(64) std::atomic<std::uint64_t> read_index_{0};
    std::atomic<std::uint64_t> next_sequence_{1};
    std::atomic<std::uint64_t> stream_epoch_{1};
    std::atomic<std::uint64_t> packets_captured_{0};
    std::atomic<std::uint64_t> fences_captured_{0};
    std::atomic<std::uint64_t> fences_dropped_{0};
    std::atomic<std::uint64_t> ignored_mouse_packets_{0};
    std::atomic<std::uint64_t> dropped_packets_{0};
    std::atomic<std::uint64_t> oversize_packets_{0};
    std::atomic<std::uint64_t> read_failures_{0};
    std::atomic<std::uint64_t> malformed_packets_{0};
    std::atomic<std::uint64_t> device_change_boundaries_{0};
    std::atomic<std::uint64_t> clock_reversals_{0};
    std::atomic<std::uint64_t> reentrant_boundaries_{0};
    std::atomic<std::uint64_t> capture_clock_snapshots_{0};

    // Coherent producer cursor: the same seqlock pattern as the clock
    // publication. Writer-only fields follow.
    alignas(64) std::atomic<std::uint64_t> cursor_sequence_{0};
    std::atomic<std::int64_t> cursor_qpc_{0};
    std::atomic<std::uint64_t> cursor_record_{0};
    std::atomic<std::uint64_t> cursor_epoch_{0};
    std::atomic<std::uint64_t> cursor_flags_{0}; // bit0 valid, bit1 qpc_exact
    bool has_cursor_ = false;
    std::int64_t cursor_qpc_writer_ = 0;
    bool cursor_exact_writer_ = true;
    std::uint64_t cursor_record_writer_ = 0;
    std::uint64_t cursor_epoch_writer_ = 0;

    // Producer-only: last accepted exact QPC. Never advanced by a regressed
    // value, and never rewritten onto a rejected record.
    std::int64_t last_qpc_ = 0;
    bool has_last_qpc_ = false;
};

// Exposed for deterministic filtering tests; the live hook uses this same
// predicate and never modifies the MSG.
[[nodiscard]] bool IsRawInputCaptureMessage(int hook_code, WPARAM action,
                                           const MSG& message,
                                           HWND target) noexcept;

// Call after the module is pinned and Spice has registered its raw-input
// window. Captures only packet copies, producer fences and stream boundaries;
// it does not map controls or change input state. `clock` may be null; when
// supplied, each record retains the snapshot published before its QPC was read.
// The pointer must outlive the capture. A producer heartbeat (a helper thread
// that only posts WM_NULL to the input window) makes an idle keyboard advance
// the processed-through fence without key events.
[[nodiscard]] bool StartRawInputCapture(
    std::string& reason,
    const SongClockPublication* clock = nullptr);
void StopRawInputCapture() noexcept;
[[nodiscard]] bool TryPopRawInputPacket(RawInputPacket& packet) noexcept;
[[nodiscard]] RawInputCaptureStats RawInputCaptureStatsNow() noexcept;

// The process-wide queue the installed hook writes to. Bind an InputPipeline to
// this reference before the capture starts. No queue operation here allocates.
[[nodiscard]] RawInputPacketQueue& RawInputCaptureQueue() noexcept;

} // namespace nosfix
