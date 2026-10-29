#include "nosfix/input_pipeline.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <new>

namespace nosfix {
namespace {

constexpr std::uint16_t kNoDevice = 0xFFFF;
constexpr std::size_t kRawInputHeaderSize = sizeof(RAWINPUTHEADER);

[[nodiscard]] constexpr bool IsValidLane(std::uint8_t lane) noexcept {
    return lane < kInputLaneCount;
}

[[nodiscard]] std::size_t CountHeldLanes(std::uint32_t mask) noexcept {
    std::size_t count = 0;
    while (mask != 0) {
        mask &= mask - 1;
        ++count;
    }
    return count;
}

[[nodiscard]] bool ValidThresholds(const PipelineBindingSpec& spec) noexcept {
    if (!std::isfinite(spec.press_threshold) || !std::isfinite(spec.release_threshold)) {
        return false;
    }
    if (spec.press_threshold < 0.0f || spec.press_threshold > 1.0f) return false;
    if (spec.release_threshold < 0.0f || spec.release_threshold > 1.0f) return false;
    return spec.release_threshold <= spec.press_threshold;
}

[[nodiscard]] bool SameControl(const PipelineBindingSpec& a,
                               const PipelineBindingSpec& b) noexcept {
    return a.source_id == b.source_id && a.control_id == b.control_id;
}

[[nodiscard]] bool SameDecoderConfig(const PipelineBindingSpec& a,
                                     const PipelineBindingSpec& b) noexcept {
    return a.kind == b.kind && a.invert == b.invert &&
           a.press_threshold == b.press_threshold &&
           a.release_threshold == b.release_threshold;
}

} // namespace

struct InputPipeline::DeviceTable final {
    struct DeviceEntry final {
        bool used = false;
        bool removed = false;
        bool failed = false;
        bool keyboard = false;
        bool hid = false;
        std::uint64_t source_id = 0;
        std::uint64_t device_generation = 0;
        alignas(8) std::array<std::byte, kHidMaxPreparsedBytes> preparsed{};
        std::uint32_t preparsed_size = 0;
        std::uint32_t vendor_id = 0;
        std::uint32_t product_id = 0;
        std::uint16_t usage_page = 0;
        std::uint16_t usage = 0;
        HidDescriptorStatus descriptor_status = HidDescriptorStatus::RejectedCaps;
        DeviceBindingStatus binding_status = DeviceBindingStatus::NotConfigured;
        HidDescriptor descriptor{};
        std::array<HidAnalogConfig, kHidMaxControlCount> analog{};
        KeyboardControlState keyboard_state{};
        HidControlState hid_state{};
    };

    struct HandleEntry final {
        bool used = false;
        bool unsupported = false;
        bool needs_probe = false;
        std::uint64_t handle = 0;
        std::uint64_t stream_epoch = 0;
        std::uint16_t device_index = kNoDevice;
        DeviceProbeStatus unsupported_status = DeviceProbeStatus::Unavailable;
    };

