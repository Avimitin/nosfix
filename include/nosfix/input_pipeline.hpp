#pragma once

#include "nosfix/input_decoder.hpp"
#include "nosfix/input_normalizer.hpp"
#include "nosfix/raw_input_capture.hpp"
#include "nosfix/timing_events.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>

namespace nosfix {

// ===========================================================================
// Captured-packet -> decoded control -> normalized lane edge -> timed event
// ===========================================================================
//
// One consumer thread owns an InputPipeline. The producer is the verified
// Spice input-window hook, which only copies packets, captures a clock segment
// before reading each packet's QPC, and publishes a producer cursor.
//
// Per record the pipeline:
//   1. keeps the packet's own immutable RawInputCaptureClock (never the latest
//      publication), and maps the packet QPC through it exactly once;
//   2. resolves the captured HDEVICE to a stable source_id plus a per-enrollment
//      device_generation (probing is lazy, on the consumer thread only);
//   3. decodes one HID report at a time (button/value, report-ID exact) or one
//      keyboard record, leaving controls of other report IDs untouched;
//   4. applies each control transition to InputNormalizer with an explicit
//      (sequence, ordinal) identity and the exact packet stamp;
//   5. emits PipelineInputEvent values carrying the mapped SongTime, or
//      has_song_time = false when playback was paused/unstarted at capture.
//
// Failure handling is fail-closed: a descriptor/report that cannot be decoded,
// an unresolvable device, a malformed payload, or a stale-identity record is
// turned into a normalizer Reset at that record's own stamp. Held lanes are
// released synthetically; no release is ever manufactured for a control that
// the report did not describe, and no lane is bound implicitly.
//
// Capacity: a caller-supplied span may be too small for every edge of a record.
// Events written before NeedOutputCapacity are valid and already emitted; the
// remaining transitions stay pending with their original ordinals. Retrying
// continues exactly where the call stopped, so no edge is lost or duplicated.
//
// Watermark: Watermark() reports the last fully processed record. Its QPC and
// song time come from that record's own capture, never from render time and
// never from a later clock publication. The producer cursor proves that no
// future record can carry an earlier QPC. An empty queue alone is not proof and
// is never used as one.

inline constexpr std::size_t kInputMaxDeviceCount = 16;
inline constexpr std::size_t kInputMaxHandleCount = 32;
inline constexpr std::size_t kInputMaxPendingTransitions = kHidMaxControlCount;

// One explicit lane association. `source_id` is the hash of the raw-input device
// interface path (SourceIdFromDeviceName), so the intent survives a reconnect:
// the pipeline installs it for each new device_generation only after the new
// descriptor validates the control. A binding for a device that is not present
// is kept dormant; it is never guessed or applied to another device.
struct PipelineBindingSpec final {
    std::uint64_t source_id = 0;
    std::uint32_t control_id = 0;
    std::uint8_t lane = 0;
    InputControlKind kind = InputControlKind::Button;
    float press_threshold = 0.0f;   // Value only
    float release_threshold = 0.0f; // Value only, must be <= press_threshold
    bool invert = false;            // Value only
};

enum class PipelineConfigStatus : std::uint8_t {
    Accepted,
    RejectedTooManyBindings,
    RejectedInvalidBinding,     // zero source/control, lane >= kInputLaneCount
    RejectedDuplicateBinding,   // same (source, control, lane) twice
    RejectedInconsistentBinding,// same (source, control) with different kind/thresholds
    RejectedInvalidThreshold,   // non-finite/out of [0,1], release > press, Button with thresholds/invert
    RejectedUnknownControl,     // an already-enrolled device lacks that control
    RejectedControlKindMismatch,// control exists but is not a button/value as declared
    RejectedDeviceBindingError, // a known device's descriptor failed
    RejectedNormalizer,         // the normalizer rejected the assembled table
};

enum class PipelineStep : std::uint8_t {
    Processed,          // one queued record was completely processed
    NoRecord,           // queue empty
    NeedOutputCapacity, // pending transitions did not fit; retry with more space
    Faulted,            // ordering contract violated; Reset() required
};

enum class PipelineRecordKind : std::uint8_t {
    None,
    Input,
    Fence,
    StreamReset,
    UnresolvedDevice,
    DeviceFailure,
    StaleRecord,
};

enum class DeviceBindingStatus : std::uint8_t {
    NotConfigured,   // no spec names this source_id
    Installed,       // every spec validated against the descriptor
    Rejected,        // at least one spec did not validate; no spec was installed
};

struct PipelineInputEvent final {
    LogicalInputEvent input;
    SongTime song_time{};
    bool has_song_time = false;
    // False when the packet's captured segment came from a publication whose
    // writer declared it possibly regressed (or no snapshot was available).
    bool clock_monotonic = false;
};

struct InputCursor final {
    bool valid = false;
    std::uint64_t sequence = 0;
    std::uint64_t stream_epoch = 0;
    std::int64_t qpc = 0;
    // False when the last processed record could not establish an exact QPC.
    // `qpc` then remains the last exact value, for ordering only.
    bool qpc_exact = false;
};

enum class SongWatermarkStatus : std::uint8_t {
    None,
    Valid,
    InvalidatedGenerationChange,
    InvalidatedRegression,   // backwards mapping or a possibly-regressed segment
    InvalidatedStreamLoss,   // reset/device loss/stale record
};

// A processed-through song-time watermark is certified only while every mapped
// record since the generation started was non-decreasing and came from a
// Monotonic publication. A generation change, a stream loss, a backwards
// mapping, or a PossiblyRegressed segment invalidates it; the cursor keeps
// advancing so the QPC/sequence fence remains usable. `time` is retained for
// diagnostics even when the status is not Valid.
struct InputSongWatermark final {
    SongWatermarkStatus status = SongWatermarkStatus::None;
    SongTime time{};
    std::uint64_t sequence = 0;

