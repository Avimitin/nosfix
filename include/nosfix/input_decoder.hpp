#pragma once

#include <Windows.h>
#include <hidsdi.h> // brings hidusage.h and hidpi.h in the required order

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string_view>

namespace nosfix {

// ===========================================================================
// Decoded physical controls: keyboard scan/VKey identity, HID descriptor model,
// button/value report decoding and stable device identity.
// ===========================================================================
//
// This layer only turns a captured RAWINPUT payload into explicit physical
// control transitions. It never maps a control to a game lane; that is the
// pipeline's validated binding table. It never guesses a usage or a report ID:
// a control that the descriptor does not describe cannot be produced.
//
// All functions here are called on the consumer (game/plugin) thread. Nothing
// in this file allocates inside a decode call or touches the input callback.
//
// Report-ID rule
// --------------
// A report carries the controls of exactly one report ID (or of report ID 0
// when the descriptor uses no report IDs). Controls belonging to other report
// IDs are left untouched. A report whose ID is unknown, whose length does not
// match the descriptor, or whose HidP queries fail is unsupported: the caller
// must fail closed (reset held state), never treat the absent usages of another
// report as releases.

// ---------------------------------------------------------------------------
// Bounds (fixed, no growth on any input path)
// ---------------------------------------------------------------------------

inline constexpr std::size_t kHidMaxCaps = 128;            // caps per descriptor/report type
inline constexpr std::size_t kHidMaxControlCount = 256;    // expanded controls per device
inline constexpr std::size_t kHidMaxReportIds = 32;        // distinct input report IDs
inline constexpr std::size_t kHidMaxButtonGroups = 64;     // (report id, page, link collection)
inline constexpr std::size_t kHidMaxReportBytes = 512;     // one HID report, ID byte included
inline constexpr std::size_t kHidMaxReportsPerPacket = 64; // RAWHID::dwCount bound
inline constexpr std::size_t kHidMaxPreparsedBytes = 8192;  // RIDI_PREPARSEDDATA bound
inline constexpr std::size_t kKeyboardControlCount = 768;   // (E0|E1)*256 + VKey

// Control-id domains. They are disjoint so a binding can never be interpreted
// as belonging to a different control class.
inline constexpr std::uint32_t kHidButtonControlBase = 1U << 16; // 0x00010000
inline constexpr std::uint32_t kHidValueControlBase = 2U << 16;  // 0x00020000
inline constexpr std::uint32_t kKeyboardControlBase = 3U << 16;   // 0x00030000

// ---------------------------------------------------------------------------
// HidP_* seam
// ---------------------------------------------------------------------------
// Production uses HidApi::Windows(). Tests install a synthetic table so
// descriptor/report-ID behaviour is exercised without a physical device.

struct HidApi final {
    NTSTATUS(NTAPI* get_caps)(PHIDP_PREPARSED_DATA, PHIDP_CAPS) = nullptr;
    NTSTATUS(NTAPI* get_button_caps)(HIDP_REPORT_TYPE, PHIDP_BUTTON_CAPS, PUSHORT,
                                     PHIDP_PREPARSED_DATA) = nullptr;
    NTSTATUS(NTAPI* get_value_caps)(HIDP_REPORT_TYPE, PHIDP_VALUE_CAPS, PUSHORT,
                                    PHIDP_PREPARSED_DATA) = nullptr;
    NTSTATUS(NTAPI* get_usages)(HIDP_REPORT_TYPE, USAGE, USHORT, PUSAGE, PULONG,
                                PHIDP_PREPARSED_DATA, PCHAR, ULONG) = nullptr;
    NTSTATUS(NTAPI* get_usage_value)(HIDP_REPORT_TYPE, USAGE, USHORT, USAGE, PULONG,
                                     PHIDP_PREPARSED_DATA, PCHAR, ULONG) = nullptr;

    [[nodiscard]] bool Complete() const noexcept;
    [[nodiscard]] static const HidApi& Windows() noexcept;
};

// ---------------------------------------------------------------------------
// HID descriptor model
// ---------------------------------------------------------------------------

enum class InputControlKind : std::uint8_t { Button, Value };

struct HidControlInfo final {
    std::uint32_t control_id = 0;
    // cfg compatibility: for a button this is Spice's flat button index over
    // HidP_GetButtonCaps order (ranges expanded); for a value it is Spice's
    // value-cap index (range caps are not expanded by Spice).
    std::uint32_t spice_index = 0;
    std::uint16_t report_id = 0; // 0 only when the descriptor uses no report IDs
    std::uint16_t usage_page = 0;
    std::uint16_t usage = 0;
    std::uint16_t link_collection = 0;
    std::uint16_t data_index = 0;
    InputControlKind kind = InputControlKind::Button;

