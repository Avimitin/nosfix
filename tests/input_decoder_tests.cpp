// Always-on Release tests for nosfix::input_decoder.
//
// No physical device is touched: descriptors and reports are synthetic (see
// synthetic_hid.hpp). Every check is counted, not <cassert>, because the parent
// Release build defines NDEBUG.

#include "synthetic_hid.hpp"

#include "nosfix/input_decoder.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>
#include <span>
#include <vector>

namespace allocation_probe {
inline bool tracking = false;
inline std::uint64_t count = 0;
} // namespace allocation_probe

void* operator new(std::size_t size) {
    if (allocation_probe::tracking) ++allocation_probe::count;
    void* memory = std::malloc(size == 0 ? 1 : size);
    if (memory == nullptr) throw std::bad_alloc();
    return memory;
}
void* operator new[](std::size_t size) { return ::operator new(size); }
void operator delete(void* memory) noexcept { std::free(memory); }
void operator delete(void* memory, std::size_t) noexcept { std::free(memory); }
void operator delete[](void* memory) noexcept { std::free(memory); }
void operator delete[](void* memory, std::size_t) noexcept { std::free(memory); }

namespace {

int g_checks = 0;
int g_failures = 0;

#define NOSFIX_CHECK(condition)                                                   \
    do {                                                                          \
        ++g_checks;                                                               \
        if (!(condition)) {                                                       \
            ++g_failures;                                                         \
            std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition);      \
        }                                                                         \
    } while (false)

using namespace nosfix;
using synthetic::ButtonCapSpec;
using synthetic::DeviceSpec;
using synthetic::Registry;
using synthetic::ValueCapSpec;

PHIDP_PREPARSED_DATA AsPreparsed(const std::uint64_t* token) {
    return reinterpret_cast<PHIDP_PREPARSED_DATA>(const_cast<std::uint64_t*>(token));
}

DeviceSpec ButtonDevice() {
    DeviceSpec spec;
    spec.source_id = 0x1111'2222'3333'4444ULL;
    spec.input_report_bytes = 3;
    spec.buttons = {
        ButtonCapSpec{1, 0, 0, true, 1, 4, 0},
        ButtonCapSpec{1, 0, 0, false, 9, 9, 4},
    };
    spec.values = {
        ValueCapSpec{1, 0, 0, false, 0x30, 0x30, 3, 0, 255, 8, false},
    };
    return spec;
}

