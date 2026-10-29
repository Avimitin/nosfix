#pragma once

// Synthetic HID descriptors and reports for the input decoder/pipeline tests.
//
// No physical device is touched. A synthetic device is described once, a token
// is stored in the preparsed-data buffer and the fake HidP_* table resolves that
// token back to the description. Reports carry only the report-ID byte; the
// fake returns the preset ON usages / raw values for that exact report ID, so a
// test can prove that a decoder never derives state from another report ID.

#include "nosfix/input_decoder.hpp"

#include <Windows.h>

#include <array>
#include <cstdint>
#include <cstring>
#include <map>
#include <utility>
#include <vector>

namespace synthetic {

struct ButtonCapSpec final {
    USAGE page = 1;
    UCHAR report_id = 0;
    USHORT link_collection = 0;
    bool is_range = false;
    USAGE min_usage = 1;
    USAGE max_usage = 1;
    USHORT data_index_min = 0;
};

struct ValueCapSpec final {
    USAGE page = 1;
    UCHAR report_id = 0;
    USHORT link_collection = 0;
    bool is_range = false;
    USAGE min_usage = 1;
    USAGE max_usage = 1;
    USHORT data_index_min = 0;
    LONG logical_min = 0;
    LONG logical_max = 255;
    USHORT bit_size = 8;
    bool has_null = false;
};

using GroupKey = std::array<unsigned, 3>;     // report id, page, link collection
using ValueKey = std::array<unsigned, 4>;     // report id, page, link collection, usage

struct DeviceSpec final {
    bool keyboard = false;
    std::uint64_t source_id = 0;
    USHORT input_report_bytes = 1;
    USAGE usage_page = 1;
    USAGE usage = 5;
    std::vector<ButtonCapSpec> buttons;
    std::vector<ValueCapSpec> values;
    std::map<GroupKey, std::vector<USAGE>> on_usages;
    std::map<ValueKey, ULONG> raw_values;
    std::map<ValueKey, bool> null_values;

    // One-shot injected failures.
    NTSTATUS next_usages_error = HIDP_STATUS_SUCCESS;
    NTSTATUS next_value_error = HIDP_STATUS_SUCCESS;

    [[nodiscard]] bool uses_report_ids() const noexcept {
        for (const auto& cap : buttons) if (cap.report_id != 0) return true;
        for (const auto& cap : values) if (cap.report_id != 0) return true;
        return false;
    }

    void SetOn(unsigned report_id, USAGE page, USHORT link_collection,
               std::vector<USAGE> usages) {
        on_usages[GroupKey{report_id, page, link_collection}] = std::move(usages);
    }
    void SetRaw(unsigned report_id, USAGE page, USHORT link_collection, USAGE usage_id,
                ULONG raw, bool is_null = false) {
        const ValueKey key{report_id, page, link_collection, usage_id};
        raw_values[key] = raw;
        null_values[key] = is_null;
    }
};

class Registry final {
public:
    Registry() noexcept { active_ = this; }
    ~Registry() { if (active_ == this) active_ = nullptr; }
    Registry(const Registry&) = delete;
    Registry& operator=(const Registry&) = delete;

    [[nodiscard]] std::uint64_t Add(DeviceSpec spec) {
        const std::uint64_t token = next_token_++;
        devices_[token] = std::move(spec);
        return token;
    }

    [[nodiscard]] DeviceSpec& Get(std::uint64_t token) { return devices_.at(token); }
    [[nodiscard]] const DeviceSpec& Get(std::uint64_t token) const { return devices_.at(token); }
    [[nodiscard]] static Registry* Active() noexcept { return active_; }

    [[nodiscard]] static nosfix::HidApi Api() noexcept {
        nosfix::HidApi api;
        api.get_caps = &GetCaps;
        api.get_button_caps = &GetButtonCaps;
        api.get_value_caps = &GetValueCaps;
        api.get_usages = &GetUsages;
        api.get_usage_value = &GetUsageValue;
        return api;
    }

private:
    static std::uint64_t Token(PHIDP_PREPARSED_DATA preparsed) noexcept {
        std::uint64_t token = 0;
        std::memcpy(&token, preparsed, sizeof(token));
        return token;
    }

    static DeviceSpec& Device(PHIDP_PREPARSED_DATA preparsed) {
        return Active()->Get(Token(preparsed));
    }

