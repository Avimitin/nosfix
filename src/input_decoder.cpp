#include "nosfix/input_decoder.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>

namespace nosfix {
namespace {

constexpr std::size_t kDeviceNameBytes = 1024;
constexpr std::size_t kRawInputHeaderSize = sizeof(RAWINPUTHEADER);
constexpr std::size_t kRawHidDataOffset = offsetof(RAWINPUT, data.hid.bRawData);
constexpr std::size_t kRawKeyboardOffset = offsetof(RAWINPUT, data.keyboard);
constexpr std::size_t kRawKeyboardSize = sizeof(RAWKEYBOARD);

[[nodiscard]] bool IsValidUsageRange(USAGE usage_min, USAGE usage_max) noexcept {
    return usage_min != 0 && usage_max >= usage_min;
}

struct ButtonGroupKey final {
    std::uint16_t report_id = 0;
    std::uint16_t usage_page = 0;
    std::uint16_t link_collection = 0;

    [[nodiscard]] constexpr bool operator==(const ButtonGroupKey& other) const noexcept {
        return report_id == other.report_id && usage_page == other.usage_page &&
               link_collection == other.link_collection;
    }
};

struct PendingControl final {
    HidControlInfo info{};
    std::uint16_t group_index = 0; // 0xffff for value controls
    bool button = false;
};

[[nodiscard]] bool SameUsageKey(const HidControlInfo& a, const HidControlInfo& b) noexcept {
    return a.report_id == b.report_id && a.usage_page == b.usage_page &&
           a.link_collection == b.link_collection && a.usage == b.usage;
}

// Inserts one expanded usage into the pending table. Returns false when the
// table is full or the key duplicates an existing control.
[[nodiscard]] bool AddPendingControl(std::array<PendingControl, kHidMaxControlCount>& pending,
                                     std::size_t& count,
                                     const HidControlInfo& info,
                                     std::uint16_t group_index,
                                     bool button) noexcept {
    if (count >= pending.size()) return false;
    for (std::size_t i = 0; i < count; ++i) {
        if (SameUsageKey(pending[i].info, info)) return false;
    }
    pending[count].info = info;
    pending[count].group_index = group_index;
    pending[count].button = button;
    ++count;
    return true;
}

// Builds the (report id, usage page, link collection) group list and records
// whether a usage page mixes link collection 0 with non-zero collections. The
// latter makes a HidP_GetUsages(LinkCollection=0) call return usages that do not
// belong to the zero collection, so such a descriptor is rejected instead of
// being decoded ambiguously.
struct GroupTable final {
    std::array<ButtonGroupKey, kHidMaxButtonGroups> keys{};
    std::size_t count = 0;