void TestDescriptorBasics() {
    Registry registry;
    const std::uint64_t token = registry.Add(ButtonDevice());
    HidDescriptor descriptor;
    const auto status = BuildHidDescriptor(Registry::Api(), AsPreparsed(&token), descriptor);
    NOSFIX_CHECK(status == HidDescriptorStatus::Accepted);
    NOSFIX_CHECK(descriptor.Accepted());
    NOSFIX_CHECK(!descriptor.uses_report_ids);
    NOSFIX_CHECK(descriptor.input_report_bytes == 3);
    NOSFIX_CHECK(descriptor.control_count == 6);
    NOSFIX_CHECK(descriptor.button_control_count == 5);
    NOSFIX_CHECK(descriptor.button_group_count == 1);
    NOSFIX_CHECK(descriptor.button_groups[0].first_control == 0);
    NOSFIX_CHECK(descriptor.button_groups[0].control_count == 5);
    for (std::size_t i = 0; i < 5; ++i) {
        NOSFIX_CHECK(descriptor.controls[i].kind == InputControlKind::Button);
        NOSFIX_CHECK(descriptor.controls[i].control_id == MakeHidButtonControlId(
            static_cast<std::uint32_t>(i)));
        NOSFIX_CHECK(descriptor.controls[i].control_id != 0);
        NOSFIX_CHECK(descriptor.controls[i].spice_index == i);
        NOSFIX_CHECK(descriptor.controls[i].report_id == 0);
    }
    // Usage-sorted inside a group.
    NOSFIX_CHECK(descriptor.controls[0].usage == 1);
    NOSFIX_CHECK(descriptor.controls[3].usage == 4);
    NOSFIX_CHECK(descriptor.controls[4].usage == 9);
    NOSFIX_CHECK(descriptor.controls[5].kind == InputControlKind::Value);
    NOSFIX_CHECK(descriptor.controls[5].control_id == MakeHidValueControlId(0));
    NOSFIX_CHECK(descriptor.controls[5].spice_index == 0);
    NOSFIX_CHECK(descriptor.controls[5].logical_min == 0);
    NOSFIX_CHECK(descriptor.controls[5].logical_max == 255);
    NOSFIX_CHECK(descriptor.controls[5].bit_size == 8);
    NOSFIX_CHECK(descriptor.FindControl(MakeHidButtonControlId(2)) != nullptr);
    NOSFIX_CHECK(descriptor.FindControl(MakeHidValueControlId(0)) != nullptr);
    NOSFIX_CHECK(descriptor.FindControl(0xDEAD'BEEF) == nullptr);
    NOSFIX_CHECK(descriptor.FindSpiceButton(4) != nullptr);
    NOSFIX_CHECK(descriptor.FindSpiceButton(4)->usage == 9);
    NOSFIX_CHECK(descriptor.FindSpiceValue(0, 0x30) != nullptr);
    NOSFIX_CHECK(descriptor.FindSpiceValue(1, 0x30) == nullptr);
}

void TestMultiReportDescriptor() {
    Registry registry;
    DeviceSpec spec;
    spec.source_id = 0x9999;
    spec.input_report_bytes = 5;
    spec.buttons = {
        ButtonCapSpec{1, 1, 0, true, 1, 2, 0},
        ButtonCapSpec{1, 2, 0, true, 1, 2, 0},
    };
    spec.values = {
        ValueCapSpec{1, 2, 0, false, 0x30, 0x30, 0, 0, 255, 8, false},
    };
    const std::uint64_t token = registry.Add(spec);
    HidDescriptor descriptor;
    NOSFIX_CHECK(BuildHidDescriptor(Registry::Api(), AsPreparsed(&token), descriptor) ==
                 HidDescriptorStatus::Accepted);
    NOSFIX_CHECK(descriptor.uses_report_ids);
    NOSFIX_CHECK(descriptor.input_report_bytes == 5);
    NOSFIX_CHECK(descriptor.control_count == 5);
    NOSFIX_CHECK(descriptor.button_control_count == 4);
    NOSFIX_CHECK(descriptor.button_group_count == 2);
    NOSFIX_CHECK(descriptor.button_groups[0].report_id == 1);
    NOSFIX_CHECK(descriptor.button_groups[1].report_id == 2);
    NOSFIX_CHECK(descriptor.controls[0].report_id == 1);
    NOSFIX_CHECK(descriptor.controls[2].report_id == 2);
    NOSFIX_CHECK(descriptor.controls[4].kind == InputControlKind::Value);
    NOSFIX_CHECK(descriptor.controls[4].report_id == 2);
    // Spice compatibility: flat index runs over cap order, not storage order.
    NOSFIX_CHECK(descriptor.controls[0].spice_index == 0);
    NOSFIX_CHECK(descriptor.controls[1].spice_index == 1);
    NOSFIX_CHECK(descriptor.controls[2].spice_index == 2);
    NOSFIX_CHECK(descriptor.controls[3].spice_index == 3);
}

void TestDescriptorRejections() {
    Registry registry;
    HidDescriptor descriptor;

    auto mixed = ButtonDevice();
    mixed.buttons.push_back(ButtonCapSpec{1, 1, 0, false, 20, 20, 8});
    std::uint64_t token = registry.Add(mixed);
    NOSFIX_CHECK(BuildHidDescriptor(Registry::Api(), AsPreparsed(&token), descriptor) ==
                 HidDescriptorStatus::RejectedMixedReportIds);

    DeviceSpec empty; // no controls
    token = registry.Add(empty);
    NOSFIX_CHECK(BuildHidDescriptor(Registry::Api(), AsPreparsed(&token), descriptor) ==
                 HidDescriptorStatus::RejectedNoInputControls);

    DeviceSpec duplicate;
    duplicate.buttons = {
        ButtonCapSpec{1, 0, 0, false, 5, 5, 0},
        ButtonCapSpec{1, 0, 0, false, 5, 5, 1},
    };
    token = registry.Add(duplicate);
    NOSFIX_CHECK(BuildHidDescriptor(Registry::Api(), AsPreparsed(&token), descriptor) ==
                 HidDescriptorStatus::RejectedDuplicateControl);

    DeviceSpec too_many;
    too_many.buttons = {ButtonCapSpec{1, 0, 0, true, 1, 300, 0}};
    token = registry.Add(too_many);
    NOSFIX_CHECK(BuildHidDescriptor(Registry::Api(), AsPreparsed(&token), descriptor) ==
                 HidDescriptorStatus::RejectedTooManyControls);

    DeviceSpec zero_report;
    zero_report.input_report_bytes = 0;
    zero_report.buttons = {ButtonCapSpec{1, 0, 0, false, 1, 1, 0}};
    token = registry.Add(zero_report);
    NOSFIX_CHECK(BuildHidDescriptor(Registry::Api(), AsPreparsed(&token), descriptor) ==
                 HidDescriptorStatus::RejectedReportLength);

    DeviceSpec ambiguous;
    ambiguous.buttons = {
        ButtonCapSpec{1, 0, 0, false, 1, 1, 0},
        ButtonCapSpec{1, 0, 1, false, 2, 2, 1},
    };
    token = registry.Add(ambiguous);
    NOSFIX_CHECK(BuildHidDescriptor(Registry::Api(), AsPreparsed(&token), descriptor) ==
                 HidDescriptorStatus::RejectedAmbiguousLinkCollection);

    DeviceSpec bad_value;
    bad_value.buttons = {ButtonCapSpec{1, 0, 0, false, 1, 1, 0}};
    bad_value.values = {ValueCapSpec{1, 0, 0, false, 0x30, 0x30, 1, 10, 10, 8, false}};
    token = registry.Add(bad_value);
    NOSFIX_CHECK(BuildHidDescriptor(Registry::Api(), AsPreparsed(&token), descriptor) ==
                 HidDescriptorStatus::Accepted);
    NOSFIX_CHECK(descriptor.control_count == 1); // the invalid value cap is skipped
}

void TestKeyboardDecoding() {
    RAWKEYBOARD key{};
    key.VKey = 'A';
    key.MakeCode = 0x1E;
    key.Flags = 0;
    const std::uint32_t expected = MakeKeyboardControlId(key);
    NOSFIX_CHECK(expected == MakeKeyboardControlId(static_cast<std::uint32_t>('A')));
    NOSFIX_CHECK(IsKeyboardControlId(expected));

    KeyboardControlState state;
    ControlTransition transition{};
    NOSFIX_CHECK(state.Apply(key, transition));
    NOSFIX_CHECK(transition.control_id == expected);
    NOSFIX_CHECK(transition.down && transition.velocity == 1.0f);
    NOSFIX_CHECK(state.IsDown(expected));
    NOSFIX_CHECK(!state.Apply(key, transition)); // typematic make

    key.Flags = RI_KEY_BREAK;
    NOSFIX_CHECK(state.Apply(key, transition));
    NOSFIX_CHECK(!transition.down && transition.velocity == 0.0f);
    NOSFIX_CHECK(!state.IsDown(expected));
    NOSFIX_CHECK(!state.Apply(key, transition)); // unmatched break

    // Spice's ambiguous Ctrl normalisation.
    RAWKEYBOARD ctrl{};
    ctrl.VKey = 17;
    ctrl.MakeCode = 0x1D;
    ctrl.Flags = RI_KEY_E0;
    NOSFIX_CHECK(MakeKeyboardControlId(ctrl) ==
                 MakeKeyboardControlId(static_cast<std::uint32_t>(256 + VK_RCONTROL)));
    ctrl.Flags = 0;
    NOSFIX_CHECK(MakeKeyboardControlId(ctrl) ==
                 MakeKeyboardControlId(static_cast<std::uint32_t>(VK_LCONTROL)));

    // Shift is disambiguated by make code.
    RAWKEYBOARD shift{};
    shift.VKey = VK_SHIFT;
    shift.MakeCode = 42;
    NOSFIX_CHECK(MakeKeyboardControlId(shift) ==
                 MakeKeyboardControlId(static_cast<std::uint32_t>(VK_LSHIFT)));
    shift.MakeCode = 54;
    NOSFIX_CHECK(MakeKeyboardControlId(shift) ==
                 MakeKeyboardControlId(static_cast<std::uint32_t>(VK_RSHIFT)));

    RAWKEYBOARD invalid{};
    invalid.VKey = 255;
    NOSFIX_CHECK(MakeKeyboardControlId(invalid) == 0);
    invalid.VKey = 0;
    NOSFIX_CHECK(MakeKeyboardControlId(invalid) == 0);

    state.Reset();
    NOSFIX_CHECK(!state.IsDown(expected));
}

void TestHidButtonReportIdIsolation() {
    Registry registry;
    DeviceSpec spec;
    spec.source_id = 0x1234;
    spec.input_report_bytes = 3;
    spec.buttons = {
        ButtonCapSpec{1, 1, 0, true, 1, 2, 0},
        ButtonCapSpec{1, 2, 0, true, 1, 2, 0},
    };
    const std::uint64_t token = registry.Add(spec);
    HidDescriptor descriptor;
    NOSFIX_CHECK(BuildHidDescriptor(Registry::Api(), AsPreparsed(&token), descriptor) ==
                 HidDescriptorStatus::Accepted);

    HidControlState state;
    std::array<HidAnalogConfig, kHidMaxControlCount> analog{};
    std::array<ControlTransition, kHidMaxControlCount> transitions{};
    std::size_t written = 0;

    // Report ID 1, usage 1 ON.
    registry.Get(token).SetOn(1, 1, 0, {1});
    std::vector<std::byte> report = synthetic::Report(1, 3);
    NOSFIX_CHECK(DecodeHidReport(Registry::Api(), AsPreparsed(&token), descriptor, state,
                                 analog, report, transitions, written) ==
                 HidDecodeStatus::Decoded);
    NOSFIX_CHECK(written == 1);
    const std::uint32_t id1_usage1 = descriptor.controls[0].control_id;
    NOSFIX_CHECK(transitions[0].control_id == id1_usage1 && transitions[0].down);

    // Report ID 2, usage 1 ON: must not release report ID 1's control.
    registry.Get(token).SetOn(2, 1, 0, {1});
    report = synthetic::Report(2, 3);
    NOSFIX_CHECK(DecodeHidReport(Registry::Api(), AsPreparsed(&token), descriptor, state,
                                 analog, report, transitions, written) ==
                 HidDecodeStatus::Decoded);
    NOSFIX_CHECK(written == 1);
    NOSFIX_CHECK(transitions[0].control_id == descriptor.controls[2].control_id);
    NOSFIX_CHECK(state.IsDownByIndex(0));
    NOSFIX_CHECK(state.IsDownByIndex(2));

    // Report ID 1 with no usages: only ID 1's control is released.
    registry.Get(token).SetOn(1, 1, 0, {});
    report = synthetic::Report(1, 3);
    NOSFIX_CHECK(DecodeHidReport(Registry::Api(), AsPreparsed(&token), descriptor, state,
                                 analog, report, transitions, written) ==
                 HidDecodeStatus::Decoded);
    NOSFIX_CHECK(written == 1);
    NOSFIX_CHECK(transitions[0].control_id == id1_usage1 && !transitions[0].down);
    NOSFIX_CHECK(!state.IsDownByIndex(0));
    NOSFIX_CHECK(state.IsDownByIndex(2));

    // Repeated report: no transition.
    NOSFIX_CHECK(DecodeHidReport(Registry::Api(), AsPreparsed(&token), descriptor, state,
                                 analog, report, transitions, written) ==
                 HidDecodeStatus::Decoded);
    NOSFIX_CHECK(written == 0);

    // Unknown report ID: fail closed, state unchanged.
    report = synthetic::Report(3, 3);
    NOSFIX_CHECK(DecodeHidReport(Registry::Api(), AsPreparsed(&token), descriptor, state,
                                 analog, report, transitions, written) ==
                 HidDecodeStatus::UnsupportedReport);
    NOSFIX_CHECK(written == 0);
    NOSFIX_CHECK(state.IsDownByIndex(2));

    // Wrong length: fail closed.
    report = synthetic::Report(1, 2);
    NOSFIX_CHECK(DecodeHidReport(Registry::Api(), AsPreparsed(&token), descriptor, state,
                                 analog, report, transitions, written) ==
                 HidDecodeStatus::UnsupportedReport);
    NOSFIX_CHECK(state.IsDownByIndex(2));

    // HidP failure: fail closed, no state change.
    registry.Get(token).next_usages_error = HIDP_STATUS_INVALID_REPORT_LENGTH;
    report = synthetic::Report(1, 3);
    NOSFIX_CHECK(DecodeHidReport(Registry::Api(), AsPreparsed(&token), descriptor, state,
                                 analog, report, transitions, written) ==
                 HidDecodeStatus::UnsupportedReport);
    NOSFIX_CHECK(state.IsDownByIndex(2));

    // Output capacity: two transitions do not fit into one slot, state is
    // untouched and the identical call succeeds with two slots.
    registry.Get(token).SetOn(1, 1, 0, {1, 2});
    std::array<ControlTransition, 1> small{};
    NOSFIX_CHECK(DecodeHidReport(Registry::Api(), AsPreparsed(&token), descriptor, state,
                                 analog, report, small, written) ==
                 HidDecodeStatus::OutputTooSmall);
    NOSFIX_CHECK(written == 0);
    NOSFIX_CHECK(!state.IsDownByIndex(0));
    NOSFIX_CHECK(!state.IsDownByIndex(1));
    NOSFIX_CHECK(DecodeHidReport(Registry::Api(), AsPreparsed(&token), descriptor, state,
                                 analog, report, transitions, written) ==
                 HidDecodeStatus::Decoded);
    NOSFIX_CHECK(written == 2);
    NOSFIX_CHECK(state.IsDownByIndex(0) && state.IsDownByIndex(1));
}

void TestHidValueDecoding() {
    Registry registry;
    DeviceSpec spec;
    spec.source_id = 0x5678;
    spec.input_report_bytes = 3;
    spec.buttons = {ButtonCapSpec{1, 1, 0, false, 1, 1, 0}};
    spec.values = {ValueCapSpec{1, 1, 0, false, 0x30, 0x30, 0, 0, 100, 8, true}};
    const std::uint64_t token = registry.Add(spec);
    HidDescriptor descriptor;
    NOSFIX_CHECK(BuildHidDescriptor(Registry::Api(), AsPreparsed(&token), descriptor) ==
                 HidDescriptorStatus::Accepted);
    NOSFIX_CHECK(descriptor.control_count == 2);

    HidControlState state;
    std::array<HidAnalogConfig, kHidMaxControlCount> analog{};
    const std::size_t value_index = 1;
    analog[value_index] = HidAnalogConfig{true, 0.6f, 0.4f, false};
    std::array<ControlTransition, kHidMaxControlCount> transitions{};
    std::size_t written = 0;
    std::vector<std::byte> report = synthetic::Report(1, 3);
    auto decode = [&](ULONG raw, bool is_null = false) {
        registry.Get(token).SetRaw(1, 1, 0, 0x30, raw, is_null);
        return DecodeHidReport(Registry::Api(), AsPreparsed(&token), descriptor, state,
                               analog, report, transitions, written);
    };
    const std::uint32_t value_id = descriptor.controls[value_index].control_id;

    NOSFIX_CHECK(decode(50) == HidDecodeStatus::Decoded);
    NOSFIX_CHECK(written == 0);
    NOSFIX_CHECK(decode(60) == HidDecodeStatus::Decoded);
    NOSFIX_CHECK(written == 1 && transitions[0].control_id == value_id);
    NOSFIX_CHECK(transitions[0].down);
    NOSFIX_CHECK(std::abs(transitions[0].velocity - 0.6f) < 1e-6f);
    NOSFIX_CHECK(decode(45) == HidDecodeStatus::Decoded); // hysteresis: still held
    NOSFIX_CHECK(written == 0);
    NOSFIX_CHECK(decode(40) == HidDecodeStatus::Decoded); // release at threshold
    NOSFIX_CHECK(written == 1 && !transitions[0].down);
    NOSFIX_CHECK(decode(39) == HidDecodeStatus::Decoded);
    NOSFIX_CHECK(written == 0);
    NOSFIX_CHECK(decode(90) == HidDecodeStatus::Decoded);
    NOSFIX_CHECK(written == 1 && transitions[0].down);
    NOSFIX_CHECK(decode(0, true) == HidDecodeStatus::Decoded); // NULL releases
    NOSFIX_CHECK(written == 1 && !transitions[0].down);

    // Inverted control: a low raw value presses.
    analog[value_index] = HidAnalogConfig{true, 0.6f, 0.4f, true};
    NOSFIX_CHECK(decode(30) == HidDecodeStatus::Decoded);
    NOSFIX_CHECK(written == 1 && transitions[0].down);
    NOSFIX_CHECK(std::abs(transitions[0].velocity - 0.7f) < 1e-6f);
    NOSFIX_CHECK(decode(70) == HidDecodeStatus::Decoded);
    NOSFIX_CHECK(written == 1 && !transitions[0].down);

    // An unbound value control is never polled (no HidP call, no transition).
    analog[value_index] = HidAnalogConfig{};
    registry.Get(token).next_value_error = HIDP_STATUS_USAGE_NOT_FOUND;
    NOSFIX_CHECK(decode(99) == HidDecodeStatus::Decoded);
    NOSFIX_CHECK(written == 0);
}

void TestValueNormalization() {
    double value = 0.0;
    NOSFIX_CHECK(NormalizeHidValue(0, 255, 8, 128, value));
    NOSFIX_CHECK(std::abs(value - (128.0 / 255.0)) < 1e-9);
    NOSFIX_CHECK(NormalizeHidValue(-128, 127, 8, 0xFF, value));
    NOSFIX_CHECK(std::abs(value - ((-1.0 + 128.0) / 255.0)) < 1e-9);
    NOSFIX_CHECK(NormalizeHidValue(0, 100, 16, 300, value));
    NOSFIX_CHECK(value == 1.0);
    NOSFIX_CHECK(!NormalizeHidValue(10, 10, 8, 5, value));
    NOSFIX_CHECK(!NormalizeHidValue(0, 10, 0, 5, value));
    NOSFIX_CHECK(!NormalizeHidValue(0, 10, 33, 5, value));
}

void TestPayloadViews() {
    const auto keyboard = synthetic::KeyboardPayload(7, 'A', 0x1E, 0);
    RAWKEYBOARD key{};
    NOSFIX_CHECK(ViewKeyboard(keyboard, key));
    NOSFIX_CHECK(key.VKey == 'A');
    auto short_keyboard = keyboard;
    short_keyboard.resize(offsetof(RAWINPUT, data.keyboard) + 2);
    NOSFIX_CHECK(!ViewKeyboard(short_keyboard, key));

    auto hid = synthetic::HidPayload(9, 3, {synthetic::Report(1, 3)});
    std::array<HidReportView, kHidMaxReportsPerPacket> reports{};
    std::size_t count = 0;
    std::uint32_t bytes = 0;
    NOSFIX_CHECK(ViewHidReports(hid, reports, count, bytes));
    NOSFIX_CHECK(count == 1 && bytes == 3);
    NOSFIX_CHECK(reports[0].bytes.size() == 3);

    auto bad_header = hid;
    RAWINPUTHEADER header{};
    std::memcpy(&header, bad_header.data(), sizeof(header));
    header.dwSize += 1;
    std::memcpy(bad_header.data(), &header, sizeof(header));
    NOSFIX_CHECK(!ViewHidReports(bad_header, reports, count, bytes));

    auto zero_count = hid;
    DWORD zero = 0;
    std::memcpy(zero_count.data() + offsetof(RAWINPUT, data.hid.dwCount), &zero,
                sizeof(zero));
    NOSFIX_CHECK(!ViewHidReports(zero_count, reports, count, bytes));
}

void TestReportBatchAndAllocationFree() {
    Registry registry;
    DeviceSpec spec;
    spec.source_id = 0x77;
    spec.input_report_bytes = 3;
    spec.buttons = {ButtonCapSpec{1, 1, 0, true, 1, 4, 0}};
    const std::uint64_t token = registry.Add(spec);
    HidDescriptor descriptor;
    NOSFIX_CHECK(BuildHidDescriptor(Registry::Api(), AsPreparsed(&token), descriptor) ==
                 HidDescriptorStatus::Accepted);

    HidControlState state;
    std::array<HidAnalogConfig, kHidMaxControlCount> analog{};
    std::array<ControlTransition, kHidMaxControlCount> transitions{};
    std::size_t written = 0;
    registry.Get(token).SetOn(1, 1, 0, {2, 3, 4});
    const auto report = synthetic::Report(1, 3);

    allocation_probe::tracking = true;
    allocation_probe::count = 0;
    const auto status = DecodeHidReport(Registry::Api(), AsPreparsed(&token), descriptor,
                                        state, analog, report, transitions, written);
    allocation_probe::tracking = false;
    NOSFIX_CHECK(status == HidDecodeStatus::Decoded);
    NOSFIX_CHECK(written == 3);
    NOSFIX_CHECK(allocation_probe::count == 0);
    NOSFIX_CHECK(transitions[0].control_id == descriptor.controls[1].control_id);
    NOSFIX_CHECK(transitions[1].control_id == descriptor.controls[2].control_id);
    NOSFIX_CHECK(transitions[2].control_id == descriptor.controls[3].control_id);
}

void TestDeviceIdentity() {
    const auto a = SourceIdFromDeviceName("\\\\?\\HID#VID_1234&PID_5678#8&1");
    const auto b = SourceIdFromDeviceName("\\\\?\\HID#VID_1234&PID_5678#8&1");
    const auto c = SourceIdFromDeviceName("\\\\?\\HID#VID_1234&PID_5678#8&2");
    NOSFIX_CHECK(a != 0 && a == b && a != c);
    NOSFIX_CHECK(SourceIdFromDeviceName("") != 0);
}

void TestPanbObservation() {
    Registry registry;
    DeviceSpec spec;
    spec.source_id = 0x9999;
    spec.input_report_bytes = 3;
    spec.values = {ValueCapSpec{1, 0, 0, false, 0x30, 0x30, 0, 0, 255, 8, false}};
    const std::uint64_t token = registry.Add(spec);
    HidDescriptor descriptor;
    NOSFIX_CHECK(BuildHidDescriptor(Registry::Api(), AsPreparsed(&token), descriptor) ==
                 HidDescriptorStatus::Accepted);

    HidControlState state;
    std::array<HidAnalogConfig, kHidMaxControlCount> analog{};
    analog[0] = HidAnalogConfig{true, kPanbPressThreshold, kPanbPressThreshold, false};
    std::array<ControlTransition, kHidMaxControlCount> transitions{};
    std::array<AnalogObservation, kHidMaxControlCount> observations{};
    std::size_t written = 0;
    std::size_t observation_count = 0;
    const auto report = synthetic::Report(1, 3);
    auto decode = [&](ULONG raw, bool null_value = false) {
        registry.Get(token).SetRaw(0, 1, 0, 0x30, raw, null_value);
        return DecodeHidReport(Registry::Api(), AsPreparsed(&token), descriptor, state,
                               analog, report, transitions, written, observations,
                               &observation_count);
    };

    NOSFIX_CHECK(decode(153) == HidDecodeStatus::Decoded); // 0.6
    NOSFIX_CHECK(observation_count == 1);
    NOSFIX_CHECK(std::abs(observations[0].level - 0.6f) < 1e-6f);
    NOSFIX_CHECK(observations[0].panb_velocity == 9); // 0.6 * 15.999 -> 9
    NOSFIX_CHECK(written == 1);

    // Below the first PANB nibble: no press, but the continuous level is kept.
    NOSFIX_CHECK(decode(15) == HidDecodeStatus::Decoded); // 0.0588
    NOSFIX_CHECK(observation_count == 1);
    NOSFIX_CHECK(observations[0].panb_velocity == 0);
    NOSFIX_CHECK(decode(16) == HidDecodeStatus::Decoded); // 0.0627 -> nibble 1
    NOSFIX_CHECK(observation_count == 1);
    NOSFIX_CHECK(observations[0].panb_velocity == 1);

    NOSFIX_CHECK(decode(0, true) == HidDecodeStatus::Decoded); // NULL
    NOSFIX_CHECK(observation_count == 1);
    NOSFIX_CHECK(observations[0].level == 0.0f);
    NOSFIX_CHECK(observations[0].panb_velocity == 0);
}

} // namespace

int main() {
    TestDescriptorBasics();
    TestMultiReportDescriptor();
    TestDescriptorRejections();
    TestKeyboardDecoding();
    TestHidButtonReportIdIsolation();
    TestHidValueDecoding();
    TestValueNormalization();
    TestPayloadViews();
    TestReportBatchAndAllocationFree();
    TestPanbObservation();
    TestDeviceIdentity();
    std::printf("input_decoder: %d checks, %d failures\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