    // Value controls only; zero for buttons.
    std::int32_t logical_min = 0;
    std::int32_t logical_max = 0;
    std::uint16_t bit_size = 0;
    bool has_null_state = false;
    bool value_range_cap = false;
};

struct HidButtonGroup final {
    std::uint16_t report_id = 0;
    std::uint16_t usage_page = 0;
    std::uint16_t link_collection = 0;
    std::uint16_t first_control = 0; // into HidDescriptor::controls
    std::uint16_t control_count = 0;
};

enum class HidDescriptorStatus : std::uint8_t {
    Accepted,
    RejectedCaps,               // HidP_GetCaps failed or impossible caps
    RejectedNoInputControls,    // nothing can ever be bound
    RejectedTooManyCaps,        // cap count above kHidMaxCaps
    RejectedTooManyControls,    // expanded control count above kHidMaxControlCount
    RejectedDuplicateControl,   // same (report id, page, link collection, usage)
    RejectedMixedReportIds,     // report id 0 mixed with non-zero report ids
    RejectedTooManyReportIds,   // above kHidMaxReportIds
    RejectedTooManyGroups,      // above kHidMaxButtonGroups
    RejectedReportLength,       // InputReportByteLength 0 or above kHidMaxReportBytes
    RejectedInvalidValueRange,  // LogicalMin >= LogicalMax, or BitSize outside 1..32
    RejectedAmbiguousLinkCollection, // page has both link collection 0 and non-zero
    RejectedBadCapRange,        // negative/overflowing usage range
};

struct HidDescriptor final {
    HidDescriptorStatus status = HidDescriptorStatus::RejectedCaps;
    std::uint16_t input_report_bytes = 0; // includes the report-ID byte when used
    bool uses_report_ids = false;
    std::size_t control_count = 0;
    std::size_t button_control_count = 0; // buttons are [0, button_control_count)
    std::size_t button_group_count = 0;
    std::array<HidControlInfo, kHidMaxControlCount> controls{};
    std::array<HidButtonGroup, kHidMaxButtonGroups> button_groups{};