    [[nodiscard]] constexpr bool valid() const noexcept {
        return status == SongWatermarkStatus::Valid;
    }
};

struct PipelineWatermark final {
    InputCursor cursor;
    InputSongWatermark song;
};

struct PipelineProcessResult final {
    PipelineStep step = PipelineStep::NoRecord;
    PipelineRecordKind kind = PipelineRecordKind::None;
    std::size_t events_written = 0;
    std::size_t required_event_count = 0;
    RawInputResetReason reset_reason = RawInputResetReason::None;
    DeviceProbeStatus probe_status = DeviceProbeStatus::Accepted;
};

struct PipelineDrainResult final {
    std::size_t records = 0;
    std::size_t events = 0;
    PipelineStep stop = PipelineStep::NoRecord;
    std::size_t required_event_count = 0;
};

struct DeviceSummary final {
    std::uint64_t source_id = 0;
    std::uint64_t device_generation = 0;
    std::uint32_t vendor_id = 0;
    std::uint32_t product_id = 0;
    std::uint16_t usage_page = 0;
    std::uint16_t usage = 0;
    bool keyboard = false;
    bool hid = false;
    bool failed = false;          // a report/descriptor failure latched
    DeviceBindingStatus binding_status = DeviceBindingStatus::NotConfigured;
    HidDescriptorStatus descriptor_status = HidDescriptorStatus::RejectedCaps;
    std::size_t control_count = 0;
};

struct DeviceControlView final {
    std::uint32_t control_id = 0;
    std::uint32_t spice_index = 0;
    InputControlKind kind = InputControlKind::Button;
    std::uint16_t report_id = 0;
    std::uint16_t usage_page = 0;
    std::uint16_t usage = 0;
    std::uint16_t link_collection = 0;
    std::uint16_t data_index = 0;
    bool bound = false; // at least one lane spec is installed for this generation
};

// Latest observed level of one lane. This is state, not a historical stream:
// the native PANB path also publishes a latest-value status buffer. The exact
// press level is carried by PipelineInputEvent (velocity) at its capture stamp;
// this view lets a consumer sample held pressure without frame-timing an edge.
struct AnalogLevelView final {
    bool valid = false;
    std::uint8_t lane = 0;
    std::uint64_t source_id = 0;
    std::uint64_t device_generation = 0;
    std::uint32_t control_id = 0;
    float level = 0.0f;          // normalized [0,1], invert applied
    std::uint8_t panb_velocity = 0;
    InputStamp stamp{};
};

struct PipelineStats final {
    std::uint64_t records_popped = 0;
    std::uint64_t input_records = 0;
    std::uint64_t reset_records = 0;
    std::uint64_t records_dropped = 0;
    std::uint64_t stale_records = 0;
    std::uint64_t device_probes = 0;
    std::uint64_t device_probe_failures = 0;
    std::uint64_t device_enrollments = 0;
    std::uint64_t device_removals = 0;
    std::uint64_t device_reenrollments = 0;
    std::uint64_t unsupported_descriptors = 0;
    std::uint64_t unsupported_reports = 0;
    std::uint64_t malformed_payloads = 0;
    std::uint64_t unresolved_device_records = 0;
    std::uint64_t device_failure_boundaries = 0;
    std::uint64_t binding_validation_failures = 0;
    std::uint64_t keyboard_transitions = 0;
    std::uint64_t hid_button_transitions = 0;
    std::uint64_t hid_value_transitions = 0;
    std::uint64_t transitions_applied = 0;
    std::uint64_t transitions_unmapped = 0;
    std::uint64_t events_emitted = 0;
    std::uint64_t synthetic_releases = 0;
    std::uint64_t capacity_retries = 0;
    std::uint64_t clock_unmapped_records = 0;
    std::uint64_t implicit_boundaries = 0;
    std::uint64_t fence_records = 0;
    std::uint64_t song_watermark_breaks = 0;
    std::uint64_t qpc_regressions = 0;
    std::uint64_t generation_changes = 0;
    std::uint64_t analog_observations = 0;
    RawInputResetReason last_reset_reason = RawInputResetReason::None;
    std::int64_t last_rejected_qpc = 0; // exact observed value of a rejected QPC
    std::uint64_t faults = 0;
};

struct PipelineResetResult final {
    InputResetStatus status = InputResetStatus::RejectedInvalidStamp;
    std::size_t event_count = 0;
    std::size_t required_event_count = 0;
};

class InputPipeline final {
public:
    // `probe` and `hid_api` must outlive the pipeline. `queue` is the same
    // bounded queue the capture hook writes to (RawInputCaptureQueue() for the
    // live one). Construction allocates the fixed device table once; nothing on
    // ProcessOne allocates or locks.
    InputPipeline(RawInputPacketQueue& queue, DeviceProbe& probe,
                  const HidApi& hid_api = HidApi::Windows()) noexcept;
    ~InputPipeline();
    InputPipeline(const InputPipeline&) = delete;
    InputPipeline& operator=(const InputPipeline&) = delete;
    InputPipeline(InputPipeline&&) = delete;
    InputPipeline& operator=(InputPipeline&&) = delete;

