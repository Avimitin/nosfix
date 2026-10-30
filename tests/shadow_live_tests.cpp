#ifdef NDEBUG
#undef NDEBUG
#endif
#include "nosfix/shadow_live.hpp"

#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <vector>

namespace {

template <class T>
void Put(std::vector<std::byte>& image, std::size_t offset, const T& value) {
    std::memcpy(image.data() + offset, &value, sizeof(T));
}

template <class T>
void Put(std::byte* image, std::size_t offset, const T& value) {
    std::memcpy(image + offset, &value, sizeof(T));
}

} // namespace

int main() {
    constexpr std::size_t kImageSize = 0x1400000;
    std::vector<std::byte> image(kImageSize);

    // The initializer's eight window values plus the outer clamps.
    Put(image, 0x13668E0, std::int32_t{9});
    Put(image, 0x13668E4, std::int32_t{-11});
    Put(image, 0x13668E8, std::int32_t{7});
    Put(image, 0x13668EC, std::int32_t{-6});
    Put(image, 0x13668F0, std::int32_t{4});
    Put(image, 0x13668F4, std::int32_t{-4});
    Put(image, 0x13668F8, std::int32_t{2});
    Put(image, 0x13668FC, std::int32_t{-2});
    Put(image, 0x1366900, std::int32_t{9999});
    Put(image, 0x1366904, std::int32_t{-9999});
    Put(image, 0x13E4A84, std::int32_t{3});
    Put(image, 0x13E5724, 0.4f);

    std::array<std::byte, 784> staging{};
    void* staging_pointer = staging.data();
    Put(image, 0x13DEA10, staging_pointer);

    std::array<std::byte, 512> note{};
    std::array<std::byte, 128> chart{};
    void* chart_pointer = chart.data();
    Put(note.data(), 8, chart_pointer);
    Put(note.data(), 20, std::int32_t{4});
    Put(note.data(), 24, std::int32_t{60});
    Put(note.data(), 44, std::int32_t{5});
    Put(note.data(), 48, std::int32_t{5});
    Put(note.data(), 52, std::int32_t{5});
    Put(note.data(), 56, 5.0f);
    Put(note.data(), 64, std::int32_t{0});
    Put(note.data(), 84, std::int32_t{1});
    Put(note.data(), 96, std::int32_t{0});
    Put(chart.data(), 0x08, std::int32_t{0});
    Put(chart.data(), 0x10, std::int32_t{4});
    Put(chart.data(), 0x14, std::int32_t{60});
    Put(chart.data(), 0x30, std::int32_t{11});
    Put(chart.data(), 0x38, std::int32_t{0});
    Put(chart.data(), 0x58, std::int32_t{5});
    Put(chart.data(), 0x5C, std::int32_t{5});
    Put(chart.data(), 0x61, std::uint8_t{0});
    Put(chart.data(), 0x64, std::int32_t{0});

    nosfix::ConfigureShadowLive(image.data(), image.size(), image.data());
    assert(nosfix::ShadowLiveEnabled() || nosfix::ShadowLivePending());
    nosfix::ShadowLiveObserveNormalNote(note.data());

    // Slot 0: key 5 pressed this frame, value 1.
    Put(staging.data(), 0, std::int32_t{5});
    Put(staging.data(), 4, std::int32_t{1});
    Put(staging.data(), 8, std::uint8_t{1});

    nosfix::ShadowLiveTick(1'000'000.0, image.data());
    const auto metrics = nosfix::TakeShadowLiveMetrics();
    assert(metrics.frames >= 1);
    assert(metrics.outcomes >= 1);
    assert(metrics.effects >= 1);

    nosfix::DisableShadowLive();
    assert(!nosfix::ShadowLiveEnabled());
    return 0;
}
