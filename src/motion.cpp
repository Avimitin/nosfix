#include "nosfix/motion.hpp"

#include <algorithm>
#include <cmath>

namespace nosfix {

bool GeometryPredictor::ValidVertices(std::span<const Vertex2> vertices) noexcept {
    if (vertices.empty()) return false;
    for (const auto& vertex : vertices) {
        if (!std::isfinite(vertex.x) || !std::isfinite(vertex.y) ||
            std::abs(static_cast<double>(vertex.x)) > kMaximumCoordinateMagnitude ||
            std::abs(static_cast<double>(vertex.y)) > kMaximumCoordinateMagnitude) {
            return false;
        }
    }
    return true;
}

bool GeometryPredictor::SafeDelta(std::span<const Vertex2> previous,
                                  std::span<const Vertex2> current) noexcept {
    if (previous.size() != current.size()) return false;
    constexpr double maximum_distance_squared =
        kMaximumDeltaPerTick * kMaximumDeltaPerTick;
    for (std::size_t i = 0; i < previous.size(); ++i) {
        const double dx = static_cast<double>(current[i].x) - previous[i].x;
        const double dy = static_cast<double>(current[i].y) - previous[i].y;
        if (dx * dx + dy * dy > maximum_distance_squared) return false;
    }
    return true;
}

void GeometryPredictor::SetCurrentOnly(GeometryKey key,
                                       std::uint64_t tick_serial,
                                       std::span<const Vertex2> vertices) {
    current_.assign(vertices.begin(), vertices.end());
    previous_.clear();
    key_ = key;
    current_tick_ = tick_serial;
    has_current_ = true;
    has_previous_ = false;
}

bool GeometryPredictor::Capture(GeometryKey key, std::uint64_t tick_serial,
                                std::span<const Vertex2> vertices,
                                bool discontinuity) {
    if (!ValidVertices(vertices)) {
        Reset();
        return false;
    }

    try {
        if (!has_current_ || key != key_ || discontinuity ||
            vertices.size() != current_.size() || tick_serial < current_tick_) {
            SetCurrentOnly(key, tick_serial, vertices);
            return true;
        }

        if (tick_serial == current_tick_) {
            current_.assign(vertices.begin(), vertices.end());
            if (has_previous_ && !SafeDelta(previous_, current_)) {
                previous_.clear();
                has_previous_ = false;
            }
            return true;
        }

        const bool consecutive = current_tick_ != UINT64_MAX &&
                                 tick_serial == current_tick_ + 1;
        if (!consecutive) {
            SetCurrentOnly(key, tick_serial, vertices);
            return true;
        }

        previous_.assign(current_.begin(), current_.end());
        current_.assign(vertices.begin(), vertices.end());
        current_tick_ = tick_serial;
        has_previous_ = SafeDelta(previous_, current_);
        if (!has_previous_) previous_.clear();
        return true;
    } catch (...) {
        Reset();
        return false;
    }
}

void GeometryPredictor::CopyCurrent(std::span<Vertex2> output) const noexcept {
    std::copy(current_.begin(), current_.end(), output.begin());
}

bool GeometryPredictor::Evaluate(GeometryKey key, std::uint64_t tick_serial,
                                 double alpha,
                                 std::span<Vertex2> output) const noexcept {
    if (!has_current_ || key != key_ || tick_serial != current_tick_ ||
        output.size() != current_.size()) return false;

    if (!std::isfinite(alpha)) {
        CopyCurrent(output);
        return true;
    }
    alpha = std::clamp(alpha, 0.0, 1.0);
    if (!has_previous_ || alpha == 0.0) {
        CopyCurrent(output);
        return true;
    }

    // Validate the complete candidate first so a single unsafe vertex falls
    // back to the entire latest snapshot without partially writing the span.
    for (std::size_t i = 0; i < current_.size(); ++i) {
        const double x = current_[i].x +
            (static_cast<double>(current_[i].x) - previous_[i].x) * alpha;
        const double y = current_[i].y +
            (static_cast<double>(current_[i].y) - previous_[i].y) * alpha;
        if (!std::isfinite(x) || !std::isfinite(y) ||
            std::abs(x) > kMaximumCoordinateMagnitude ||
            std::abs(y) > kMaximumCoordinateMagnitude ||
            !std::isfinite(static_cast<float>(x)) ||
            !std::isfinite(static_cast<float>(y))) {
            CopyCurrent(output);
            return true;
        }
    }

    for (std::size_t i = 0; i < current_.size(); ++i) {
        output[i].x = static_cast<float>(current_[i].x +
            (static_cast<double>(current_[i].x) - previous_[i].x) * alpha);
        output[i].y = static_cast<float>(current_[i].y +
            (static_cast<double>(current_[i].y) - previous_[i].y) * alpha);
    }
    return true;
}

std::span<const Vertex2> GeometryPredictor::CurrentSnapshot(
    GeometryKey key, std::uint64_t tick_serial) const noexcept {
    if (!has_current_ || key != key_ || tick_serial != current_tick_) return {};
    return current_;
}

void GeometryPredictor::Reset() noexcept {
    previous_.clear();
    current_.clear();
    key_ = {};
    current_tick_ = 0;
    has_current_ = false;
    has_previous_ = false;
}

} // namespace nosfix
