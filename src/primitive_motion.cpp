#include "nosfix/primitive_motion.hpp"

#include <array>
#include <cmath>
#include <cstring>

namespace nosfix {
namespace {

constexpr std::size_t kRibbonStride = 120;
constexpr std::size_t kTexturedStride = 160;
constexpr std::size_t kRibbonVerticesPerPrimitive = 6;
constexpr std::size_t kTexturedVerticesPerPrimitive = 4;
constexpr std::size_t kVertexStride = 24;
constexpr std::size_t kRibbonVertexStride = 20;
constexpr std::size_t kFlagsOffset = 144;
constexpr std::size_t kTextureOffset = 152;
constexpr std::array<std::size_t, 4> kUvOffsets{16, 40, 64, 88};

std::uint32_t ReadU32(const std::byte* address) noexcept {
    std::uint32_t value = 0;
    std::memcpy(&value, address, sizeof(value));
    return value;
}

} // namespace

bool PrimitiveMotion::Layout(PrimitiveKind kind, std::uint32_t count,
                             std::size_t& stride,
                             std::size_t& vertex_count) noexcept {
    if (count == 0 || count > kMaximumPrimitiveCount) return false;
    switch (kind) {
    case PrimitiveKind::Ribbon:
        stride = kRibbonStride;
        vertex_count = static_cast<std::size_t>(count) * kRibbonVerticesPerPrimitive;
        return true;
    case PrimitiveKind::Textured:
        stride = kTexturedStride;
        vertex_count = static_cast<std::size_t>(count) * kTexturedVerticesPerPrimitive;
        return true;
    default:
        return false;
    }
}

bool PrimitiveMotion::ValidRaw(PrimitiveKind kind, std::uint32_t count,
                               std::span<const std::byte> raw,
                               std::size_t stride) noexcept {
    if (raw.size() != static_cast<std::size_t>(count) * stride) return false;
    if (kind != PrimitiveKind::Textured) return true;

    for (std::uint32_t primitive = 0; primitive < count; ++primitive) {
        const auto* record = raw.data() + static_cast<std::size_t>(primitive) * stride;
        const std::uint32_t flags = ReadU32(record + kFlagsOffset);
        if ((flags & kTexturedActivePersistentFlags) !=
            kTexturedActivePersistentFlags) return false;
    }
    return true;
}

bool PrimitiveMotion::StableTexturedMetadata(
    std::span<const std::byte> previous,
    std::span<const std::byte> current,
    std::uint32_t count) noexcept {
    if (previous.size() != current.size() ||
        previous.size() != static_cast<std::size_t>(count) * kTexturedStride) return false;

    for (std::uint32_t primitive = 0; primitive < count; ++primitive) {
        const auto offset = static_cast<std::size_t>(primitive) * kTexturedStride;
        const auto* before = previous.data() + offset;
        const auto* after = current.data() + offset;
        if (std::memcmp(before + kFlagsOffset, after + kFlagsOffset, sizeof(std::uint32_t)) ||
            std::memcmp(before + kTextureOffset, after + kTextureOffset, sizeof(std::uint64_t)))
            return false;
        for (const auto uv_offset : kUvOffsets) {
            if (std::memcmp(before + uv_offset, after + uv_offset, sizeof(std::uint64_t)))
                return false;
        }
    }
    return true;
}

void PrimitiveMotion::ReadPositions(PrimitiveKind kind, std::uint32_t count,
                                    std::span<const std::byte> raw,
                                    std::span<Vertex2> positions) noexcept {
    const std::size_t vertices_per_primitive = kind == PrimitiveKind::Ribbon
        ? kRibbonVerticesPerPrimitive : kTexturedVerticesPerPrimitive;
    const std::size_t vertex_stride = kind == PrimitiveKind::Ribbon
        ? kRibbonVertexStride : kVertexStride;
    std::size_t point = 0;
    for (std::uint32_t primitive = 0; primitive < count; ++primitive) {
        const auto* record = raw.data() + static_cast<std::size_t>(primitive) *
            (kind == PrimitiveKind::Ribbon ? kRibbonStride : kTexturedStride);
        for (std::size_t vertex = 0; vertex < vertices_per_primitive; ++vertex) {
            const auto* position = record + vertex * vertex_stride;
            std::memcpy(&positions[point].x, position, sizeof(float));
            std::memcpy(&positions[point].y, position + sizeof(float), sizeof(float));
            ++point;
        }
    }
}

void PrimitiveMotion::WritePositions(PrimitiveKind kind, std::uint32_t count,
                                     std::span<std::byte> raw,
                                     std::span<const Vertex2> positions) noexcept {
    const std::size_t vertices_per_primitive = kind == PrimitiveKind::Ribbon
        ? kRibbonVerticesPerPrimitive : kTexturedVerticesPerPrimitive;
    const std::size_t vertex_stride = kind == PrimitiveKind::Ribbon
        ? kRibbonVertexStride : kVertexStride;
    std::size_t point = 0;
    for (std::uint32_t primitive = 0; primitive < count; ++primitive) {
        auto* record = raw.data() + static_cast<std::size_t>(primitive) *
            (kind == PrimitiveKind::Ribbon ? kRibbonStride : kTexturedStride);
        for (std::size_t vertex = 0; vertex < vertices_per_primitive; ++vertex) {
            auto* position = record + vertex * vertex_stride;
            std::memcpy(position, &positions[point].x, sizeof(float));
            std::memcpy(position + sizeof(float), &positions[point].y, sizeof(float));
            ++point;
        }
    }
}

bool PrimitiveMotion::Capture(GeometryKey key, std::uint64_t tick_serial,
                              PrimitiveKind kind, std::uint32_t count,
                              std::span<const std::byte> raw,
                              bool discontinuity) noexcept {
    std::size_t stride = 0;
    std::size_t vertex_count = 0;
    if (!Layout(kind, count, stride, vertex_count) ||
        !ValidRaw(kind, count, raw, stride)) {
        Reset();
        return false;
    }

    try {
        current_positions_.resize(vertex_count);
        predicted_positions_.resize(vertex_count);
        draw_copy_.resize(raw.size());
        ReadPositions(kind, count, raw, current_positions_);

        bool reset_history = discontinuity || !has_current_ || key != key_ ||
            kind != kind_ || count != count_;
        if (!reset_history && kind == PrimitiveKind::Textured &&
            !StableTexturedMetadata(current_raw_, raw, count)) {
            reset_history = true;
        }

        if (!predictor_.Capture(key, tick_serial, current_positions_, reset_history)) {
            Reset();
            return false;
        }

        current_raw_.assign(raw.begin(), raw.end());
        key_ = key;
        current_tick_ = tick_serial;
        kind_ = kind;
        count_ = count;
        has_current_ = true;
        return true;
    } catch (...) {
        Reset();
        return false;
    }
}

std::span<const std::byte> PrimitiveMotion::DrawCopy(
    GeometryKey key, std::uint64_t tick_serial, double alpha,
    PrimitiveKind kind, std::uint32_t count,
    std::span<const std::byte> live) noexcept {
    if (!has_current_) return {};
    if (key != key_ || tick_serial != current_tick_ || kind != kind_ || count != count_) {
        Reset();
        return {};
    }
    if (!std::isfinite(alpha) || alpha <= 0.0 || live.size() != current_raw_.size())
        return {};
    if (std::memcmp(live.data(), current_raw_.data(), current_raw_.size()) != 0) {
        Reset();
        return {};
    }
    if (!predictor_.HasMotionHistory() || predicted_positions_.size() != current_positions_.size())
        return {};
    if (!predictor_.Evaluate(key, tick_serial, alpha, predicted_positions_)) return {};

    std::memcpy(draw_copy_.data(), current_raw_.data(), current_raw_.size());
    WritePositions(kind, count, draw_copy_, predicted_positions_);
    return draw_copy_;
}

void PrimitiveMotion::Reset() noexcept {
    predictor_.Reset();
    current_raw_.clear();
    draw_copy_.clear();
    current_positions_.clear();
    predicted_positions_.clear();
    key_ = {};
    current_tick_ = 0;
    kind_ = PrimitiveKind::Ribbon;
    count_ = 0;
    has_current_ = false;
}

} // namespace nosfix
