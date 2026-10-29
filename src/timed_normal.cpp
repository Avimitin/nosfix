#include "nosfix/timed_normal.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>

namespace nosfix {
namespace {

constexpr std::uint32_t kActivePersistentFlags = 0xC0000000U;
constexpr std::size_t kVertexStride = 24;
constexpr std::size_t kColorOffset = 12;
constexpr std::size_t kFlagsOffset = 144;
constexpr double kMaximumTimeMs = 1.0e12;
constexpr double kMaximumDistance = 1.0e7;
constexpr float kMaximumGeometry = 1.0e6f;

template <typename T>
T Read(const std::byte* bytes, std::size_t offset) noexcept {
    T value{};
    std::memcpy(&value, bytes + offset, sizeof(value));
    return value;
}

template <typename T>
void Write(std::byte* bytes, std::size_t offset, const T& value) noexcept {
    std::memcpy(bytes + offset, &value, sizeof(value));
}

bool ValidSpec(const TimedNormalSpec& spec) noexcept {
    return std::isfinite(spec.target_ms) &&
        std::abs(spec.target_ms) <= kMaximumTimeMs &&
        std::isfinite(spec.visual_offset_ms) &&
        std::abs(spec.visual_offset_ms) <= kMaximumTimeMs &&
        std::isfinite(spec.legacy_units_per_second) &&
        spec.legacy_units_per_second > 0.0f &&
        spec.legacy_units_per_second <= kMaximumGeometry &&
        std::isfinite(spec.half_width) && spec.half_width > 0.0f &&
        spec.half_width <= kMaximumGeometry &&
        std::isfinite(spec.half_height) && spec.half_height > 0.0f &&
        spec.half_height <= kMaximumGeometry &&
        std::isfinite(spec.x_shift) &&
        std::abs(spec.x_shift) <= kMaximumGeometry &&
        std::isfinite(spec.opacity);
}

bool ValidRaw(std::span<const std::byte> raw) noexcept {
    if (raw.size() != TimedNormal::kPrimitiveBytes) return false;
    if ((Read<std::uint32_t>(raw.data(), kFlagsOffset) & kActivePersistentFlags) !=
        kActivePersistentFlags) {
        return false;
    }
    for (std::size_t vertex = 0; vertex < 4; ++vertex) {
        const auto offset = vertex * kVertexStride;
        const float x = Read<float>(raw.data(), offset);
        const float y = Read<float>(raw.data(), offset + sizeof(float));
        if (!std::isfinite(x) || !std::isfinite(y) ||
            std::abs(x) > kMaximumGeometry || std::abs(y) > kMaximumGeometry) {
            return false;
        }
    }
    return true;
}

} // namespace

bool TimedNormal::Capture(const TimedNormalSpec& spec,
                          std::span<const std::byte> raw) noexcept {
    if (!ValidSpec(spec) || !ValidRaw(raw)) {
        Reset();
        return false;
    }

    std::memcpy(captured_.data(), raw.data(), captured_.size());
    spec_ = spec;
    has_capture_ = true;
    return true;
}

std::span<const std::byte> TimedNormal::Draw(
    double song_ms, std::span<const std::byte> live,
    NoteProjectionCallback project, void* context) noexcept {
    if (!has_capture_ || !project || !std::isfinite(song_ms) ||
        std::abs(song_ms) > kMaximumTimeMs || live.size() != captured_.size()) {
        return {};
    }
    if (std::memcmp(live.data(), captured_.data(), captured_.size()) != 0) {
        Reset();
        return {};
    }

    const double time_delta_ms = spec_.target_ms - song_ms - spec_.visual_offset_ms;
    const double distance = time_delta_ms * 180.0 /
                            (static_cast<double>(spec_.legacy_units_per_second) * 1000.0);
    if (!std::isfinite(distance) || std::abs(distance) > kMaximumDistance) return {};

    NoteProjection projection{};
    if (!project(context, spec_.lane, static_cast<float>(distance),
                  spec_.left_key, spec_.right_key, projection)) {
        return {};
    }
    if (!std::isfinite(projection.x) || !std::isfinite(projection.y) ||
        !std::isfinite(projection.scale) || projection.scale <= 0.0f ||
        projection.scale > kMaximumGeometry || !std::isfinite(projection.opacity) ||
        std::abs(projection.x) > kMaximumGeometry ||
        std::abs(projection.y) > kMaximumGeometry) {
        return {};
    }

    const float half_width = spec_.half_width * projection.scale;
    const float half_height = spec_.half_height * projection.scale;
    const float left = projection.x + spec_.x_shift - half_width;
    const float right = projection.x + spec_.x_shift + half_width - 1.0f;
    const float top = projection.y - half_height;
    const float bottom = projection.y + half_height - 1.0f;
    if (!std::isfinite(left) || !std::isfinite(right) ||
        !std::isfinite(top) || !std::isfinite(bottom) ||
        right < left || bottom < top ||
        std::abs(left) > kMaximumGeometry || std::abs(right) > kMaximumGeometry ||
        std::abs(top) > kMaximumGeometry || std::abs(bottom) > kMaximumGeometry) {
        return {};
    }

    std::memcpy(draw_copy_.data(), captured_.data(), captured_.size());
    const std::array<float, 4> x{left, right, left, right};
    const std::array<float, 4> y{top, top, bottom, bottom};
    for (std::size_t vertex = 0; vertex < 4; ++vertex) {
        const std::size_t offset = vertex * kVertexStride;
        Write(draw_copy_.data(), offset, x[vertex]);
        Write(draw_copy_.data(), offset + sizeof(float), y[vertex]);

        const std::uint32_t old_color = Read<std::uint32_t>(draw_copy_.data(),
                                                            offset + kColorOffset);
        const float alpha = std::clamp(projection.opacity * spec_.opacity, 0.0f, 1.0f);
        const auto alpha_byte = static_cast<std::uint32_t>(alpha * 255.0f);
        const std::uint32_t color = (alpha_byte << 24) | (old_color & 0x00FFFFFFU);
        Write(draw_copy_.data(), offset + kColorOffset, color);
    }

    return draw_copy_;
}

void TimedNormal::Reset() noexcept {
    spec_ = {};
    has_capture_ = false;
}

} // namespace nosfix
