#pragma once

#include "nosfix/timed_normal.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace nosfix {

// Explicit reason for every decision to fall back to the original game path.
enum class GeometryDiagnostic : std::uint8_t {
    None = 0,
    NotCaptured,
    ScrollModeUnsupported,
    LayoutUnverified,
    PartCountMismatch,
    MissingAnchorTime,
    GlissDataBaked,
    NonFiniteProjection,
    ProjectionRejected,
    SegmentOverflow,
    TailOutOfBand,
    TimeRange,
};

[[nodiscard]] const char* GeometryDiagnosticName(GeometryDiagnostic value) noexcept;

enum class NotePartShape : std::uint8_t {
    // One 160-byte textured record, four vertices of stride 24. Rebuilt from
    // the projected anchor centre and the part's own half extents.
    TexturedQuad = 0,
    // N box-fill records of 120 bytes, six vertices of stride 20. A segmented
    // strip between two anchors; half width = base width * projected scale.
    BoxRibbon = 1,
    // N box-fill records of 120 bytes. Same strip with a fixed half width
    // (the four-pixel hold line the long-note update appends).
    BoxLine = 2,
    // Trill primary strip (note+368). Step is approach/80 and the native
    // tremolo shrinks one side by a quarter on alternate segments.
    TrillRibbon = 3,
    // Trill edge strip (note+424). A ten-pixel-wide band on the left or right
    // side of the primary strip, alternating on the same phase.
    TrillGlow = 4,
};

// Which chart-time anchors a part is rebuilt from. StartToLink is the gliss
// connector: its native geometry uses a baked screen Y from the gliss point
// record, so a pure rebuild is refused unless the integration supplies a
// link anchor, which this model does not invent.
enum class NoteAnchorRule : std::uint8_t {
    Start = 0,
    End = 1,
    StartToEnd = 2,
    StartToLink = 3,
};

struct NotePartInput final {
    NotePartShape shape = NotePartShape::TexturedQuad;
    NoteAnchorRule anchors = NoteAnchorRule::Start;
    std::uint32_t record_count = 0;
    std::span<const std::byte> raw;
    // Identity of the game buffer this part was captured from. Used only as a
    // lookup key by the draw hook; it is never dereferenced or retained.
    const void* source = nullptr;
    // Unscaled half extents for a textured part. Zero selects the note spec,
    // which keeps single-quad callers unchanged. Long and trill tails carry
    // their own dimensions (note+272/+276), so they are captured per part.
    float quad_half_width = 0.0f;
    float quad_half_height = 0.0f;
};

// One captured note primitive ready for a render-only copy.
struct NotePartView final {
    const std::byte* data = nullptr;
    std::uint32_t count = 0;
};

// Static, chart-derived description of one note. The integration layer fills
// this from the live note/plot record; nothing here is a game pointer.
struct NoteGeometrySpec final {
    double start_ms = 0.0;
    double end_ms = 0.0;
    bool has_end = false;
    float visual_offset_ms = 0.0f;
    float legacy_rate = 60.0f;
    int lane = 0;
    int left_key = 0;
    int right_key = 0;
    int scroll_mode = 0;
    float x_shift = 0.0f;
    float opacity_scale = 1.0f;
    float quad_half_width = 0.0f;
    float quad_half_height = 0.0f;
    // Integer divisor from the native head/tail quad rule (2, or 4 for wide
    // notes) and the global height multiplier (0x1B725C4 / 10). The integration
    // uses them to derive per-part extents for +196/+200, +240/+244 and
    // +272/+276; pure callers may set the half extents directly.
    int quad_width_divisor = 2;
    float quad_height_scale = 1.0f;
    // (shrink * wide-note offset / 2 - 10 + note+60 / 2) from the native ribbon update.
    float ribbon_base_width = 0.0f;
    float ribbon_line_half_width = 2.0f;
    // 0x13668D4 approach distance and 0x13E4758 start bias used by the
    // long/trill strip segmentation.
    float approach = 0.0f;
    float ribbon_start_bias = 0.0f;
    // Segment step divisor. The long update divides the approach distance by
    // the literal 30; the trill path divides by the constant at 0x180687298
    // (80.0). It is a parameter so the caller supplies the evidenced value.
    float ribbon_step_divisor = 30.0f;
    std::uint32_t max_ribbon_segments = 37;
};