    [[nodiscard]] bool FindOrAdd(const ButtonGroupKey& key, std::uint16_t& index) noexcept {
        for (std::size_t i = 0; i < count; ++i) {
            if (keys[i] == key) {
                index = static_cast<std::uint16_t>(i);
                return true;
            }
        }
        if (count >= keys.size()) return false;
        keys[count] = key;
        index = static_cast<std::uint16_t>(count);
        ++count;
        return true;
    }
};

[[nodiscard]] bool PageMixesLinkCollections(
    const std::array<PendingControl, kHidMaxControlCount>& pending,
    std::size_t count) noexcept {
    for (std::size_t i = 0; i < count; ++i) {
        if (!pending[i].button) continue;
        for (std::size_t j = i + 1; j < count; ++j) {
            if (!pending[j].button) continue;
            if (pending[i].info.usage_page != pending[j].info.usage_page) continue;
            const bool zero_i = pending[i].info.link_collection == 0;
            const bool zero_j = pending[j].info.link_collection == 0;
            if (zero_i != zero_j) return true;
        }
    }
    return false;
}

[[nodiscard]] std::int64_t SignExtendValue(ULONG raw, std::uint16_t bit_size) noexcept {
    const std::uint64_t mask = bit_size >= 32
        ? 0xFFFF'FFFFULL
        : ((std::uint64_t{1} << bit_size) - 1ULL);
    const std::uint64_t value = static_cast<std::uint64_t>(raw) & mask;
    const std::uint64_t sign = std::uint64_t{1} << (bit_size - 1U);
    // C++20: the unsigned subtraction wraps to the two's-complement pattern and
    // the conversion to int64_t is modulo 2^64, i.e. the intended signed value.
    return static_cast<std::int64_t>((value ^ sign) - sign);
}

} // namespace

// ---------------------------------------------------------------------------
// HidApi
// ---------------------------------------------------------------------------

bool HidApi::Complete() const noexcept {
    return get_caps && get_button_caps && get_value_caps && get_usages && get_usage_value;
}

const HidApi& HidApi::Windows() noexcept {
    static const HidApi api{
        &HidP_GetCaps,
        &HidP_GetButtonCaps,
        &HidP_GetValueCaps,
        &HidP_GetUsages,
        &HidP_GetUsageValue,
    };
    return api;
}

// ---------------------------------------------------------------------------
// HidDescriptor queries
// ---------------------------------------------------------------------------

const HidControlInfo* HidDescriptor::FindControl(std::uint32_t control_id) const noexcept {
    for (std::size_t i = 0; i < control_count; ++i) {
        if (controls[i].control_id == control_id) return &controls[i];
    }
    return nullptr;
}

const HidControlInfo* HidDescriptor::FindSpiceButton(std::uint32_t spice_index) const noexcept {
    for (std::size_t i = 0; i < button_control_count; ++i) {
        if (controls[i].spice_index == spice_index) return &controls[i];
    }
    return nullptr;
}

const HidControlInfo* HidDescriptor::FindSpiceValue(std::uint32_t spice_index,
                                                    std::uint16_t usage) const noexcept {
    const HidControlInfo* found = nullptr;
    for (std::size_t i = button_control_count; i < control_count; ++i) {
        const HidControlInfo& control = controls[i];
        if (control.spice_index != spice_index) continue;
        if (usage != 0) {
            if (control.usage == usage) return &control;
            continue;
        }
        if (found) return nullptr; // a range cap is inherently ambiguous by index
        found = &control;
    }
    return found;
}

// ---------------------------------------------------------------------------
// Descriptor construction
// ---------------------------------------------------------------------------

HidDescriptorStatus BuildHidDescriptor(const HidApi& api, PHIDP_PREPARSED_DATA preparsed,
                                       HidDescriptor& out) noexcept {
    out = {};
    out.status = HidDescriptorStatus::RejectedCaps;
    if (!api.Complete() || preparsed == nullptr) return out.status;

    HIDP_CAPS caps{};
    if (api.get_caps(preparsed, &caps) != HIDP_STATUS_SUCCESS) return out.status;

    if (caps.InputReportByteLength == 0 || caps.InputReportByteLength > kHidMaxReportBytes) {
        out.status = HidDescriptorStatus::RejectedReportLength;
        return out.status;
    }
    const std::uint16_t report_bytes = caps.InputReportByteLength;

    if (caps.NumberInputButtonCaps > kHidMaxCaps ||
        caps.NumberInputValueCaps > kHidMaxCaps) {
        out.status = HidDescriptorStatus::RejectedTooManyCaps;
        return out.status;
    }

    std::array<HIDP_BUTTON_CAPS, kHidMaxCaps> button_caps{};
    USHORT button_cap_length = caps.NumberInputButtonCaps;
    if (button_cap_length != 0) {
        if (api.get_button_caps(HidP_Input, button_caps.data(), &button_cap_length,
                                preparsed) != HIDP_STATUS_SUCCESS) {
            out.status = HidDescriptorStatus::RejectedCaps;
            return out.status;
        }
        if (button_cap_length > kHidMaxCaps) {
            out.status = HidDescriptorStatus::RejectedTooManyCaps;
            return out.status;
        }
    }

    std::array<HIDP_VALUE_CAPS, kHidMaxCaps> value_caps{};
    USHORT value_cap_length = caps.NumberInputValueCaps;
    if (value_cap_length != 0) {
        if (api.get_value_caps(HidP_Input, value_caps.data(), &value_cap_length,
                               preparsed) != HIDP_STATUS_SUCCESS) {
            out.status = HidDescriptorStatus::RejectedCaps;
            return out.status;
        }
        if (value_cap_length > kHidMaxCaps) {
            out.status = HidDescriptorStatus::RejectedTooManyCaps;
            return out.status;
        }
    }

    std::array<PendingControl, kHidMaxControlCount> pending{};
    std::size_t pending_count = 0;
    GroupTable groups{};
    std::uint32_t spice_button_index = 0;
    std::uint32_t value_expanded_index = 0;
    std::array<std::uint8_t, kHidMaxReportIds> report_ids{};
    std::size_t report_id_count = 0;
    bool any_report_id = false;
    bool any_zero_report_id = false;

    for (USHORT cap_index = 0; cap_index < button_cap_length; ++cap_index) {
        const HIDP_BUTTON_CAPS& cap = button_caps[cap_index];
        USAGE usage_min = 0;
        USAGE usage_max = 0;
        USHORT data_index_min = 0;
        if (cap.IsRange) {
            usage_min = cap.Range.UsageMin;
            usage_max = cap.Range.UsageMax;
            data_index_min = cap.Range.DataIndexMin;
        } else {
            usage_min = cap.NotRange.Usage;
            usage_max = cap.NotRange.Usage;
            data_index_min = cap.NotRange.DataIndex;
        }

        // Spice skips malformed ranges instead of failing the whole device. We
        // do the same, and the skipped usages stay unbindable.
        if (!IsValidUsageRange(usage_min, usage_max)) continue;
        const std::uint32_t usage_count =
            static_cast<std::uint32_t>(usage_max) - usage_min + 1U;
        if (usage_count >= 0xFFFFU) continue;

        const ButtonGroupKey key{cap.ReportID, cap.UsagePage, cap.LinkCollection};
        std::uint16_t group_index = 0;
        if (!groups.FindOrAdd(key, group_index)) {
            out.status = HidDescriptorStatus::RejectedTooManyGroups;
            return out.status;
        }

        for (std::uint32_t offset = 0; offset < usage_count; ++offset) {
            HidControlInfo info{};
            info.control_id = MakeHidButtonControlId(spice_button_index + offset);
            info.spice_index = spice_button_index + offset;
            info.report_id = cap.ReportID;
            info.usage_page = cap.UsagePage;
            info.usage = static_cast<std::uint16_t>(usage_min + offset);
            info.link_collection = cap.LinkCollection;
            info.data_index = static_cast<std::uint16_t>(data_index_min + offset);
            info.kind = InputControlKind::Button;
            if (!AddPendingControl(pending, pending_count, info, group_index, true)) {
                out.status = pending_count >= pending.size()
                    ? HidDescriptorStatus::RejectedTooManyControls
                    : HidDescriptorStatus::RejectedDuplicateControl;
                return out.status;
            }
        }
        spice_button_index += usage_count;
        if (spice_button_index > 0xFFFFU) {
            out.status = HidDescriptorStatus::RejectedTooManyControls;
            return out.status;
        }

        if (cap.ReportID == 0) {
            any_zero_report_id = true;
        } else {
            any_report_id = true;
            bool seen = false;
            for (std::size_t i = 0; i < report_id_count; ++i) {
                if (report_ids[i] == cap.ReportID) {
                    seen = true;
                    break;
                }
            }
            if (!seen) {
                if (report_id_count >= report_ids.size()) {
                    out.status = HidDescriptorStatus::RejectedTooManyReportIds;
                    return out.status;
                }
                report_ids[report_id_count++] = cap.ReportID;
            }
        }
    }

    for (USHORT cap_index = 0; cap_index < value_cap_length; ++cap_index) {
        const HIDP_VALUE_CAPS& cap = value_caps[cap_index];
        USAGE usage_min = 0;
        USAGE usage_max = 0;
        USHORT data_index_min = 0;
        if (cap.IsRange) {
            usage_min = cap.Range.UsageMin;
            usage_max = cap.Range.UsageMax;
            data_index_min = cap.Range.DataIndexMin;
        } else {
            usage_min = cap.NotRange.Usage;
            usage_max = cap.NotRange.Usage;
            data_index_min = cap.NotRange.DataIndex;
        }

        if (!IsValidUsageRange(usage_min, usage_max)) continue;
        if (cap.BitSize == 0 || cap.BitSize > 32) continue;
        if (cap.LogicalMin >= cap.LogicalMax) continue;

        const std::uint32_t usage_count =
            static_cast<std::uint32_t>(usage_max) - usage_min + 1U;
        if (usage_count >= 0xFFFFU) continue;
        if (pending_count + usage_count > pending.size()) {
            out.status = HidDescriptorStatus::RejectedTooManyControls;
            return out.status;
        }

        for (std::uint32_t offset = 0; offset < usage_count; ++offset) {
            HidControlInfo info{};
            info.control_id = MakeHidValueControlId(value_expanded_index);
            info.spice_index = cap_index; // Spice does not expand value range caps
            info.report_id = cap.ReportID;
            info.usage_page = cap.UsagePage;
            info.usage = static_cast<std::uint16_t>(usage_min + offset);
            info.link_collection = cap.LinkCollection;
            info.data_index = static_cast<std::uint16_t>(data_index_min + offset);
            info.kind = InputControlKind::Value;
            info.logical_min = cap.LogicalMin;
            info.logical_max = cap.LogicalMax;
            info.bit_size = cap.BitSize;
            info.has_null_state = cap.HasNull != FALSE;
            info.value_range_cap = usage_count > 1;
            if (!AddPendingControl(pending, pending_count, info, 0xFFFF, false)) {
                out.status = pending_count >= pending.size()
                    ? HidDescriptorStatus::RejectedTooManyControls
                    : HidDescriptorStatus::RejectedDuplicateControl;
                return out.status;
            }
            ++value_expanded_index;
        }

        if (cap.ReportID == 0) {
            any_zero_report_id = true;
        } else {
            any_report_id = true;
            bool seen = false;
            for (std::size_t i = 0; i < report_id_count; ++i) {
                if (report_ids[i] == cap.ReportID) {
                    seen = true;
                    break;
                }
            }
            if (!seen) {
                if (report_id_count >= report_ids.size()) {
                    out.status = HidDescriptorStatus::RejectedTooManyReportIds;
                    return out.status;
                }
                report_ids[report_id_count++] = cap.ReportID;
            }
        }
    }

    if (pending_count == 0) {
        out.status = HidDescriptorStatus::RejectedNoInputControls;
        return out.status;
    }
    if (report_id_count > kHidMaxReportIds) {
        out.status = HidDescriptorStatus::RejectedTooManyReportIds;
        return out.status;
    }

    const bool uses_report_ids = any_report_id;
    if (uses_report_ids && any_zero_report_id) {
        out.status = HidDescriptorStatus::RejectedMixedReportIds;
        return out.status;
    }
    if (PageMixesLinkCollections(pending, pending_count)) {
        out.status = HidDescriptorStatus::RejectedAmbiguousLinkCollection;
        return out.status;
    }

    // Assemble controls grouped per button group, buttons first, then values.
    // Within a group they are ordered by usage so a returned usage can be found
    // with a binary search.
    std::size_t control_count = 0;
    for (std::size_t group_index = 0; group_index < groups.count; ++group_index) {
        HidButtonGroup& group = out.button_groups[group_index];
        group.report_id = groups.keys[group_index].report_id;
        group.usage_page = groups.keys[group_index].usage_page;
        group.link_collection = groups.keys[group_index].link_collection;
        group.first_control = static_cast<std::uint16_t>(control_count);

        // Collect this group's pending controls in usage order.
        std::array<std::uint16_t, kHidMaxControlCount> order{};
        std::size_t order_count = 0;
        for (std::size_t i = 0; i < pending_count; ++i) {
            if (!pending[i].button) continue;
            if (pending[i].group_index != group_index) continue;
            // Insertion sort keyed by usage (ties are impossible: duplicates
            // were rejected above).
            std::size_t position = order_count;
            while (position > 0 && pending[order[position - 1]].info.usage > pending[i].info.usage) {
                order[position] = order[position - 1];
                --position;
            }
            order[position] = static_cast<std::uint16_t>(i);
            ++order_count;
        }
        for (std::size_t i = 0; i < order_count; ++i) {
            out.controls[control_count++] = pending[order[i]].info;
        }
        group.control_count = static_cast<std::uint16_t>(order_count);
    }
    out.button_group_count = groups.count;
    out.button_control_count = control_count;

    for (std::size_t i = 0; i < pending_count; ++i) {
        if (pending[i].button) continue;
        out.controls[control_count++] = pending[i].info;
    }

    out.control_count = control_count;
    out.input_report_bytes = report_bytes;
    out.uses_report_ids = uses_report_ids;
    out.status = HidDescriptorStatus::Accepted;
    return out.status;
}

// ---------------------------------------------------------------------------
// Payload views
// ---------------------------------------------------------------------------

bool ViewHidReports(std::span<const std::byte> payload,
                    std::span<HidReportView> reports,
                    std::size_t& report_count,
                    std::uint32_t& report_bytes) noexcept {
    report_count = 0;
    report_bytes = 0;
    if (payload.size() < kRawInputHeaderSize) return false;

    RAWINPUTHEADER header{};
    std::memcpy(&header, payload.data(), sizeof(header));
    if (header.dwType != RIM_TYPEHID) return false;
    if (header.dwSize != payload.size()) return false;
    if (payload.size() < kRawHidDataOffset) return false;

    DWORD size = 0;
    DWORD count = 0;
    std::memcpy(&size, payload.data() + offsetof(RAWINPUT, data.hid.dwSizeHid), sizeof(size));
    std::memcpy(&count, payload.data() + offsetof(RAWINPUT, data.hid.dwCount), sizeof(count));
    if (size == 0 || count == 0) return false;
    if (size > kHidMaxReportBytes || count > kHidMaxReportsPerPacket) return false;

    const std::uint64_t total = static_cast<std::uint64_t>(size) * count;
    if (total > payload.size() - kRawHidDataOffset) return false;
    if (reports.size() < count) return false;

    for (std::size_t i = 0; i < count; ++i) {
        reports[i].bytes = payload.subspan(kRawHidDataOffset + i * size, size);
    }
    report_count = count;
    report_bytes = size;
    return true;
}

bool ViewKeyboard(std::span<const std::byte> payload, RAWKEYBOARD& keyboard) noexcept {
    keyboard = {};
    if (payload.size() < kRawInputHeaderSize) return false;

    RAWINPUTHEADER header{};
    std::memcpy(&header, payload.data(), sizeof(header));
    if (header.dwType != RIM_TYPEKEYBOARD) return false;
    if (header.dwSize != payload.size()) return false;
    if (payload.size() < kRawKeyboardOffset + kRawKeyboardSize) return false;

    std::memcpy(&keyboard, payload.data() + kRawKeyboardOffset, sizeof(keyboard));
    return true;
}

// ---------------------------------------------------------------------------
// Keyboard
// ---------------------------------------------------------------------------

std::uint32_t MakeKeyboardControlId(const RAWKEYBOARD& keyboard) noexcept {
    std::uint32_t index = 0;
    if ((keyboard.Flags & RI_KEY_E0) != 0) index += 256;
    if ((keyboard.Flags & RI_KEY_E1) != 0) index += 512;

    USHORT vkey = keyboard.VKey;
    // Spice's normalisation of the ambiguous Control keys (cfg/rawinput parity).
    switch (index + vkey) {
        case 17:
            vkey = VK_LCONTROL;
            break;
        case 273:
            vkey = VK_RCONTROL;
            break;
        default:
            break;
    }
    switch (keyboard.MakeCode) {
        case 42:
            vkey = VK_LSHIFT;
            break;
        case 54:
            vkey = VK_RSHIFT;
            break;
        default:
            break;
    }
    if (vkey == 0 || vkey >= 255) return 0;
    return MakeKeyboardControlId(index + vkey);
}

void KeyboardControlState::Reset() noexcept {
    down_.fill(0);
}

bool KeyboardControlState::IsDown(std::uint32_t control_id) const noexcept {
    if (!IsKeyboardControlId(control_id)) return false;
    const std::uint32_t index = control_id - kKeyboardControlBase;
    if (index >= kKeyboardControlCount) return false;
    return (down_[index / 64] & (std::uint64_t{1} << (index % 64))) != 0;
}

bool KeyboardControlState::Apply(const RAWKEYBOARD& keyboard,
                                 ControlTransition& out) noexcept {
    out = {};
    const std::uint32_t control_id = MakeKeyboardControlId(keyboard);
    if (control_id == 0) return false;
    const std::uint32_t index = control_id - kKeyboardControlBase;
    const std::uint64_t bit = std::uint64_t{1} << (index % 64);
    const bool was_down = (down_[index / 64] & bit) != 0;
    const bool down = (keyboard.Flags & RI_KEY_BREAK) == 0;

    // A typematic make (WINDOWS repeats WM_KEYDOWN) and an unmatched break are
    // not physical edges. Only a real level change is reported.
    if (was_down == down) return false;

    if (down) {
        down_[index / 64] |= bit;
    } else {
        down_[index / 64] &= ~bit;
    }
    out.control_id = control_id;
    out.down = down;
    out.velocity = down ? 1.0f : 0.0f;
    return true;
}

// ---------------------------------------------------------------------------
// Values
// ---------------------------------------------------------------------------

bool NormalizeHidValue(std::int32_t logical_min, std::int32_t logical_max,
                       std::uint16_t bit_size, ULONG raw_value,
                       double& normalized) noexcept {
    normalized = 0.0;
    if (bit_size == 0 || bit_size > 32) return false;
    if (logical_min >= logical_max) return false;

    std::int64_t value = 0;
    if (logical_min < 0) {
        value = SignExtendValue(raw_value, bit_size);
    } else {
        const std::uint64_t mask = bit_size >= 32
            ? 0xFFFF'FFFFULL
            : ((std::uint64_t{1} << bit_size) - 1ULL);
        value = static_cast<std::int64_t>(static_cast<std::uint64_t>(raw_value) & mask);
    }

    const double span = static_cast<double>(logical_max) - static_cast<double>(logical_min);
    if (!(span > 0.0)) return false;
    const double scaled = (static_cast<double>(value) - static_cast<double>(logical_min)) / span;
    if (!std::isfinite(scaled)) return false;
    normalized = std::clamp(scaled, 0.0, 1.0);
    return true;
}

// ---------------------------------------------------------------------------
// HID report decoding
// ---------------------------------------------------------------------------

void HidControlState::Reset() noexcept {
    down_.fill(0);
    analog_down_.fill(0);
    value_.fill(0.0f);
}

bool HidControlState::IsDownByIndex(std::size_t control_index) const noexcept {
    if (control_index >= down_.size()) return false;
    return down_[control_index] != 0;
}

HidDecodeStatus DecodeHidReport(const HidApi& api,
                                PHIDP_PREPARSED_DATA preparsed,
                                const HidDescriptor& descriptor,
                                HidControlState& state,
                                std::span<const HidAnalogConfig> analog,
                                std::span<const std::byte> report,
                                std::span<ControlTransition> out,
                                std::size_t& written,
                                std::span<AnalogObservation> observations,
                                std::size_t* observation_count) noexcept {
    written = 0;
    if (observation_count != nullptr) *observation_count = 0;
    if (!descriptor.Accepted() || preparsed == nullptr || !api.Complete()) {
        return HidDecodeStatus::MalformedArgument;
    }
    if (analog.size() < descriptor.control_count) return HidDecodeStatus::MalformedArgument;
    if (report.size() != descriptor.input_report_bytes) return HidDecodeStatus::UnsupportedReport;
    if (report.size() > std::numeric_limits<ULONG>::max()) return HidDecodeStatus::UnsupportedReport;

    std::uint16_t report_id = 0;
    if (descriptor.uses_report_ids) {
        report_id = static_cast<std::uint8_t>(report[0]);
        if (report_id == 0) return HidDecodeStatus::UnsupportedReport;
    }

    bool report_id_known = !descriptor.uses_report_ids;
    for (std::size_t i = 0; i < descriptor.control_count; ++i) {
        if (descriptor.controls[i].report_id == report_id) {
            report_id_known = true;
            break;
        }
    }
    if (!report_id_known) return HidDecodeStatus::UnsupportedReport;

    std::array<std::uint8_t, kHidMaxControlCount> next_down{};
    std::array<std::uint8_t, kHidMaxControlCount> next_analog{};
    std::array<float, kHidMaxControlCount> next_value{};
    for (std::size_t i = 0; i < descriptor.control_count; ++i) {
        next_down[i] = state.down_[i];
        next_analog[i] = state.analog_down_[i];
        next_value[i] = state.value_[i];
    }

    std::array<ControlTransition, kHidMaxControlCount> staged{};
    std::size_t staged_count = 0;
    std::array<AnalogObservation, kHidMaxControlCount> staged_observations{};
    std::size_t staged_observation_count = 0;
    std::array<std::uint8_t, kHidMaxControlCount> pressed{};
    std::array<USAGE, kHidMaxControlCount> usages{};

    // --- buttons of this report ID -----------------------------------------
    for (std::size_t group_index = 0; group_index < descriptor.button_group_count;
         ++group_index) {
        const HidButtonGroup& group = descriptor.button_groups[group_index];
        if (group.report_id != report_id) continue;
        if (group.control_count == 0) continue;
        if (group.first_control > descriptor.control_count ||
            group.control_count > descriptor.control_count - group.first_control) {
            return HidDecodeStatus::MalformedArgument;
        }

        ULONG usage_length = group.control_count;
        const NTSTATUS status = api.get_usages(
            HidP_Input, group.usage_page, group.link_collection, usages.data(),
            &usage_length, preparsed,
            const_cast<PCHAR>(reinterpret_cast<const char*>(report.data())),
            static_cast<ULONG>(report.size()));
        if (status != HIDP_STATUS_SUCCESS) {
            return HidDecodeStatus::UnsupportedReport;
        }
        if (usage_length > group.control_count) {
            return HidDecodeStatus::UnsupportedReport;
        }

        pressed.fill(0);
        for (ULONG usage_index = 0; usage_index < usage_length; ++usage_index) {
            const USAGE usage = usages[usage_index];
            // Binary search within the group's usage-sorted control range.
            std::size_t low = group.first_control;
            std::size_t high = group.first_control + group.control_count;
            while (low < high) {
                const std::size_t mid = low + (high - low) / 2;
                const USAGE mid_usage = descriptor.controls[mid].usage;
                if (mid_usage < usage) {
                    low = mid + 1;
                } else {
                    high = mid;
                }
            }
            if (low < group.first_control + group.control_count &&
                descriptor.controls[low].usage == usage) {
                pressed[low] = 1;
            }
        }

        for (std::size_t i = group.first_control;
             i < group.first_control + group.control_count; ++i) {
            const bool down = pressed[i] != 0;
            if (next_down[i] == static_cast<std::uint8_t>(down)) continue;
            next_down[i] = static_cast<std::uint8_t>(down);
            if (staged_count >= staged.size()) return HidDecodeStatus::UnsupportedReport;
            staged[staged_count].control_id = descriptor.controls[i].control_id;
            staged[staged_count].down = down;
            staged[staged_count].velocity = down ? 1.0f : 0.0f;
            ++staged_count;
        }
    }

    // --- values of this report ID that are explicitly bound -----------------
    for (std::size_t i = descriptor.button_control_count; i < descriptor.control_count; ++i) {
        const HidControlInfo& control = descriptor.controls[i];
        if (control.report_id != report_id) continue;
        const HidAnalogConfig& config = analog[i];
        if (!config.bound) continue;

        ULONG raw_value = 0;
        const NTSTATUS status = api.get_usage_value(
            HidP_Input, control.usage_page, control.link_collection, control.usage,
            &raw_value, preparsed,
            const_cast<PCHAR>(reinterpret_cast<const char*>(report.data())),
            static_cast<ULONG>(report.size()));
        bool value_valid = false;
        double normalized = 0.0;
        if (status == HIDP_STATUS_SUCCESS) {
            value_valid = NormalizeHidValue(control.logical_min, control.logical_max,
                                            control.bit_size, raw_value, normalized);
        } else if (status == HIDP_STATUS_NULL) {
            value_valid = true; // null means "no value": treat as released
            normalized = 0.0;
        }
        if (!value_valid) return HidDecodeStatus::UnsupportedReport;

        if (config.invert) normalized = 1.0 - normalized;
        const float normalized_float = static_cast<float>(normalized);
        next_value[i] = normalized_float;
        if (staged_observation_count < staged_observations.size()) {
            staged_observations[staged_observation_count++] = AnalogObservation{
                control.control_id, normalized_float, PanbVelocity(normalized_float)};
        }

        const bool was_down = next_analog[i] != 0;
        // Compare in float: the configured thresholds are float, and an exact
        // threshold value must press (0.6f >= 0.6f), not fail via a wider
        // double representation.
        const bool down = was_down
            ? normalized_float > config.release_threshold
            : normalized_float >= config.press_threshold;
        if (down == was_down) continue;
        next_analog[i] = static_cast<std::uint8_t>(down);
        if (staged_count >= staged.size()) return HidDecodeStatus::UnsupportedReport;
        staged[staged_count].control_id = control.control_id;
        staged[staged_count].down = down;
        staged[staged_count].velocity = down ? normalized_float : 0.0f;
        ++staged_count;
    }

    if (staged_count > out.size()) return HidDecodeStatus::OutputTooSmall;

    for (std::size_t i = 0; i < staged_count; ++i) out[i] = staged[i];
    const std::size_t observation_copy =
        std::min(staged_observation_count, observations.size());
    for (std::size_t i = 0; i < observation_copy; ++i) observations[i] = staged_observations[i];
    if (observation_count != nullptr) *observation_count = observation_copy;
    for (std::size_t i = 0; i < descriptor.control_count; ++i) {
        state.down_[i] = next_down[i];
        state.analog_down_[i] = next_analog[i];
        state.value_[i] = next_value[i];
    }
    written = staged_count;
    return HidDecodeStatus::Decoded;
}

// ---------------------------------------------------------------------------
// Identity
// ---------------------------------------------------------------------------

std::uint64_t SourceIdFromDeviceName(std::string_view ansi_device_name) noexcept {
    // FNV-1a, then a final avalanche so similar interface paths are well spread.
    std::uint64_t hash = 1'469'598'103'934'665'603ULL;
    for (const char character : ansi_device_name) {
        hash ^= static_cast<unsigned char>(character);
        hash *= 1'099'511'628'211ULL;
    }
    hash ^= hash >> 33;
    hash *= 0xFF51'AFD7'ED55'8CCDULL;
    hash ^= hash >> 33;
    return hash == 0 ? 1 : hash;
}

// ---------------------------------------------------------------------------
// Windows device probe
// ---------------------------------------------------------------------------

namespace {

class WindowsDeviceProbe final : public DeviceProbe {
public:
    explicit WindowsDeviceProbe(const HidApi& api) noexcept : api_(api) {}