    // Installs the whole explicit binding intent. Transactional: on any
    // rejection the previous intent and the installed normalizer table stay.
    // Bindings for absent devices are dormant until that source_id enrolls;
    // bindings for present devices are validated against their descriptor now.
    [[nodiscard]] PipelineConfigStatus ConfigureBindings(
        std::span<const PipelineBindingSpec> specs) noexcept;

    // Processes at most one queued record (or continues the pending one).
    // See the file comment for output-capacity semantics.
    [[nodiscard]] PipelineProcessResult ProcessOne(
        std::span<PipelineInputEvent> out) noexcept;

    // Convenience loop: calls ProcessOne until the queue is empty, `max_records`
    // is reached, output runs out, or the pipeline faults.
    [[nodiscard]] PipelineDrainResult ProcessAll(std::span<PipelineInputEvent> out,
                                                 std::size_t max_records) noexcept;

    // Repair/discontinuity path: releases every held lane at `stamp`, clears a
    // fault and all decoded device state, and invalidates the watermark because
    // the gap's extent is unknown. Synthetic releases carry no song time.
    [[nodiscard]] PipelineResetResult Reset(const InputStamp& stamp,
                                            std::span<PipelineInputEvent> out) noexcept;

    [[nodiscard]] PipelineWatermark Watermark() const noexcept;
    [[nodiscard]] RawInputProducerCursor ProducerCursor() const noexcept;
    // Snapshot comparison only: a record may be published immediately after.
    [[nodiscard]] bool FullyProcessed() const noexcept;

    [[nodiscard]] std::size_t Devices(std::span<DeviceSummary> out) const noexcept;
    [[nodiscard]] std::size_t Controls(std::uint64_t source_id,
                                       std::uint64_t device_generation,
                                       std::span<DeviceControlView> out) const noexcept;
    [[nodiscard]] std::size_t AnalogLevels(std::span<AnalogLevelView> out) const noexcept;

    [[nodiscard]] PipelineStats Stats() const noexcept;
    [[nodiscard]] bool Faulted() const noexcept;
    [[nodiscard]] bool Ready() const noexcept;
    [[nodiscard]] std::size_t PendingTransitionCount() const noexcept;

private:
    struct DeviceTable;

    enum class PendingPhase : std::uint8_t { None, Reset, Keyboard, HidReports };