    [[nodiscard]] bool Accepted() const noexcept {
        return status == HidDescriptorStatus::Accepted;
    }
    [[nodiscard]] const HidControlInfo* FindControl(std::uint32_t control_id) const noexcept;
    [[nodiscard]] const HidControlInfo* FindSpiceButton(std::uint32_t spice_index) const noexcept;
    // A Spice value cap index can cover a usage range; the caller must name the
    // usage it means. Returns null when absent or still ambiguous.
    [[nodiscard]] const HidControlInfo* FindSpiceValue(std::uint32_t spice_index,
                                                       std::uint16_t usage) const noexcept;
};

// Fills `out` completely; never partially validates. `preparsed` is opaque and
// only passed back to the HidP functions in `api`.
[[nodiscard]] HidDescriptorStatus BuildHidDescriptor(const HidApi& api,
                                                     PHIDP_PREPARSED_DATA preparsed,
                                                     HidDescriptor& out) noexcept;

// ---------------------------------------------------------------------------
// Spice PANB velocity domain
// ---------------------------------------------------------------------------
// The Nostalgia PANB module quantizes each analog key as
//   level = min(uint8(normalized * 15.999), 14)
// and stores two keys per byte (high nibble = even key, low = odd). 15 is
// deliberately excluded because it does not register as a hard press in
// recital mode. The decoder preserves the continuous normalized level; this
// helper is the exact PANB quantization used by Spice acio/panb/panb.cpp.
inline constexpr float kPanbVelocityScale = 15.999f;
inline constexpr std::uint8_t kPanbMaxVelocity = 14;
// Smallest normalized level whose PANB nibble is 1. This is the derived digital
// press threshold for an analog key (no invented 0.5).
inline constexpr float kPanbPressThreshold = 1.0f / kPanbVelocityScale;

[[nodiscard]] inline std::uint8_t PanbVelocity(float normalized) noexcept {
    if (!(normalized > 0.0f)) return 0;
    if (normalized >= 1.0f) return kPanbMaxVelocity;
    const auto value = static_cast<std::uint8_t>(normalized * kPanbVelocityScale);
    return value > kPanbMaxVelocity ? kPanbMaxVelocity : value;
}

// One bound value control's current level for one report. The pipeline keeps
// the latest observation per lane; the press event itself carries the exact
// level and is never sampled at render time.
struct AnalogObservation final {
    std::uint32_t control_id = 0;
    float level = 0.0f;          // normalized [0,1], invert already applied
    std::uint8_t panb_velocity = 0;
};

// ---------------------------------------------------------------------------
// Payload views and identity helpers
// ---------------------------------------------------------------------------

struct HidReportView final {
    std::span<const std::byte> bytes;
};

// Validates the RAWINPUT header, the RAWHID header and the report bounds.
// `report_bytes` is dwSizeHid (the same for every report in the packet).
[[nodiscard]] bool ViewHidReports(std::span<const std::byte> payload,
                                  std::span<HidReportView> reports,
                                  std::size_t& report_count,
                                  std::uint32_t& report_bytes) noexcept;

// Validates the RAWINPUT header and copies the keyboard record.
[[nodiscard]] bool ViewKeyboard(std::span<const std::byte> payload,
                                RAWKEYBOARD& keyboard) noexcept;

// Spice-compatible keyboard index: (E0 ? 256 : 0) + (E1 ? 512 : 0) + VKey,
// with Spice's Control/Shift normalisation. Returns 0 for an unusable record.
[[nodiscard]] std::uint32_t MakeKeyboardControlId(const RAWKEYBOARD& keyboard) noexcept;

[[nodiscard]] constexpr std::uint32_t MakeKeyboardControlId(std::uint32_t spice_index) noexcept {
    return kKeyboardControlBase + spice_index;
}
[[nodiscard]] constexpr std::uint32_t MakeHidButtonControlId(std::uint32_t spice_index) noexcept {
    return kHidButtonControlBase + spice_index + 1; // +1 keeps every id non-zero
}
[[nodiscard]] constexpr std::uint32_t MakeHidValueControlId(std::uint32_t expanded_index) noexcept {
    return kHidValueControlBase + expanded_index + 1;
}
[[nodiscard]] constexpr bool IsKeyboardControlId(std::uint32_t control_id) noexcept {
    return control_id >= kKeyboardControlBase &&
           control_id < kKeyboardControlBase + kKeyboardControlCount;
}
[[nodiscard]] constexpr bool IsHidControlId(std::uint32_t control_id) noexcept {
    return control_id >= kHidButtonControlBase && control_id < kKeyboardControlBase;
}

// Stable 64-bit identity for the raw-input device interface path. This is the
// same ANSI string Spice stores in rawinput::Device::name (RIDI_DEVICENAME via
// the A entry point), so a configuration adapter can hash the configured device
// identifier and obtain the very same source_id. Never returns 0.
[[nodiscard]] std::uint64_t SourceIdFromDeviceName(std::string_view ansi_device_name) noexcept;

// ---------------------------------------------------------------------------
// Decoded transitions and per-device state
// ---------------------------------------------------------------------------

struct ControlTransition final {
    std::uint32_t control_id = 0;
    bool down = false;
    float velocity = 0.0f; // [0,1]; 1.0 for a digital button press, 0 for release
};

enum class HidDecodeStatus : std::uint8_t {
    Decoded,           // every control of this report was evaluated
    UnsupportedReport, // unknown report ID, bad length, HidP failure -> fail closed
    OutputTooSmall,    // `out` cannot hold every transition; nothing was written
    MalformedArgument, // null/short descriptor/analog table, or bad report span
};

// Explicit analog binding for one control. `bound == false` means a digital
// button; a value control that is not bound here is never polled.
struct HidAnalogConfig final {
    bool bound = false;
    float press_threshold = 0.0f;   // press when normalized >= press_threshold
    float release_threshold = 0.0f; // release when normalized <= release_threshold
    bool invert = false;            // normalized = 1 - normalized
};

class HidControlState;

// Decodes one HID report (exactly dwSizeHid bytes, report-ID byte included when
// the descriptor uses report IDs). `analog` must have descriptor.control_count
// entries; only bound value controls are read. On OutputTooSmall nothing is
// written and nothing changes, so the identical call can be retried with a
// larger span. On UnsupportedReport the state is unchanged and the caller must
// fail closed for the device.
[[nodiscard]] HidDecodeStatus DecodeHidReport(const HidApi& api,
                                              PHIDP_PREPARSED_DATA preparsed,
                                              const HidDescriptor& descriptor,
                                              HidControlState& state,
                                              std::span<const HidAnalogConfig> analog,
                                              std::span<const std::byte> report,
                                              std::span<ControlTransition> out,
                                              std::size_t& written,
                                              std::span<AnalogObservation> observations = {},
                                              std::size_t* observation_count = nullptr) noexcept;

class HidControlState final {
public:
    HidControlState() noexcept = default;

