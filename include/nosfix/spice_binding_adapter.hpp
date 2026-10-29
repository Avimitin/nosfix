#pragma once

#include "nosfix/input_pipeline.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

namespace nosfix {

// ===========================================================================
// Spice input-configuration adapter (Nostalgia PANB path)
// ===========================================================================
//
// The live config exported from %APPDATA%\spicetools.xml binds the 28 Nostalgia
// keys as ANALOG entries on one HID interface:
//   Key 1..Key 28 -> index 4..31 of \\?\HID#VID_0E8F&PID_1212&MI_04#...
// with deadzone 0, sensitivity 1, multiplier 1, invert false, smoothing off,
// relative off, delay 0. The analog index is Spice's value-cap index, which is
// exactly HidControlInfo::spice_index for a value control.
//
// Spice's acio/panb/panb.cpp packs each key level as
//   min(uint8(level * 15.999), 14)
// into two keys per byte (high nibble first key, low nibble second key) at
// status buffer offset 3. The game's CPianoDevice consumes that level; it is
// not a boolean at 0.5. The adapter therefore binds each key as an analog
// control with the derived PANB press threshold (1/15.999) and exposes the
// continuous level through InputPipeline::AnalogLevels and the press event's
// velocity. A non-default Spice transform is rejected instead of being applied
// incorrectly; the decoder only implements the identity LinearPositive path
// that this configuration uses.

inline constexpr std::size_t kSpiceMaxAnalogBindings = 64;

struct SpiceAnalogBinding final {
    std::string_view name;      // "Key 1".."Key 28" (points into the XML)
    std::string_view device_id; // escaped spelling as it appears in the XML
    std::uint64_t source_id = 0;// hash of the unescaped device path
    std::uint32_t index = 0;    // Spice value-cap index
    std::uint32_t key_number = 0; // 1..28 parsed from the name
    float deadzone = 0.0f;
    float sensitivity = 1.0f;   // XML attribute is spelled "sensivity"
    float multiplier = 1.0f;
    bool invert = false;
    bool relative = false;
    bool smoothing = false;
    std::uint32_t delay_ms = 0;
};

enum class SpiceParseStatus : std::uint8_t {
    Accepted,
    NoAnalogEntries,
    TooManyEntries,
    MalformedTag,
    MalformedAttribute,
    MalformedNumber,
    MalformedDeviceId,
    KeyNumberOutOfRange,
};

// Parses every <analog .../> element. `xml` must outlive the result because the
// views point into it. This is a focused parser for the exported Spice format,
// not a general XML implementation; unknown attributes are ignored.
[[nodiscard]] SpiceParseStatus ParseSpiceAnalogBindings(
    std::string_view xml, std::span<SpiceAnalogBinding> out,
    std::size_t& count) noexcept;

enum class SpiceBindingStatus : std::uint8_t {
    Accepted,
    NoAnalogs,
    TooManyBindings,
    UnsupportedTransform, // non-default deadzone/sensitivity/multiplier/invert/etc.
    UnknownValueIndex,    // descriptor has no value control with that spice index
    AmbiguousValueIndex,  // a range value cap: the adapter needs the usage
    NotAValueControl,
    DuplicateLane,
    DuplicateControl,
    WrongDevice,
};

// Builds explicit PipelineBindingSpec rows for the 28 keys. Lanes come from the
// parsed key number (Key N -> lane N-1), never from list position. The device
// must be enrolled so its descriptor is available; no control is guessed.
[[nodiscard]] SpiceBindingStatus BuildNostalgiaKeyBindings(
    std::span<const SpiceAnalogBinding> analogs, const HidDescriptor& descriptor,
    std::uint64_t source_id, std::uint64_t device_generation,
    std::span<PipelineBindingSpec> out, std::size_t& count) noexcept;

// Exposed for tests: hashes the unescaped device path exactly like
// SourceIdFromDeviceName. Returns 0 if the path contains an unsupported escape
// or is longer than the fixed decode buffer.
[[nodiscard]] std::uint64_t SourceIdFromEscapedDeviceName(
    std::string_view escaped) noexcept;

} // namespace nosfix
