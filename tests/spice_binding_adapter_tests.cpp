// Always-on Release tests for the Spice config adapter and the PANB velocity
// domain. Synthetic descriptors only; no physical device is touched.

#include "synthetic_hid.hpp"

#include "nosfix/input_decoder.hpp"
#include "nosfix/spice_binding_adapter.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace {

int g_checks = 0;
int g_failures = 0;

#define NOSFIX_CHECK(condition)                                                   \
    do {                                                                          \
        ++g_checks;                                                               \
        if (!(condition)) {                                                       \
            ++g_failures;                                                         \
            std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition); \
        }                                                                         \
    } while (false)

using namespace nosfix;
using synthetic::ButtonCapSpec;
using synthetic::DeviceSpec;
using synthetic::Registry;
using synthetic::ValueCapSpec;

constexpr std::string_view kEscapedDevice =
    "\\\\?\\HID#VID_0E8F&amp;PID_1212&amp;MI_04#8&amp;1cb9ee58&amp;0&amp;0000"
    "#{4d1e55b2-f16f-11cf-88cb-001111000030}";
constexpr std::string_view kPlainDevice =
    "\\\\?\\HID#VID_0E8F&PID_1212&MI_04#8&1cb9ee58&0&0000"
    "#{4d1e55b2-f16f-11cf-88cb-001111000030}";

constexpr std::string_view kXmlPrefix =
    "<nostalgia_input_bindings><buttons>"
    "<button name=\"Service\" vkey=\"255\" devid=\"\" />"
    "</buttons><analogs>\n";
constexpr std::string_view kXmlSuffix = "</analogs></nostalgia_input_bindings>\n";

std::string_view AnalogTag(std::string_view name, std::uint32_t index,
                           std::string_view extra = {}) {
    static std::array<std::string, 32> storage{};
    static std::size_t next = 0;
    std::string& text = storage[next++ % storage.size()];
    text.clear();
    text += "<analog name=\"";
    text += name;
    text += "\" index=\"";
    text += std::to_string(index);
    text += "\" sensivity=\"1\" deadzone=\"0\" devid=\"";
    text += kEscapedDevice;
    text += "\" deadzone_mirror=\"false\" invert=\"false\" smoothing=\"false\" "
            "multiplier=\"1\" relative=\"false\" delay=\"0\" delay_ms=\"0\" ";
    text += extra;
    text += "/>\n";
    return text;
}

std::uint64_t AddValueDescriptor(Registry& registry, bool range_at_zero = false) {
    DeviceSpec spec;
    spec.source_id = 0x1234'5678;
    spec.input_report_bytes = 40;
    if (range_at_zero) {
        spec.values.push_back(
            ValueCapSpec{1, 0, 0, true, 0x30, 0x31, 0, 0, 255, 8, false});
    }
    for (std::uint32_t i = range_at_zero ? 2 : 0; i < 32; ++i) {
        spec.values.push_back(ValueCapSpec{1, 0, 0, false,
                                           static_cast<USAGE>(0x30 + i),
                                           static_cast<USAGE>(0x30 + i),
                                           static_cast<USHORT>(i), 0, 255, 8,
                                           false});
    }
    return registry.Add(spec);
}

HidDescriptor Describe(Registry& registry, std::uint64_t token) {
    (void)registry;
    HidDescriptor descriptor;
    const std::uint64_t local = token;
    const auto status = BuildHidDescriptor(
        Registry::Api(),
        reinterpret_cast<PHIDP_PREPARSED_DATA>(const_cast<std::uint64_t*>(&local)),
        descriptor);
    NOSFIX_CHECK(status == HidDescriptorStatus::Accepted);
    return descriptor;
}

void TestPanbVelocityDomain() {
    NOSFIX_CHECK(kPanbMaxVelocity == 14);
    NOSFIX_CHECK(PanbVelocity(0.0f) == 0);
    NOSFIX_CHECK(PanbVelocity(0.01f) == 0);
    NOSFIX_CHECK(PanbVelocity(kPanbPressThreshold) == 1);
    NOSFIX_CHECK(PanbVelocity(0.5f) == 7); // 7.9995 truncates
    NOSFIX_CHECK(PanbVelocity(0.9f) == 14);
    NOSFIX_CHECK(PanbVelocity(1.0f) == 14);
    NOSFIX_CHECK(std::abs(kPanbPressThreshold - (1.0f / 15.999f)) < 1e-9f);
}