    // Returns every control to "up" and forgets last values. Used on stream
    // loss, device loss and descriptor replacement, never to invent a report.
    void Reset() noexcept;

    [[nodiscard]] bool IsDownByIndex(std::size_t control_index) const noexcept;

private:
    friend HidDecodeStatus DecodeHidReport(const HidApi&, PHIDP_PREPARSED_DATA,
                                           const HidDescriptor&, HidControlState&,
                                           std::span<const HidAnalogConfig>,
                                           std::span<const std::byte>,
                                           std::span<ControlTransition>,
                                           std::size_t&,
                                           std::span<AnalogObservation>,
                                           std::size_t*) noexcept;

    std::array<std::uint8_t, kHidMaxControlCount> down_{};
    std::array<std::uint8_t, kHidMaxControlCount> analog_down_{};
    std::array<float, kHidMaxControlCount> value_{};
};

class KeyboardControlState final {
public:
    KeyboardControlState() noexcept = default;

    void Reset() noexcept;
    [[nodiscard]] bool IsDown(std::uint32_t control_id) const noexcept;

    // Applies one RAWINPUT keyboard record. Returns true and fills `out` only
    // when the record is a new physical edge. A typematic make (key already
    // down), an unmatched break, an out-of-range VKey or a non-key record
    // produces no transition. `out.control_id == 0` also means "no edge".
    [[nodiscard]] bool Apply(const RAWKEYBOARD& keyboard, ControlTransition& out) noexcept;

private:
    static constexpr std::size_t kWords = (kKeyboardControlCount + 63) / 64;
    std::array<std::uint64_t, kWords> down_{};
};

// Normalizes a raw value against its logical range. Signed values are sign
// extended from `bit_size` (HidP returns raw bits in a ULONG). Returns false for
// an invalid range, a null value or a non-finite result.
[[nodiscard]] bool NormalizeHidValue(std::int32_t logical_min,
                                     std::int32_t logical_max,
                                     std::uint16_t bit_size,
                                     ULONG raw_value,
                                     double& normalized) noexcept;

// ---------------------------------------------------------------------------
// Device probing (consumer thread only; may call Win32, may allocate once)
// ---------------------------------------------------------------------------

enum class DeviceProbeStatus : std::uint8_t {
    Accepted,
    Unavailable,        // the captured device handle can no longer be queried
    UnsupportedType,    // mouse or a non-keyboard/non-HID record
    NameMissing,        // RIDI_DEVICENAME failed or empty
    NameTooLong,        // path above the fixed probe buffer
    PreparsedTooLarge,  // descriptor above the fixed probe buffer
    DescriptorRejected, // HidDescriptorStatus is not Accepted
};

struct ProbedDevice final {
    std::uint64_t source_id = 0;
    // Opaque HidP preparsed data retained for report decoding. DecodeHidReport
    // receives a pointer to this storage; for a keyboard it stays empty. Kept
    // first (after the 8-byte id) so the 8-byte alignment costs no padding.
    alignas(8) std::array<std::byte, kHidMaxPreparsedBytes> preparsed{};
    std::uint32_t preparsed_size = 0;
    std::uint32_t vendor_id = 0;
    std::uint32_t product_id = 0;
    std::uint16_t usage_page = 0;
    std::uint16_t usage = 0;
    bool keyboard = false;
    bool hid = false;
    HidDescriptor hid_descriptor{};
};

class DeviceProbe {
public:
    virtual ~DeviceProbe() = default;
    [[nodiscard]] virtual DeviceProbeStatus Probe(std::uint64_t raw_handle,
                                                  ProbedDevice& out) = 0;
};

// Real Win32 probe. Uses GetRawInputDeviceInfoA(RIDI_DEVICENAME),
// RIDI_DEVICEINFO and, for HID, RIDI_PREPARSEDDATA through `api`.
[[nodiscard]] std::unique_ptr<DeviceProbe> MakeWindowsDeviceProbe(
    const HidApi& api = HidApi::Windows());

} // namespace nosfix