    static NTSTATUS NTAPI GetCaps(PHIDP_PREPARSED_DATA preparsed, PHIDP_CAPS caps) {
        if (preparsed == nullptr || caps == nullptr) return HIDP_STATUS_INVALID_PREPARSED_DATA;
        const DeviceSpec& device = Device(preparsed);
        *caps = {};
        caps->UsagePage = device.usage_page;
        caps->Usage = device.usage;
        caps->InputReportByteLength = device.input_report_bytes;
        caps->NumberInputButtonCaps = static_cast<USHORT>(device.buttons.size());
        caps->NumberInputValueCaps = static_cast<USHORT>(device.values.size());
        return HIDP_STATUS_SUCCESS;
    }

    static NTSTATUS NTAPI GetButtonCaps(HIDP_REPORT_TYPE type, PHIDP_BUTTON_CAPS caps,
                                        PUSHORT length, PHIDP_PREPARSED_DATA preparsed) {
        if (type != HidP_Input) return HIDP_STATUS_INVALID_REPORT_TYPE;
        if (preparsed == nullptr || caps == nullptr || length == nullptr) {
            return HIDP_STATUS_INVALID_PREPARSED_DATA;
        }
        const DeviceSpec& device = Device(preparsed);
        const USHORT capacity = *length;
        const USHORT count = static_cast<USHORT>(device.buttons.size());
        const USHORT written = capacity < count ? capacity : count;
        for (USHORT i = 0; i < written; ++i) {
            const ButtonCapSpec& spec = device.buttons[i];
            HIDP_BUTTON_CAPS& cap = caps[i];
            cap = {};
            cap.UsagePage = spec.page;
            cap.ReportID = spec.report_id;
            cap.LinkCollection = spec.link_collection;
            cap.IsRange = spec.is_range ? TRUE : FALSE;
            if (spec.is_range) {
                cap.Range.UsageMin = spec.min_usage;
                cap.Range.UsageMax = spec.max_usage;
                cap.Range.DataIndexMin = spec.data_index_min;
                cap.Range.DataIndexMax = static_cast<USHORT>(
                    spec.data_index_min + (spec.max_usage - spec.min_usage));
            } else {
                cap.NotRange.Usage = spec.min_usage;
                cap.NotRange.DataIndex = spec.data_index_min;
            }
        }
        *length = count;
        return capacity < count ? HIDP_STATUS_BUFFER_TOO_SMALL : HIDP_STATUS_SUCCESS;
    }

    static NTSTATUS NTAPI GetValueCaps(HIDP_REPORT_TYPE type, PHIDP_VALUE_CAPS caps,
                                       PUSHORT length, PHIDP_PREPARSED_DATA preparsed) {
        if (type != HidP_Input) return HIDP_STATUS_INVALID_REPORT_TYPE;
        if (preparsed == nullptr || caps == nullptr || length == nullptr) {
            return HIDP_STATUS_INVALID_PREPARSED_DATA;
        }
        const DeviceSpec& device = Device(preparsed);
        const USHORT capacity = *length;
        const USHORT count = static_cast<USHORT>(device.values.size());
        const USHORT written = capacity < count ? capacity : count;
        for (USHORT i = 0; i < written; ++i) {
            const ValueCapSpec& spec = device.values[i];
            HIDP_VALUE_CAPS& cap = caps[i];
            cap = {};
            cap.UsagePage = spec.page;
            cap.ReportID = spec.report_id;
            cap.LinkCollection = spec.link_collection;
            cap.IsRange = spec.is_range ? TRUE : FALSE;
            cap.BitSize = spec.bit_size;
            cap.ReportCount = 1;
            cap.HasNull = spec.has_null ? TRUE : FALSE;
            cap.LogicalMin = spec.logical_min;
            cap.LogicalMax = spec.logical_max;
            if (spec.is_range) {
                cap.Range.UsageMin = spec.min_usage;
                cap.Range.UsageMax = spec.max_usage;
                cap.Range.DataIndexMin = spec.data_index_min;
                cap.Range.DataIndexMax = static_cast<USHORT>(
                    spec.data_index_min + (spec.max_usage - spec.min_usage));
            } else {
                cap.NotRange.Usage = spec.min_usage;
                cap.NotRange.DataIndex = spec.data_index_min;
            }
        }
        *length = count;
        return capacity < count ? HIDP_STATUS_BUFFER_TOO_SMALL : HIDP_STATUS_SUCCESS;
    }

    static bool ReportId(const DeviceSpec& device, const PCHAR report, ULONG report_length,
                         UCHAR& report_id) {
        report_id = 0;
        if (report == nullptr || report_length != device.input_report_bytes) return false;
        if (device.uses_report_ids()) {
            if (report_length < 1) return false;
            report_id = static_cast<UCHAR>(report[0]);
            return report_id != 0;
        }
        return true;
    }

