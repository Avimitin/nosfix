#include "nosfix/timed_note_geometry.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <new>
#include <span>
#include <vector>

// Global allocation probe: proves that a warmed-up capture/build pair performs
// no allocation, and that an allocation failure is contained inside Capture.
static std::size_t g_allocations = 0;
static bool g_fail_allocations = false;

void* operator new(std::size_t size) {
    if (g_fail_allocations) throw std::bad_alloc();
    void* memory = std::malloc(size == 0 ? 1 : size);
    if (!memory) throw std::bad_alloc();
    ++g_allocations;
    return memory;
}
void* operator new[](std::size_t size) { return ::operator new(size); }
void operator delete(void* memory) noexcept { std::free(memory); }
void operator delete[](void* memory) noexcept { std::free(memory); }
void operator delete(void* memory, std::size_t) noexcept { std::free(memory); }
void operator delete[](void* memory, std::size_t) noexcept { std::free(memory); }

namespace {

void CheckImpl(bool condition, const char* expression, int line) {
    if (!condition) {
        std::fprintf(stderr, "timed-note-geometry test failed at line %d: %s\n",
                     line, expression);
        std::abort();
    }
}
#define Check(condition) CheckImpl((condition), #condition, __LINE__)

template <typename T>
void Store(std::span<std::byte> bytes, std::size_t offset, const T& value) {
    std::memcpy(bytes.data() + offset, &value, sizeof(value));
}

template <typename T>
T Load(std::span<const std::byte> bytes, std::size_t offset) {
    T value{};
    std::memcpy(&value, bytes.data() + offset, sizeof(value));
    return value;
}

std::vector<std::byte> MakeTextured() {
    std::vector<std::byte> raw(nosfix::TimedNoteGeometry::kTexturedStride);
    for (std::size_t vertex = 0; vertex < 4; ++vertex) {
        const std::size_t offset = vertex * 24;
        Store(std::span(raw), offset, static_cast<float>(vertex * 4));
        Store(std::span(raw), offset + 4, static_cast<float>(vertex * 5));
        Store(std::span(raw), offset + 8, 0.25f + static_cast<float>(vertex));
        Store(std::span(raw), offset + 12,
              0xA0123456U + static_cast<std::uint32_t>(vertex) * 0x100U);
        Store(std::span(raw), offset + 16, 0.1f * static_cast<float>(vertex + 1));
        Store(std::span(raw), offset + 20, 0.2f * static_cast<float>(vertex + 1));
    }
    Store(std::span(raw), 144, 0xD0000000U);
    Store(std::span(raw), 152, 0x123456789ABCDEF0ULL);
    return raw;
}

// Box records: vertex colours encode the record and corner so a test can check
// that the captured colour pattern survives a rebuild.
std::vector<std::byte> MakeBoxRecords(std::uint32_t count) {
    std::vector<std::byte> raw(count * nosfix::TimedNoteGeometry::kBoxStride);
    for (std::uint32_t record = 0; record < count; ++record) {
        for (std::size_t vertex = 0; vertex < 6; ++vertex) {
            const std::size_t offset =
                record * nosfix::TimedNoteGeometry::kBoxStride + vertex * 20;
            Store(std::span(raw), offset, static_cast<float>(record * 10 + vertex));
            Store(std::span(raw), offset + 4, static_cast<float>(500 + vertex));
            Store(std::span(raw), offset + 8, 0.0f);
            Store(std::span(raw), offset + 12, 1.0f);
            Store(std::span(raw), offset + 16,
                  0x40000000U + (record << 8) + static_cast<std::uint32_t>(vertex));
        }
    }
    return raw;
}

struct FakeProjector {
    float base_x = 200.0f;
    float base_y = 300.0f;
    float scale = 1.0f;
    float opacity = 1.0f;
    bool reject = false;
    bool nan = false;
    unsigned calls = 0;
    float last_distance = 0.0f;
    int last_lane = -1;
    int last_left = -1;
    int last_right = -1;
};

bool Project(void* context, int lane, float distance, int left_key, int right_key,
             nosfix::NoteProjection& result) noexcept {
    auto* state = static_cast<FakeProjector*>(context);
    state->calls++;
    state->last_distance = distance;
    state->last_lane = lane;
    state->last_left = left_key;
    state->last_right = right_key;
    if (state->reject) return false;
    if (state->nan) {
        result = {std::numeric_limits<float>::quiet_NaN(), 0.0f, 1.0f, 1.0f};
        return true;
    }
    result = {state->base_x + distance * 0.25f,
              state->base_y - distance * 0.5f,
              state->scale, state->opacity};
    return true;
}

nosfix::NoteGeometrySpec MakeQuadSpec() {
    nosfix::NoteGeometrySpec spec;
    spec.start_ms = 1'000.0;
    spec.visual_offset_ms = 0.0f;
    spec.legacy_rate = 60.0f;
    spec.lane = 3;
    spec.left_key = 2;
    spec.right_key = 4;
    spec.scroll_mode = 3;
    spec.x_shift = 3.0f;
    spec.opacity_scale = 1.0f;
    spec.quad_half_width = 10.0f;
    spec.quad_half_height = 20.0f;
    return spec;
}

nosfix::NoteGeometrySpec MakeRibbonSpec() {
    nosfix::NoteGeometrySpec spec = MakeQuadSpec();
    spec.start_ms = 1'000.0;
    spec.end_ms = 4'000.0;
    spec.has_end = true;
    spec.ribbon_base_width = 14.0f;
    spec.approach = 6.0f;
    spec.ribbon_start_bias = 0.0f;
    spec.max_ribbon_segments = 37;
    return spec;
}

std::vector<nosfix::NotePartInput> QuadParts(std::vector<std::byte>& raw) {
    return {{nosfix::NotePartShape::TexturedQuad, nosfix::NoteAnchorRule::Start, 1, raw}};
}

struct Snapshot {
    std::uint32_t count = 0;
    std::vector<std::byte> bytes;
    friend bool operator==(const Snapshot&, const Snapshot&) = default;
};

Snapshot Take(const nosfix::NotePartView& view, std::size_t stride) {
    Snapshot snapshot;
    snapshot.count = view.count;
    snapshot.bytes.assign(view.data, view.data + view.count * stride);
    return snapshot;
}

void TestFractionalDistance() {
    const auto spec = MakeQuadSpec();
    float distance = 0.0f;
    Check(nosfix::TimedNoteGeometry::DistanceFor(spec, 1'000.5, 500.25, distance));
    Check(std::abs(distance - 1.500'75f) < 1.0e-6f);
    Check(nosfix::TimedNoteGeometry::DistanceFor(spec, 1'000.0, 1'000.0, distance));
    Check(distance == 0.0f);

    auto offset_spec = spec;
    offset_spec.visual_offset_ms = 12.5f;
    Check(nosfix::TimedNoteGeometry::DistanceFor(offset_spec, 1'000.0, 900.0, distance));
    Check(std::abs(distance - 0.262'5f) < 1.0e-6f);

    auto fast = spec;
    fast.legacy_rate = 120.0f;
    Check(nosfix::TimedNoteGeometry::DistanceFor(fast, 1'000.5, 500.25, distance));
    Check(std::abs(distance - 0.750'375f) < 1.0e-6f);

    Check(!nosfix::TimedNoteGeometry::DistanceFor(
        spec, std::numeric_limits<double>::quiet_NaN(), 0.0, distance));
    Check(!nosfix::TimedNoteGeometry::DistanceFor(spec, 1.0e13, 0.0, distance));
}

void TestQuadGeometry() {
    auto raw = MakeTextured();
    const auto original = raw;
    nosfix::TimedNoteGeometry note;
    auto parts = QuadParts(raw);
    Check(note.Capture(MakeQuadSpec(), parts));

    FakeProjector projector;
    projector.scale = 2.0f;
    projector.opacity = 0.5f;
    std::array<nosfix::NotePartView, nosfix::TimedNoteGeometry::kMaxParts> views{};
    std::size_t count = 0;
    nosfix::GeometryDiagnostic diagnostic = nosfix::GeometryDiagnostic::None;
    Check(note.Build(1'000.0, Project, &projector, views, count, diagnostic));
    Check(count == 1);
    Check(diagnostic == nosfix::GeometryDiagnostic::None);
    Check(projector.last_lane == 3);
    Check(projector.last_left == 2);
    Check(projector.last_right == 4);
    Check(projector.last_distance == 0.0f);

    const std::span<const std::byte> out(views[0].data, 160);
    const float left = 200.0f + 3.0f - 10.0f * 2.0f;
    const float right = 200.0f + 3.0f + 10.0f * 2.0f - 1.0f;
    const float top = 300.0f - 20.0f * 2.0f;
    const float bottom = 300.0f + 20.0f * 2.0f - 1.0f;
    const std::array<float, 4> xs{left, right, left, right};
    const std::array<float, 4> ys{top, top, bottom, bottom};
    for (std::size_t vertex = 0; vertex < 4; ++vertex) {
        const std::size_t offset = vertex * 24;
        Check(Load<float>(out, offset) == xs[vertex]);
        Check(Load<float>(out, offset + 4) == ys[vertex]);
        Check(Load<float>(out, offset + 8) == Load<float>(original, offset + 8));
        Check(Load<float>(out, offset + 16) == Load<float>(original, offset + 16));
        Check(Load<float>(out, offset + 20) == Load<float>(original, offset + 20));
        const auto before = Load<std::uint32_t>(original, offset + 12);
        const auto after = Load<std::uint32_t>(out, offset + 12);
        Check((after & 0x00FFFFFFU) == (before & 0x00FFFFFFU));
        Check((after >> 24) == 127U); // 0.5 * 0.5? no: opacity 0.5 * 1.0 -> 127
    }
    Check(Load<std::uint32_t>(out, 144) == 0xD0000000U);
    Check(Load<std::uint64_t>(out, 152) == 0x123456789ABCDEF0ULL);
    Check(raw == original);
}

void TestFadeAndOpacity() {
    auto raw = MakeTextured();
    nosfix::TimedNoteGeometry note;
    auto parts = QuadParts(raw);
    auto spec = MakeQuadSpec();
    spec.opacity_scale = 0.25f;
    Check(note.Capture(spec, parts));

    FakeProjector projector;
    projector.opacity = 1.0f;
    std::array<nosfix::NotePartView, 4> views{};
    std::size_t count = 0;
    nosfix::GeometryDiagnostic diagnostic = nosfix::GeometryDiagnostic::None;
    Check(note.Build(1'000.0, Project, &projector, views, count, diagnostic));
    const std::span<const std::byte> out(views[0].data, 160);
    Check((Load<std::uint32_t>(out, 12) >> 24) == 63U); // 0.25 * 255 = 63.75
}

void TestLongEndpointsAndRibbon() {
    auto quad_raw = MakeTextured();
    auto box_raw = MakeBoxRecords(12);
    const auto box_original = box_raw;
    nosfix::TimedNoteGeometry note;
    std::vector<nosfix::NotePartInput> parts{
        {nosfix::NotePartShape::TexturedQuad, nosfix::NoteAnchorRule::Start, 1, quad_raw},
        {nosfix::NotePartShape::TexturedQuad, nosfix::NoteAnchorRule::End, 1, quad_raw},
        {nosfix::NotePartShape::BoxRibbon, nosfix::NoteAnchorRule::StartToEnd, 12, box_raw},
        {nosfix::NotePartShape::BoxLine, nosfix::NoteAnchorRule::StartToEnd, 12, box_raw},
    };
    Check(note.Capture(MakeRibbonSpec(), parts));

    FakeProjector projector;
    std::array<nosfix::NotePartView, 4> views{};
    std::size_t count = 0;
    nosfix::GeometryDiagnostic diagnostic = nosfix::GeometryDiagnostic::None;
    Check(note.Build(0.0, Project, &projector, views, count, diagnostic));
    Check(count == 4);

    float head_distance = 0.0f;
    float tail_distance = 0.0f;
    Check(nosfix::TimedNoteGeometry::DistanceFor(MakeRibbonSpec(), 1'000.0, 0.0, head_distance));
    Check(nosfix::TimedNoteGeometry::DistanceFor(MakeRibbonSpec(), 4'000.0, 0.0, tail_distance));
    Check(std::abs(head_distance - 3.0f) < 1e-6f);
    Check(std::abs(tail_distance - 12.0f) < 1e-6f);

    const std::uint32_t expected = nosfix::TimedNoteGeometry::RibbonSegments(
        MakeRibbonSpec(), head_distance, tail_distance);
    Check(expected >= 10 && expected <= 37);
    Check(views[2].count == expected);
    Check(views[3].count == expected);

    // Head and tail quads use their own anchor times.
    const float head_y = 300.0f - head_distance * 0.5f;
    const float tail_y = 300.0f - tail_distance * 0.5f;
    Check(Load<float>(std::span<const std::byte>(views[0].data, 160), 4) <
          300.0f); // head centre is 10 px above the anchor
    const float head_top = Load<float>(std::span<const std::byte>(views[0].data, 160), 4);
    Check(std::abs(head_top - (head_y - 20.0f)) < 1e-4f);
    const float tail_top = Load<float>(std::span<const std::byte>(views[1].data, 160), 4);
    Check(std::abs(tail_top - (tail_y - 20.0f)) < 1e-4f);

    // Ribbon strip: first quad starts at the snapped tail, last one ends at the
    // head. Corners follow the native quad order and preserve z/RHW/colours.
    const std::span<const std::byte> strip(views[2].data, views[2].count * 120);
    const std::span<const std::byte> first(strip.data(), 120);
    const std::span<const std::byte> last(strip.data() + (views[2].count - 1) * 120, 120);
    Check(Load<float>(first, 8) == 0.0f);
    Check(Load<float>(first, 12) == 1.0f);
    Check(Load<float>(last, 4) == Load<float>(last, 24)); // p1.y == p2.y? see below
    Check(Load<std::uint32_t>(first, 16) == 0x40000000U);
    Check(Load<std::uint32_t>(first, 36) == 0x40000001U);
    Check(Load<std::uint32_t>(first, 56) == 0x40000002U);
    Check(Load<std::uint32_t>(first, 96) == 0x40000001U);
    const float last_bottom = Load<float>(last, 44); // v2 y (p4)
    Check(std::abs(last_bottom - head_y) < 1e-3f);
    Check(box_raw == box_original);
    Check(Load<std::uint32_t>(first, 116) == 0x40000005U);
}

void TestNoSourceMutation() {
    auto raw = MakeTextured();
    auto box = MakeBoxRecords(8);
    const auto raw_before = raw;
    const auto box_before = box;
    nosfix::TimedNoteGeometry note;
    std::vector<nosfix::NotePartInput> parts{
        {nosfix::NotePartShape::TexturedQuad, nosfix::NoteAnchorRule::Start, 1, raw},
        {nosfix::NotePartShape::BoxRibbon, nosfix::NoteAnchorRule::StartToEnd, 8, box},
    };
    Check(note.Capture(MakeRibbonSpec(), parts));
    FakeProjector projector;
    std::array<nosfix::NotePartView, 4> views{};
    for (int step = 0; step < 500; ++step) {
        std::size_t count = 0;
        nosfix::GeometryDiagnostic diagnostic = nosfix::GeometryDiagnostic::None;
        Check(note.Build(static_cast<double>(step) * 7.5, Project, &projector,
                         views, count, diagnostic));
        Check(raw == raw_before);
        Check(box == box_before);
    }
}

void TestResetAndRecapture() {
    auto raw = MakeTextured();
    nosfix::TimedNoteGeometry note;
    FakeProjector projector;
    std::array<nosfix::NotePartView, 4> views{};
    std::size_t count = 99;
    nosfix::GeometryDiagnostic diagnostic = nosfix::GeometryDiagnostic::None;
    Check(!note.Build(0.0, Project, &projector, views, count, diagnostic));
    Check(diagnostic == nosfix::GeometryDiagnostic::NotCaptured);
    Check(count == 0);

    auto parts = QuadParts(raw);
    Check(note.Capture(MakeQuadSpec(), parts));
    Check(note.Build(0.0, Project, &projector, views, count, diagnostic));
    Check(count == 1);
    note.Reset();
    Check(!note.HasCapture());
    Check(!note.Build(0.0, Project, &projector, views, count, diagnostic));
    Check(diagnostic == nosfix::GeometryDiagnostic::NotCaptured);

    Check(note.Capture(MakeQuadSpec(), parts));
    Check(note.Build(0.0, Project, &projector, views, count, diagnostic));
    Check(count == 1);
    // Too small an output span must be rejected, not truncated.
    std::array<nosfix::NotePartView, 4> none{};
    Check(!note.Build(0.0, Project, &projector, std::span(none).first(0), count, diagnostic));
    Check(diagnostic == nosfix::GeometryDiagnostic::PartCountMismatch);
}

void TestScrollModes() {
    auto raw = MakeTextured();
    for (int mode = 0; mode <= 5; ++mode) {
        auto spec = MakeQuadSpec();
        spec.scroll_mode = mode;
        nosfix::TimedNoteGeometry note;
        auto parts = QuadParts(raw);
        Check(note.Capture(spec, parts));
        FakeProjector projector;
        std::array<nosfix::NotePartView, 4> views{};
        std::size_t count = 0;
        nosfix::GeometryDiagnostic diagnostic = nosfix::GeometryDiagnostic::None;
        Check(note.Build(0.0, Project, &projector, views, count, diagnostic));
        Check(count == 1);
    }
    auto bad = MakeQuadSpec();
    bad.scroll_mode = 6;
    nosfix::TimedNoteGeometry note;
    auto parts = QuadParts(raw);
    Check(!note.Capture(bad, parts));
}

void TestUnsupportedLayouts() {
    auto raw = MakeTextured();
    auto box = MakeBoxRecords(4);
    {
        auto spec = MakeQuadSpec();
        spec.quad_half_width = 0.0f;
        nosfix::TimedNoteGeometry note;
        auto parts = QuadParts(raw);
        Check(note.Capture(spec, parts));
        FakeProjector projector;
        std::array<nosfix::NotePartView, 4> views{};
        std::size_t count = 0;
        nosfix::GeometryDiagnostic diagnostic = nosfix::GeometryDiagnostic::None;
        Check(!note.Build(0.0, Project, &projector, views, count, diagnostic));
        Check(diagnostic == nosfix::GeometryDiagnostic::LayoutUnverified);
    }
    {
        auto spec = MakeRibbonSpec();
        spec.ribbon_base_width = 0.0f;
        nosfix::TimedNoteGeometry note;
        std::vector<nosfix::NotePartInput> parts{
            {nosfix::NotePartShape::BoxRibbon, nosfix::NoteAnchorRule::StartToEnd, 4, box}};
        Check(note.Capture(spec, parts));
        FakeProjector projector;
        std::array<nosfix::NotePartView, 4> views{};
        std::size_t count = 0;
        nosfix::GeometryDiagnostic diagnostic = nosfix::GeometryDiagnostic::None;
        Check(!note.Build(0.0, Project, &projector, views, count, diagnostic));
        Check(diagnostic == nosfix::GeometryDiagnostic::LayoutUnverified);
    }
    {
        nosfix::TimedNoteGeometry note;
        std::vector<nosfix::NotePartInput> parts{
            {nosfix::NotePartShape::BoxRibbon, nosfix::NoteAnchorRule::StartToLink, 4, box}};
        Check(note.Capture(MakeRibbonSpec(), parts));
        FakeProjector projector;
        std::array<nosfix::NotePartView, 4> views{};
        std::size_t count = 0;
        nosfix::GeometryDiagnostic diagnostic = nosfix::GeometryDiagnostic::None;
        Check(!note.Build(0.0, Project, &projector, views, count, diagnostic));
        Check(diagnostic == nosfix::GeometryDiagnostic::GlissDataBaked);
    }
    {
        // A part that asks for an end anchor on a note without an end time.
        auto spec = MakeQuadSpec();
        spec.has_end = false;
        nosfix::TimedNoteGeometry note;
        std::vector<nosfix::NotePartInput> parts{
            {nosfix::NotePartShape::TexturedQuad, nosfix::NoteAnchorRule::End, 1, raw}};
        Check(note.Capture(spec, parts));
        FakeProjector projector;
        std::array<nosfix::NotePartView, 4> views{};
        std::size_t count = 0;
        nosfix::GeometryDiagnostic diagnostic = nosfix::GeometryDiagnostic::None;
        Check(!note.Build(0.0, Project, &projector, views, count, diagnostic));
        Check(diagnostic == nosfix::GeometryDiagnostic::MissingAnchorTime);
    }
}

void TestProjectionFailures() {
    auto raw = MakeTextured();
    nosfix::TimedNoteGeometry note;
    auto parts = QuadParts(raw);
    Check(note.Capture(MakeQuadSpec(), parts));
    std::array<nosfix::NotePartView, 4> views{};
    std::size_t count = 0;
    nosfix::GeometryDiagnostic diagnostic = nosfix::GeometryDiagnostic::None;
    {
        FakeProjector projector;
        projector.reject = true;
        Check(!note.Build(0.0, Project, &projector, views, count, diagnostic));
        Check(diagnostic == nosfix::GeometryDiagnostic::ProjectionRejected);
    }
    {
        FakeProjector projector;
        projector.nan = true;
        Check(!note.Build(0.0, Project, &projector, views, count, diagnostic));
        Check(diagnostic == nosfix::GeometryDiagnostic::NonFiniteProjection);
    }
    {
        FakeProjector projector;
        Check(!note.Build(std::numeric_limits<double>::quiet_NaN(), Project, &projector,
                          views, count, diagnostic));
        Check(diagnostic == nosfix::GeometryDiagnostic::TimeRange);
    }
    {
        FakeProjector projector;
        Check(!note.Build(0.0, nullptr, &projector, views, count, diagnostic));
        Check(diagnostic == nosfix::GeometryDiagnostic::ProjectionRejected);
    }
}

void TestSegmentCap() {
    auto box = MakeBoxRecords(4);
    auto spec = MakeRibbonSpec();
    nosfix::TimedNoteGeometry note;
    std::vector<nosfix::NotePartInput> parts{
        {nosfix::NotePartShape::BoxRibbon, nosfix::NoteAnchorRule::StartToEnd, 4, box}};
    Check(note.Capture(spec, parts));
    FakeProjector projector;
    std::array<nosfix::NotePartView, 4> views{};
    std::size_t count = 0;
    nosfix::GeometryDiagnostic diagnostic = nosfix::GeometryDiagnostic::None;
    // Song time past the note start makes the head distance negative, so the
    // strip is longer than the visible band and the 37-segment cap applies.
    Check(note.Build(1'500.0, Project, &projector, views, count, diagnostic));
    Check(views[0].count <= spec.max_ribbon_segments);
    Check(views[0].count == spec.max_ribbon_segments);

    float head = 0.0f;
    float tail = 0.0f;
    Check(nosfix::TimedNoteGeometry::DistanceFor(spec, spec.start_ms, 1'500.0, head));
    Check(nosfix::TimedNoteGeometry::DistanceFor(spec, spec.end_ms, 1'500.0, tail));
    Check(head < 0.0f);
    Check(nosfix::TimedNoteGeometry::RibbonSegments(spec, head, tail) ==
          spec.max_ribbon_segments);
}

void TestExtremeInputs() {
    // A tiny approach distance makes the tail snap loop run out of iterations.
    auto spec = MakeRibbonSpec();
    spec.approach = 1.0e-30f;
    auto box = MakeBoxRecords(4);
    nosfix::TimedNoteGeometry note;
    std::vector<nosfix::NotePartInput> parts{
        {nosfix::NotePartShape::BoxRibbon, nosfix::NoteAnchorRule::StartToEnd, 4, box}};
    Check(note.Capture(spec, parts));
    FakeProjector projector;
    std::array<nosfix::NotePartView, 4> views{};
    std::size_t count = 0;
    nosfix::GeometryDiagnostic diagnostic = nosfix::GeometryDiagnostic::None;
    Check(!note.Build(0.0, Project, &projector, views, count, diagnostic));
    Check(diagnostic == nosfix::GeometryDiagnostic::TailOutOfBand);

    // Public helpers reject non-finite input and clamp before converting.
    const auto normal = MakeRibbonSpec();
    Check(nosfix::TimedNoteGeometry::RibbonSegments(
              normal, 0.0f, std::numeric_limits<float>::infinity()) == 0);
    Check(nosfix::TimedNoteGeometry::RibbonSegments(
              normal, std::numeric_limits<float>::quiet_NaN(), 1.0f) == 0);
    Check(nosfix::TimedNoteGeometry::RibbonSegments(normal, 0.0f, 0.0f) == 0);
    Check(nosfix::TimedNoteGeometry::RibbonSegments(normal, 5.0f, 1.0f) == 0);
    // A span far beyond 2^32 segments must clamp, never wrap.
    Check(nosfix::TimedNoteGeometry::RibbonSegments(normal, -1.0e9f, 6.0f) ==
          normal.max_ribbon_segments);
    Check(nosfix::TimedNoteGeometry::RibbonSegments(normal, -1.0e6f, 6.0f) ==
          normal.max_ribbon_segments);
    auto zero_approach = normal;
    zero_approach.approach = 0.0f;
    Check(nosfix::TimedNoteGeometry::RibbonSegments(zero_approach, 0.0f, 6.0f) == 0);

    // The step divisor is a spec field because the trill path divides the
    // approach distance by the module constant 0x180687298 (80.0), not 30.
    auto trill_step = normal;
    trill_step.ribbon_step_divisor = 80.0f;
    Check(nosfix::TimedNoteGeometry::RibbonSegments(trill_step, 0.0f, 6.0f) ==
          trill_step.max_ribbon_segments);   // 80x finer step, capped
    auto bad_divisor = normal;
    bad_divisor.ribbon_step_divisor = 0.0f;
    {
        auto box2 = MakeBoxRecords(2);
        nosfix::TimedNoteGeometry probe;
        std::vector<nosfix::NotePartInput> ribbon{
            {nosfix::NotePartShape::BoxRibbon, nosfix::NoteAnchorRule::StartToEnd, 2, box2}};
        Check(!probe.Capture(bad_divisor, ribbon));
    }
    auto zero_rate = normal;
    zero_rate.legacy_rate = 0.0f;
    float distance = 0.0f;
    Check(!nosfix::TimedNoteGeometry::DistanceFor(zero_rate, 1'000.0, 0.0, distance));
}

void TestInvalidPartDescriptors() {
    auto raw = MakeTextured();
    auto box = MakeBoxRecords(2);
    nosfix::TimedNoteGeometry note;
    const auto spec = MakeRibbonSpec();
    {
        nosfix::NotePartInput bad{
            static_cast<nosfix::NotePartShape>(9), nosfix::NoteAnchorRule::Start, 1, raw};
        Check(!note.Capture(spec, {&bad, 1}));
    }
    {
        nosfix::NotePartInput bad{
            nosfix::NotePartShape::TexturedQuad, static_cast<nosfix::NoteAnchorRule>(9),
            1, raw};
        Check(!note.Capture(spec, {&bad, 1}));
    }
    {
        // A textured record cannot span two anchors: that would emit 120-byte
        // box records into a 160-byte quad buffer.
        nosfix::NotePartInput bad{
            nosfix::NotePartShape::TexturedQuad, nosfix::NoteAnchorRule::StartToEnd,
            1, raw};
        Check(!note.Capture(spec, {&bad, 1}));
    }
    {
        // A box strip cannot be anchored at a single point.
        nosfix::NotePartInput bad{
            nosfix::NotePartShape::BoxRibbon, nosfix::NoteAnchorRule::Start, 2, box};
        Check(!note.Capture(spec, {&bad, 1}));
    }
    Check(!note.HasCapture());
}

void TestNoAllocationAfterSetup() {
    auto quad_raw = MakeTextured();
    auto box_raw = MakeBoxRecords(8);
    nosfix::TimedNoteGeometry note;
    std::vector<nosfix::NotePartInput> parts{
        {nosfix::NotePartShape::TexturedQuad, nosfix::NoteAnchorRule::Start, 1, quad_raw},
        {nosfix::NotePartShape::TexturedQuad, nosfix::NoteAnchorRule::End, 1, quad_raw},
        {nosfix::NotePartShape::BoxRibbon, nosfix::NoteAnchorRule::StartToEnd, 8, box_raw},
        {nosfix::NotePartShape::BoxLine, nosfix::NoteAnchorRule::StartToEnd, 8, box_raw},
    };
    const auto spec = MakeRibbonSpec();
    const std::size_t before_setup = g_allocations;
    Check(note.Capture(spec, parts));   // first capture is allocation-free too
    Check(g_allocations == before_setup);

    FakeProjector projector;
    std::array<nosfix::NotePartView, 4> views{};
    std::size_t peak = 0;
    for (int step = 0; step < 200; ++step) {
        // Vary the live record count the way the game's ribbon rebuild does.
        const std::uint32_t records = 1 + static_cast<std::uint32_t>(step % 8);
        parts[2].record_count = records;
        parts[2].raw = std::span<const std::byte>(box_raw).first(records * 120);
        parts[3].record_count = records;
        parts[3].raw = std::span<const std::byte>(box_raw).first(records * 120);
        const std::size_t before_capture = g_allocations;
        Check(note.Capture(spec, parts));
        std::size_t count = 0;
        nosfix::GeometryDiagnostic diagnostic = nosfix::GeometryDiagnostic::None;
        Check(note.Build(1'000.0 + static_cast<double>(step) * 3.5, Project, &projector,
                         views, count, diagnostic));
        const std::size_t used = g_allocations - before_capture;
        peak = std::max(peak, used);
    }
    Check(peak == 0);   // no allocation in capture/build, including buffer reuse
}

void TestAllocationFailureIsContained() {
    auto raw = MakeTextured();
    nosfix::TimedNoteGeometry note;
    std::vector<nosfix::NotePartInput> parts{
        {nosfix::NotePartShape::TexturedQuad, nosfix::NoteAnchorRule::Start, 1, raw}};
    const auto spec = MakeQuadSpec();

    // The fixed cache has no allocating step left, so a global allocation
    // failure must not affect capture or a render-only build at all.
    const std::size_t before = g_allocations;
    g_fail_allocations = true;
    Check(note.Capture(spec, parts));
    Check(note.HasCapture());
    FakeProjector projector;
    std::array<nosfix::NotePartView, 4> views{};
    std::size_t count = 0;
    nosfix::GeometryDiagnostic diagnostic = nosfix::GeometryDiagnostic::None;
    Check(note.Build(1'000.0, Project, &projector, views, count, diagnostic));
    Check(count == 1);
    Check(g_allocations == before);
    g_fail_allocations = false;

    // Reuse for a different note, then back again: no capacity churn.
    auto box = MakeBoxRecords(6);
    std::vector<nosfix::NotePartInput> ribbon{
        {nosfix::NotePartShape::TrillRibbon, nosfix::NoteAnchorRule::StartToEnd, 6, box}};
    auto trill_spec = MakeRibbonSpec();
    trill_spec.ribbon_step_divisor = 80.0f;
    trill_spec.max_ribbon_segments = 90;
    const std::size_t reuse_before = g_allocations;
    g_fail_allocations = true;
    Check(note.Capture(trill_spec, ribbon));
    Check(note.PartCount() == 1);
    Check(note.PartShape(0) == nosfix::NotePartShape::TrillRibbon);
    Check(note.Capture(spec, parts));
    Check(note.PartCount() == 1);
    Check(note.PartShape(0) == nosfix::NotePartShape::TexturedQuad);
    g_fail_allocations = false;
    Check(g_allocations == reuse_before);
}

// Cadence invariance: the same song time must produce byte-identical geometry
// at 60, 90, 120, 144, 165, 240 and 360 Hz, after jitter and after a stall.
void TestCadenceInvariance() {
    const std::array<double, 7> rates{60.0, 90.0, 120.0, 144.0, 165.0, 240.0, 360.0};
    auto quad_raw = MakeTextured();
    auto box_raw = MakeBoxRecords(8);
    nosfix::TimedNoteGeometry note;
    std::vector<nosfix::NotePartInput> parts{
        {nosfix::NotePartShape::TexturedQuad, nosfix::NoteAnchorRule::Start, 1, quad_raw},
        {nosfix::NotePartShape::TexturedQuad, nosfix::NoteAnchorRule::End, 1, quad_raw},
        {nosfix::NotePartShape::BoxRibbon, nosfix::NoteAnchorRule::StartToEnd, 8, box_raw},
    };
    Check(note.Capture(MakeRibbonSpec(), parts));

    const std::array<double, 4> checkpoints{1'000.0, 2'000.0, 3'000.0, 4'000.0};
    std::vector<Snapshot> reference;
    std::uint64_t draws60 = 0;
    std::uint64_t draws360 = 0;

    for (const double rate : rates) {
        std::vector<Snapshot> results;
        std::uint64_t frame = 0;
        double last = 0.0;
        while (true) {
            const double now = (static_cast<double>(frame) * 1000.0) / rate;
            if (now > 4'000.0) break;
            FakeProjector projector;
            std::array<nosfix::NotePartView, 4> views{};
            std::size_t count = 0;
            nosfix::GeometryDiagnostic diagnostic = nosfix::GeometryDiagnostic::None;
            Check(note.Build(now, Project, &projector, views, count, diagnostic));
            Check(count == 3);
            for (const double checkpoint : checkpoints) {
                if (std::abs(now - checkpoint) < 1e-9) {
                    results.push_back(Take(views[0], 160));
                    results.push_back(Take(views[1], 160));
                    results.push_back(Take(views[2], 120));
                }
            }
            last = now;
            ++frame;
        }
        Check(last <= 4'000.0);
        if (rate == 60.0) draws60 = frame;
        if (rate == 360.0) draws360 = frame;
        Check(results.size() == checkpoints.size() * 3);
        if (reference.empty()) {
            reference = results;
        } else {
            Check(results.size() == reference.size());
            for (std::size_t index = 0; index < results.size(); ++index) {
                Check(results[index].count == reference[index].count);
                Check(results[index].bytes == reference[index].bytes);
            }
        }
    }
    // A higher cadence really did produce more evaluations, and still the
    // geometry at the shared instants is byte identical.
    Check(draws60 > 200 && draws60 < 260);
    Check(draws360 > draws60 * 5);

    // Jitter and a long stall before the checkpoint must not change the result.
    {
        FakeProjector projector;
        std::array<nosfix::NotePartView, 4> views{};
        std::size_t count = 0;
        nosfix::GeometryDiagnostic diagnostic = nosfix::GeometryDiagnostic::None;
        std::uint32_t seed = 12345;
        for (int step = 0; step < 400; ++step) {
            seed = seed * 1664525U + 1013904223U;
            const double jitter = static_cast<double>(seed % 7000) / 1000.0;
            Check(note.Build(jitter, Project, &projector, views, count, diagnostic));
        }
        Check(note.Build(2'000.0, Project, &projector, views, count, diagnostic));
        const auto flat = Take(views[2], 120);
        Check(flat == reference[1 * 3 + 2]);
    }
}

void TestBuildPartMatchesFullBuild() {
    auto quad_raw = MakeTextured();
    auto box_raw = MakeBoxRecords(6);
    const std::int32_t fake_source_a = 0;
    const std::int32_t fake_source_b = 1;
    nosfix::TimedNoteGeometry note;
    std::vector<nosfix::NotePartInput> parts{
        {nosfix::NotePartShape::TexturedQuad, nosfix::NoteAnchorRule::Start, 1, quad_raw,
         &fake_source_a},
        {nosfix::NotePartShape::TexturedQuad, nosfix::NoteAnchorRule::End, 1, quad_raw,
         &fake_source_b},
        {nosfix::NotePartShape::BoxRibbon, nosfix::NoteAnchorRule::StartToEnd, 6, box_raw,
         &fake_source_b},
    };
    Check(note.Capture(MakeRibbonSpec(), parts));
    Check(note.PartCount() == 3);
    Check(note.PartSource(0) == &fake_source_a);
    Check(note.PartSource(1) == &fake_source_b);
    Check(note.PartSource(3) == nullptr);
    Check(note.PartShape(2) == nosfix::NotePartShape::BoxRibbon);

    FakeProjector projector;
    std::array<nosfix::NotePartView, 4> views{};
    std::size_t count = 0;
    nosfix::GeometryDiagnostic diagnostic = nosfix::GeometryDiagnostic::None;
    const double now = 2'345.5;
    Check(note.Build(now, Project, &projector, views, count, diagnostic));
    Check(count == 3);
    const auto full = Take(views[2], 120);

    nosfix::NotePartView single{};
    const unsigned calls_before = projector.calls;
    Check(note.BuildPart(2, now, Project, &projector, single, diagnostic));
    Check(projector.calls > calls_before);
    Check(single.count == full.count);
    const auto part = Take(single, 120);
    Check(part == full);

    Check(!note.BuildPart(7, now, Project, &projector, single, diagnostic));
    Check(diagnostic == nosfix::GeometryDiagnostic::PartCountMismatch);
}

void TestPerPartQuadExtents() {
    auto raw = MakeTextured();
    auto spec = MakeQuadSpec();
    spec.quad_half_width = 10.0f;
    spec.quad_half_height = 20.0f;
    spec.has_end = true;
    spec.end_ms = 2'000.0;
    nosfix::TimedNoteGeometry note;

    // The long/trill head and tail use different native words, so each textured
    // part carries its own extents. A changed tail width must move only the
    // tail, never reuse the head size.
    std::vector<nosfix::NotePartInput> parts{
        {nosfix::NotePartShape::TexturedQuad, nosfix::NoteAnchorRule::Start, 1, raw,
         nullptr, 30.0f, 10.0f},
        {nosfix::NotePartShape::TexturedQuad, nosfix::NoteAnchorRule::End, 1, raw,
         nullptr, 50.0f, 15.0f},
    };
    Check(note.Capture(spec, parts));
    FakeProjector projector;
    std::array<nosfix::NotePartView, 4> views{};
    std::size_t count = 0;
    nosfix::GeometryDiagnostic diagnostic = nosfix::GeometryDiagnostic::None;
    Check(note.Build(1'000.0, Project, &projector, views, count, diagnostic));
    Check(count == 2);
    Check(Load<float>({views[0].data, 160}, 0) == 200.0f + 3.0f - 30.0f);
    Check(Load<float>({views[0].data, 160}, 4) == 300.0f - 10.0f);
    // Tail distance 3.0 in the fake projector.
    const float tail_x = 200.0f + 3.0f * 0.25f;
    const float tail_y = 300.0f - 3.0f * 0.5f;
    Check(Load<float>({views[1].data, 160}, 0) == tail_x + 3.0f - 50.0f);
    Check(Load<float>({views[1].data, 160}, 4) == tail_y - 15.0f);

    // A partial extent pair is refused instead of mixing a stale dimension.
    nosfix::NotePartInput partial{
        nosfix::NotePartShape::TexturedQuad, nosfix::NoteAnchorRule::Start, 1, raw,
        nullptr, 30.0f, 0.0f};
    Check(!note.Capture(spec, {&partial, 1}));
    Check(!note.HasCapture());

    // Zero extents select the spec's own quad size (single-quad callers).
    auto single = QuadParts(raw);
    Check(note.Capture(MakeQuadSpec(), single));
    Check(note.Build(1'000.0, Project, &projector, views, count, diagnostic));
    Check(Load<float>({views[0].data, 160}, 0) == 200.0f + 3.0f - 10.0f);
}

void TestTrillStrip() {
    auto box = MakeBoxRecords(90);
    const auto box_before = box;
    auto spec = MakeRibbonSpec();
    spec.start_ms = 1'000.0;
    spec.end_ms = 2'000.0;
    spec.approach = 2.0f;
    spec.ribbon_base_width = 20.0f;
    spec.ribbon_step_divisor = 80.0f;
    spec.max_ribbon_segments = 90;

    nosfix::TimedNoteGeometry note;
    std::vector<nosfix::NotePartInput> parts{
        {nosfix::NotePartShape::TrillRibbon, nosfix::NoteAnchorRule::StartToEnd, 90, box},
        {nosfix::NotePartShape::TrillGlow, nosfix::NoteAnchorRule::StartToEnd, 90, box},
    };
    Check(note.Capture(spec, parts));
    FakeProjector projector;
    std::array<nosfix::NotePartView, 4> views{};
    std::size_t count = 0;
    nosfix::GeometryDiagnostic diagnostic = nosfix::GeometryDiagnostic::None;
    Check(note.Build(1'000.0, Project, &projector, views, count, diagnostic));
    Check(count == 2);

    // Tail 3.0 snaps to 2.0 in forty steps of 0.025, so 80 segments remain and
    // the native phase is 4 - (80 % 4) = 4. Phase 4 is even: the previous half
    // width decays 20 -> 15 and the glow uses the left edge (4 % 4 <= 1).
    Check(views[0].count == 80);
    Check(views[1].count == 80);
    const std::span<const std::byte> primary(views[0].data, 120);
    const float previous_x = 200.0f + 2.0f * 0.25f;
    const float previous_y = 300.0f - 2.0f * 0.5f;
    const float current_x = 200.0f + 1.975f * 0.25f;
    const float current_y = 300.0f - 1.975f * 0.5f;
    const float shift = 3.0f;
    Check(Load<float>(primary, 0) == previous_x + shift - 15.0f);
    Check(Load<float>(primary, 20) == previous_x + shift + 15.0f);
    Check(Load<float>(primary, 40) == current_x + shift - 20.0f);   // record order p4
    Check(Load<float>(primary, 60) == current_x + shift - 20.0f);
    Check(Load<float>(primary, 80) == previous_x + shift + 15.0f);
    Check(Load<float>(primary, 100) == current_x + shift + 20.0f);
    Check(Load<float>(primary, 4) == previous_y);
    Check(Load<float>(primary, 24) == previous_y);
    Check(Load<float>(primary, 44) == current_y);
    Check(Load<float>(primary, 64) == current_y);
    Check(Load<float>(primary, 84) == previous_y);
    Check(Load<float>(primary, 104) == current_y);
    // Colours (including alpha) come from the captured single-tick template.
    Check(Load<std::uint32_t>(primary, 16) == 0x40000000U);
    Check(Load<std::uint32_t>(primary, 36) == 0x40000001U);
    Check(Load<std::uint32_t>(primary, 56) == 0x40000002U);
    Check(Load<std::uint32_t>(primary, 116) == 0x40000005U);

    // Phase 4 left-edge glow: p1=prev-15, p2=prev-5, p3=cur-10, p4=cur-20.
    const std::span<const std::byte> glow(views[1].data, 120);
    Check(Load<float>(glow, 0) == previous_x + shift - 15.0f);
    Check(Load<float>(glow, 20) == previous_x + shift - 5.0f);
    Check(Load<float>(glow, 40) == current_x + shift - 20.0f);
    Check(Load<float>(glow, 60) == current_x + shift - 20.0f);
    Check(Load<float>(glow, 80) == previous_x + shift - 5.0f);
    Check(Load<float>(glow, 100) == current_x + shift - 10.0f);

    // Phase 2 (short span 0.051 -> round(2.04) = 2) uses the right edge.
    auto right_spec = spec;
    right_spec.end_ms = 1'017.0;
    nosfix::TimedNoteGeometry right;
    Check(right.Capture(right_spec, parts));
    Check(right.Build(1'000.0, Project, &projector, views, count, diagnostic));
    const std::span<const std::byte> right_glow(views[1].data, 120);
    const float right_tail = 0.051f;
    const float right_current = right_tail - 0.025f;
    const float right_previous_x = 200.0f + right_tail * 0.25f;
    const float right_current_x = 200.0f + right_current * 0.25f;
    Check(Load<float>(right_glow, 0) == right_previous_x + shift + 5.0f);   // 15 - 10
    Check(Load<float>(right_glow, 20) == right_previous_x + shift + 15.0f);
    Check(Load<float>(right_glow, 40) == right_current_x + shift + 10.0f);
    Check(Load<float>(right_glow, 60) == right_current_x + shift + 10.0f);
    Check(Load<float>(right_glow, 80) == right_previous_x + shift + 15.0f);
    Check(Load<float>(right_glow, 100) == right_current_x + shift + 20.0f);

    // A changed note width changes the strip; the native width arithmetic is
    // integral, so 30 decays to 23 (30 + 30/-4), not 22.5.
    auto wider = spec;
    wider.ribbon_base_width = 30.0f;
    nosfix::TimedNoteGeometry wide;
    Check(wide.Capture(wider, parts));
    Check(wide.Build(1'000.0, Project, &projector, views, count, diagnostic));
    Check(Load<float>({views[0].data, 120}, 0) == previous_x + shift - 23.0f);
    Check(box == box_before);
}

void TestDiagnosticNames() {
    Check(std::strcmp(nosfix::GeometryDiagnosticName(
              nosfix::GeometryDiagnostic::GlissDataBaked), "gliss-data-baked") == 0);
    Check(std::strcmp(nosfix::GeometryDiagnosticName(
              nosfix::GeometryDiagnostic::None), "none") == 0);
}

} // namespace

int main() {
    TestFractionalDistance();
    TestQuadGeometry();
    TestFadeAndOpacity();
    TestLongEndpointsAndRibbon();
    TestNoSourceMutation();
    TestResetAndRecapture();
    TestScrollModes();
    TestUnsupportedLayouts();
    TestProjectionFailures();
    TestSegmentCap();
    TestExtremeInputs();
    TestInvalidPartDescriptors();
    TestNoAllocationAfterSetup();
    TestAllocationFailureIsContained();
    TestBuildPartMatchesFullBuild();
    TestPerPartQuadExtents();
    TestTrillStrip();
    TestCadenceInvariance();
    TestDiagnosticNames();
    std::printf("timed_note_geometry: all tests passed\n");
    return 0;
}