    std::array<DeviceEntry, kInputMaxDeviceCount> devices{};
    std::array<HandleEntry, kInputMaxHandleCount> handles{};
    std::uint64_t next_generation = 1;
};

InputPipeline::InputPipeline(RawInputPacketQueue& queue, DeviceProbe& probe,
                             const HidApi& hid_api) noexcept
    : queue_(queue),
      probe_(probe),
      hid_api_(hid_api),
      table_(new (std::nothrow) DeviceTable()) {}

InputPipeline::~InputPipeline() = default;

bool InputPipeline::Ready() const noexcept { return table_ != nullptr; }

std::size_t InputPipeline::PendingTransitionCount() const noexcept {
    if (!pending_.active) return 0;
    const std::size_t transitions = pending_.transition_count - pending_.transition_index;
    const std::size_t events = pending_.event_count - pending_.event_index;
    return transitions + events;
}

// ---------------------------------------------------------------------------
// Timing
// ---------------------------------------------------------------------------

bool InputPipeline::SongTimeForPacket(const RawInputPacket& packet,
                                      SongTime& song_time) const noexcept {
    if (!packet.clock.valid) return false;
    const auto mapped = packet.clock.segment.Map(packet.qpc);
    if (!mapped) return false;
    song_time = *mapped;
    return true;
}

void InputPipeline::AdvanceWatermark(const RawInputPacket& packet, bool has_song_time,
                                     const SongTime& song_time,
                                     bool discontinuity) noexcept {
    watermark_.cursor.valid = true;
    watermark_.cursor.sequence = packet.sequence;
    watermark_.cursor.stream_epoch = packet.stream_epoch;
    if (packet.qpc_exact) {
        if (!watermark_.cursor.qpc_exact || packet.qpc >= watermark_.cursor.qpc) {
            watermark_.cursor.qpc = packet.qpc;
            watermark_.cursor.qpc_exact = true;
        } else {
            // The producer faults a reversed QPC; a consumer-side reversal means
            // the ordering contract was violated. Never lower or fabricate.
            ++stats_.qpc_regressions;
            watermark_.cursor.qpc_exact = false;
        }
    } else {
        ++stats_.qpc_regressions;
        watermark_.cursor.qpc_exact = false;
    }

    if (discontinuity) {
        if (watermark_.song.valid()) ++stats_.song_watermark_breaks;
        watermark_.song.status = SongWatermarkStatus::InvalidatedStreamLoss;
        watermark_.song.sequence = packet.sequence;
        return;
    }
    if (!has_song_time) {
        ++stats_.clock_unmapped_records;
        return;
    }
    if (!packet.clock.valid ||
        packet.clock.continuity != SongClockContinuity::Monotonic) {
        if (watermark_.song.valid()) ++stats_.song_watermark_breaks;
        watermark_.song.status = SongWatermarkStatus::InvalidatedRegression;
        watermark_.song.time = song_time;
        watermark_.song.sequence = packet.sequence;
        return;
    }
    if (watermark_.song.status == SongWatermarkStatus::Valid) {
        if (song_time.generation != watermark_.song.time.generation) {
            // A watermark from another timeline must not certify this one.
            ++stats_.song_watermark_breaks;
            ++stats_.generation_changes;
            watermark_.song.status = SongWatermarkStatus::Valid;
            watermark_.song.time = song_time;
            watermark_.song.sequence = packet.sequence;
            return;
        }
        if (song_time.microseconds < watermark_.song.time.microseconds) {
            // A backwards mapping within one generation (e.g. a negative audio
            // correction) invalidates the song watermark for the rest of it.
            // The per-event time is still the best capture-time estimate, but
            // no processed-through claim is made.
            ++stats_.song_watermark_breaks;
            watermark_.song.status = SongWatermarkStatus::InvalidatedRegression;
            watermark_.song.time = song_time;
            watermark_.song.sequence = packet.sequence;
            return;
        }
        watermark_.song.time = song_time;
        watermark_.song.sequence = packet.sequence;
        return;
    }
    if (watermark_.song.status == SongWatermarkStatus::InvalidatedRegression &&
        watermark_.song.time.generation == song_time.generation) {
        return; // fail closed until a new generation establishes a baseline
    }
    // No baseline yet, a stream loss, a generation change, or a regression from
    // an older generation: this mapped record establishes the new baseline.
    watermark_.song.status = SongWatermarkStatus::Valid;
    watermark_.song.time = song_time;
    watermark_.song.sequence = packet.sequence;
}

PipelineWatermark InputPipeline::Watermark() const noexcept { return watermark_; }

RawInputProducerCursor InputPipeline::ProducerCursor() const noexcept {
    return queue_.ProducerCursor();
}

bool InputPipeline::FullyProcessed() const noexcept {
    const RawInputProducerCursor producer = queue_.ProducerCursor();
    return producer.valid && watermark_.cursor.valid &&
           producer.sequence == watermark_.cursor.sequence;
}

// ---------------------------------------------------------------------------
// Device state helpers
// ---------------------------------------------------------------------------

void InputPipeline::ClearDecodedState() noexcept {
    if (!table_) return;
    for (auto& device : table_->devices) {
        if (!device.used) continue;
        device.keyboard_state.Reset();
        device.hid_state.Reset();
    }
    lane_analog_.fill(AnalogLevelView{});
}

void InputPipeline::DropHandleForDevice(std::uint16_t device_index) noexcept {
    if (!table_) return;
    for (auto& handle : table_->handles) {
        if (handle.used && handle.device_index == device_index) handle.used = false;
    }
}

void InputPipeline::MarkDeviceFailed(std::uint16_t device_index) noexcept {
    if (!table_ || device_index >= table_->devices.size()) return;
    auto& device = table_->devices[device_index];
    device.failed = true;
    device.keyboard_state.Reset();
    device.hid_state.Reset();
    DropHandleForDevice(device_index);
    bindings_dirty_ = true;
}

void InputPipeline::RevalidateHandles() noexcept {
    if (!table_) return;
    for (auto& handle : table_->handles) {
        if (!handle.used) continue;
        if (handle.unsupported) {
            // A device change may have replaced the device that failed before.
            handle.used = false;
        } else {
            handle.needs_probe = true;
        }
    }
}

// ---------------------------------------------------------------------------
// Device resolution (consumer thread only)
// ---------------------------------------------------------------------------

PipelineRecordKind InputPipeline::ResolveDevice(std::uint64_t raw_handle,
                                               std::uint64_t stream_epoch,
                                               std::uint16_t& device_index,
                                               DeviceProbeStatus& probe_status) noexcept {
    probe_status = DeviceProbeStatus::Accepted;
    if (!table_ || raw_handle == 0) return PipelineRecordKind::UnresolvedDevice;
    auto& table = *table_;

    DeviceTable::HandleEntry* entry = nullptr;
    for (auto& handle : table.handles) {
        if (handle.used && handle.handle == raw_handle) {
            entry = &handle;
            break;
        }
    }

    if (entry != nullptr && entry->unsupported) {
        probe_status = entry->unsupported_status;
        return PipelineRecordKind::UnresolvedDevice;
    }

    if (entry != nullptr && !entry->needs_probe && entry->device_index < table.devices.size()) {
        const auto& device = table.devices[entry->device_index];
        if (device.used && !device.removed && !device.failed) {
            device_index = entry->device_index;
            return PipelineRecordKind::None;
        }
        entry->used = false;
        entry = nullptr;
    }

    // A fresh probe may only happen while the packet's stream epoch is still
    // the producer's current epoch. This closes the hotplug/handle-reuse window:
    // a device removal bumps the epoch before a new device can reuse the handle.
    const std::uint64_t epoch_before = queue_.Stats().stream_epoch;
    if (stream_epoch != epoch_before) {
        ++stats_.stale_records;
        return PipelineRecordKind::StaleRecord;
    }

    ProbedDevice probed;
    ++stats_.device_probes;
    const DeviceProbeStatus status = probe_.Probe(raw_handle, probed);
    const std::uint64_t epoch_after = queue_.Stats().stream_epoch;
    if (epoch_after != stream_epoch || epoch_before != epoch_after) {
        ++stats_.stale_records;
        return PipelineRecordKind::StaleRecord;
    }

    if (status != DeviceProbeStatus::Accepted) {
        ++stats_.device_probe_failures;
        if (status == DeviceProbeStatus::DescriptorRejected) {
            ++stats_.unsupported_descriptors;
        }
        probe_status = status;
        if (entry != nullptr) {
            if (entry->device_index < table.devices.size()) {
                auto& old = table.devices[entry->device_index];
                if (old.used && !old.removed) {
                    old.removed = true;
                    ++stats_.device_removals;
                    bindings_dirty_ = true;
                    DropHandleForDevice(entry->device_index);
                }
            }
            entry->unsupported = true;
            entry->unsupported_status = status;
            entry->needs_probe = false;
            entry->stream_epoch = stream_epoch;
            entry->device_index = kNoDevice;
        } else {
            for (auto& handle : table.handles) {
                if (handle.used) continue;
                handle.used = true;
                handle.unsupported = true;
                handle.unsupported_status = status;
                handle.handle = raw_handle;
                handle.stream_epoch = stream_epoch;
                handle.device_index = kNoDevice;
                break;
            }
        }
        return PipelineRecordKind::UnresolvedDevice;
    }

    // The handle may have been reused by a different device interface.
    if (entry != nullptr && entry->device_index < table.devices.size()) {
        auto& old = table.devices[entry->device_index];
        if (old.used && !old.removed && old.source_id != probed.source_id) {
            old.removed = true;
            ++stats_.device_removals;
            DropHandleForDevice(entry->device_index);
            bindings_dirty_ = true;
            entry = nullptr;
        }
    }

    std::uint16_t index = kNoDevice;
    for (std::size_t i = 0; i < table.devices.size(); ++i) {
        auto& device = table.devices[i];
        if (device.used && !device.removed && !device.failed &&
            device.source_id == probed.source_id) {
            index = static_cast<std::uint16_t>(i);
            break;
        }
    }

    bool enrolled = false;
    if (index == kNoDevice) {
        for (std::size_t i = 0; i < table.devices.size(); ++i) {
            if (!table.devices[i].used || table.devices[i].removed ||
                table.devices[i].failed) {
                index = static_cast<std::uint16_t>(i);
                break;
            }
        }
        if (index == kNoDevice) {
            // Device table exhausted. Leave the handle unmapped so a later
            // record can retry once a slot is free.
            probe_status = DeviceProbeStatus::Unavailable;
            ++stats_.records_dropped;
            return PipelineRecordKind::UnresolvedDevice;
        }

        bool reenrollment = false;
        for (const auto& device : table.devices) {
            if (device.source_id == probed.source_id && device.device_generation != 0) {
                reenrollment = true;
                break;
            }
        }

        auto& device = table.devices[index];
        device = {};
        device.used = true;
        device.source_id = probed.source_id;
        device.device_generation = table.next_generation++;
        device.keyboard = probed.keyboard;
        device.hid = probed.hid;
        device.vendor_id = probed.vendor_id;
        device.product_id = probed.product_id;
        device.usage_page = probed.usage_page;
        device.usage = probed.usage;
        device.descriptor_status = probed.hid_descriptor.status;
        device.descriptor = probed.hid_descriptor;
        device.preparsed = probed.preparsed;
        device.preparsed_size = probed.preparsed_size;
        device.binding_status = DeviceBindingStatus::NotConfigured;
        device.keyboard_state.Reset();
        device.hid_state.Reset();
        ++stats_.device_enrollments;
        if (reenrollment) ++stats_.device_reenrollments;
        enrolled = true;
    }

    // Reuse the existing handle entry when this was a revalidation, otherwise
    // take a free one. If the table is full, fail closed for this packet.
    if (entry == nullptr) {
        for (auto& handle : table.handles) {
            if (!handle.used) {
                entry = &handle;
                break;
            }
        }
        if (entry == nullptr) {
            for (auto& handle : table.handles) {
                if (handle.unsupported) {
                    entry = &handle;
                    break;
                }
            }
        }
        if (entry == nullptr) {
            probe_status = DeviceProbeStatus::Unavailable;
            ++stats_.records_dropped;
            return PipelineRecordKind::UnresolvedDevice;
        }
    }
    entry->used = true;
    entry->unsupported = false;
    entry->needs_probe = false;
    entry->handle = raw_handle;
    entry->stream_epoch = stream_epoch;
    entry->device_index = index;

    if (enrolled) InstallDeviceBindings(index);
    device_index = index;
    return PipelineRecordKind::None;
}

// ---------------------------------------------------------------------------
// Bindings
// ---------------------------------------------------------------------------

bool InputPipeline::RebuildBindings() noexcept {
    if (!table_) return false;
    std::array<InputBinding, kInputMaxBindings> table{};
    std::size_t count = 0;
    for (const auto& device : table_->devices) {
        if (!device.used || device.removed || device.failed) continue;
        if (device.binding_status != DeviceBindingStatus::Installed) continue;
        for (std::size_t i = 0; i < spec_count_; ++i) {
            const PipelineBindingSpec& spec = specs_[i];
            if (spec.source_id != device.source_id) continue;
            if (count >= table.size()) {
                ++stats_.binding_validation_failures;
                bindings_dirty_ = true;
                return false;
            }
            table[count++] = InputBinding{device.source_id, device.device_generation,
                                          spec.control_id, spec.lane};
        }
    }

    const InputConfigStatus status = normalizer_.Configure(
        std::span<const InputBinding>(table.data(), count));
    if (status != InputConfigStatus::Accepted) {
        ++stats_.binding_validation_failures;
        bindings_dirty_ = true;
        return false;
    }
    return true;
}

void InputPipeline::InstallDeviceBindings(std::uint16_t device_index) noexcept {
    if (!table_ || device_index >= table_->devices.size()) return;
    auto& device = table_->devices[device_index];
    device.analog.fill(HidAnalogConfig{});
    device.binding_status = DeviceBindingStatus::NotConfigured;

    std::size_t spec_count = 0;
    bool rejected = false;
    for (std::size_t i = 0; i < spec_count_; ++i) {
        const PipelineBindingSpec& spec = specs_[i];
        if (spec.source_id != device.source_id) continue;
        ++spec_count;
        if (device.keyboard) {
            if (spec.kind != InputControlKind::Button ||
                !IsKeyboardControlId(spec.control_id)) {
                rejected = true;
                break;
            }
            continue;
        }
        const HidControlInfo* control = device.descriptor.FindControl(spec.control_id);
        if (control == nullptr || control->kind != spec.kind) {
            rejected = true;
            break;
        }
        if (control->kind == InputControlKind::Value) {
            const std::size_t index =
                static_cast<std::size_t>(control - device.descriptor.controls.data());
            if (index < device.analog.size()) {
                device.analog[index] = HidAnalogConfig{true, spec.press_threshold,
                                                       spec.release_threshold,
                                                       spec.invert};
            }
        }
    }

    if (spec_count == 0) {
        device.binding_status = DeviceBindingStatus::NotConfigured;
    } else if (rejected) {
        device.binding_status = DeviceBindingStatus::Rejected;
        ++stats_.binding_validation_failures;
    } else {
        device.binding_status = DeviceBindingStatus::Installed;
    }
    (void)RebuildBindings();
}

PipelineConfigStatus InputPipeline::ApplyBindings(
    std::span<const PipelineBindingSpec> specs) noexcept {
    if (!table_) return PipelineConfigStatus::RejectedNormalizer;
    if (specs.size() > kInputMaxBindings) {
        return PipelineConfigStatus::RejectedTooManyBindings;
    }

    std::array<PipelineBindingSpec, kInputMaxBindings> candidate{};
    for (std::size_t i = 0; i < specs.size(); ++i) {
        const PipelineBindingSpec& spec = specs[i];
        if (spec.source_id == 0 || spec.control_id == 0 || !IsValidLane(spec.lane)) {
            return PipelineConfigStatus::RejectedInvalidBinding;
        }
        if (spec.kind == InputControlKind::Button) {
            if (spec.invert || spec.press_threshold != 0.0f ||
                spec.release_threshold != 0.0f) {
                return PipelineConfigStatus::RejectedInvalidThreshold;
            }
        } else if (!ValidThresholds(spec)) {
            return PipelineConfigStatus::RejectedInvalidThreshold;
        }
        for (std::size_t j = 0; j < i; ++j) {
            if (!SameControl(candidate[j], spec)) continue;
            if (candidate[j].lane == spec.lane) {
                return PipelineConfigStatus::RejectedDuplicateBinding;
            }
            if (!SameDecoderConfig(candidate[j], spec)) {
                return PipelineConfigStatus::RejectedInconsistentBinding;
            }
        }
        candidate[i] = spec;
    }

    // Validate against every active device that this intent names.
    for (std::size_t i = 0; i < specs.size(); ++i) {
        const PipelineBindingSpec& spec = specs[i];
        for (const auto& device : table_->devices) {
            if (!device.used || device.removed) continue;
            if (device.source_id != spec.source_id) continue;
            if (device.failed) return PipelineConfigStatus::RejectedDeviceBindingError;
            if (device.keyboard) {
                if (spec.kind != InputControlKind::Button) {
                    return PipelineConfigStatus::RejectedControlKindMismatch;
                }
                if (!IsKeyboardControlId(spec.control_id)) {
                    return PipelineConfigStatus::RejectedUnknownControl;
                }
                continue;
            }
            const HidControlInfo* control = device.descriptor.FindControl(spec.control_id);
            if (control == nullptr) return PipelineConfigStatus::RejectedUnknownControl;
            if (control->kind != spec.kind) {
                return PipelineConfigStatus::RejectedControlKindMismatch;
            }
        }
    }

    std::array<InputBinding, kInputMaxBindings> normalizer_bindings{};
    std::size_t binding_count = 0;
    for (const auto& device : table_->devices) {
        if (!device.used || device.removed || device.failed) continue;
        for (const auto& spec : specs) {
            if (spec.source_id != device.source_id) continue;
            if (binding_count >= normalizer_bindings.size()) {
                return PipelineConfigStatus::RejectedTooManyBindings;
            }
            normalizer_bindings[binding_count++] = InputBinding{
                device.source_id, device.device_generation, spec.control_id, spec.lane};
        }
    }

    const InputConfigStatus normalizer_status = normalizer_.Configure(
        std::span<const InputBinding>(normalizer_bindings.data(), binding_count));
    if (normalizer_status != InputConfigStatus::Accepted) {
        return PipelineConfigStatus::RejectedNormalizer;
    }

    specs_ = candidate;
    spec_count_ = specs.size();
    bindings_dirty_ = false;

    // The normalizer table is already installed; refresh each device's decoder
    // configuration and status without rebuilding it.
    for (std::size_t i = 0; i < table_->devices.size(); ++i) {
        auto& device = table_->devices[i];
        if (!device.used || device.removed) continue;
        device.analog.fill(HidAnalogConfig{});
        std::size_t device_specs = 0;
        bool rejected = false;
        for (std::size_t s = 0; s < spec_count_; ++s) {
            const PipelineBindingSpec& spec = specs_[s];
            if (spec.source_id != device.source_id) continue;
            ++device_specs;
            if (device.keyboard) continue;
            const HidControlInfo* control = device.descriptor.FindControl(spec.control_id);
            if (control == nullptr || control->kind != spec.kind) {
                rejected = true;
                break;
            }
            if (control->kind == InputControlKind::Value) {
                const std::size_t index =
                    static_cast<std::size_t>(control - device.descriptor.controls.data());
                device.analog[index] = HidAnalogConfig{true, spec.press_threshold,
                                                       spec.release_threshold, spec.invert};
            }
        }
        if (device_specs == 0) {
            device.binding_status = DeviceBindingStatus::NotConfigured;
        } else if (rejected) {
            device.binding_status = DeviceBindingStatus::Rejected;
            ++stats_.binding_validation_failures;
        } else {
            device.binding_status = DeviceBindingStatus::Installed;
        }
    }
    return PipelineConfigStatus::Accepted;
}

PipelineConfigStatus InputPipeline::ConfigureBindings(
    std::span<const PipelineBindingSpec> specs) noexcept {
    return ApplyBindings(specs);
}

// ---------------------------------------------------------------------------
// Decoding setup
// ---------------------------------------------------------------------------

bool InputPipeline::SetupInputDecode(bool after_reset) noexcept {
    (void)after_reset;
    if (!table_ || packet_.payload_size < kRawInputHeaderSize) return false;

    RAWINPUTHEADER header{};
    std::memcpy(&header, packet_.payload.data(), sizeof(header));

    std::uint16_t device_index = kNoDevice;
    DeviceProbeStatus probe_status = DeviceProbeStatus::Accepted;
    const PipelineRecordKind resolve = ResolveDevice(
        static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(header.hDevice)),
        packet_.stream_epoch, device_index, probe_status);
    if (resolve != PipelineRecordKind::None) {
        pending_.kind = resolve;
        pending_.probe_status = probe_status;
        pending_.reset_reason = resolve == PipelineRecordKind::StaleRecord
            ? RawInputResetReason::DeviceChange
            : RawInputResetReason::None;
        return false;
    }

