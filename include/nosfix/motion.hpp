#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace nosfix {

struct GeometryKey {
    std::uint64_t note_id = 0;
    std::uint32_t primitive_role = 0;

    friend constexpr bool operator==(const GeometryKey&, const GeometryKey&) = default;
};

struct Vertex2 {
    float x = 0.0f;
    float y = 0.0f;

    friend constexpr bool operator==(const Vertex2&, const Vertex2&) = default;
};

// One predictor belongs to one note primitive. Call Capture at simulation
// boundaries and Evaluate from render-only code into caller-owned storage.
class GeometryPredictor final {
public:
    static constexpr double kMaximumDeltaPerTick = 128.0;
    static constexpr double kMaximumCoordinateMagnitude = 1'000'000.0;

    // A discontinuity marks pause/seek/correction. Invalid or empty geometry
    // clears this track and returns false. Snapshot allocation happens here.
    [[nodiscard]] bool Capture(GeometryKey key, std::uint64_t tick_serial,
                               std::span<const Vertex2> vertices,
                               bool discontinuity = false);

    // Writes the exact latest snapshot when motion history is unavailable or
    // alpha is nonfinite. Finite alpha is clamped to [0, 1]. No allocation.
    [[nodiscard]] bool Evaluate(GeometryKey key, std::uint64_t tick_serial,
                                double alpha,
                                std::span<Vertex2> output) const noexcept;

    // Read-only current snapshot for render-boundary identity checks. Empty
    // means the requested key/tick is not the current captured geometry.
    [[nodiscard]] std::span<const Vertex2> CurrentSnapshot(
        GeometryKey key, std::uint64_t tick_serial) const noexcept;

    void Reset() noexcept;
    [[nodiscard]] bool HasCurrent() const noexcept { return has_current_; }
    [[nodiscard]] bool HasMotionHistory() const noexcept { return has_previous_; }
    [[nodiscard]] std::size_t VertexCount() const noexcept { return current_.size(); }

private:
    [[nodiscard]] static bool ValidVertices(std::span<const Vertex2> vertices) noexcept;
    [[nodiscard]] static bool SafeDelta(std::span<const Vertex2> previous,
                                        std::span<const Vertex2> current) noexcept;
    void SetCurrentOnly(GeometryKey key, std::uint64_t tick_serial,
                        std::span<const Vertex2> vertices);
    void CopyCurrent(std::span<Vertex2> output) const noexcept;

    GeometryKey key_{};
    std::uint64_t current_tick_ = 0;
    std::vector<Vertex2> previous_;
    std::vector<Vertex2> current_;
    bool has_current_ = false;
    bool has_previous_ = false;
};

} // namespace nosfix
