#include "nosfix/timed_note_geometry.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>

namespace nosfix {
namespace {

constexpr std::uint32_t kTexturedPersistentFlags = 0xC0000000U;
constexpr std::size_t kVertexStride = 24;
constexpr std::size_t kColorOffset = 12;
constexpr std::size_t kTexturedFlagsOffset = 144;
constexpr std::size_t kBoxVertexStride = 20;
constexpr std::size_t kBoxColorOffset = 16;
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

bool Finite(float value, float limit = kMaximumGeometry) noexcept {
    return std::isfinite(value) && std::abs(value) <= limit;
}

bool ValidSpec(const NoteGeometrySpec& spec) noexcept {
    if (!std::isfinite(spec.start_ms) || std::abs(spec.start_ms) > kMaximumTimeMs) return false;
    if (spec.has_end &&
        (!std::isfinite(spec.end_ms) || std::abs(spec.end_ms) > kMaximumTimeMs)) {
        return false;
    }
    if (!Finite(spec.visual_offset_ms) || !Finite(spec.opacity_scale, 4.0f)) return false;
    if (!std::isfinite(spec.legacy_rate) || spec.legacy_rate < 55.0f || spec.legacy_rate > 1000.0f) {
        return false;
    }
    if (spec.lane < 0 || spec.lane >= 28 || spec.left_key < 0 ||
        spec.right_key < spec.left_key || spec.right_key > 28) {
        return false;
    }
    if (spec.scroll_mode < 0 || spec.scroll_mode > 5) return false;
    if (!Finite(spec.x_shift) || !Finite(spec.quad_half_width) ||
        !Finite(spec.quad_half_height) || !Finite(spec.ribbon_base_width) ||
        !Finite(spec.ribbon_line_half_width) || !Finite(spec.approach) ||
        !Finite(spec.ribbon_start_bias)) {
        return false;
    }
    if (spec.quad_width_divisor < 1 || spec.quad_width_divisor > 16) return false;
    if (!std::isfinite(spec.quad_height_scale) || spec.quad_height_scale <= 0.0f ||
        spec.quad_height_scale > 1000.0f) {
        return false;
    }
    if (!std::isfinite(spec.ribbon_step_divisor) ||
        spec.ribbon_step_divisor < 1.0f || spec.ribbon_step_divisor > 1000.0f) {
        return false;
    }
    if (spec.max_ribbon_segments == 0 ||
        spec.max_ribbon_segments > TimedNoteGeometry::kMaximumSegments) {
        return false;
    }
    return true;
}

bool ValidTexturedRecord(std::span<const std::byte> raw) noexcept {
    if (raw.size() != TimedNoteGeometry::kTexturedStride) return false;
    if ((Read<std::uint32_t>(raw.data(), kTexturedFlagsOffset) & kTexturedPersistentFlags) !=
        kTexturedPersistentFlags) {
        return false;
    }
    for (std::size_t vertex = 0; vertex < 4; ++vertex) {
        const std::size_t offset = vertex * kVertexStride;
        if (!Finite(Read<float>(raw.data(), offset)) ||
            !Finite(Read<float>(raw.data(), offset + sizeof(float)))) {
            return false;
        }
    }
    return true;
}

bool ValidBoxRecord(const std::byte* record) noexcept {
    for (std::size_t vertex = 0; vertex < 6; ++vertex) {
        const std::size_t offset = vertex * kBoxVertexStride;
        if (!Finite(Read<float>(record, offset)) ||
            !Finite(Read<float>(record, offset + sizeof(float)))) {
            return false;
        }
    }
    return true;
}

std::uint32_t ColorAlpha(std::uint32_t color, float opacity) noexcept {
    const float alpha = std::clamp(opacity, 0.0f, 1.0f);
    const auto byte = static_cast<std::uint32_t>(alpha * 255.0f);
    return (byte << 24) | (color & 0x00FFFFFFU);
}

bool ValidShape(NotePartShape shape) noexcept {
    switch (shape) {
    case NotePartShape::TexturedQuad:
    case NotePartShape::BoxRibbon:
    case NotePartShape::BoxLine:
    case NotePartShape::TrillRibbon:
    case NotePartShape::TrillGlow:
        return true;
    }
    return false;
}

bool IsBoxShape(NotePartShape shape) noexcept {
    return shape == NotePartShape::BoxRibbon || shape == NotePartShape::BoxLine ||
        shape == NotePartShape::TrillRibbon || shape == NotePartShape::TrillGlow;
}

bool IsTrillShape(NotePartShape shape) noexcept {
    return shape == NotePartShape::TrillRibbon || shape == NotePartShape::TrillGlow;
}

bool ValidAnchors(NoteAnchorRule rule) noexcept {
    switch (rule) {
    case NoteAnchorRule::Start:
    case NoteAnchorRule::End:
    case NoteAnchorRule::StartToEnd:
    case NoteAnchorRule::StartToLink:
        return true;
    }
    return false;
}

// Pulls the ribbon tail into the visible band exactly as the native update
// does: `while (d > approach) d -= approach / divisor`. Returns false when the
// bounded loop cannot reach the band, so the caller falls back instead of
// drawing a strip that starts outside the visible range.
bool SnapTail(const NoteGeometrySpec& spec, float tail_distance, float& tail) noexcept {
    if (!(spec.approach > 0.0f) || !std::isfinite(spec.approach) ||
        !std::isfinite(tail_distance) || !std::isfinite(spec.ribbon_start_bias)) {
        return false;
    }
    const float step = spec.approach / spec.ribbon_step_divisor;
    if (!(step > 0.0f) || !std::isfinite(step)) return false;
    float value = tail_distance - spec.ribbon_start_bias;
    for (int guard = 0; guard < 4096 && value > spec.approach; ++guard) value -= step;
    if (!(value <= spec.approach)) return false;
    tail = value;
    return true;
}

// Native 0xD5460 corner order: p1, p2, p4, p4, p2, p3 with the four colour
// groups sampled from the captured record template.
void WriteBoxQuad(std::byte* record, const std::uint32_t colors[4],
                  float p1x, float p2x, float p3x, float p4x,
                  float previous_y, float current_y) noexcept {
    const float xs[6] = {p1x, p2x, p4x, p4x, p2x, p3x};
    const float ys[6] = {previous_y, previous_y, current_y,
                         current_y, previous_y, current_y};
    const std::uint32_t vertex_colors[6] = {colors[0], colors[1], colors[2],
                                            colors[2], colors[1], colors[3]};
    for (std::size_t vertex = 0; vertex < 6; ++vertex) {
        const std::size_t offset = vertex * kBoxVertexStride;
        Write(record, offset, xs[vertex]);
        Write(record, offset + 4, ys[vertex]);
        Write(record, offset + 8, 0.0f);          // z, native writes 0
        Write(record, offset + 12, 1.0f);         // RHW, native writes 1.0f
        Write(record, offset + kBoxColorOffset, vertex_colors[vertex]);
    }
}

void TemplateColors(const std::byte* captured, std::uint32_t record_count,
                    std::uint32_t index, std::uint32_t colors[4]) noexcept {
    const std::uint32_t template_index = std::min(index, record_count - 1);
    const std::byte* source = captured + template_index * TimedNoteGeometry::kBoxStride;
    colors[0] = Read<std::uint32_t>(source, 0 * kBoxVertexStride + kBoxColorOffset);
    colors[1] = Read<std::uint32_t>(source, 1 * kBoxVertexStride + kBoxColorOffset);
    colors[2] = Read<std::uint32_t>(source, 2 * kBoxVertexStride + kBoxColorOffset);
    colors[3] = Read<std::uint32_t>(source, 5 * kBoxVertexStride + kBoxColorOffset);
}

} // namespace

const char* GeometryDiagnosticName(GeometryDiagnostic value) noexcept {
    switch (value) {
    case GeometryDiagnostic::None: return "none";
    case GeometryDiagnostic::NotCaptured: return "not-captured";
    case GeometryDiagnostic::ScrollModeUnsupported: return "scroll-mode-unsupported";
    case GeometryDiagnostic::LayoutUnverified: return "layout-unverified";
    case GeometryDiagnostic::PartCountMismatch: return "part-count-mismatch";
    case GeometryDiagnostic::MissingAnchorTime: return "missing-anchor-time";
    case GeometryDiagnostic::GlissDataBaked: return "gliss-data-baked";
    case GeometryDiagnostic::NonFiniteProjection: return "non-finite-projection";
    case GeometryDiagnostic::ProjectionRejected: return "projection-rejected";
    case GeometryDiagnostic::SegmentOverflow: return "segment-overflow";
    case GeometryDiagnostic::TailOutOfBand: return "tail-out-of-band";
    case GeometryDiagnostic::TimeRange: return "time-range";
    }
    return "unknown";
}

bool TimedNoteGeometry::DistanceFor(const NoteGeometrySpec& spec, double note_ms,
                                    double song_ms, float& distance) noexcept {
    if (!std::isfinite(note_ms) || !std::isfinite(song_ms) ||
        std::abs(note_ms) > kMaximumTimeMs || std::abs(song_ms) > kMaximumTimeMs) {
        return false;
    }
    // Defensive: this helper is public and may be called with any spec.
    if (!std::isfinite(spec.legacy_rate) || !(spec.legacy_rate > 0.0f) ||
        !std::isfinite(spec.visual_offset_ms)) {
        return false;
    }
    const double delta_ms = note_ms - song_ms - static_cast<double>(spec.visual_offset_ms);
    const double value = delta_ms * 180.0 /
        (static_cast<double>(spec.legacy_rate) * 1000.0);
    if (!std::isfinite(value) || std::abs(value) > kMaximumDistance) return false;
    distance = static_cast<float>(value);
    return std::isfinite(distance);
}

std::uint32_t TimedNoteGeometry::RibbonSegments(const NoteGeometrySpec& spec,
                                                float head_distance,
                                                float tail_distance) noexcept {
    // Defensive: this helper is public and may be called with any spec.
    const float approach = spec.approach;
    const float step = (std::isfinite(spec.ribbon_step_divisor) &&
                        spec.ribbon_step_divisor >= 1.0f)
        ? approach / spec.ribbon_step_divisor
        : 0.0f;
    if (!std::isfinite(approach) || !(approach > 0.0f) ||
        !std::isfinite(step) || !(step > 0.0f) ||
        !std::isfinite(head_distance) || !std::isfinite(tail_distance) ||
        !(tail_distance > head_distance)) {
        return 0;
    }
    float tail = 0.0f;
    if (!SnapTail(spec, tail_distance, tail)) return 0;
    if (tail <= head_distance) return 1;
    // Clamp in double before any integer conversion: a tiny step or a huge span
    // must never wrap a uint32.
    const double span = static_cast<double>(tail) - static_cast<double>(head_distance);
    const double count = std::ceil(span / static_cast<double>(step) - 1.0e-4);
    if (!std::isfinite(count) || count < 1.0) return count < 1.0 ? 1u : 0u;
    const double limit = static_cast<double>(
        std::min<std::uint32_t>(spec.max_ribbon_segments, kMaximumSegments));
    return static_cast<std::uint32_t>(std::min(count, limit));
}

bool TimedNoteGeometry::Capture(const NoteGeometrySpec& spec,
                                std::span<const NotePartInput> parts) noexcept {
    // Invalidates the previous note before anything else: a rejected capture
    // must never leave a half-updated geometry behind.
    part_count_ = 0;
    for (auto& part : parts_) part = {};
    if (!ValidSpec(spec) || parts.empty() || parts.size() > kMaxParts) return false;

    std::size_t quad_count = 0;
    std::size_t box_count = 0;
    for (const auto& input : parts) {
        if (!ValidShape(input.shape) || !ValidAnchors(input.anchors)) return false;
        // A textured record can only be anchored at one point; box strips need
        // two anchors. Mixing them would write the wrong record size.
        const bool quad = input.shape == NotePartShape::TexturedQuad;
        const bool point = input.anchors == NoteAnchorRule::Start ||
            input.anchors == NoteAnchorRule::End;
        if (quad != point) return false;
        if (input.record_count == 0 || input.record_count > kMaximumRecords) return false;
        const std::size_t stride = quad ? kTexturedStride : kBoxStride;
        if (input.raw.size() != stride * input.record_count) return false;
        if (quad) {
            if (input.record_count != 1 || !ValidTexturedRecord(input.raw)) return false;
            if (input.quad_half_width != 0.0f || input.quad_half_height != 0.0f) {
                // Partial or non-finite extents would silently use the wrong
                // quad size; refuse instead of mixing parts.
                if (!(input.quad_half_width > 0.0f) || !(input.quad_half_height > 0.0f) ||
                    !Finite(input.quad_half_width) || !Finite(input.quad_half_height)) {
                    return false;
                }
            }
            ++quad_count;
        } else {
            for (std::uint32_t record = 0; record < input.record_count; ++record) {
                if (!ValidBoxRecord(input.raw.data() + record * stride)) return false;
            }
            ++box_count;
        }
        if (quad_count > kMaxQuadParts || box_count > kMaxBoxParts) return false;
    }

    // Copies into the fixed cache. No allocation, no exception, no capacity
    // change: a failed or reused capture never touches the heap.
    std::size_t quad_slot = 0;
    std::size_t box_slot = 0;
    for (std::size_t index = 0; index < parts.size(); ++index) {
        const auto& input = parts[index];
        auto& part = parts_[index];
        part.shape = input.shape;
        part.anchors = input.anchors;
        part.record_count = input.record_count;
        part.source = input.source;
        if (IsBoxShape(input.shape)) {
            part.captured = box_captured_[box_slot].data();
            part.draw = box_draw_[box_slot].data();
            part.captured_capacity = box_captured_[box_slot].size();
            part.draw_capacity = box_draw_[box_slot].size();
            ++box_slot;
        } else {
            part.captured = quad_captured_[quad_slot].data();
            part.draw = quad_draw_[quad_slot].data();
            part.captured_capacity = quad_captured_[quad_slot].size();
            part.draw_capacity = quad_draw_[quad_slot].size();
            part.quad_half_width = input.quad_half_width;
            part.quad_half_height = input.quad_half_height;
            ++quad_slot;
        }
        if (input.raw.size() > part.captured_capacity ||
            input.raw.size() > part.draw_capacity) {
            part_count_ = 0;
            for (auto& cleared : parts_) cleared = {};
            return false;
        }
        std::memcpy(part.captured, input.raw.data(), input.raw.size());
    }
    spec_ = spec;
    part_count_ = parts.size();
    return true;
}

void TimedNoteGeometry::Reset() noexcept {
    spec_ = {};
    for (auto& part : parts_) part = {};
    part_count_ = 0;
}

const void* TimedNoteGeometry::PartSource(std::size_t index) const noexcept {
    if (index >= part_count_) return nullptr;
    return parts_[index].source;
}

NotePartShape TimedNoteGeometry::PartShape(std::size_t index) const noexcept {
    if (index >= part_count_) return NotePartShape::TexturedQuad;
    return parts_[index].shape;
}

bool TimedNoteGeometry::FillQuad(Part& part, const NoteProjection& projection,
                                 NotePartView& view,
                                 GeometryDiagnostic& diagnostic) noexcept {
    if (!Finite(projection.x) || !Finite(projection.y) ||
        !Finite(projection.scale) || projection.scale <= 0.0f ||
        !Finite(projection.opacity, 4.0f)) {
        diagnostic = GeometryDiagnostic::NonFiniteProjection;
        return false;
    }
    const float half_width = part.quad_half_width > 0.0f
        ? part.quad_half_width : spec_.quad_half_width;
    const float half_height = part.quad_half_height > 0.0f
        ? part.quad_half_height : spec_.quad_half_height;
    if (!(half_width > 0.0f) || !(half_height > 0.0f) ||
        !Finite(half_width) || !Finite(half_height)) {
        diagnostic = GeometryDiagnostic::LayoutUnverified;
        return false;
    }
    // Verified against the long/gliss/trill head code: four corners at
    // (x +/- hw, y +/- hh) with the one-pixel right/bottom convention.
    const float scaled_width = half_width * projection.scale;
    const float scaled_height = half_height * projection.scale;
    const float left = projection.x + spec_.x_shift - scaled_width;
    const float right = projection.x + spec_.x_shift + scaled_width - 1.0f;
    const float top = projection.y - scaled_height;
    const float bottom = projection.y + scaled_height - 1.0f;
    if (!Finite(left) || !Finite(right) || !Finite(top) || !Finite(bottom) ||
        right < left || bottom < top) {
        diagnostic = GeometryDiagnostic::NonFiniteProjection;
        return false;
    }

    std::memcpy(part.draw, part.captured, part.captured_capacity);
    const std::array<float, 4> x{left, right, left, right};
    const std::array<float, 4> y{top, top, bottom, bottom};
    const float alpha = std::clamp(projection.opacity * spec_.opacity_scale, 0.0f, 1.0f);
    for (std::size_t vertex = 0; vertex < 4; ++vertex) {
        const std::size_t offset = vertex * kVertexStride;
        Write(part.draw, offset, x[vertex]);
        Write(part.draw, offset + sizeof(float), y[vertex]);
        const std::uint32_t color = Read<std::uint32_t>(part.draw, offset + kColorOffset);
        Write(part.draw, offset + kColorOffset, ColorAlpha(color, alpha));
    }
    view = {part.draw, 1};
    return true;
}

bool TimedNoteGeometry::FillStrip(Part& part, float head_distance,
                                  float tail_distance, NoteProjectionCallback project,
                                  void* context, NotePartView& view,
                                  GeometryDiagnostic& diagnostic) noexcept {
    if ((part.shape == NotePartShape::BoxRibbon && !(spec_.ribbon_base_width > 0.0f)) ||
        (part.shape == NotePartShape::BoxLine && !(spec_.ribbon_line_half_width > 0.0f))) {
        diagnostic = GeometryDiagnostic::LayoutUnverified;
        return false;
    }
    const std::uint32_t segments = RibbonSegments(spec_, head_distance, tail_distance);
    if (segments == 0) {
        float unused = 0.0f;
        diagnostic = SnapTail(spec_, tail_distance, unused)
            ? GeometryDiagnostic::MissingAnchorTime
            : GeometryDiagnostic::TailOutOfBand;
        return false;
    }
    if (segments > spec_.max_ribbon_segments || segments > kMaximumSegments) {
        diagnostic = GeometryDiagnostic::SegmentOverflow;
        return false;
    }
    if (part.draw_capacity < segments * kBoxStride) {
        diagnostic = GeometryDiagnostic::SegmentOverflow;
        return false;
    }

    const float step = spec_.approach / spec_.ribbon_step_divisor;
    float current_distance = 0.0f;
    if (!SnapTail(spec_, tail_distance, current_distance)) {
        diagnostic = GeometryDiagnostic::TailOutOfBand;
        return false;
    }

    NoteProjection current{};
    if (!project(context, spec_.lane, current_distance, spec_.left_key, spec_.right_key,
                 current)) {
        diagnostic = GeometryDiagnostic::ProjectionRejected;
        return false;
    }
    std::uint32_t emitted = 0;
    while (emitted < segments) {
        current_distance = std::max(head_distance, current_distance - step);
        const NoteProjection previous = current;
        if (!project(context, spec_.lane, current_distance, spec_.left_key,
                     spec_.right_key, current)) {
            diagnostic = GeometryDiagnostic::ProjectionRejected;
            return false;
        }
        if (!Finite(previous.x) || !Finite(previous.y) || !Finite(current.x) ||
            !Finite(current.y) || !Finite(previous.scale) || !Finite(current.scale) ||
            previous.scale <= 0.0f || current.scale <= 0.0f) {
            diagnostic = GeometryDiagnostic::NonFiniteProjection;
            return false;
        }

        const float half_previous = part.shape == NotePartShape::BoxLine
            ? spec_.ribbon_line_half_width
            : spec_.ribbon_base_width * previous.scale;
        const float half_current = part.shape == NotePartShape::BoxLine
            ? spec_.ribbon_line_half_width
            : spec_.ribbon_base_width * current.scale;
        const float p1x = previous.x + spec_.x_shift - half_previous;
        const float p2x = previous.x + spec_.x_shift + half_previous;
        const float p3x = current.x + spec_.x_shift + half_current;
        const float p4x = current.x + spec_.x_shift - half_current;
        if (!Finite(p1x) || !Finite(p2x) || !Finite(p3x) || !Finite(p4x)) {
            diagnostic = GeometryDiagnostic::NonFiniteProjection;
            return false;
        }

        // Colour template: RGB and alpha stay as the game last wrote them (the
        // native gradient tables are not re-evaluated); the per-record pattern
        // is reused, clamped to the captured records.
        std::uint32_t colors[4]{};
        TemplateColors(part.captured, part.record_count, emitted, colors);
        WriteBoxQuad(part.draw + emitted * kBoxStride, colors,
                     p1x, p2x, p3x, p4x, previous.y, current.y);
        ++emitted;
        if (head_distance >= current_distance) break;
    }
    if (emitted == 0) {
        diagnostic = GeometryDiagnostic::SegmentOverflow;
        return false;
    }
    view = {part.draw, emitted};
    return true;
}

bool TimedNoteGeometry::FillTrillStrip(Part& part, float head_distance,
                                       float tail_distance,
                                       NoteProjectionCallback project, void* context,
                                       NotePartView& view,
                                       GeometryDiagnostic& diagnostic) noexcept {
    // Verified against 0x1A84F0: the strip half width is
    // (shrink*0x13E476C/2 - 10 + note+60/2) * scale, the step is
    // approach/0x180687298 (80.0), the tail is snapped with the same loop,
    // and only 90 segments are ever emitted per object.
    if (!(spec_.ribbon_base_width > 0.0f)) {
        diagnostic = GeometryDiagnostic::LayoutUnverified;
        return false;
    }
    const std::uint32_t segments = RibbonSegments(spec_, head_distance, tail_distance);
    if (segments == 0) {
        float unused = 0.0f;
        diagnostic = SnapTail(spec_, tail_distance, unused)
            ? GeometryDiagnostic::MissingAnchorTime
            : GeometryDiagnostic::TailOutOfBand;
        return false;
    }
    if (segments > spec_.max_ribbon_segments || segments > kMaximumSegments) {
        diagnostic = GeometryDiagnostic::SegmentOverflow;
        return false;
    }
    if (part.draw_capacity < segments * kBoxStride) {
        diagnostic = GeometryDiagnostic::SegmentOverflow;
        return false;
    }

    const float step = spec_.approach / spec_.ribbon_step_divisor;
    float current_distance = 0.0f;
    if (!SnapTail(spec_, tail_distance, current_distance)) {
        diagnostic = GeometryDiagnostic::TailOutOfBand;
        return false;
    }
    NoteProjection current{};
    if (!project(context, spec_.lane, current_distance, spec_.left_key, spec_.right_key,
                 current)) {
        diagnostic = GeometryDiagnostic::ProjectionRejected;
        return false;
    }

    // Native phase: 4 - round((snapped_tail - head) / step) % 4. It selects
    // the side of the ten-pixel glow strip and which half width decays first.
    const double ratio =
        (static_cast<double>(current_distance) - static_cast<double>(head_distance)) /
        static_cast<double>(step);
    if (!std::isfinite(ratio)) {
        diagnostic = GeometryDiagnostic::NonFiniteProjection;
        return false;
    }
    const double clamped_ratio = std::clamp(ratio + 0.5, 0.0, 1.0e9);
    int phase = 4 - static_cast<int>(clamped_ratio) % 4;

    std::uint32_t emitted = 0;
    while (emitted < segments) {
        current_distance = std::max(head_distance, current_distance - step);
        const NoteProjection previous = current;
        if (!project(context, spec_.lane, current_distance, spec_.left_key,
                     spec_.right_key, current)) {
            diagnostic = GeometryDiagnostic::ProjectionRejected;
            return false;
        }
        if (!Finite(previous.x) || !Finite(previous.y) || !Finite(current.x) ||
            !Finite(current.y) || !Finite(previous.scale) || !Finite(current.scale) ||
            previous.scale <= 0.0f || current.scale <= 0.0f) {
            diagnostic = GeometryDiagnostic::NonFiniteProjection;
            return false;
        }

        // The native trill arithmetic is integer: the half widths are
        // (int)(scale * base) and one side loses a quarter on alternate
        // segments (`w += w / -4`, truncating toward zero). Reject a product
        // outside the exact integer range instead of invoking undefined
        // conversion in a hook.
        const float previous_width = previous.scale * spec_.ribbon_base_width;
        const float current_width = current.scale * spec_.ribbon_base_width;
        if (!std::isfinite(previous_width) || !std::isfinite(current_width) ||
            previous_width < 0.0f || current_width < 0.0f ||
            previous_width > 1.0e8f || current_width > 1.0e8f) {
            diagnostic = GeometryDiagnostic::NonFiniteProjection;
            return false;
        }
        int half_previous = static_cast<int>(previous_width);
        int half_current = static_cast<int>(current_width);
        if (half_previous < 0 || half_current < 0) {
            diagnostic = GeometryDiagnostic::NonFiniteProjection;
            return false;
        }
        if ((phase & 1) != 0) half_current += half_current / -4;
        else half_previous += half_previous / -4;

        const float shift = spec_.x_shift;
        float p1x = 0.0f;
        float p2x = 0.0f;
        float p3x = 0.0f;
        float p4x = 0.0f;
        if (part.shape == NotePartShape::TrillGlow) {
            // The second object is a ten-pixel band on one edge, not a wider
            // ribbon: phase 0/1 keeps the left edge, phase 2/3 the right one.
            if (static_cast<unsigned>(phase) % 4 <= 1) {
                p1x = previous.x + shift - static_cast<float>(half_previous);
                p2x = p1x + 10.0f;
                p3x = current.x + shift - static_cast<float>(half_current) + 10.0f;
                p4x = p3x - 10.0f;
            } else {
                p1x = previous.x + shift + static_cast<float>(half_previous) - 10.0f;
                p2x = p1x + 10.0f;
                p3x = current.x + shift + static_cast<float>(half_current);
                p4x = p3x - 10.0f;
            }
        } else {
            p1x = previous.x + shift - static_cast<float>(half_previous);
            p2x = previous.x + shift + static_cast<float>(half_previous);
            p3x = current.x + shift + static_cast<float>(half_current);
            p4x = current.x + shift - static_cast<float>(half_current);
        }
        if (!Finite(p1x) || !Finite(p2x) || !Finite(p3x) || !Finite(p4x)) {
            diagnostic = GeometryDiagnostic::NonFiniteProjection;
            return false;
        }

        std::uint32_t colors[4]{};
        TemplateColors(part.captured, part.record_count, emitted, colors);
        WriteBoxQuad(part.draw + emitted * kBoxStride, colors,
                     p1x, p2x, p3x, p4x, previous.y, current.y);
        ++emitted;
        ++phase;
        if (head_distance >= current_distance) break;
    }
    if (emitted == 0) {
        diagnostic = GeometryDiagnostic::SegmentOverflow;
        return false;
    }
    view = {part.draw, emitted};
    return true;
}

bool TimedNoteGeometry::BuildPart(std::size_t index, double song_ms,
                                  NoteProjectionCallback project, void* context,
                                  NotePartView& view,
                                  GeometryDiagnostic& diagnostic) noexcept {
    view = {};
    diagnostic = GeometryDiagnostic::None;
    if (part_count_ == 0) {
        diagnostic = GeometryDiagnostic::NotCaptured;
        return false;
    }
    if (index >= part_count_) {
        diagnostic = GeometryDiagnostic::PartCountMismatch;
        return false;
    }
    if (!project) {
        diagnostic = GeometryDiagnostic::ProjectionRejected;
        return false;
    }
    if (!std::isfinite(song_ms) || std::abs(song_ms) > kMaximumTimeMs) {
        diagnostic = GeometryDiagnostic::TimeRange;
        return false;
    }
    if (spec_.scroll_mode < 0 || spec_.scroll_mode > 5) {
        diagnostic = GeometryDiagnostic::ScrollModeUnsupported;
        return false;
    }

    Part& part = parts_[index];
    if (part.anchors == NoteAnchorRule::StartToLink) {
        // Gliss connector: the native quad uses a baked screen Y stored in the
        // gliss point record; a pure rebuild would invent it.
        diagnostic = GeometryDiagnostic::GlissDataBaked;
        return false;
    }

    float head_distance = 0.0f;
    if (!DistanceFor(spec_, spec_.start_ms, song_ms, head_distance)) {
        diagnostic = GeometryDiagnostic::TimeRange;
        return false;
    }
    if (part.anchors == NoteAnchorRule::Start) {
        NoteProjection head{};
        if (!project(context, spec_.lane, head_distance, spec_.left_key, spec_.right_key,
                     head)) {
            diagnostic = GeometryDiagnostic::ProjectionRejected;
            return false;
        }
        return FillQuad(part, head, view, diagnostic);
    }

    float tail_distance = 0.0f;
    if (!spec_.has_end) {
        diagnostic = GeometryDiagnostic::MissingAnchorTime;
        return false;
    }
    if (!DistanceFor(spec_, spec_.end_ms, song_ms, tail_distance)) {
        diagnostic = GeometryDiagnostic::MissingAnchorTime;
        return false;
    }

    if (part.anchors == NoteAnchorRule::End) {
        NoteProjection tail{};
        if (!project(context, spec_.lane, tail_distance, spec_.left_key, spec_.right_key,
                     tail)) {
            diagnostic = GeometryDiagnostic::ProjectionRejected;
            return false;
        }
        return FillQuad(part, tail, view, diagnostic);
    }
    if (part.anchors == NoteAnchorRule::StartToEnd) {
        if (IsTrillShape(part.shape)) {
            return FillTrillStrip(part, head_distance, tail_distance, project, context,
                                  view, diagnostic);
        }
        return FillStrip(part, head_distance, tail_distance, project, context, view,
                         diagnostic);
    }
    diagnostic = GeometryDiagnostic::GlissDataBaked;
    return false;
}

bool TimedNoteGeometry::Build(double song_ms, NoteProjectionCallback project,
                              void* context, std::span<NotePartView> out,
                              std::size_t& out_count,
                              GeometryDiagnostic& diagnostic) noexcept {
    out_count = 0;
    diagnostic = GeometryDiagnostic::None;
    if (part_count_ == 0) {
        diagnostic = GeometryDiagnostic::NotCaptured;
        return false;
    }
    if (out.size() < part_count_) {
        diagnostic = GeometryDiagnostic::PartCountMismatch;
        return false;
    }
    for (std::size_t index = 0; index < part_count_; ++index) {
        if (!BuildPart(index, song_ms, project, context, out[index], diagnostic)) {
            out_count = 0;
            return false;
        }
        ++out_count;
    }
    return true;
}

} // namespace nosfix
