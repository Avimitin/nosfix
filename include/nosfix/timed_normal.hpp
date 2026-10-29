#pragma once

#include <array>
#include <cstddef>
#include <span>

namespace nosfix {

struct TimedNormalSpec final {
    double target_ms = 0.0;
    double visual_offset_ms = 0.0;
    float legacy_units_per_second = 60.0f;
    int lane = 0;
    int left_key = 0;
    int right_key = 0;
    float half_width = 0.0f;
    float half_height = 0.0f;
    float x_shift = 0.0f;
    float opacity = 1.0f;
};

struct NoteProjection final {
    float x = 0.0f;
    float y = 0.0f;
    float scale = 1.0f;
    float opacity = 1.0f;
};

using NoteProjectionCallback = bool (*)(void* context, int lane, float distance,
                                        int left_key, int right_key,
                                        NoteProjection& result) noexcept;

// Stateless song-time evaluator for one captured, 160-byte textured quad.
// Draw writes only to private storage and returns an empty span for passthrough.
class TimedNormal final {
public:
    static constexpr std::size_t kPrimitiveBytes = 160;

    [[nodiscard]] bool Capture(const TimedNormalSpec& spec,
                               std::span<const std::byte> raw) noexcept;
    [[nodiscard]] std::span<const std::byte> Draw(
        double song_ms, std::span<const std::byte> live,
        NoteProjectionCallback project, void* context) noexcept;
    void Reset() noexcept;

private:
    TimedNormalSpec spec_{};
    std::array<std::byte, kPrimitiveBytes> captured_{};
    std::array<std::byte, kPrimitiveBytes> draw_copy_{};
    bool has_capture_ = false;
};

} // namespace nosfix