    auto& device = table_->devices[device_index];
    if (device.failed) {
        pending_.kind = PipelineRecordKind::DeviceFailure;
        pending_.probe_status = DeviceProbeStatus::DescriptorRejected;
        pending_.reset_reason = RawInputResetReason::None;
        return false;
    }

    pending_.kind = PipelineRecordKind::Input;
    pending_.device_index = device_index;
    pending_.source_id = device.source_id;
    pending_.device_generation = device.device_generation;
    pending_.next_ordinal = 0;
    pending_.transition_count = 0;
    pending_.transition_index = 0;
    pending_.event_count = 0;
    pending_.event_index = 0;
    pending_.has_song_time = SongTimeForPacket(packet_, pending_.song_time);

    if (device.keyboard) {
        RAWKEYBOARD keyboard{};
        if (!ViewKeyboard({packet_.payload.data(), packet_.payload_size}, keyboard)) {
            ++stats_.malformed_payloads;
            pending_.kind = PipelineRecordKind::DeviceFailure;
            pending_.reset_reason = RawInputResetReason::Malformed;
            pending_.latch_device = false;
            return false;
        }
        ControlTransition transition{};
        if (device.keyboard_state.Apply(keyboard, transition)) {
            pending_.transitions[0] = transition;
            pending_.transition_count = 1;
            ++stats_.keyboard_transitions;
        }
        pending_.phase = PendingPhase::Keyboard;
        return true;
    }