void TestParseActualShape() {
    Registry registry;
    std::string xml;
    xml.append(kXmlPrefix.data(), kXmlPrefix.size());
    { const auto tag = AnalogTag("Key 1", 4); xml.append(tag.data(), tag.size()); }
    { const auto tag = AnalogTag("Key 2", 5); xml.append(tag.data(), tag.size()); }
    { const auto tag = AnalogTag("Key 28", 31); xml.append(tag.data(), tag.size()); }
    xml.append(kXmlSuffix.data(), kXmlSuffix.size());

    std::array<SpiceAnalogBinding, kSpiceMaxAnalogBindings> bindings{};
    std::size_t count = 0;
    const auto status = ParseSpiceAnalogBindings(xml, bindings, count);
    NOSFIX_CHECK(status == SpiceParseStatus::Accepted);
    NOSFIX_CHECK(count == 3);
    if (count == 3) {
        NOSFIX_CHECK(bindings[0].key_number == 1 && bindings[0].index == 4);
        NOSFIX_CHECK(bindings[1].key_number == 2 && bindings[1].index == 5);
        NOSFIX_CHECK(bindings[2].key_number == 28 && bindings[2].index == 31);
        NOSFIX_CHECK(bindings[0].source_id == SourceIdFromDeviceName(kPlainDevice));
        NOSFIX_CHECK(bindings[0].deadzone == 0.0f);
        NOSFIX_CHECK(bindings[0].sensitivity == 1.0f);
        NOSFIX_CHECK(bindings[0].multiplier == 1.0f);
        NOSFIX_CHECK(!bindings[0].invert && !bindings[0].relative &&
                     !bindings[0].smoothing);
    }

    const std::uint64_t token = AddValueDescriptor(registry);
    const HidDescriptor descriptor = Describe(registry, token);
    std::array<PipelineBindingSpec, 28> specs{};
    std::size_t spec_count = 0;
    const auto built = BuildNostalgiaKeyBindings(
        std::span<const SpiceAnalogBinding>(bindings.data(), count), descriptor,
        bindings[0].source_id, 7, specs, spec_count);
    NOSFIX_CHECK(built == SpiceBindingStatus::Accepted);
    NOSFIX_CHECK(spec_count == 3);
    if (spec_count == 3) {
        const HidControlInfo* key1 = descriptor.FindSpiceValue(4, 0);
        const HidControlInfo* key2 = descriptor.FindSpiceValue(5, 0);
        const HidControlInfo* key28 = descriptor.FindSpiceValue(31, 0);
        NOSFIX_CHECK(key1 && key2 && key28);
        NOSFIX_CHECK(specs[0].control_id == key1->control_id);
        NOSFIX_CHECK(specs[1].control_id == key2->control_id);
        NOSFIX_CHECK(specs[2].control_id == key28->control_id);
        NOSFIX_CHECK(specs[0].lane == 0 && specs[1].lane == 1 && specs[2].lane == 27);
        NOSFIX_CHECK(specs[0].kind == InputControlKind::Value);
        NOSFIX_CHECK(std::abs(specs[0].press_threshold - kPanbPressThreshold) < 1e-9f);
        NOSFIX_CHECK(specs[0].release_threshold == specs[0].press_threshold);
        NOSFIX_CHECK(!specs[0].invert);
        NOSFIX_CHECK(specs[0].source_id == bindings[0].source_id);
    }
}

void TestParseErrors() {
    std::array<SpiceAnalogBinding, kSpiceMaxAnalogBindings> bindings{};
    std::size_t count = 0;

    NOSFIX_CHECK(ParseSpiceAnalogBindings("<nostalgia/>", bindings, count) ==
                 SpiceParseStatus::NoAnalogEntries);

    {
        const std::string xml = std::string(kXmlPrefix) +
            "<analog index=\"4\" devid=\"x\" />\n" + std::string(kXmlSuffix);
        NOSFIX_CHECK(ParseSpiceAnalogBindings(xml, bindings, count) ==
                     SpiceParseStatus::MalformedAttribute);
    }
    {
        const std::string xml = std::string(kXmlPrefix) +
            "<analog name=\"Key 1\" index=\"nope\" devid=\"x\" />\n" +
            std::string(kXmlSuffix);
        NOSFIX_CHECK(ParseSpiceAnalogBindings(xml, bindings, count) ==
                     SpiceParseStatus::MalformedNumber);
    }
    {
        const std::string xml = std::string(kXmlPrefix) +
            "<analog name=\"Key 29\" index=\"4\" devid=\"x\" />\n" +
            std::string(kXmlSuffix);
        NOSFIX_CHECK(ParseSpiceAnalogBindings(xml, bindings, count) ==
                     SpiceParseStatus::KeyNumberOutOfRange);
    }
    {
        std::string xml = std::string(kXmlPrefix);
        { const auto tag = AnalogTag("Key 1", 4); xml.append(tag.data(), tag.size()); }
        xml.append(kXmlSuffix.data(), kXmlSuffix.size());
        std::array<SpiceAnalogBinding, 0> none{};
        NOSFIX_CHECK(ParseSpiceAnalogBindings(xml, none, count) ==
                     SpiceParseStatus::TooManyEntries);
    }
}

