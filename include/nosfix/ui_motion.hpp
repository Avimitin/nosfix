#pragma once

#include <array>
#include <cstdint>
#include <span>

namespace nosfix {

// AFP display nodes are pool allocated. The allocator stamp supplements the
// address so a reused node starts a fresh history; parent, type, and resource
// pointer also invalidate the track after reparenting or identity changes.
struct UiNodeKey {
    std::uintptr_t node = 0;
    std::uint32_t allocation_stamp = 0;
    std::uintptr_t parent = 0;
    std::uint16_t type = 0;
    std::uintptr_t resource = 0;

    friend constexpr bool operator==(const UiNodeKey&, const UiNodeKey&) = default;
};

// A single track is owned by the caller for one AFP display node. Captures
// sample its local column-major affine matrix at simulation ticks; evaluation
// predicts translation only from the latest two distinct ticks.
class UiTransformPredictor final {
public:
    static constexpr std::size_t kElementCount = 16;
    static constexpr double kMaximumTranslationDeltaPerTick = 128.0;
    static constexpr double kMaximumCoordinateMagnitude = 1'000'000.0;

    [[nodiscard]] bool Capture(UiNodeKey key, std::uint64_t tick_serial,
                               std::span<const float, kElementCount> matrix,
                               bool discontinuity = false) noexcept;

    // Writes the complete latest matrix, predicting only translation (indices
    // 12..14). Orientation, scale, shear, and the homogeneous row stay exactly
    // as captured. Nonfinite alpha and missing history return the latest
    // matrix; finite alpha clamps to [0,1].
    [[nodiscard]] bool Evaluate(UiNodeKey key, std::uint64_t tick_serial,
                                double alpha,
                                std::span<float, kElementCount> output) const noexcept;

    void Reset() noexcept;
    [[nodiscard]] bool HasHistory() const noexcept { return has_previous_; }

private:
    using Matrix = std::array<float, kElementCount>;

    [[nodiscard]] static bool ValidMatrix(
        std::span<const float, kElementCount> matrix) noexcept;
    [[nodiscard]] static bool SafeDelta(const Matrix& previous,
                                        const Matrix& current) noexcept;
    void SetCurrentOnly(UiNodeKey key, std::uint64_t tick_serial,
                        std::span<const float, kElementCount> matrix) noexcept;

    UiNodeKey key_{};
    std::uint64_t current_tick_ = 0;
    Matrix previous_{};
    Matrix current_{};
    bool has_current_ = false;
    bool has_previous_ = false;
};

} // namespace nosfix
