#include "nosfix/primitive_motion.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <vector>

namespace {

void CheckImpl(bool condition, const char* expression, int line) {
    if (!condition) {
        std::fprintf(stderr, "primitive-motion test failed at line %d: %s\n",
                     line, expression);
        std::abort();
    }
}
#define Check(condition) CheckImpl((condition), #condition, __LINE__)

constexpr nosfix::GeometryKey kKeyA{0xAABBCCDD, 1};
constexpr nosfix::GeometryKey kKeyB{0xAABBCCDE, 1};
constexpr std::uint32_t kFlags = 0xD0000000;

template <typename T>
void Store(std::vector<std::byte>& bytes, std::size_t offset, const T& value) {
    std::memcpy(bytes.data() + offset, &value, sizeof(value));
}

template <typename T>
T Load(std::span<const std::byte> bytes, std::size_t offset) {
    T value{};
    std::memcpy(&value, bytes.data() + offset, sizeof(value));
    return value;
}

std::vector<std::byte> MakeTextured(std::uint32_t count, float x_shift,
                                    float y_shift, std::uint32_t color,
                                    std::uint32_t flags = kFlags,
                                    std::uint64_t texture = 0x123456789ABCDEF0ull) {
    std::vector<std::byte> bytes(static_cast<std::size_t>(count) * 160);
    for (std::uint32_t primitive = 0; primitive < count; ++primitive) {
        const auto record = static_cast<std::size_t>(primitive) * 160;
        for (std::size_t vertex = 0; vertex < 4; ++vertex) {
            const auto at = record + vertex * 24;
            const float x = x_shift + static_cast<float>(primitive * 100 + vertex * 3);
            const float y = y_shift + static_cast<float>(primitive * 20 + vertex * 2);
            const float z = 0.25f + static_cast<float>(vertex);
            const float u = 0.1f * static_cast<float>(vertex + 1);
            const float v = 0.2f * static_cast<float>(vertex + 1);
            Store(bytes, at, x);
            Store(bytes, at + 4, y);
            Store(bytes, at + 8, z);
            Store(bytes, at + 12, color + static_cast<std::uint32_t>(vertex));
            Store(bytes, at + 16, u);
            Store(bytes, at + 20, v);
        }
        Store(bytes, record + 144, flags);
        Store(bytes, record + 152, texture + primitive);
    }
    return bytes;
}

std::vector<std::byte> MakeRibbon(std::uint32_t count, float x_shift,
                                  float y_shift, std::uint32_t color) {
    std::vector<std::byte> bytes(static_cast<std::size_t>(count) * 120);
    for (std::uint32_t primitive = 0; primitive < count; ++primitive) {
        const auto record = static_cast<std::size_t>(primitive) * 120;
        for (std::size_t vertex = 0; vertex < 6; ++vertex) {
            const auto at = record + vertex * 20;
            const float x = x_shift + static_cast<float>(primitive * 50 + vertex * 2);
            const float y = y_shift + static_cast<float>(primitive * 10 + vertex);
            const float z = 0.5f + static_cast<float>(vertex);
            const float rhw = 1.0f + static_cast<float>(vertex) * 0.1f;
            Store(bytes, at, x);
            Store(bytes, at + 4, y);
            Store(bytes, at + 8, z);
            Store(bytes, at + 12, rhw);
            Store(bytes, at + 16, color + static_cast<std::uint32_t>(vertex));
        }
    }
    return bytes;
}

bool TexturedXYByte(std::size_t byte) {
    const auto primitive = byte / 160;
    const auto within = byte % 160;
    (void)primitive;
    for (std::size_t vertex = 0; vertex < 4; ++vertex) {
        const auto at = vertex * 24;
        if (within >= at && within < at + 8) return true;
    }
    return false;
}

bool RibbonXYByte(std::size_t byte) {
    const auto within = byte % 120;
    for (std::size_t vertex = 0; vertex < 6; ++vertex) {
        const auto at = vertex * 20;
        if (within >= at && within < at + 8) return true;
    }
    return false;
}

void CheckOnlyTexturedXYChanged(std::span<const std::byte> output,
                                std::span<const std::byte> current) {
    Check(output.size() == current.size());
    for (std::size_t i = 0; i < output.size(); ++i) {
        if (!TexturedXYByte(i)) Check(output[i] == current[i]);
    }
}

void CheckOnlyRibbonXYChanged(std::span<const std::byte> output,
                              std::span<const std::byte> current) {
    Check(output.size() == current.size());
    for (std::size_t i = 0; i < output.size(); ++i) {
        if (!RibbonXYByte(i)) Check(output[i] == current[i]);
    }
}

void TestTexturedPredictedCopyAndOpaqueBytes() {
    nosfix::PrimitiveMotion motion;
    auto first = MakeTextured(2, 10.0f, 20.0f, 0x10203040);
    auto current = MakeTextured(2, 20.0f, 15.0f, 0xA0B0C0D0);
    const auto unchanged = current;
    Check(motion.Capture(kKeyA, 100, nosfix::PrimitiveKind::Textured, 2, first));
    Check(motion.Capture(kKeyA, 101, nosfix::PrimitiveKind::Textured, 2, current));
    Check(motion.HasMotionHistory());

    for (const double alpha : {0.25, 0.5, 0.75}) {
        const auto output = motion.DrawCopy(kKeyA, 101, alpha,
            nosfix::PrimitiveKind::Textured, 2, current);
        Check(!output.empty());
        CheckOnlyTexturedXYChanged(output, current);
        Check(current == unchanged); // Draw never mutates the game-owned buffer.

        const float x0 = Load<float>(output, 0);
        const float y0 = Load<float>(output, 4);
        Check(std::abs(x0 - (20.0f + 10.0f * alpha)) < 0.001f);
        Check(std::abs(y0 - (15.0f - 5.0f * alpha)) < 0.001f);
        Check(Load<std::uint32_t>(output, 12) == 0xA0B0C0D0);
        Check(Load<std::uint32_t>(output, 144) == kFlags);
        Check(Load<std::uint64_t>(output, 152) == 0x123456789ABCDEF0ull);
        Check(Load<float>(output, 16) == Load<float>(current, 16));
    }

    const auto endpoint = motion.DrawCopy(kKeyA, 101, 2.0,
        nosfix::PrimitiveKind::Textured, 2, current); // Clamped to one tick.
    Check(!endpoint.empty());
    Check(std::abs(Load<float>(endpoint, 0) - 30.0f) < 0.001f);

    auto changed_live = current;
    changed_live[12] ^= std::byte{1}; // Even a color mutation invalidates the source copy.
    Check(motion.DrawCopy(kKeyA, 101, 0.5,
        nosfix::PrimitiveKind::Textured, 2, changed_live).empty());
    Check(!motion.HasCurrent());
    Check(current == unchanged);
}

void TestRibbonPredictedCopy() {
    nosfix::PrimitiveMotion motion;
    auto first = MakeRibbon(2, 100.0f, 40.0f, 0x01020304);
    auto current = MakeRibbon(2, 108.0f, 44.0f, 0xA1A2A3A4);
    const auto original = current;
    Check(motion.Capture(kKeyA, 20, nosfix::PrimitiveKind::Ribbon, 2, first));
    Check(motion.Capture(kKeyA, 21, nosfix::PrimitiveKind::Ribbon, 2, current));
    const auto output = motion.DrawCopy(kKeyA, 21, 0.5,
        nosfix::PrimitiveKind::Ribbon, 2, current);
    Check(!output.empty());
    CheckOnlyRibbonXYChanged(output, current);
    Check(std::abs(Load<float>(output, 0) - 112.0f) < 0.001f);
    Check(std::abs(Load<float>(output, 4) - 46.0f) < 0.001f);
    Check(Load<float>(output, 8) == Load<float>(current, 8));
    Check(Load<float>(output, 12) == Load<float>(current, 12));
    Check(Load<std::uint32_t>(output, 16) == 0xA1A2A3A4);
    Check(current == original);
}

void TestSameTickReplacementAndDeltaGate() {
    nosfix::PrimitiveMotion motion;
    auto first = MakeRibbon(1, 0.0f, 0.0f, 0x01010101);
    auto current = MakeRibbon(1, 10.0f, 0.0f, 0x02020202);
    Check(motion.Capture(kKeyA, 1, nosfix::PrimitiveKind::Ribbon, 1, first));
    Check(motion.Capture(kKeyA, 2, nosfix::PrimitiveKind::Ribbon, 1, current));
    auto replacement = MakeRibbon(1, 12.0f, 0.0f, 0x03030303);
    Check(motion.Capture(kKeyA, 2, nosfix::PrimitiveKind::Ribbon, 1, replacement));
    const auto repeated = motion.DrawCopy(kKeyA, 2, 0.5,
        nosfix::PrimitiveKind::Ribbon, 1, replacement);
    Check(!repeated.empty());
    Check(std::abs(Load<float>(repeated, 0) - 18.0f) < 0.001f);

    auto jump = MakeRibbon(1, 141.0f, 0.0f, 0x04040404);
    Check(motion.Capture(kKeyA, 3, nosfix::PrimitiveKind::Ribbon, 1, jump));
    Check(!motion.HasMotionHistory()); // 129 px/tick drops prediction.
    Check(motion.DrawCopy(kKeyA, 3, 0.5,
        nosfix::PrimitiveKind::Ribbon, 1, jump).empty());
}

void TestFallbackAndIdentityChanges() {
    nosfix::PrimitiveMotion motion;
    auto current = MakeTextured(1, 30.0f, 40.0f, 0x55667788);
    Check(motion.DrawCopy(kKeyA, 1, 0.5, nosfix::PrimitiveKind::Textured,
                          1, current).empty());
    Check(motion.Capture(kKeyA, 1, nosfix::PrimitiveKind::Textured, 1, current));
    Check(motion.DrawCopy(kKeyA, 1, 0.5, nosfix::PrimitiveKind::Textured,
                          1, current).empty()); // First snapshot has no history.

    auto next = MakeTextured(1, 35.0f, 42.0f, 0x66778899);
    Check(motion.Capture(kKeyA, 2, nosfix::PrimitiveKind::Textured, 1, next));
    Check(motion.HasMotionHistory());
    Check(motion.Capture(kKeyB, 3, nosfix::PrimitiveKind::Textured, 1, next));
    Check(!motion.HasMotionHistory());
    Check(motion.DrawCopy(kKeyB, 3, 0.5, nosfix::PrimitiveKind::Textured,
                          1, next).empty());

    Check(motion.Capture(kKeyB, 5, nosfix::PrimitiveKind::Textured, 1, next));
    Check(!motion.HasMotionHistory()); // Tick gap.
    Check(motion.DrawCopy(kKeyB, 3, 0.5, nosfix::PrimitiveKind::Textured,
                          1, next).empty()); // Stale tick.
    Check(motion.DrawCopy(kKeyB, 5, 0.0, nosfix::PrimitiveKind::Textured,
                          1, next).empty()); // Alpha zero is pass-through.
    Check(motion.DrawCopy(kKeyB, 5, std::numeric_limits<double>::infinity(),
                          nosfix::PrimitiveKind::Textured, 1, next).empty());
}

void TestTexturedMetadataResetAndFlags() {
    nosfix::PrimitiveMotion motion;
    auto previous = MakeTextured(1, 0.0f, 0.0f, 0x11111111);
    auto current = MakeTextured(1, 4.0f, 0.0f, 0x22222222);
    Check(motion.Capture(kKeyA, 1, nosfix::PrimitiveKind::Textured, 1, previous));
    Check(motion.Capture(kKeyA, 2, nosfix::PrimitiveKind::Textured, 1, current));
    Check(motion.HasMotionHistory());

    auto flags_changed = MakeTextured(1, 8.0f, 0.0f, 0x33333333, 0xC0000000);
    Check(motion.Capture(kKeyA, 3, nosfix::PrimitiveKind::Textured, 1, flags_changed));
    Check(!motion.HasMotionHistory()); // Both pass mask, full flags still differ.

    auto texture_changed = MakeTextured(1, 9.0f, 0.0f, 0x44444444,
                                        0xC0000000, 0x9999999999999999ull);
    Check(motion.Capture(kKeyA, 4, nosfix::PrimitiveKind::Textured, 1, texture_changed));
    Check(!motion.HasMotionHistory());

    auto uv_changed = MakeTextured(1, 10.0f, 0.0f, 0x55555555,
                                   0xC0000000, 0x9999999999999999ull);
    const float new_u = 0.875f;
    Store(uv_changed, 16, new_u);
    Check(motion.Capture(kKeyA, 5, nosfix::PrimitiveKind::Textured, 1, uv_changed));
    Check(!motion.HasMotionHistory());

    auto invalid_flags = MakeTextured(1, 11.0f, 0.0f, 0x66666666, 0x90000000);
    Check(!motion.Capture(kKeyA, 6, nosfix::PrimitiveKind::Textured, 1, invalid_flags));
    Check(!motion.HasCurrent());
}

void TestDiscontinuityCountKindAndInvalidSources() {
    nosfix::PrimitiveMotion motion;
    auto textured = MakeTextured(1, 10.0f, 10.0f, 0x11111111);
    auto next = MakeTextured(1, 12.0f, 10.0f, 0x22222222);
    Check(motion.Capture(kKeyA, 1, nosfix::PrimitiveKind::Textured, 1, textured));
    Check(motion.Capture(kKeyA, 2, nosfix::PrimitiveKind::Textured, 1, next));
    Check(motion.HasMotionHistory());
    Check(motion.Capture(kKeyA, 3, nosfix::PrimitiveKind::Textured, 1, next, true));
    Check(!motion.HasMotionHistory()); // Caller signals pause/seek/correction.

    auto two = MakeTextured(2, 20.0f, 10.0f, 0x33333333);
    Check(motion.Capture(kKeyA, 4, nosfix::PrimitiveKind::Textured, 2, two));
    Check(!motion.HasMotionHistory()); // Count change starts a new track.
    auto ribbon = MakeRibbon(2, 20.0f, 10.0f, 0x44444444);
    Check(motion.Capture(kKeyA, 5, nosfix::PrimitiveKind::Ribbon, 2, ribbon));
    Check(!motion.HasMotionHistory()); // Primitive kind/stride change.

    auto nonfinite = ribbon;
    const float nan = std::numeric_limits<float>::quiet_NaN();
    Store(nonfinite, 0, nan);
    Check(!motion.Capture(kKeyA, 6, nosfix::PrimitiveKind::Ribbon, 2, nonfinite));
    Check(!motion.HasCurrent());

    Check(!motion.Capture(kKeyA, 7, nosfix::PrimitiveKind::Ribbon, 0, ribbon));
    Check(!motion.Capture(kKeyA, 7, nosfix::PrimitiveKind::Ribbon,
                          nosfix::PrimitiveMotion::kMaximumPrimitiveCount + 1,
                          std::span<const std::byte>(ribbon)));
    Check(!motion.Capture(kKeyA, 7, nosfix::PrimitiveKind::Ribbon, 2,
                          std::span<const std::byte>(ribbon).first(1)));
    Check(!motion.HasCurrent());
}

} // namespace

int main() {
    TestTexturedPredictedCopyAndOpaqueBytes();
    TestRibbonPredictedCopy();
    TestSameTickReplacementAndDeltaGate();
    TestFallbackAndIdentityChanges();
    TestTexturedMetadataResetAndFlags();
    TestDiscontinuityCountKindAndInvalidSources();
}