    std::array<HidReportView, kHidMaxReportsPerPacket> reports{};
    std::size_t report_count = 0;
    std::uint32_t report_bytes = 0;
    if (!ViewHidReports({packet_.payload.data(), packet_.payload_size}, reports,
                        report_count, report_bytes)) {
        ++stats_.malformed_payloads;
        pending_.kind = PipelineRecordKind::DeviceFailure;
        pending_.reset_reason = RawInputResetReason::Malformed;
        pending_.latch_device = false;
        return false;
    }
    if (report_bytes != device.descriptor.input_report_bytes) {
        ++stats_.unsupported_reports;
        pending_.kind = PipelineRecordKind::DeviceFailure;
        pending_.reset_reason = RawInputResetReason::Malformed;
        pending_.latch_device = true;
        return false;
    }

    pending_.reports = reports;
    pending_.report_count = report_count;
    pending_.next_report = 0;
    pending_.report_decoded = false;
    pending_.transition_count = 0;
    pending_.transition_index = 0;
    pending_.phase = PendingPhase::HidReports;
    return true;
}

// ---------------------------------------------------------------------------
// Pending state machine
// ---------------------------------------------------------------------------

bool InputPipeline::FlushPendingEvents(std::span<PipelineInputEvent> out,
                                       std::size_t& written,
                                       std::size_t& required) noexcept {
    const std::size_t remaining = pending_.event_count - pending_.event_index;
    const std::size_t room = out.size() - written;
    const std::size_t count = std::min(remaining, room);
    for (std::size_t i = 0; i < count; ++i) {
        PipelineInputEvent& event = out[written + i];
        event.input = pending_.events[pending_.event_index + i];
        event.song_time = pending_.song_time;
        event.has_song_time = pending_.has_song_time;
        event.clock_monotonic = packet_.clock.valid &&
            packet_.clock.continuity == SongClockContinuity::Monotonic;
        ++stats_.events_emitted;
    }
    written += count;
    pending_.event_index += count;
    if (pending_.event_index < pending_.event_count) {
        required = pending_.event_count - pending_.event_index;
        return false;
    }
    pending_.event_count = 0;
    pending_.event_index = 0;
    return true;
}

