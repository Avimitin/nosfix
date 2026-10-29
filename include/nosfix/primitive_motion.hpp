#pragma once

#include "nosfix/motion.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace nosfix {

enum class PrimitiveKind : std::uint32_t {
    Ribbon = 1,
    Textured = 2,
};

// Adapts a captured game-owned primitive buffer to a render-only predicted
// copy. Each instance owns one buffer track; it never writes into the input.
class PrimitiveMotion final {
public:
    // The trill update emits up to 90 box records per strip; the long note
    // emits at most 37. The legacy predictor accepts the larger object too.
    static constexpr std::uint32_t kMaximumPrimitiveCount = 96;
    static constexpr std::uint32_t kTexturedActivePersistentFlags = 0xC0000000;

    [[nodiscard]] bool Capture(GeometryKey key, std::uint64_t tick_serial,
                               PrimitiveKind kind, std::uint32_t count,
                               std::span<const std::byte> raw,
                               bool discontinuity = false) noexcept;

    // Returns an owned predicted copy, or an empty span to pass the original
    // live buffer through. Consume the returned span before the next draw/capture.
    [[nodiscard]] std::span<const std::byte> DrawCopy(
        GeometryKey key, std::uint64_t tick_serial, double alpha,
        PrimitiveKind kind, std::uint32_t count,
        std::span<const std::byte> live) noexcept;

    void Reset() noexcept;
    [[nodiscard]] bool HasCurrent() const noexcept { return has_current_; }
    [[nodiscard]] bool HasMotionHistory() const noexcept {
        return predictor_.HasMotionHistory();
    }
    [[nodiscard]] std::uint32_t Count() const noexcept { return count_; }
    [[nodiscard]] PrimitiveKind Kind() const noexcept { return kind_; }

private:
    [[nodiscard]] static bool Layout(PrimitiveKind kind, std::uint32_t count,
                                     std::size_t& stride,
                                     std::size_t& vertex_count) noexcept;
    [[nodiscard]] static bool ValidRaw(PrimitiveKind kind, std::uint32_t count,
                                      std::span<const std::byte> raw,
                                      std::size_t stride) noexcept;
    [[nodiscard]] static bool StableTexturedMetadata(
        std::span<const std::byte> previous,
        std::span<const std::byte> current,
        std::uint32_t count) noexcept;
    static void ReadPositions(PrimitiveKind kind, std::uint32_t count,
                              std::span<const std::byte> raw,
                              std::span<Vertex2> positions) noexcept;
    static void WritePositions(PrimitiveKind kind, std::uint32_t count,
                               std::span<std::byte> raw,
                               std::span<const Vertex2> positions) noexcept;

    GeometryPredictor predictor_;
    GeometryKey key_{};
    std::uint64_t current_tick_ = 0;
    PrimitiveKind kind_ = PrimitiveKind::Ribbon;
    std::uint32_t count_ = 0;
    bool has_current_ = false;
    std::vector<std::byte> current_raw_;
    std::vector<std::byte> draw_copy_;
    std::vector<Vertex2> current_positions_;
    std::vector<Vertex2> predicted_positions_;
};

} // namespace nosfix
