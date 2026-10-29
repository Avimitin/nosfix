#include "nosfix/ui_motion.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace nosfix {
namespace {

constexpr std::array<std::size_t, 3> kPredictedElements{12, 13, 14};

} // namespace

bool UiTransformPredictor::ValidMatrix(
    std::span<const float, kElementCount> matrix) noexcept {
    for (const float value : matrix) {
        if (!std::isfinite(value) ||
            std::abs(static_cast<double>(value)) > kMaximumCoordinateMagnitude) {
            return false;
        }
    }

    // The renderer consumes affine transforms. Require the exact affine row
    // used by AFP's identity/local matrices; projective values pass through.
    return matrix[3] == 0.0f && matrix[7] == 0.0f && matrix[11] == 0.0f &&
           matrix[15] == 1.0f;
}

bool UiTransformPredictor::SafeDelta(const Matrix& previous,
                                     const Matrix& current) noexcept {
    for (const std::size_t i : kPredictedElements) {
        const double delta = static_cast<double>(current[i]) - previous[i];
        if (std::abs(delta) > kMaximumTranslationDeltaPerTick) return false;
    }

    // Any change to the homogeneous row invalidates this affine sample pair.
    return current[3] == previous[3] && current[7] == previous[7] &&
           current[11] == previous[11] && current[15] == previous[15];
}

void UiTransformPredictor::SetCurrentOnly(
    UiNodeKey key, std::uint64_t tick_serial,
    std::span<const float, kElementCount> matrix) noexcept {
    std::copy(matrix.begin(), matrix.end(), current_.begin());
    previous_ = {};
    key_ = key;
    current_tick_ = tick_serial;
    has_current_ = true;
    has_previous_ = false;
}

bool UiTransformPredictor::Capture(
    UiNodeKey key, std::uint64_t tick_serial,
    std::span<const float, kElementCount> matrix,
    bool discontinuity) noexcept {
    if (key.node == 0 || !ValidMatrix(matrix)) {
        Reset();
        return false;
    }

    if (!has_current_ || key != key_ || discontinuity ||
        tick_serial < current_tick_) {
        SetCurrentOnly(key, tick_serial, matrix);
        return true;
    }

    if (tick_serial == current_tick_) {
        if (std::equal(matrix.begin(), matrix.end(), current_.begin())) return true;
        // More than one distinct matrix for a node in the same simulation
        // tick means its identity or update phase is ambiguous. Start over.
        SetCurrentOnly(key, tick_serial, matrix);
        return true;
    }

    const bool consecutive = current_tick_ != UINT64_MAX &&
                             tick_serial == current_tick_ + 1;
    if (!consecutive) {
        SetCurrentOnly(key, tick_serial, matrix);
        return true;
    }

    previous_ = current_;
    std::copy(matrix.begin(), matrix.end(), current_.begin());
    current_tick_ = tick_serial;
    has_previous_ = SafeDelta(previous_, current_);
    if (!has_previous_) previous_ = {};
    return true;
}

bool UiTransformPredictor::Evaluate(
    UiNodeKey key, std::uint64_t tick_serial, double alpha,
    std::span<float, kElementCount> output) const noexcept {
    if (!has_current_ || key != key_ || tick_serial != current_tick_) return false;
    std::copy(current_.begin(), current_.end(), output.begin());

    if (!std::isfinite(alpha)) return true;
    alpha = std::clamp(alpha, 0.0, 1.0);
    if (!has_previous_ || alpha == 0.0) return true;

    Matrix candidate = current_;
    for (const std::size_t i : kPredictedElements) {
        const double value = static_cast<double>(current_[i]) +
            (static_cast<double>(current_[i]) - previous_[i]) * alpha;
        if (!std::isfinite(value) ||
            std::abs(value) > kMaximumCoordinateMagnitude ||
            !std::isfinite(static_cast<float>(value))) {
            return true; // Keep the already-copied exact latest matrix.
        }
        candidate[i] = static_cast<float>(value);
    }

    std::copy(candidate.begin(), candidate.end(), output.begin());
    return true;
}

void UiTransformPredictor::Reset() noexcept {
    key_ = {};
    current_tick_ = 0;
    previous_ = {};
    current_ = {};
    has_current_ = false;
    has_previous_ = false;
}

} // namespace nosfix
