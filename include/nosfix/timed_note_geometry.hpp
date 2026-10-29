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
    // A whole-object part was rebuilt with zero quads. Nothing may be drawn
    // from this part, and drawing the original payload would be wrong, so the
    // caller must skip the draw instead of falling back.
    EmptyObject,
    // More connector quads than the native object can hold (two).
    ObjectOverflow,
    // BuildPart() was used for a whole-object part; it needs BuildObjectPart().
    ObjectQuadsRequired,
    // Glissando: the connector distance formula itself is proven from the bytes
    // at 0x1801A6D20..0x1801A6DE4 (180*(element+0x14 - frame - offsets)/rate^2),
    // but element+0x14 is a frame whose chart-millisecond mapping is unproven, so
    // the model has no chart-time anchor to feed it.
    GlissAnchorUnproven,
    // Glissando, both connectors: the producer of the 104-byte chart element
    // field +0x50 (read at 0x1801A6DE9) is unidentified. Reserved as the second
    // blocker; TimedNoteRefusal() reports GlissAnchorUnproven first.
    GlissClipUnproven,
    // Velocity zone: the raster at note+352 is the long-ribbon shape with a
    // different width, but its end anchor (note+296) and the held-head state
    // (note+300/+316) are not proven to be chart derived.
    VelocityAnchorUnproven,
    // Beats: the update derives its position from the live frame counter and a
    // constant x = 640, not from a chart time, so there is nothing to rebuild.
    ChartTimeAbsent,
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
    // A complete box-fill object whose record count is decided at build time.
    // The glissando update appends both of its connector quads to the single
    // object at note+216 (0x1801A6D19 begins it, 0x1801A7326 ends it), so one
    // draw call owns 0-2 quads and a one-part-per-buffer binding cannot
    // represent it. Corner coordinates are supplied by the caller: the native
    // forms them from a projector call whose distance formula is proven but
    // whose chart-time anchor is not, so this model never derives them.
    BoxObject = 5,
};

// Which chart-time anchors a part is rebuilt from. StartToLink is the gliss
// connector: its native geometry uses a baked screen Y from the gliss point
// record, so a pure rebuild is refused unless the integration supplies a
// link anchor, which this model does not invent. None marks a whole-object
// part, which is not anchored at all.
enum class NoteAnchorRule : std::uint8_t {
    Start = 0,
    End = 1,
    StartToEnd = 2,
    StartToLink = 3,
    None = 4,
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

// One quad of a whole-object part, already in the object's own corner order:
// p1/p2 are the two corners of the first row (previous_y) and p3/p4 the two
// corners of the second row (current_y). This is exactly the argument order of
// the native append helper 0xD5370, whose record builder 0xD5460 emits
// p1, p2, p4, p4, p2, p3.
//
// Both row orientations occur natively: the glissando head connector writes its
// near row first (left to right) and the tail connector writes its far row
// first (right to left), so a row must not be assumed to run left to right.
struct NoteObjectQuad final {
    float p1x = 0.0f;
    float p2x = 0.0f;
    float p3x = 0.0f;
    float p4x = 0.0f;
    float previous_y = 0.0f;
    float current_y = 0.0f;
};

// One end of a glissando connector: a projected screen centre and the already
// scaled half width the native multiplies out
// ((note+208/2 + wide/2 - 6) * scale at the note end,
//  (out_right - out_left)/2 + wide/2 - 6) * scale at the linked end).
// `x` is the final screen x, i.e. the wide-note shift is applied by the caller.
struct GlissConnectorRow final {
    float x = 0.0f;
    float y = 0.0f;
    float half_width = 0.0f;
};

// The two ends of one connector. `note` is the glissando note's own projected
// point and half width; `link` is the linked 104-byte chart element's, i.e.
// 0x180305430(element+0x10, distance, ..., element+0x58, element+0x5C).
struct GlissConnectorEnds final {
    GlissConnectorRow note;
    GlissConnectorRow link;
};

// The connector object at note+216. The head connector is chart+0x40 (appended
// first, 0x1801A6F80) and the tail is chart+0x48 (appended second, 0x1801A72Cx);
// either may be absent, so the object holds 0, 1 or 2 quads.
struct GlissConnectorSet final {
    bool has_head = false;
    bool has_tail = false;
    GlissConnectorEnds head{};
    GlissConnectorEnds tail{};
    // element+0x50 for the head connector: the partner note's screen Y as baked
    // by 0x1801A647C during that note's own render, or <= 0 for "no clip". It
    // is a per-frame value, never a chart constant, so the model only consumes
    // it and never derives it.
    float head_clip_y = 0.0f;
};

// Builds the 0-2 connector quads of one object in native record order. All
// coordinates must be supplied: this never projects and never reads a field.
// Refusals: EmptyObject when neither connector is present, NonFiniteProjection
// for a non-finite input, ObjectOverflow when `out` is too small.
[[nodiscard]] bool BuildGlissConnectorQuads(const GlissConnectorSet& set,
                                            std::span<NoteObjectQuad> out,
                                            std::uint32_t& out_count,
                                            GeometryDiagnostic& diagnostic) noexcept;

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
    // The native connector object receives at most one quad per connector and
    // there are two connectors, so its record count is 0, 1 or 2.
    static constexpr std::size_t kMaxObjectQuads = 2;
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

    // Rebuilds a whole-object box part (NotePartShape::BoxObject) from quad
    // corners the integration already owns. `quads` must hold 1..kMaxObjectQuads
    // entries; zero is refused with EmptyObject because the caller must not
    // draw the original payload either. RGB/alpha come from the captured
    // records so the object keeps the game's last palette.
    [[nodiscard]] bool BuildObjectPart(std::size_t index,
                                       std::span<const NoteObjectQuad> quads,
                                       NotePartView& view,
                                       GeometryDiagnostic& diagnostic) noexcept;

    void Reset() noexcept;
    [[nodiscard]] bool HasCapture() const noexcept { return part_count_ != 0; }
    [[nodiscard]] std::size_t PartCount() const noexcept { return part_count_; }
    [[nodiscard]] const NoteGeometrySpec& Spec() const noexcept { return spec_; }
    // Captured game-buffer identity for a part, or null. Lookup key only.
    [[nodiscard]] const void* PartSource(std::size_t index) const noexcept;
    [[nodiscard]] NotePartShape PartShape(std::size_t index) const noexcept;
    [[nodiscard]] bool PartIsObject(std::size_t index) const noexcept;

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
