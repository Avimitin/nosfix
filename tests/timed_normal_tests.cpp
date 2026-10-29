#include "nosfix/timed_normal.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <span>
#include <vector>

namespace {

void CheckImpl(bool condition, const char* expression, int line) {
    if (!condition) {
        std::fprintf(stderr, "timed-normal test failed at line %d: %s\n",
                     line, expression);
        std::abort();
    }
}
#define Check(condition) CheckImpl((condition), #condition, __LINE__)

template <typename T>
void Store(std::span<std::byte> bytes, std::size_t offset, const T& value) {
    std::memcpy(bytes.data() + offset, &value, sizeof(value));
}

template <typename T>
T Load(std::span<const std::byte> bytes, std::size_t offset) {
    T value{};
    std::memcpy(&value, bytes.data() + offset, sizeof(value));
    return value;
}

std::vector<std::byte> MakeQuad() {
    std::vector<std::byte> raw(nosfix::TimedNormal::kPrimitiveBytes);
    for (std::size_t vertex = 0; vertex < 4; ++vertex) {
        const std::size_t offset = vertex * 24;
        Store(std::span(raw), offset, static_cast<float>(vertex * 4));
        Store(std::span(raw), offset + 4, static_cast<float>(vertex * 5));
        Store(std::span(raw), offset + 8, 0.25f + static_cast<float>(vertex));
        Store(std::span(raw), offset + 12,
              0xA0123456U + static_cast<std::uint32_t>(vertex) * 0x100U);
        Store(std::span(raw), offset + 16, 0.1f * static_cast<float>(vertex + 1));
        Store(std::span(raw), offset + 20, 0.2f * static_cast<float>(vertex + 1));
    }
    Store(std::span(raw), 144, 0xD0000000U);
    Store(std::span(raw), 152, 0x123456789ABCDEF0ULL);
    return raw;
}

struct CallbackState {
    float distance = 0.0f;
    int lane = -1;
    int left_key = -1;
    int right_key = -1;
    unsigned calls = 0;
};

bool NonlinearProjection(void* context, int lane, float distance,
                         int left_key, int right_key,
                         nosfix::NoteProjection& result) noexcept {
    auto* state = static_cast<CallbackState*>(context);
    state->distance = distance;
    state->lane = lane;
    state->left_key = left_key;
    state->right_key = right_key;
    ++state->calls;
    result = {100.0f + distance * distance,
              200.0f - 2.0f * distance,
              1.0f + distance * 0.1f,
              0.5f};
    return true;
}

bool AlternateProjection(void*, int, float distance, int, int,
                         nosfix::NoteProjection& result) noexcept {
    result = {300.0f - 2.0f * distance,
              400.0f + distance,
              2.0f,
              1.0f};
    return true;
}

bool RejectProjection(void*, int, float, int, int,
                      nosfix::NoteProjection&) noexcept {
    return false;
}

bool InvalidProjection(void*, int, float, int, int,
                       nosfix::NoteProjection& result) noexcept {
    result = {0.0f, 0.0f, std::numeric_limits<float>::quiet_NaN(), 1.0f};
    return true;
}

nosfix::TimedNormalSpec MakeSpec() {
    return {5'000.25, 0.0, 60.0f, 3, 2, 4,
            10.0f, 20.0f, 3.0f, 0.5f};
}

void CheckMetadataPreserved(std::span<const std::byte> output,
                            std::span<const std::byte> original) {
    Check(output.size() == original.size());
    for (std::size_t vertex = 0; vertex < 4; ++vertex) {
        const std::size_t offset = vertex * 24;
        Check(Load<float>(output, offset + 8) == Load<float>(original, offset + 8));
        Check(Load<float>(output, offset + 16) == Load<float>(original, offset + 16));
        Check(Load<float>(output, offset + 20) == Load<float>(original, offset + 20));
        const auto before = Load<std::uint32_t>(original, offset + 12);
        const auto after = Load<std::uint32_t>(output, offset + 12);
        Check((after & 0x00FFFFFFU) == (before & 0x00FFFFFFU));
    }
    Check(Load<std::uint32_t>(output, 144) == Load<std::uint32_t>(original, 144));
    Check(Load<std::uint64_t>(output, 152) == Load<std::uint64_t>(original, 152));
}

void TestFractionalSongTimeAndStatelessRepeat() {
    nosfix::TimedNormal normal;
    auto raw = MakeQuad();
    const auto original = raw;
    auto spec = MakeSpec();

    Check(normal.Capture(spec, raw));
    Check(normal.Capture(spec, raw)); // A repeated update replaces the capture only.

    CallbackState callback{};
    const auto first = normal.Draw(3'000.0, raw, NonlinearProjection, &callback);
    Check(!first.empty());
    const float expected_distance = static_cast<float>((5'000.25 - 3'000.0) * 180.0 /
                                                       (60.0 * 1000.0));
    Check(std::abs(callback.distance - expected_distance) < 1e-6f);
    Check(callback.lane == 3 && callback.left_key == 2 && callback.right_key == 4);
    Check(callback.calls == 1);

    const float center_x = 100.0f + expected_distance * expected_distance;
    const float center_y = 200.0f - 2.0f * expected_distance;
    const float scale = 1.0f + expected_distance * 0.1f;
    const float half_width = 10.0f * scale;
    const float half_height = 20.0f * scale;
    const std::array<float, 4> expected_x{
        center_x + 3.0f - half_width,
        center_x + 3.0f + half_width - 1.0f,
        center_x + 3.0f - half_width,
        center_x + 3.0f + half_width - 1.0f};
    const std::array<float, 4> expected_y{
        center_y - half_height,
        center_y - half_height,
        center_y + half_height - 1.0f,
        center_y + half_height - 1.0f};
    for (std::size_t vertex = 0; vertex < 4; ++vertex) {
        Check(std::abs(Load<float>(first, vertex * 24) - expected_x[vertex]) < 1e-5f);
        Check(std::abs(Load<float>(first, vertex * 24 + 4) - expected_y[vertex]) < 1e-5f);
        Check((Load<std::uint32_t>(first, vertex * 24 + 12) >> 24) == 63U);
    }
    CheckMetadataPreserved(first, original);
    Check(raw == original); // Capture/draw never mutate game-owned bytes.

    const std::vector<std::byte> first_copy(first.begin(), first.end());
    const auto held = normal.Draw(3'000.0, raw, NonlinearProjection, &callback);
    Check(std::equal(held.begin(), held.end(), first_copy.begin(), first_copy.end()));
    const auto another_time = normal.Draw(3'500.0, raw, NonlinearProjection, &callback);
    Check(!another_time.empty());
    const auto same_again = normal.Draw(3'000.0, raw, NonlinearProjection, &callback);
    Check(std::equal(same_again.begin(), same_again.end(), first_copy.begin(), first_copy.end()));
    Check(callback.calls == 4);

    // The nonlinear callback is evaluated at the sampled song time; this is
    // deliberately not a previous-frame velocity extrapolation.
    const auto different_curve = normal.Draw(3'000.0, raw, AlternateProjection, nullptr);
    Check(!different_curve.empty());
    Check(std::abs(Load<float>(different_curve, 0) -
                   (300.0f - 2.0f * expected_distance + 3.0f - 20.0f)) < 1e-5f);
    Check(std::abs(Load<float>(different_curve, 4) -
                   (400.0f + expected_distance - 40.0f)) < 1e-5f);
}

void TestFallbacksAndInvalidCapture() {
    nosfix::TimedNormal normal;
    auto raw = MakeQuad();
    const auto spec = MakeSpec();
    Check(normal.Draw(3'000.0, raw, NonlinearProjection, nullptr).empty());

    Check(normal.Capture(spec, raw));
    Check(normal.Draw(std::numeric_limits<double>::quiet_NaN(), raw,
                      NonlinearProjection, nullptr).empty());
    Check(normal.Draw(3'000.0, raw, nullptr, nullptr).empty());
    Check(normal.Draw(3'000.0, raw, RejectProjection, nullptr).empty());
    Check(normal.Draw(3'000.0, raw, InvalidProjection, nullptr).empty());

    auto modified = raw;
    modified[16] ^= std::byte{1}; // UV/source mutation must pass through.
    Check(normal.Draw(3'000.0, modified, NonlinearProjection, nullptr).empty());
    Check(normal.Draw(3'000.0, raw, NonlinearProjection, nullptr).empty()); // Invalidated.

    Check(!normal.Capture(spec, std::span<const std::byte>(raw).first(159)));
    Check(!normal.Capture(spec, std::span<const std::byte>(raw).first(161)));
    auto inactive = raw;
    Store(std::span(inactive), 144, 0x10000000U);
    Check(!normal.Capture(spec, inactive));
    auto nonfinite_vertex = raw;
    Store(std::span(nonfinite_vertex), 0, std::numeric_limits<float>::infinity());
    Check(!normal.Capture(spec, nonfinite_vertex));

    auto bad = spec;
    bad.legacy_units_per_second = 0.0f;
    Check(!normal.Capture(bad, raw));
    bad = spec;
    bad.half_width = 0.0f;
    Check(!normal.Capture(bad, raw));
    bad = spec;
    bad.target_ms = std::numeric_limits<double>::infinity();
    Check(!normal.Capture(bad, raw));
    bad = spec;
    bad.x_shift = std::numeric_limits<float>::quiet_NaN();
    Check(!normal.Capture(bad, raw));
}

} // namespace

int main() {
    TestFractionalSongTimeAndStatelessRepeat();
    TestFallbacksAndInvalidCapture();
}