void TestBuildRejections() {
    Registry registry;
    const std::uint64_t token = AddValueDescriptor(registry);
    const HidDescriptor descriptor = Describe(registry, token);
    const std::uint64_t source = SourceIdFromDeviceName(kPlainDevice);
    std::array<PipelineBindingSpec, 28> specs{};
    std::size_t count = 0;

    {
        std::array<SpiceAnalogBinding, 1> binding{};
        binding[0].name = "Key 1";
        binding[0].source_id = source;
        binding[0].index = 4;
        binding[0].key_number = 1;
        binding[0].sensitivity = 2.0f;
        NOSFIX_CHECK(BuildNostalgiaKeyBindings(binding, descriptor, source, 1,
                                               specs, count) ==
                     SpiceBindingStatus::UnsupportedTransform);
    }
    {
        std::array<SpiceAnalogBinding, 1> binding{};
        binding[0].name = "Key 1";
        binding[0].source_id = source;
        binding[0].index = 99;
        binding[0].key_number = 1;
        NOSFIX_CHECK(BuildNostalgiaKeyBindings(binding, descriptor, source, 1,
                                               specs, count) ==
                     SpiceBindingStatus::UnknownValueIndex);
    }
    {
        std::array<SpiceAnalogBinding, 1> binding{};
        binding[0].name = "Key 1";
        binding[0].source_id = source + 1;
        binding[0].index = 4;
        binding[0].key_number = 1;
        NOSFIX_CHECK(BuildNostalgiaKeyBindings(binding, descriptor, source, 1,
                                               specs, count) ==
                     SpiceBindingStatus::WrongDevice);
    }
    {
        std::array<SpiceAnalogBinding, 2> binding{};
        for (auto& entry : binding) {
            entry.name = "Key 1";
            entry.source_id = source;
            entry.index = 4;
            entry.key_number = 1;
        }
        binding[1].index = 5;
        NOSFIX_CHECK(BuildNostalgiaKeyBindings(binding, descriptor, source, 1,
                                               specs, count) ==
                     SpiceBindingStatus::DuplicateLane);
    }
    {
        // A range value cap expands to two controls with one spice index; the
        // adapter cannot choose a usage and fails closed.
        Registry range_registry;
        const std::uint64_t range_token = AddValueDescriptor(range_registry, true);
        const HidDescriptor range_descriptor = Describe(range_registry, range_token);
        std::array<SpiceAnalogBinding, 1> binding{};
        binding[0].name = "Key 1";
        binding[0].source_id = source;
        binding[0].index = 0;
        binding[0].key_number = 1;
        NOSFIX_CHECK(BuildNostalgiaKeyBindings(binding, range_descriptor, source, 1,
                                               specs, count) ==
                     SpiceBindingStatus::AmbiguousValueIndex);
    }
}

void TestEscapedIdentity() {
    NOSFIX_CHECK(SourceIdFromEscapedDeviceName(kEscapedDevice) ==
                 SourceIdFromDeviceName(kPlainDevice));
    NOSFIX_CHECK(SourceIdFromEscapedDeviceName("plain") ==
                 SourceIdFromDeviceName("plain"));
    NOSFIX_CHECK(SourceIdFromEscapedDeviceName("a&bogus;b") == 0);
}

} // namespace

int main() {
    TestPanbVelocityDomain();
    TestParseActualShape();
    TestParseErrors();
    TestBuildRejections();
    TestEscapedIdentity();
    std::fprintf(stderr, "spice_binding_adapter: %d checks, %d failures\n",
                 g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