// Chart-time geometry for one note. Capture stores private copies of every
// source buffer and never retains a game pointer.
//
// Storage is a fixed, bounded cache: at most two textured parts (160 bytes
// each) and two box parts (kMaximumSegments * 120 bytes each), captured and
// draw copies. Capture and Build therefore perform no allocation at all, so
// no bad_alloc can escape a native update or draw hook and no erase/recapture
// cycle can churn the heap. The object is intentionally non-copyable and
// non-movable because its part views point into that inline storage.
class TimedNoteGeometry final {
public:
    TimedNoteGeometry() = default;
    TimedNoteGeometry(const TimedNoteGeometry&) = delete;
    TimedNoteGeometry& operator=(const TimedNoteGeometry&) = delete;
    TimedNoteGeometry(TimedNoteGeometry&&) = delete;
    TimedNoteGeometry& operator=(TimedNoteGeometry&&) = delete;

    static constexpr std::size_t kMaxParts = 4;
    static constexpr std::size_t kMaxQuadParts = 2;
    static constexpr std::size_t kMaxBoxParts = 2;
    static constexpr std::uint32_t kMaximumRecords = 96;
    static constexpr std::uint32_t kMaximumSegments = 96;
    static constexpr std::size_t kTexturedStride = 160;
    static constexpr std::size_t kBoxStride = 120;

    [[nodiscard]] bool Capture(const NoteGeometrySpec& spec,
                               std::span<const NotePartInput> parts) noexcept;

    // Fills up to `out.size()` part views. Returns false when the caller must
    // draw the original payload; `diagnostic` then states the exact reason.
    // On success `out_count` views are valid until the next Build/Capture.
    [[nodiscard]] bool Build(double song_ms, NoteProjectionCallback project,
                             void* context, std::span<NotePartView> out,
                             std::size_t& out_count,
                             GeometryDiagnostic& diagnostic) noexcept;

    // Rebuilds exactly one part. This is the draw-hook entry point: it keeps
    // one draw call from paying for every other part of the same note.
    [[nodiscard]] bool BuildPart(std::size_t index, double song_ms,
                                 NoteProjectionCallback project, void* context,
                                 NotePartView& view,
                                 GeometryDiagnostic& diagnostic) noexcept;

    void Reset() noexcept;
    [[nodiscard]] bool HasCapture() const noexcept { return part_count_ != 0; }
    [[nodiscard]] std::size_t PartCount() const noexcept { return part_count_; }
    [[nodiscard]] const NoteGeometrySpec& Spec() const noexcept { return spec_; }
    // Captured game-buffer identity for a part, or null. Lookup key only.
    [[nodiscard]] const void* PartSource(std::size_t index) const noexcept;
    [[nodiscard]] NotePartShape PartShape(std::size_t index) const noexcept;

    // Exposed for tests and for the integration layer's own validation.
    [[nodiscard]] static bool DistanceFor(const NoteGeometrySpec& spec,
                                          double note_ms, double song_ms,
                                          float& distance) noexcept;
    [[nodiscard]] static std::uint32_t RibbonSegments(const NoteGeometrySpec& spec,
                                                      float head_distance,
                                                      float tail_distance) noexcept;

private:
    struct Part final {
        NotePartShape shape = NotePartShape::TexturedQuad;
        NoteAnchorRule anchors = NoteAnchorRule::Start;
        std::uint32_t record_count = 0;
        const void* source = nullptr;
        std::byte* captured = nullptr;
        std::byte* draw = nullptr;
        std::size_t captured_capacity = 0;
        std::size_t draw_capacity = 0;
        float quad_half_width = 0.0f;
        float quad_half_height = 0.0f;
    };

    [[nodiscard]] bool FillQuad(Part& part, const NoteProjection& projection,
                                NotePartView& view,
                                GeometryDiagnostic& diagnostic) noexcept;
    [[nodiscard]] bool FillStrip(Part& part, float head_distance, float tail_distance,
                                 NoteProjectionCallback project, void* context,
                                 NotePartView& view,
                                 GeometryDiagnostic& diagnostic) noexcept;
    [[nodiscard]] bool FillTrillStrip(Part& part, float head_distance,
                                      float tail_distance,
                                      NoteProjectionCallback project, void* context,
                                      NotePartView& view,
                                      GeometryDiagnostic& diagnostic) noexcept;

    NoteGeometrySpec spec_{};
    std::array<Part, kMaxParts> parts_{};
    std::size_t part_count_ = 0;

    // Fixed backing stores. Two of each shape cover every supported note:
    // normal (one quad), long (head+tail quad, ribbon+line box) and trill
    // (head+tail quad, tremolo+glow box).
    std::array<std::array<std::byte, kTexturedStride>, kMaxQuadParts> quad_captured_{};
    std::array<std::array<std::byte, kTexturedStride>, kMaxQuadParts> quad_draw_{};
    std::array<std::array<std::byte, kMaximumSegments * kBoxStride>, kMaxBoxParts>
        box_captured_{};
    std::array<std::array<std::byte, kMaximumSegments * kBoxStride>, kMaxBoxParts>
        box_draw_{};
};

} // namespace nosfix