    static NTSTATUS NTAPI GetUsages(HIDP_REPORT_TYPE type, USAGE page, USHORT link_collection,
                                    PUSAGE list, PULONG length,
                                    PHIDP_PREPARSED_DATA preparsed, PCHAR report,
                                    ULONG report_length) {
        if (type != HidP_Input) return HIDP_STATUS_INVALID_REPORT_TYPE;
        if (preparsed == nullptr || length == nullptr) {
            return HIDP_STATUS_INVALID_PREPARSED_DATA;
        }
        DeviceSpec& device = Device(preparsed);
        UCHAR report_id = 0;
        if (!ReportId(device, report, report_length, report_id)) {
            return HIDP_STATUS_INVALID_REPORT_LENGTH;
        }
        bool page_present = false;
        bool page_other_report = false;
        for (const auto& cap : device.buttons) {
            if (cap.page != page || cap.link_collection != link_collection) continue;
            if (cap.report_id == report_id) {
                page_present = true;
            } else {
                page_other_report = true;
            }
        }
        if (!page_present) {
            return page_other_report ? HIDP_STATUS_INCOMPATIBLE_REPORT_ID
                                     : HIDP_STATUS_USAGE_NOT_FOUND;
        }
        if (device.next_usages_error != HIDP_STATUS_SUCCESS) {
            const NTSTATUS error = device.next_usages_error;
            device.next_usages_error = HIDP_STATUS_SUCCESS;
            return error;
        }
        static const std::vector<USAGE> empty;
        const auto it = device.on_usages.find(GroupKey{report_id, page, link_collection});
        const std::vector<USAGE>& source = it == device.on_usages.end() ? empty : it->second;
        const ULONG capacity = *length;
        if (capacity < source.size()) {
            *length = static_cast<ULONG>(source.size());
            return HIDP_STATUS_BUFFER_TOO_SMALL;
        }
        for (std::size_t i = 0; i < source.size(); ++i) list[i] = source[i];
        *length = static_cast<ULONG>(source.size());
        return HIDP_STATUS_SUCCESS;
    }

    static NTSTATUS NTAPI GetUsageValue(HIDP_REPORT_TYPE type, USAGE page,
                                        USHORT link_collection, USAGE usage, PULONG value,
                                        PHIDP_PREPARSED_DATA preparsed, PCHAR report,
                                        ULONG report_length) {
        if (type != HidP_Input) return HIDP_STATUS_INVALID_REPORT_TYPE;
        if (preparsed == nullptr || value == nullptr) {
            return HIDP_STATUS_INVALID_PREPARSED_DATA;
        }
        DeviceSpec& device = Device(preparsed);
        UCHAR report_id = 0;
        if (!ReportId(device, report, report_length, report_id)) {
            return HIDP_STATUS_INVALID_REPORT_LENGTH;
        }
        bool found = false;
        bool other_report = false;
        for (const auto& cap : device.values) {
            if (cap.page != page || cap.link_collection != link_collection) continue;
            const USAGE min = cap.min_usage;
            const USAGE max = cap.is_range ? cap.max_usage : cap.min_usage;
            if (usage < min || usage > max) continue;
            if (cap.report_id == report_id) {
                found = true;
                break;
            }
            other_report = true;
        }
        if (!found) {
            return other_report ? HIDP_STATUS_INCOMPATIBLE_REPORT_ID
                                : HIDP_STATUS_USAGE_NOT_FOUND;
        }
        if (device.next_value_error != HIDP_STATUS_SUCCESS) {
            const NTSTATUS error = device.next_value_error;
            device.next_value_error = HIDP_STATUS_SUCCESS;
            return error;
        }
        const ValueKey key{report_id, page, link_collection, usage};
        const auto null_it = device.null_values.find(key);
        if (null_it != device.null_values.end() && null_it->second) {
            *value = 0;
            return HIDP_STATUS_NULL;
        }
        const auto it = device.raw_values.find(key);
        *value = it == device.raw_values.end() ? 0 : it->second;
        return HIDP_STATUS_SUCCESS;
    }

    inline static Registry* active_ = nullptr;
    std::uint64_t next_token_ = 1;
    std::map<std::uint64_t, DeviceSpec> devices_;
};

class SyntheticProbe final : public nosfix::DeviceProbe {
public:
    struct Entry final {
        std::uint64_t token = 0;
        nosfix::DeviceProbeStatus status = nosfix::DeviceProbeStatus::Accepted;
    };