    [[nodiscard]] DeviceProbeStatus Probe(std::uint64_t raw_handle,
                                          ProbedDevice& out) override {
        out = {};
        const HANDLE handle = reinterpret_cast<HANDLE>(static_cast<std::uintptr_t>(raw_handle));
        if (handle == nullptr) return DeviceProbeStatus::Unavailable;

        std::array<char, kDeviceNameBytes> name{};
        UINT name_size = static_cast<UINT>(name.size());
        if (GetRawInputDeviceInfoA(handle, RIDI_DEVICENAME, name.data(), &name_size) ==
            static_cast<UINT>(-1)) {
            return DeviceProbeStatus::NameMissing;
        }
        if (name_size == 0 || name[0] == '\0') return DeviceProbeStatus::NameMissing;
        if (name_size > name.size()) return DeviceProbeStatus::NameTooLong;
        out.source_id = SourceIdFromDeviceName(
            std::string_view(name.data(), static_cast<std::size_t>(name_size) - 1));

        RID_DEVICE_INFO info{};
        info.cbSize = sizeof(info);
        UINT info_size = sizeof(info);
        if (GetRawInputDeviceInfoA(handle, RIDI_DEVICEINFO, &info, &info_size) ==
                static_cast<UINT>(-1) ||
            info_size != sizeof(info)) {
            return DeviceProbeStatus::Unavailable;
        }

        switch (info.dwType) {
            case RIM_TYPEKEYBOARD:
                out.keyboard = true;
                return DeviceProbeStatus::Accepted;
            case RIM_TYPEHID:
                out.hid = true;
                out.vendor_id = info.hid.dwVendorId;
                out.product_id = info.hid.dwProductId;
                out.usage_page = info.hid.usUsagePage;
                out.usage = info.hid.usUsage;
                break;
            default:
                return DeviceProbeStatus::UnsupportedType;
        }

        UINT preparsed_size = 0;
        if (GetRawInputDeviceInfoA(handle, RIDI_PREPARSEDDATA, nullptr, &preparsed_size) ==
                static_cast<UINT>(-1) ||
            preparsed_size == 0) {
            return DeviceProbeStatus::Unavailable;
        }
        if (preparsed_size > kHidMaxPreparsedBytes) {
            return DeviceProbeStatus::PreparsedTooLarge;
        }

        alignas(8) std::array<std::byte, kHidMaxPreparsedBytes> preparsed{};
        if (GetRawInputDeviceInfoA(handle, RIDI_PREPARSEDDATA, preparsed.data(),
                                   &preparsed_size) == static_cast<UINT>(-1)) {
            return DeviceProbeStatus::Unavailable;
        }
        const HidDescriptorStatus status = BuildHidDescriptor(
            api_, reinterpret_cast<PHIDP_PREPARSED_DATA>(preparsed.data()),
            out.hid_descriptor);
        if (status != HidDescriptorStatus::Accepted) {
            return DeviceProbeStatus::DescriptorRejected;
        }
        std::memcpy(out.preparsed.data(), preparsed.data(), preparsed_size);
        out.preparsed_size = preparsed_size;
        return DeviceProbeStatus::Accepted;
    }

private:
    const HidApi& api_;
};

} // namespace

std::unique_ptr<DeviceProbe> MakeWindowsDeviceProbe(const HidApi& api) {
    return std::make_unique<WindowsDeviceProbe>(api);
}

} // namespace nosfix