bool InputPipeline::ApplyOneTransition(std::size_t& written,
                                       PipelineProcessResult& result) noexcept {
    const ControlTransition& transition = pending_.transitions[pending_.transition_index];
    PhysicalControlState state{};
    state.source_id = pending_.source_id;
    state.device_generation = pending_.device_generation;
    state.control_id = transition.control_id;
    state.down = transition.down;
    state.velocity = transition.velocity;
    const InputStamp stamp{packet_.qpc, packet_.sequence, packet_.stream_epoch,
                           pending_.next_ordinal};

    std::array<LogicalInputEvent, kInputLaneCount> events{};
    const InputApplyResult applied = normalizer_.Apply(state, stamp, events);
    if (applied.status == InputApplyStatus::InsufficientOutputCapacity) {
        ++stats_.capacity_retries;
        result.step = PipelineStep::NeedOutputCapacity;
        result.events_written = written;
        result.required_event_count = applied.required_event_count;
        return false;
    }
    if (!applied.Consumed()) {
        faulted_ = true;
        ++stats_.faults;
        result.step = PipelineStep::Faulted;
        result.events_written = written;
        return false;
    }

    if (applied.status == InputApplyStatus::IgnoredUnmappedControl) {
        ++stats_.transitions_unmapped;
    } else {
        ++stats_.transitions_applied;
    }
    pending_.event_count = applied.event_count;
    pending_.event_index = 0;
    for (std::size_t i = 0; i < applied.event_count; ++i) {
        pending_.events[i] = events[i];
    }
    // Latest analog level per bound lane: a value press uses the observed
    // level, a release is zero, a digital button is 1/14.
    for (std::size_t i = 0; i < spec_count_; ++i) {
        const PipelineBindingSpec& spec = specs_[i];
        if (spec.source_id != pending_.source_id ||
            spec.control_id != transition.control_id) {
            continue;
        }
        AnalogLevelView& view = lane_analog_[spec.lane];
        view = {};
        view.valid = true;
        view.lane = spec.lane;
        view.source_id = pending_.source_id;
        view.device_generation = pending_.device_generation;
        view.control_id = transition.control_id;
        view.level = transition.down ? transition.velocity : 0.0f;
        view.panb_velocity = transition.down ? PanbVelocity(view.level) : 0;
        view.stamp = stamp;
    }
    ++pending_.next_ordinal;
    ++pending_.transition_index;
    return true;
}