    [[nodiscard]] PipelineProcessResult ProcessInputRecord(
        std::span<PipelineInputEvent> out) noexcept;
    [[nodiscard]] PipelineProcessResult ProcessResetRecord(
        std::span<PipelineInputEvent> out) noexcept;
    [[nodiscard]] PipelineProcessResult RunPending(
        std::span<PipelineInputEvent> out) noexcept;
    // Returns true when the packet can be decoded; false when a failure has been
    // recorded in pending_ and the caller must release held state (or finish the
    // record when `after_reset` is true and nothing can be held).
    [[nodiscard]] bool SetupInputDecode(bool after_reset) noexcept;
    [[nodiscard]] PipelineProcessResult BeginDeviceFailure(
        PipelineRecordKind kind, RawInputResetReason reason,
        DeviceProbeStatus probe_status, std::span<PipelineInputEvent> out) noexcept;
    [[nodiscard]] PipelineProcessResult FinishRecord() noexcept;
    [[nodiscard]] bool FlushPendingEvents(std::span<PipelineInputEvent> out,
                                          std::size_t& written,
                                          std::size_t& required) noexcept;
    [[nodiscard]] bool ApplyOneTransition(std::size_t& written,
                                          PipelineProcessResult& result) noexcept;

    // None on success; otherwise the failure kind. Probing happens only here,
    // on the consumer thread, and only while the packet's stream epoch is still
    // the producer's current epoch (checked before and after the probe).
    [[nodiscard]] PipelineRecordKind ResolveDevice(std::uint64_t raw_handle,
                                                   std::uint64_t stream_epoch,
                                                   std::uint16_t& device_index,
                                                   DeviceProbeStatus& probe_status) noexcept;
    [[nodiscard]] PipelineConfigStatus ApplyBindings(
        std::span<const PipelineBindingSpec> specs) noexcept;
    [[nodiscard]] bool RebuildBindings() noexcept;
    void InstallDeviceBindings(std::uint16_t device_index) noexcept;
    void RevalidateHandles() noexcept;
    void MarkDeviceFailed(std::uint16_t device_index) noexcept;
    void ClearDecodedState() noexcept;
    void DropHandleForDevice(std::uint16_t device_index) noexcept;
    void AdvanceWatermark(const RawInputPacket& packet, bool has_song_time,
                          const SongTime& song_time, bool discontinuity) noexcept;
    [[nodiscard]] bool SongTimeForPacket(const RawInputPacket& packet,
                                         SongTime& song_time) const noexcept;

    RawInputPacketQueue& queue_;
    DeviceProbe& probe_;
    const HidApi& hid_api_;
    std::unique_ptr<DeviceTable> table_;

    InputNormalizer normalizer_{};
    std::array<PipelineBindingSpec, kInputMaxBindings> specs_{};
    std::size_t spec_count_ = 0;

    RawInputPacket packet_{};
    PipelineWatermark watermark_{};
    PipelineStats stats_{};
    std::array<AnalogLevelView, kInputLaneCount> lane_analog_{};
    bool faulted_ = false;
    bool bindings_dirty_ = false;

    // Pending record state. `packet_` stays valid until the record completes.
    struct Pending final {
        bool active = false;
        PipelineRecordKind kind = PipelineRecordKind::None;
        PendingPhase phase = PendingPhase::None;
        bool resume_input = false;
        PendingPhase decode_phase = PendingPhase::None;
        bool reset_applied = false;
        bool latch_device = false;
        std::uint16_t device_index = 0xFFFF; // no device
        std::uint64_t source_id = 0;
        std::uint64_t device_generation = 0;
        std::uint32_t next_ordinal = 0;
        std::size_t next_report = 0;
        std::size_t report_count = 0;
        bool report_decoded = false;
        std::size_t transition_count = 0;
        std::size_t transition_index = 0;
        bool has_song_time = false;
        SongTime song_time{};
        RawInputResetReason reset_reason = RawInputResetReason::None;
        DeviceProbeStatus probe_status = DeviceProbeStatus::Accepted;
        std::array<ControlTransition, kInputMaxPendingTransitions> transitions{};
        std::array<HidReportView, kHidMaxReportsPerPacket> reports{};
        std::array<LogicalInputEvent, kInputLaneCount> events{};
        std::size_t event_count = 0;
        std::size_t event_index = 0;
    };
    Pending pending_{};
};

} // namespace nosfix