    explicit SyntheticProbe(Registry& registry) noexcept : registry_(registry) {}

    void Set(std::uint64_t handle, std::uint64_t token,
             nosfix::DeviceProbeStatus status = nosfix::DeviceProbeStatus::Accepted) {
        entries_[handle] = Entry{token, status};
    }
    void Forget(std::uint64_t handle) { entries_.erase(handle); }

    [[nodiscard]] nosfix::DeviceProbeStatus Probe(std::uint64_t handle,
                                                  nosfix::ProbedDevice& out) override {
        ++calls;
        const auto it = entries_.find(handle);
        if (it == entries_.end()) return nosfix::DeviceProbeStatus::Unavailable;
        const Entry& entry = it->second;
        if (entry.status != nosfix::DeviceProbeStatus::Accepted) return entry.status;
        const DeviceSpec& device = registry_.Get(entry.token);
        out = {};
        out.source_id = device.source_id;
        out.keyboard = device.keyboard;
        out.hid = !device.keyboard;
        if (device.keyboard) return nosfix::DeviceProbeStatus::Accepted;
        std::memcpy(out.preparsed.data(), &entry.token, sizeof(entry.token));
        out.preparsed_size = sizeof(entry.token);
        const nosfix::HidDescriptorStatus status = nosfix::BuildHidDescriptor(
            Registry::Api(),
            reinterpret_cast<PHIDP_PREPARSED_DATA>(out.preparsed.data()),
            out.hid_descriptor);
        if (status != nosfix::HidDescriptorStatus::Accepted) {
            return nosfix::DeviceProbeStatus::DescriptorRejected;
        }
        return nosfix::DeviceProbeStatus::Accepted;
    }

    std::uint64_t calls = 0;

private:
    Registry& registry_;
    std::map<std::uint64_t, Entry> entries_;
};

// ---------------------------------------------------------------------------
// Payload builders
// ---------------------------------------------------------------------------

inline std::vector<std::byte> KeyboardPayload(std::uint64_t handle, USHORT vkey,
                                              USHORT scan_code, USHORT flags) {
    constexpr std::size_t size = offsetof(RAWINPUT, data.keyboard) + sizeof(RAWKEYBOARD);
    std::vector<std::byte> bytes(size);
    RAWINPUTHEADER header{};
    header.dwType = RIM_TYPEKEYBOARD;
    header.dwSize = static_cast<DWORD>(size);
    header.hDevice = reinterpret_cast<HANDLE>(static_cast<std::uintptr_t>(handle));
    header.wParam = RIM_INPUT;
    std::memcpy(bytes.data(), &header, sizeof(header));
    RAWKEYBOARD keyboard{};
    keyboard.MakeCode = scan_code;
    keyboard.Flags = flags;
    keyboard.VKey = vkey;
    keyboard.Message = (flags & RI_KEY_BREAK) ? WM_KEYUP : WM_KEYDOWN;
    std::memcpy(bytes.data() + offsetof(RAWINPUT, data.keyboard), &keyboard,
                sizeof(keyboard));
    return bytes;
}

inline std::vector<std::byte> HidPayload(std::uint64_t handle, DWORD report_size,
                                         const std::vector<std::vector<std::byte>>& reports) {
    constexpr std::size_t data_offset = offsetof(RAWINPUT, data.hid.bRawData);
    const std::size_t size = data_offset + report_size * reports.size();
    std::vector<std::byte> bytes(size);
    RAWINPUTHEADER header{};
    header.dwType = RIM_TYPEHID;
    header.dwSize = static_cast<DWORD>(size);
    header.hDevice = reinterpret_cast<HANDLE>(static_cast<std::uintptr_t>(handle));
    header.wParam = RIM_INPUT;
    std::memcpy(bytes.data(), &header, sizeof(header));
    const DWORD count = static_cast<DWORD>(reports.size());
    std::memcpy(bytes.data() + offsetof(RAWINPUT, data.hid.dwSizeHid), &report_size,
                sizeof(report_size));
    std::memcpy(bytes.data() + offsetof(RAWINPUT, data.hid.dwCount), &count,
                sizeof(count));
    for (std::size_t i = 0; i < reports.size(); ++i) {
        std::memcpy(bytes.data() + data_offset + i * report_size, reports[i].data(),
                    report_size);
    }
    return bytes;
}

inline std::vector<std::byte> Report(UCHAR report_id, std::size_t size) {
    std::vector<std::byte> bytes(size, std::byte{0});
    if (size != 0) bytes[0] = static_cast<std::byte>(report_id);
    return bytes;
}

} // namespace synthetic