PipelineProcessResult InputPipeline::FinishRecord() noexcept {
    PipelineProcessResult result;
    result.kind = pending_.kind;
    result.reset_reason = pending_.reset_reason;
    result.probe_status = pending_.probe_status;

    if (bindings_dirty_) {
        bindings_dirty_ = false;
        if (pending_.kind == PipelineRecordKind::StreamReset ||
            pending_.kind == PipelineRecordKind::DeviceFailure ||
            pending_.kind == PipelineRecordKind::UnresolvedDevice ||
            pending_.kind == PipelineRecordKind::StaleRecord) {
            (void)RebuildBindings();
        }
    }

    const bool discontinuity =
        pending_.kind == PipelineRecordKind::StreamReset ||
        pending_.kind == PipelineRecordKind::DeviceFailure ||
        pending_.kind == PipelineRecordKind::UnresolvedDevice ||
        pending_.kind == PipelineRecordKind::StaleRecord;
    AdvanceWatermark(packet_, pending_.has_song_time, pending_.song_time,
                     discontinuity);
    pending_ = {};
    result.step = PipelineStep::Processed;
    return result;
}

PipelineProcessResult InputPipeline::BeginDeviceFailure(
    PipelineRecordKind kind, RawInputResetReason reason,
    DeviceProbeStatus probe_status, std::span<PipelineInputEvent> out) noexcept {
    pending_.active = true;
    pending_.kind = kind;
    pending_.phase = PendingPhase::Reset;
    pending_.reset_applied = false;
    pending_.resume_input = false;
    pending_.reset_reason = reason;
    pending_.probe_status = probe_status;
    pending_.has_song_time = SongTimeForPacket(packet_, pending_.song_time);
    ++stats_.device_failure_boundaries;
    if (kind == PipelineRecordKind::UnresolvedDevice) {
        ++stats_.unresolved_device_records;
    }
    if (pending_.latch_device && pending_.device_index != kNoDevice) {
        MarkDeviceFailed(pending_.device_index);
        pending_.device_index = kNoDevice;
    }
    ClearDecodedState();
    return RunPending(out);
}

PipelineProcessResult InputPipeline::RunPending(std::span<PipelineInputEvent> out) noexcept {
    PipelineProcessResult result;
    result.kind = pending_.kind;
    result.reset_reason = pending_.reset_reason;
    result.probe_status = pending_.probe_status;
    std::size_t written = 0;
    if (!pending_.active) {
        result.step = PipelineStep::NoRecord;
        return result;
    }

    for (;;) {
        if (pending_.event_index < pending_.event_count) {
            std::size_t required = 0;
            if (!FlushPendingEvents(out, written, required)) {
                result.step = PipelineStep::NeedOutputCapacity;
                result.events_written = written;
                result.required_event_count = required;
                return result;
            }
        }

        switch (pending_.phase) {
            case PendingPhase::None:
                faulted_ = true;
                ++stats_.faults;
                result.step = PipelineStep::Faulted;
                result.events_written = written;
                return result;

            case PendingPhase::Reset: {
                if (!pending_.reset_applied) {
                    const InputStamp stamp{packet_.qpc, packet_.sequence,
                                           packet_.stream_epoch,
                                           pending_.next_ordinal};
                    std::array<LogicalInputEvent, kInputLaneCount> events{};
                    const InputResetResult reset = normalizer_.Reset(stamp, events);
                    if (reset.status == InputResetStatus::InsufficientOutputCapacity) {
                        ++stats_.capacity_retries;
                        result.step = PipelineStep::NeedOutputCapacity;
                        result.events_written = written;
                        result.required_event_count = reset.required_event_count;
                        return result;
                    }
                    if (!reset.Applied()) {
                        faulted_ = true;
                        ++stats_.faults;
                        result.step = PipelineStep::Faulted;
                        result.events_written = written;
                        return result;
                    }
                    stats_.synthetic_releases += reset.event_count;
                    pending_.event_count = reset.event_count;
                    pending_.event_index = 0;
                    for (std::size_t i = 0; i < reset.event_count; ++i) {
                        pending_.events[i] = events[i];
                    }
                    pending_.reset_applied = true;
                    continue;
                }
                if (pending_.resume_input) {
                    pending_.resume_input = false;
                    if (!SetupInputDecode(true)) {
                        // The reset released every lane, so a second boundary is
                        // unnecessary; the record is consumed with its failure.
                        result = FinishRecord();
                        result.events_written = written;
                        return result;
                    }
                    continue;
                }
                result = FinishRecord();
                result.events_written = written;
                return result;
            }

            case PendingPhase::Keyboard: {
                if (pending_.transition_index < pending_.transition_count) {
                    if (!ApplyOneTransition(written, result)) return result;
                    // Back to the top so this transition's event is flushed
                    // before the next Apply overwrites the event buffer.
                    continue;
                }
                if (pending_.event_index < pending_.event_count) continue;
                result = FinishRecord();
                result.events_written = written;
                return result;
            }

            case PendingPhase::HidReports: {
                if (pending_.transition_index < pending_.transition_count) {
                    if (!ApplyOneTransition(written, result)) return result;
                    continue;
                }
                if (pending_.event_index < pending_.event_count) continue;
                if (pending_.next_report >= pending_.report_count) {
                    result = FinishRecord();
                    result.events_written = written;
                    return result;
                }

                auto& device = table_->devices[pending_.device_index];
                std::array<ControlTransition, kHidMaxControlCount> transitions{};
                std::size_t transition_count = 0;
                std::array<AnalogObservation, kHidMaxControlCount> observations{};
                std::size_t observation_count = 0;
                const HidDecodeStatus decoded = DecodeHidReport(
                    hid_api_,
                    reinterpret_cast<PHIDP_PREPARSED_DATA>(device.preparsed.data()),
                    device.descriptor, device.hid_state,
                    std::span<const HidAnalogConfig>(device.analog.data(),
                                                     device.descriptor.control_count),
                    pending_.reports[pending_.next_report].bytes,
                    transitions, transition_count, observations, &observation_count);
                if (decoded != HidDecodeStatus::Decoded) {
                    pending_.kind = PipelineRecordKind::DeviceFailure;
                    pending_.phase = PendingPhase::Reset;
                    pending_.reset_applied = false;
                    pending_.resume_input = false;
                    pending_.latch_device = decoded == HidDecodeStatus::UnsupportedReport;
                    pending_.reset_reason = RawInputResetReason::Malformed;
                    if (decoded == HidDecodeStatus::UnsupportedReport) {
                        ++stats_.unsupported_reports;
                    } else {
                        ++stats_.malformed_payloads;
                    }
                    ++stats_.device_failure_boundaries;
                    if (pending_.latch_device) MarkDeviceFailed(pending_.device_index);
                    ClearDecodedState();
                    continue;
                }

                // Latest analog level per bound lane, including reports that
                // produced no digital edge (held pressure).
                for (std::size_t i = 0; i < observation_count; ++i) {
                    const AnalogObservation& observation = observations[i];
                    for (std::size_t s = 0; s < spec_count_; ++s) {
                        const PipelineBindingSpec& spec = specs_[s];
                        if (spec.source_id != pending_.source_id ||
                            spec.control_id != observation.control_id) {
                            continue;
                        }
                        AnalogLevelView& view = lane_analog_[spec.lane];
                        view = {};
                        view.valid = true;
                        view.lane = spec.lane;
                        view.source_id = pending_.source_id;
                        view.device_generation = pending_.device_generation;
                        view.control_id = observation.control_id;
                        view.level = observation.level;
                        view.panb_velocity = observation.panb_velocity;
                        view.stamp = InputStamp{packet_.qpc, packet_.sequence,
                                                packet_.stream_epoch,
                                                pending_.next_ordinal};
                    }
                    ++stats_.analog_observations;
                }

                for (std::size_t i = 0; i < transition_count; ++i) {
                    pending_.transitions[i] = transitions[i];
                }
                pending_.transition_count = transition_count;
                pending_.transition_index = 0;
                for (std::size_t i = 0; i < transition_count; ++i) {
                    const HidControlInfo* control =
                        device.descriptor.FindControl(transitions[i].control_id);
                    if (control != nullptr && control->kind == InputControlKind::Value) {
                        ++stats_.hid_value_transitions;
                    } else {
                        ++stats_.hid_button_transitions;
                    }
                }
                ++pending_.next_report;
                continue;
            }
        }
    }
    // Unreachable: the loop only exits through a return.
    result.step = PipelineStep::Faulted;
    return result;
}

