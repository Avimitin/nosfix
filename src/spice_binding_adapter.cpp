#include "nosfix/spice_binding_adapter.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <cstring>

namespace nosfix {
namespace {

constexpr std::size_t kDeviceNameDecodeCapacity = 512;

[[nodiscard]] bool IsSpace(char c) noexcept {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

[[nodiscard]] bool FindTagEnd(std::string_view xml, std::size_t start,
                              std::size_t& end) noexcept {
    const std::size_t found = xml.find('>', start);
    if (found == std::string_view::npos) return false;
    end = found;
    return true;
}

[[nodiscard]] bool FindAttribute(std::string_view tag, std::string_view name,
                                 std::string_view& value) noexcept {
    std::size_t position = 0;
    while (position < tag.size()) {
        position = tag.find(name, position);
        if (position == std::string_view::npos) return false;
        // Name boundary: preceded by whitespace, followed by optional spaces,
        // '=' and a quote.
        const bool left_ok = position != 0 && IsSpace(tag[position - 1]);
        std::size_t cursor = position + name.size();
        while (cursor < tag.size() && IsSpace(tag[cursor])) ++cursor;
        if (!left_ok || cursor >= tag.size() || tag[cursor] != '=') {
            position += name.size();
            continue;
        }
        ++cursor;
        while (cursor < tag.size() && IsSpace(tag[cursor])) ++cursor;
        if (cursor >= tag.size() || tag[cursor] != '"') {
            position += name.size();
            continue;
        }
        const std::size_t value_start = cursor + 1;
        const std::size_t value_end = tag.find('"', value_start);
        if (value_end == std::string_view::npos) return false;
        value = tag.substr(value_start, value_end - value_start);
        return true;
    }
    return false;
}

[[nodiscard]] bool ParseUInt(std::string_view text, std::uint32_t& value) noexcept {
    if (text.empty()) return false;
    std::uint32_t parsed = 0;
    const auto result = std::from_chars(text.data(), text.data() + text.size(), parsed);
    if (result.ec != std::errc{} || result.ptr != text.data() + text.size()) return false;
    value = parsed;
    return true;
}

[[nodiscard]] bool ParseFloat(std::string_view text, float& value) noexcept {
    if (text.empty()) return false;
    float parsed = 0.0f;
    const auto result = std::from_chars(text.data(), text.data() + text.size(), parsed);
    if (result.ec != std::errc{} || result.ptr != text.data() + text.size()) return false;
    value = parsed;
    return true;
}

[[nodiscard]] bool ParseBool(std::string_view text, bool& value) noexcept {
    if (text == "true") {
        value = true;
        return true;
    }
    if (text == "false") {
        value = false;
        return true;
    }
    return false;
}

[[nodiscard]] bool ParseKeyNumber(std::string_view name, std::uint32_t& key) noexcept {
    constexpr std::string_view prefix = "Key ";
    if (name.size() <= prefix.size() || name.substr(0, prefix.size()) != prefix) {
        return false;
    }
    std::uint32_t parsed = 0;
    const auto result = std::from_chars(name.data() + prefix.size(),
                                        name.data() + name.size(), parsed);
    if (result.ec != std::errc{} || result.ptr != name.data() + name.size()) {
        return false;
    }
    if (parsed < 1 || parsed > 28) return false;
    key = parsed;
    return true;
}

[[nodiscard]] bool DecodeEscaped(std::string_view escaped,
                                 std::array<char, kDeviceNameDecodeCapacity>& out,
                                 std::size_t& length) noexcept {
    length = 0;
    for (std::size_t i = 0; i < escaped.size();) {
        char c = escaped[i];
        if (c == '&') {
            static constexpr std::array<std::pair<std::string_view, char>, 5> kEntities{{
                {"&amp;", '&'}, {"&lt;", '<'}, {"&gt;", '>'},
                {"&quot;", '"'}, {"&apos;", '\''},
            }};
            bool matched = false;
            for (const auto& [entity, replacement] : kEntities) {
                if (escaped.substr(i, entity.size()) != entity) continue;
                if (length >= out.size()) return false;
                out[length++] = replacement;
                i += entity.size();
                matched = true;
                break;
            }
            if (!matched) return false;
            continue;
        }
        if (length >= out.size()) return false;
        out[length++] = c;
        ++i;
    }
    return true;
}

} // namespace

std::uint64_t SourceIdFromEscapedDeviceName(std::string_view escaped) noexcept {
    std::array<char, kDeviceNameDecodeCapacity> decoded{};
    std::size_t length = 0;
    if (!DecodeEscaped(escaped, decoded, length)) return 0;
    return SourceIdFromDeviceName(
        std::string_view(decoded.data(), static_cast<std::size_t>(length)));
}

SpiceParseStatus ParseSpiceAnalogBindings(std::string_view xml,
                                          std::span<SpiceAnalogBinding> out,
                                          std::size_t& count) noexcept {
    count = 0;
    std::size_t position = 0;
    while ((position = xml.find("<analog", position)) != std::string_view::npos) {
        const std::size_t after_name = position + 7; // "<analog"
        if (after_name < xml.size() && !IsSpace(xml[after_name]) &&
            xml[after_name] != '>' && xml[after_name] != '/') {
            position = after_name;
            continue;
        }
        const std::size_t tag_start = position;
        std::size_t tag_end = 0;
        if (!FindTagEnd(xml, tag_start, tag_end)) return SpiceParseStatus::MalformedTag;
        const std::string_view tag = xml.substr(tag_start, tag_end - tag_start + 1);
        position = tag_end + 1;

        SpiceAnalogBinding binding;
        std::string_view value;
        if (!FindAttribute(tag, "name", value)) return SpiceParseStatus::MalformedAttribute;
        binding.name = value;
        if (!ParseKeyNumber(binding.name, binding.key_number)) {
            return SpiceParseStatus::KeyNumberOutOfRange;
        }
        if (!FindAttribute(tag, "index", value) ||
            !ParseUInt(value, binding.index)) {
            return SpiceParseStatus::MalformedNumber;
        }
        if (!FindAttribute(tag, "devid", value)) {
            return SpiceParseStatus::MalformedAttribute;
        }
        binding.device_id = value;
        binding.source_id = SourceIdFromEscapedDeviceName(value);
        if (binding.source_id == 0) return SpiceParseStatus::MalformedDeviceId;

        if (FindAttribute(tag, "deadzone", value) &&
            !ParseFloat(value, binding.deadzone)) {
            return SpiceParseStatus::MalformedNumber;
        }
        if (FindAttribute(tag, "sensivity", value) &&
            !ParseFloat(value, binding.sensitivity)) {
            return SpiceParseStatus::MalformedNumber;
        }
        if (FindAttribute(tag, "multiplier", value) &&
            !ParseFloat(value, binding.multiplier)) {
            return SpiceParseStatus::MalformedNumber;
        }
        if (FindAttribute(tag, "invert", value) && !ParseBool(value, binding.invert)) {
            return SpiceParseStatus::MalformedNumber;
        }
        if (FindAttribute(tag, "relative", value) &&
            !ParseBool(value, binding.relative)) {
            return SpiceParseStatus::MalformedNumber;
        }
        if (FindAttribute(tag, "smoothing", value) &&
            !ParseBool(value, binding.smoothing)) {
            return SpiceParseStatus::MalformedNumber;
        }
        if (FindAttribute(tag, "delay_ms", value) &&
            !ParseUInt(value, binding.delay_ms)) {
            return SpiceParseStatus::MalformedNumber;
        }

        if (count >= out.size()) return SpiceParseStatus::TooManyEntries;
        out[count++] = binding;
    }
    if (count == 0) return SpiceParseStatus::NoAnalogEntries;
    return SpiceParseStatus::Accepted;
}

SpiceBindingStatus BuildNostalgiaKeyBindings(
    std::span<const SpiceAnalogBinding> analogs, const HidDescriptor& descriptor,
    std::uint64_t source_id, std::uint64_t device_generation,
    std::span<PipelineBindingSpec> out, std::size_t& count) noexcept {
    count = 0;
    if (analogs.empty()) return SpiceBindingStatus::NoAnalogs;
    if (!descriptor.Accepted() || device_generation == 0 || source_id == 0) {
        return SpiceBindingStatus::UnknownValueIndex;
    }

    std::array<std::uint8_t, kInputLaneCount> used_lanes{};
    std::array<std::uint32_t, kSpiceMaxAnalogBindings> used_controls{};
    std::size_t used_control_count = 0;

    for (const SpiceAnalogBinding& analog : analogs) {
        // Only the exact identity transform is implemented. Applying a
        // deadzone/sensitivity/multiplier here without matching Spice's full
        // Analog::getState path would change the PANB velocity.
        if (analog.deadzone != 0.0f || analog.sensitivity != 1.0f ||
            analog.multiplier != 1.0f || analog.invert || analog.relative ||
            analog.smoothing || analog.delay_ms != 0) {
            return SpiceBindingStatus::UnsupportedTransform;
        }
        if (analog.source_id != source_id) return SpiceBindingStatus::WrongDevice;
        if (analog.key_number < 1 || analog.key_number > kInputLaneCount) {
            return SpiceBindingStatus::WrongDevice;
        }
        const std::uint8_t lane = static_cast<std::uint8_t>(analog.key_number - 1);
        if (lane >= kInputLaneCount || used_lanes[lane] != 0) {
            return SpiceBindingStatus::DuplicateLane;
        }

        const HidControlInfo* control = descriptor.FindSpiceValue(analog.index, 0);
        if (control == nullptr) {
            bool any = false;
            bool multiple = false;
            for (std::size_t i = 0; i < descriptor.control_count; ++i) {
                const HidControlInfo& candidate = descriptor.controls[i];
                if (candidate.kind != InputControlKind::Value ||
                    candidate.spice_index != analog.index) {
                    continue;
                }
                if (any) multiple = true;
                any = true;
            }
            return multiple ? SpiceBindingStatus::AmbiguousValueIndex
                            : SpiceBindingStatus::UnknownValueIndex;
        }
        if (control->kind != InputControlKind::Value) {
            return SpiceBindingStatus::NotAValueControl;
        }
        for (std::size_t i = 0; i < used_control_count; ++i) {
            if (used_controls[i] == control->control_id) {
                return SpiceBindingStatus::DuplicateControl;
            }
        }
        if (count >= out.size()) return SpiceBindingStatus::TooManyBindings;

        PipelineBindingSpec& spec = out[count++];
        spec = {};
        spec.source_id = source_id;
        spec.control_id = control->control_id;
        spec.lane = lane;
        spec.kind = InputControlKind::Value;
        // PANB stores min(uint8(level * 15.999), 14); a nibble of 1 is the
        // first pressed level. This is derived, not an invented 0.5.
        spec.press_threshold = kPanbPressThreshold;
        spec.release_threshold = kPanbPressThreshold;
        spec.invert = false;

        used_lanes[lane] = 1;
        if (used_control_count < used_controls.size()) {
            used_controls[used_control_count++] = control->control_id;
        }
    }
    return SpiceBindingStatus::Accepted;
}

} // namespace nosfix