// ---------------------------------------------------------------------------
// Records
// ---------------------------------------------------------------------------

PipelineProcessResult InputPipeline::ProcessInputRecord(
    std::span<PipelineInputEvent> out) noexcept {
    ++stats_.input_records;
    pending_ = {};
    pending_.active = true;
    pending_.kind = PipelineRecordKind::Input;
    pending_.device_index = kNoDevice;

    if (packet_.payload_size < kRawInputHeaderSize) {
        ++stats_.malformed_payloads;
        pending_.kind = PipelineRecordKind::DeviceFailure;
        pending_.reset_reason = RawInputResetReason::Malformed;
        pending_.has_song_time = SongTimeForPacket(packet_, pending_.song_time);
        return BeginDeviceFailure(pending_.kind, pending_.reset_reason,
                                  DeviceProbeStatus::Accepted, out);
    }
    RAWINPUTHEADER header{};
    std::memcpy(&header, packet_.payload.data(), sizeof(header));
    if (header.dwSize != packet_.payload_size || header.dwType != packet_.raw_type) {
        ++stats_.malformed_payloads;
        pending_.kind = PipelineRecordKind::DeviceFailure;
        pending_.reset_reason = RawInputResetReason::Malformed;
        pending_.has_song_time = SongTimeForPacket(packet_, pending_.song_time);
        return BeginDeviceFailure(pending_.kind, pending_.reset_reason,
                                  DeviceProbeStatus::Accepted, out);
    }

    if (normalizer_.HasAdoptedEpoch()) {
        if (packet_.stream_epoch < normalizer_.Epoch()) {
            // Records are FIFO and producer epochs never decrease; this is a
            // broken contract, not a stale-but-harmless packet.
            faulted_ = true;
            ++stats_.faults;
            PipelineProcessResult result;
            result.step = PipelineStep::Faulted;
            result.kind = PipelineRecordKind::StaleRecord;
            result.reset_reason = RawInputResetReason::DeviceChange;
            return result;
        }
        if (packet_.stream_epoch > normalizer_.Epoch()) {
            // The producer advanced the stream epoch without a queued reset
            // record (possible only when the queue was completely full and the
            // boundary was coalesced). The record's own stamp is an explicit
            // boundary; release held lanes there, then decode this packet.
            ++stats_.implicit_boundaries;
            pending_.kind = PipelineRecordKind::StreamReset;
            pending_.phase = PendingPhase::Reset;
            pending_.reset_applied = false;
            pending_.resume_input = true;
            pending_.reset_reason = RawInputResetReason::None;
            pending_.has_song_time = SongTimeForPacket(packet_, pending_.song_time);
            ClearDecodedState();
            return RunPending(out);
        }
    }

    pending_.has_song_time = SongTimeForPacket(packet_, pending_.song_time);
    if (!SetupInputDecode(false)) {
        return BeginDeviceFailure(pending_.kind, pending_.reset_reason,
                                  pending_.probe_status, out);
    }
    return RunPending(out);
}

PipelineProcessResult InputPipeline::ProcessResetRecord(
    std::span<PipelineInputEvent> out) noexcept {
    ++stats_.reset_records;
    pending_ = {};
    pending_.active = true;
    pending_.kind = PipelineRecordKind::StreamReset;
    pending_.phase = PendingPhase::Reset;
    pending_.reset_applied = false;
    pending_.device_index = kNoDevice;
    pending_.reset_reason = packet_.reset_reason;
    pending_.has_song_time = SongTimeForPacket(packet_, pending_.song_time);
    stats_.last_reset_reason = packet_.reset_reason;
    if (!packet_.qpc_exact) stats_.last_rejected_qpc = packet_.observed_qpc;

    ClearDecodedState();

    if (packet_.reset_reason == RawInputResetReason::DeviceChange) {
        if (packet_.change_device != 0) {
            std::uint16_t removed_device = kNoDevice;
            for (auto& handle : table_->handles) {
                if (!handle.used || handle.handle != packet_.change_device) continue;
                if (packet_.change_kind == RawInputDeviceChangeKind::Removal) {
                    if (handle.device_index < table_->devices.size()) {
                        removed_device = handle.device_index;
                    }
                    handle.used = false;
                } else {
                    handle.unsupported = false;
                    handle.needs_probe = true;
                }
            }
            if (removed_device != kNoDevice) {
                auto& device = table_->devices[removed_device];
                if (device.used && !device.removed) {
                    device.removed = true;
                    ++stats_.device_removals;
                    bindings_dirty_ = true;
                }
                DropHandleForDevice(removed_device);
            }
        } else {
            RevalidateHandles();
        }
    }

    return RunPending(out);
}

// ---------------------------------------------------------------------------
// Public processing
// ---------------------------------------------------------------------------

PipelineProcessResult InputPipeline::ProcessOne(
    std::span<PipelineInputEvent> out) noexcept {
    PipelineProcessResult result;
    if (!table_ || faulted_) {
        result.step = PipelineStep::Faulted;
        if (!table_) ++stats_.faults;
        return result;
    }
    if (pending_.active) return RunPending(out);
    if (!queue_.TryPop(packet_)) {
        result.step = PipelineStep::NoRecord;
        return result;
    }
    ++stats_.records_popped;
    if (packet_.kind == RawInputPacketKind::Fence) {
        ++stats_.fence_records;
        // A fence carries no control report: it only advances the ordered
        // processed-through cursor (and the song watermark while it is still
        // certifiable). The normalizer is not touched.
        pending_ = {};
        pending_.active = true;
        pending_.kind = PipelineRecordKind::Fence;
        pending_.device_index = kNoDevice;
        pending_.has_song_time = SongTimeForPacket(packet_, pending_.song_time);
        return FinishRecord();
    }
    if (packet_.kind == RawInputPacketKind::StreamReset) {
        return ProcessResetRecord(out);
    }
    return ProcessInputRecord(out);
}

PipelineDrainResult InputPipeline::ProcessAll(std::span<PipelineInputEvent> out,
                                             std::size_t max_records) noexcept {
    PipelineDrainResult drain;
    while (drain.records < max_records) {
        const PipelineProcessResult result = ProcessOne(out.subspan(drain.events));
        drain.events += result.events_written;
        drain.stop = result.step;
        drain.required_event_count = result.required_event_count;
        if (result.step != PipelineStep::Processed) break;
        ++drain.records;
        if (out.size() != 0 && drain.events >= out.size()) break;
    }
    return drain;
}

PipelineResetResult InputPipeline::Reset(const InputStamp& stamp,
                                        std::span<PipelineInputEvent> out) noexcept {
    PipelineResetResult result;
    if (!table_) {
        result.status = InputResetStatus::RejectedInvalidStamp;
        return result;
    }
    const std::size_t held = CountHeldLanes(normalizer_.HeldLaneMask());
    if (out.size() < held) {
        result.status = InputResetStatus::InsufficientOutputCapacity;
        result.required_event_count = held;
        return result;
    }

    std::array<LogicalInputEvent, kInputLaneCount> events{};
    const InputResetResult reset = normalizer_.Reset(stamp, events);
    result.status = reset.status;
    result.required_event_count = reset.required_event_count;
    if (reset.status == InputResetStatus::InsufficientOutputCapacity) return result;
    if (!reset.Applied()) return result;

    for (std::size_t i = 0; i < reset.event_count; ++i) {
        out[i].input = events[i];
        out[i].song_time = SongTime{};
        out[i].has_song_time = false;
    }
    result.event_count = reset.event_count;
    stats_.synthetic_releases += reset.event_count;

    pending_ = {};
    faulted_ = false;
    watermark_ = {};
    ClearDecodedState();
    bindings_dirty_ = false;
    (void)RebuildBindings();
    return result;
}

// ---------------------------------------------------------------------------
// Inspection
// ---------------------------------------------------------------------------

std::size_t InputPipeline::Devices(std::span<DeviceSummary> out) const noexcept {
    if (!table_) return 0;
    std::size_t count = 0;
    for (const auto& device : table_->devices) {
        if (!device.used || device.removed) continue;
        if (count >= out.size()) break;
        DeviceSummary& summary = out[count++];
        summary = DeviceSummary{};
        summary.source_id = device.source_id;
        summary.device_generation = device.device_generation;
        summary.vendor_id = device.vendor_id;
        summary.product_id = device.product_id;
        summary.usage_page = device.usage_page;
        summary.usage = device.usage;
        summary.keyboard = device.keyboard;
        summary.hid = device.hid;
        summary.failed = device.failed;
        summary.binding_status = device.binding_status;
        summary.descriptor_status = device.descriptor_status;
        summary.control_count = device.keyboard ? 0 : device.descriptor.control_count;
    }
    return count;
}

std::size_t InputPipeline::Controls(std::uint64_t source_id,
                                    std::uint64_t device_generation,
                                    std::span<DeviceControlView> out) const noexcept {
    if (!table_) return 0;
    const DeviceTable::DeviceEntry* device = nullptr;
    for (const auto& candidate : table_->devices) {
        if (candidate.used && !candidate.removed &&
            candidate.source_id == source_id &&
            candidate.device_generation == device_generation) {
            device = &candidate;
            break;
        }
    }
    if (device == nullptr || device->keyboard) return 0;

    std::size_t count = 0;
    for (std::size_t i = 0; i < device->descriptor.control_count; ++i) {
        if (count >= out.size()) break;
        const HidControlInfo& control = device->descriptor.controls[i];
        DeviceControlView& view = out[count++];
        view = DeviceControlView{};
        view.control_id = control.control_id;
        view.spice_index = control.spice_index;
        view.kind = control.kind;
        view.report_id = control.report_id;
        view.usage_page = control.usage_page;
        view.usage = control.usage;
        view.link_collection = control.link_collection;
        view.data_index = control.data_index;
        for (std::size_t s = 0; s < spec_count_; ++s) {
            if (specs_[s].source_id == source_id &&
                specs_[s].control_id == control.control_id) {
                view.bound = true;
                break;
            }
        }
    }
    return count;
}

PipelineStats InputPipeline::Stats() const noexcept { return stats_; }

std::size_t InputPipeline::AnalogLevels(std::span<AnalogLevelView> out) const noexcept {
    const std::size_t count = std::min(out.size(), lane_analog_.size());
    for (std::size_t i = 0; i < count; ++i) out[i] = lane_analog_[i];
    return count;
}

bool InputPipeline::Faulted() const noexcept { return faulted_; }

} // namespace nosfix
