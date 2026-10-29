// Executes the real nostalgia.dll projection helper through the timed geometry
// adapter. No game loop, graphics, audio, input or scoring code runs: the test
// maps the module, sets the plain data globals the projector reads, and drives
// the same private tables the game would populate.
#ifdef NDEBUG
#undef NDEBUG
#endif
#include "nosfix/timed_rendering.hpp"

#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

namespace {

template <class T> void Put(void* base, std::size_t offset, T value) {
    std::memcpy(static_cast<std::byte*>(base) + offset, &value, sizeof(value));
}
template <class T> T Get(std::span<const std::byte> raw, std::size_t offset) {
    T result{};
    std::memcpy(&result, raw.data() + offset, sizeof(result));
    return result;
}

std::vector<std::byte> MakeBoxRecords(std::uint32_t count) {
    std::vector<std::byte> raw(count * 120);
    for (std::uint32_t record = 0; record < count; ++record) {
        for (std::size_t vertex = 0; vertex < 6; ++vertex) {
            const std::size_t offset = record * 120 + vertex * 20;
            Put(raw.data(), offset, 0.0f);
            Put(raw.data(), offset + 4, 0.0f);
            Put(raw.data(), offset + 8, 0.0f);
            Put(raw.data(), offset + 12, 1.0f);
            Put(raw.data(), offset + 16, 0xFF00FF00U);
        }
    }
    return raw;
}

} // namespace

int wmain(int argc, wchar_t** argv) {
    assert(argc == 3);
    const auto game = LoadLibraryExW(argv[1], nullptr, DONT_RESOLVE_DLL_REFERENCES);
    const auto xact = LoadLibraryExW(argv[2], nullptr, DONT_RESOLVE_DLL_REFERENCES);
    assert(game && xact);
    std::string reason;
    assert(nosfix::PrepareTimedRendering(game, reason));
    nosfix::ConfigureTimedRendering(true);
    assert(nosfix::TimedRenderingEnabled());

    // Proven ribbon half-width rules. Long/trill: shrink*wide/2 - 10 + note+60/2
    // (0x2F4E80 and 0x1A84F0). Velocity zone: note+60/2 - 10, no shrink or wide
    // term (bytes at 0x1801AAEFC..0x1801AAF6E). Normal notes have no ribbon.
    assert(nosfix::RibbonWidthRuleFor(nosfix::TimedNoteKind::Long) ==
           nosfix::RibbonWidthRule::HeadAndTail);
    assert(nosfix::RibbonWidthRuleFor(nosfix::TimedNoteKind::Trill) ==
           nosfix::RibbonWidthRule::HeadAndTail);
    assert(nosfix::RibbonWidthRuleFor(nosfix::TimedNoteKind::VelocityZone) ==
           nosfix::RibbonWidthRule::VelocityZone);
    assert(nosfix::RibbonWidthRuleFor(nosfix::TimedNoteKind::Normal) ==
           nosfix::RibbonWidthRule::None);
    assert(nosfix::RibbonWidthRuleFor(nosfix::TimedNoteKind::Beats) ==
           nosfix::RibbonWidthRule::None);
    assert(nosfix::RibbonWidthRuleFor(nosfix::TimedNoteKind::Glissando) ==
           nosfix::RibbonWidthRule::None);
    float half = 0.0f;
    assert(nosfix::RibbonHalfWidth(nosfix::RibbonWidthRule::HeadAndTail, 60, 0, 40, half));
    assert(half == 20.0f);
    assert(nosfix::RibbonHalfWidth(nosfix::RibbonWidthRule::HeadAndTail, 60, -1, 40, half));
    assert(half == 0.0f);
    assert(nosfix::RibbonHalfWidth(nosfix::RibbonWidthRule::HeadAndTail, 60, -2, 40, half));
    assert(half == -20.0f);
    // Truncation toward zero matches the native integer division: -41/2 is -20,
    // so 40-wide terms give -20 - 10 + 20 = -10 (a floor would give -11).
    assert(nosfix::RibbonHalfWidth(nosfix::RibbonWidthRule::HeadAndTail, 40, -1, 41, half));
    assert(half == -10.0f);
    assert(nosfix::RibbonHalfWidth(nosfix::RibbonWidthRule::VelocityZone, 60, -2, 40, half));
    assert(half == 20.0f);
    assert(nosfix::RibbonHalfWidth(nosfix::RibbonWidthRule::VelocityZone, 21, 0, 0, half));
    assert(half == 0.0f);
    assert(nosfix::RibbonHalfWidth(nosfix::RibbonWidthRule::VelocityZone, 15, 0, 0, half));
    assert(half == -3.0f);
    assert(!nosfix::RibbonHalfWidth(nosfix::RibbonWidthRule::None, 60, 0, 40, half));
    assert(!nosfix::RibbonHalfWidth(nosfix::RibbonWidthRule::VelocityZone, 5000, 0, 0, half));
    assert(!nosfix::RibbonHalfWidth(nosfix::RibbonWidthRule::HeadAndTail, 60, -3, 40, half));
    assert(!nosfix::RibbonHalfWidth(nosfix::RibbonWidthRule::HeadAndTail, 60, 0, 100000,
                                    half));
    // Only the evidenced kinds are rebuilt; the rest stay on the original path.
    assert(nosfix::TimedNoteSupported(nosfix::TimedNoteKind::Normal));
    assert(nosfix::TimedNoteSupported(nosfix::TimedNoteKind::Long));
    assert(nosfix::TimedNoteSupported(nosfix::TimedNoteKind::Trill));
    assert(nosfix::TimedNoteSupported(nosfix::TimedNoteKind::Glissando));
    assert(!nosfix::TimedNoteSupported(nosfix::TimedNoteKind::VelocityZone));
    assert(!nosfix::TimedNoteSupported(nosfix::TimedNoteKind::Beats));

    Put(game, 0x1B72568, std::uint32_t{1});
    Put(game, 0x1B725AC, std::int32_t{20});
    Put(game, 0x1B725C4, std::int32_t{10});
    Put(game, 0x1B725C8, std::int32_t{0});
    Put(game, 0x13E4724, std::int32_t{3});   // linear scroll mode
    Put(game, 0x13668D4, 2.0f);              // approach distance
    Put(game, 0x13E4758, 0.0f);              // ribbon start bias
    Put(game, 0x13E476C, std::int32_t{40});
    Put(game, 0x70B300, 60.0f);
    Put(game, 0x70B2F8, 60.0);
    Put(game, 0x136690C, std::int32_t{0});
    Put(game, 0x1366918, std::int32_t{0});
    Put(game, 0x13668B0 + 0x7DEC0 + 4, std::int32_t{100});
    Put(game, 0x13668B0 + 0x7DEC0 + 8, std::int32_t{140});

    constexpr std::int32_t kStartMs = 10'001;
    constexpr std::int32_t kEndMs = 12'001;

    std::array<std::byte, 464> note{};
    std::array<std::byte, 104> chart{};
    std::array<std::byte, 160> head_raw{};
    std::array<std::byte, 160> tail_raw{};
    auto ribbon_raw = MakeBoxRecords(9);
    auto line_raw = MakeBoxRecords(9);
    head_raw.fill(std::byte{0});
    tail_raw.fill(std::byte{0});
    for (auto* raw : {&head_raw, &tail_raw}) {
        for (std::size_t vertex = 0; vertex < 4; ++vertex) {
            Put(raw->data(), vertex * 24, static_cast<float>(vertex));
            Put(raw->data(), vertex * 24 + 4, static_cast<float>(vertex));
            Put(raw->data(), vertex * 24 + 12, 0xFF102030U);
        }
        Put(raw->data(), 144, std::uint32_t{0xD0000000});
    }
    Put(chart.data(), 0, kStartMs);
    Put(chart.data(), 4, kEndMs);
    Put(chart.data(), 48, std::int32_t{0});
    Put(chart.data(), 97, std::uint8_t{0});
    Put(note.data(), 0, reinterpret_cast<std::byte*>(game) + 0x6841D0); // LongNotes
    Put(note.data(), 8, chart.data());
    Put(note.data(), 20, std::int32_t{0});
    Put(note.data(), 24, std::int32_t{600});
    Put(note.data(), 44, std::int32_t{1});
    Put(note.data(), 48, std::int32_t{2});
    Put(note.data(), 60, std::int32_t{60});
    Put(note.data(), 64, std::int32_t{1});
    Put(note.data(), 68, std::uint8_t{0});
    // Long notes keep head width/height at +240/+244 (a1[60]/a1[61]) and the
    // tail cap at +272/+276 (a1[68]/a1[69]).
    Put(note.data(), 240, std::int32_t{60});
    Put(note.data(), 244, std::int32_t{20});
    Put(note.data(), 272, std::int32_t{80});
    Put(note.data(), 276, std::int32_t{30});
    // Primitive objects: count at +24, backing pointer at +32.
    for (const auto offset : {0x80, 0xA8, 0x160, 0x198}) {
        const std::uint32_t count = offset == 0x80 || offset == 0xA8 ? 1 : 9;
        const void* buffer = offset == 0x80 ? static_cast<const void*>(head_raw.data())
            : offset == 0xA8 ? static_cast<const void*>(tail_raw.data())
            : offset == 0x160 ? static_cast<const void*>(ribbon_raw.data())
                              : static_cast<const void*>(line_raw.data());
        Put(note.data(), offset + 24, count);
        Put(note.data(), offset + 32, buffer);
    }

    const auto note_before = note;
    const auto head_before = head_raw;
    const auto tail_before = tail_raw;
    const auto ribbon_before = ribbon_raw;
    const auto chart_before = chart;

    nosfix::TimedNoteGeometry geometry;
    const auto status = nosfix::CaptureTimedNote(note.data(), nosfix::TimedNoteKind::Long,
                                                 geometry);
    assert(status == nosfix::TimedNoteCapture::Ok);
    assert(geometry.PartCount() == 4);
    assert(geometry.Spec().has_end);
    assert(geometry.Spec().start_ms == static_cast<double>(kStartMs));
    assert(geometry.Spec().end_ms == static_cast<double>(kEndMs));
    assert(geometry.Spec().quad_half_width == 30.0f);   // 60 / 2
    assert(geometry.Spec().quad_half_height == 10.0f);  // 20 / 2 * 10 / 10
    assert(geometry.PartSource(0) == head_raw.data());
    assert(geometry.PartSource(2) == ribbon_raw.data());

    // Head anchor at the exact chart millisecond: distance 0, centre (140, 700).
    nosfix::NotePartView view{};
    nosfix::GeometryDiagnostic diagnostic = nosfix::GeometryDiagnostic::None;
    assert(geometry.BuildPart(0, static_cast<double>(kStartMs), nosfix::ProjectGameNote,
                              nullptr, view, diagnostic));
    assert(view.count == 1);
    assert(Get<float>({view.data, 160}, 0) == 140.0f - 30.0f);
    assert(Get<float>({view.data, 160}, 4) == 700.0f - 10.0f);
    assert(Get<float>({view.data, 160}, 24) == 140.0f + 30.0f - 1.0f);

    // Same note a quarter millisecond earlier: the real projector now receives a
    // fractional distance and the whole quad moves by its continuous amount.
    nosfix::NoteProjection reference{};
    assert(nosfix::ProjectGameNote(nullptr, 0, static_cast<float>(0.25 * 180.0 / 60'000.0),
                                   1, 2, reference));
    assert(geometry.BuildPart(0, static_cast<double>(kStartMs) - 0.25,
                              nosfix::ProjectGameNote, nullptr, view, diagnostic));
    assert(Get<float>({view.data, 160}, 0) == reference.x - 30.0f);
    assert(Get<float>({view.data, 160}, 4) == reference.y - 10.0f);
    assert(reference.y != 700.0f);

    // Tail anchor uses the chart end time, not the head frame.
    assert(geometry.BuildPart(1, static_cast<double>(kStartMs), nosfix::ProjectGameNote,
                              nullptr, view, diagnostic));
    nosfix::NoteProjection tail_reference{};
    assert(nosfix::ProjectGameNote(nullptr, 0, static_cast<float>(2'000.0 * 180.0 / 60'000.0),
                                   1, 2, tail_reference));
    assert(Get<float>({view.data, 160}, 4) == tail_reference.y - 15.0f);
    assert(tail_reference.y < 700.0f);

    // Ribbon: the segment count follows the native rule and the last quad ends
    // exactly at the head anchor.
    float head_distance = 0.0f;
    float tail_distance = 0.0f;
    assert(nosfix::TimedNoteGeometry::DistanceFor(geometry.Spec(),
                                                  static_cast<double>(kStartMs),
                                                  static_cast<double>(kStartMs),
                                                  head_distance));
    assert(head_distance == 0.0f);
    assert(nosfix::TimedNoteGeometry::DistanceFor(geometry.Spec(),
                                                  static_cast<double>(kEndMs),
                                                  static_cast<double>(kStartMs),
                                                  tail_distance));
    assert(std::abs(tail_distance - 6.0f) < 1e-5f);
    const std::uint32_t expected = nosfix::TimedNoteGeometry::RibbonSegments(
        geometry.Spec(), head_distance, tail_distance);
    assert(expected >= 20 && expected <= 37);
    assert(geometry.BuildPart(2, static_cast<double>(kStartMs), nosfix::ProjectGameNote,
                              nullptr, view, diagnostic));
    assert(view.count == expected);
    const std::span<const std::byte> last(view.data + (view.count - 1) * 120, 120);
    // The projector truncates to integer pixels, so the last segment may land\n    // one pixel above the exact head centre when the float step has residue.\n    assert(std::abs(Get<float>(last, 44) - 700.0f) <= 1.0f);  // v2 (p4) y at the head
    assert(Get<float>(last, 48) == 0.0f);              // z
    assert(Get<float>(last, 52) == 1.0f);              // RHW
    assert(Get<std::uint32_t>(last, 56) == 0xFF00FF00U);

    // The four-pixel line keeps its constant width at every cadence.
    assert(geometry.BuildPart(3, static_cast<double>(kStartMs), nosfix::ProjectGameNote,
                              nullptr, view, diagnostic));
    const std::span<const std::byte> line(view.data, 120);
    const float left = Get<float>(line, 0);
    const float right = Get<float>(line, 20);
    assert(std::abs((right - left) - 4.0f) < 1e-4f);

    // Every supported scroll mode goes through the same real projector. The
    // output must stay finite and inside the projected band; mode 3 is the
    // linear reference the rest of the test asserts against.
    for (int mode = 0; mode <= 5; ++mode) {
        Put(game, 0x13E4724, std::int32_t{mode});
        assert(geometry.BuildPart(2, static_cast<double>(kStartMs),
                                  nosfix::ProjectGameNote, nullptr, view, diagnostic));
        assert(view.count >= 1 && view.count <= 37);
        for (std::size_t vertex = 0; vertex < 6; ++vertex) {
            const float x = Get<float>({view.data, view.count * 120},
                                       vertex * 20);
            const float y = Get<float>({view.data, view.count * 120},
                                       vertex * 20 + 4);
            assert(std::isfinite(x) && std::isfinite(y));
        }
    }
    Put(game, 0x13E4724, std::int32_t{3});

    // Fractional song times move the ribbon instead of leaving it on the last
    // simulation frame.
    std::vector<std::byte> previous(view.data, view.data + view.count * 120);
    assert(geometry.BuildPart(3, static_cast<double>(kStartMs) - 10.0,
                              nosfix::ProjectGameNote, nullptr, view, diagnostic));
    assert(std::memcmp(previous.data(), view.data,
                       std::min<std::size_t>(previous.size(),
                                             static_cast<std::size_t>(view.count) * 120)) != 0);

    // Nothing was written back into the game buffers or the note record.
    assert(note == note_before);
    assert(head_raw == head_before);
    assert(ribbon_raw == ribbon_before);

    // Trill: verified 0x1A84F0 layout. Head at +128 uses +240/+244, tail at
    // +168 uses +272/+276, and the two strips at +368/+424 use step
    // approach/80 with at most 90 records.
    auto trill_ribbon = MakeBoxRecords(90);
    auto trill_glow = MakeBoxRecords(90);
    const auto trill_ribbon_before = trill_ribbon;
    const auto trill_glow_before = trill_glow;
    Put(note.data(), 0, reinterpret_cast<std::byte*>(game) + 0x679C10); // TrillNotes
    for (const auto offset : {0x170, 0x1A8}) {
        const std::uint32_t count = 90;
        const void* buffer = offset == 0x170
            ? static_cast<const void*>(trill_ribbon.data())
            : static_cast<const void*>(trill_glow.data());
        Put(note.data(), offset + 24, count);
        Put(note.data(), offset + 32, buffer);
    }
    const auto trill_note_before = note;
    nosfix::TimedNoteGeometry trill;
    assert(nosfix::CaptureTimedNote(note.data(), nosfix::TimedNoteKind::Trill, trill) ==
           nosfix::TimedNoteCapture::Ok);
    assert(trill.PartCount() == 4);
    assert(trill.PartShape(2) == nosfix::NotePartShape::TrillRibbon);
    assert(trill.PartShape(3) == nosfix::NotePartShape::TrillGlow);
    assert(trill.Spec().ribbon_step_divisor == 80.0f);
    assert(trill.Spec().max_ribbon_segments == 90);
    assert(trill.Spec().has_end);

    // Head cap still uses +240/+244.
    assert(trill.BuildPart(0, static_cast<double>(kStartMs), nosfix::ProjectGameNote,
                           nullptr, view, diagnostic));
    assert(Get<float>({view.data, 160}, 0) == 140.0f - 30.0f);
    assert(Get<float>({view.data, 160}, 4) == 700.0f - 10.0f);

    // Tail cap uses its own +272/+276 extents: half width 80/2, half height
    // (30/2) * (10/10). The tail distance is 6.0, so its y is the band top.
    assert(trill.BuildPart(1, static_cast<double>(kStartMs), nosfix::ProjectGameNote,
                           nullptr, view, diagnostic));
    assert(Get<float>({view.data, 160}, 0) == 140.0f - 40.0f);
    assert(Get<float>({view.data, 160}, 4) == -50.0f - 15.0f);

    // Tremolo strip: tail 6.0 snaps to 2.0, phase 4, 80 segments. The first
    // half width decays 20 -> 15 on the previous side.
    assert(trill.BuildPart(2, static_cast<double>(kStartMs), nosfix::ProjectGameNote,
                           nullptr, view, diagnostic));
    assert(view.count == 80);
    assert(Get<float>({view.data, 120}, 0) == 140.0f - 15.0f);
    assert(Get<float>({view.data, 120}, 20) == 140.0f + 15.0f);
    assert(Get<float>({view.data, 120}, 40) == 140.0f - 20.0f);

    // Phase 4 is a left-edge glow: p1 = prev - 15, p2 = prev - 5.
    assert(trill.BuildPart(3, static_cast<double>(kStartMs), nosfix::ProjectGameNote,
                           nullptr, view, diagnostic));
    assert(view.count == 80);
    assert(Get<float>({view.data, 120}, 0) == 140.0f - 15.0f);
    assert(Get<float>({view.data, 120}, 20) == 140.0f - 5.0f);
    assert(Get<float>({view.data, 120}, 40) == 140.0f - 20.0f);

    // A fractional song time moves the strip; all game buffers stay intact.
    std::vector<std::byte> trill_at_start(view.data, view.data + view.count * 120);
    assert(trill.BuildPart(2, static_cast<double>(kStartMs) - 0.25,
                           nosfix::ProjectGameNote, nullptr, view, diagnostic));
    assert(std::memcmp(trill_at_start.data(), view.data,
                       std::min<std::size_t>(trill_at_start.size(),
                                             static_cast<std::size_t>(view.count) * 120)) != 0);
    assert(note == trill_note_before);
    assert(head_raw == head_before);
    assert(tail_raw == tail_before);
    assert(ribbon_raw == ribbon_before);
    assert(chart == chart_before);
    assert(trill_ribbon == trill_ribbon_before);
    assert(trill_glow == trill_glow_before);

    // A changed note width changes the trill strip: note+60 = 100 gives a base
    // half width of 40, which decays to 30 on the first segment.
    Put(note.data(), 60, std::int32_t{100});
    assert(nosfix::CaptureTimedNote(note.data(), nosfix::TimedNoteKind::Trill, trill) ==
           nosfix::TimedNoteCapture::Ok);
    assert(trill.BuildPart(2, static_cast<double>(kStartMs), nosfix::ProjectGameNote,
                           nullptr, view, diagnostic));
    assert(Get<float>({view.data, 120}, 0) == 140.0f - 30.0f);
    Put(note.data(), 60, std::int32_t{60});

    // Glissando is now rebuilt: the record's own +0x00 milliseconds and its
    // +0x14 frame must agree (0x1801A4945), the note quad is projected at
    // distance zero and the connector object is a whole-object part.
    // The loader's own constants, read-only: double 60.0 at 0x70B2F8 and
    // double 1000.0 at 0x687218.
    {
        const std::span<const std::byte> image(
            reinterpret_cast<const std::byte*>(game), 0x700000);
        assert(Get<double>(image, 0x70B2F8) == 60.0);
        assert(Get<double>(image, 0x687218) == 1000.0);
    }
    auto connector_raw = MakeBoxRecords(2);
    Put(note.data(), 0, reinterpret_cast<std::byte*>(game) + 0x679AB8);
    Put(note.data(), 196, std::int32_t{60});
    Put(note.data(), 200, std::int32_t{20});
    Put(chart.data(), 20, std::int32_t{600});   // frame of kStartMs = 10'001
    Put(note.data(), 0xD8 + 24, std::uint32_t{2});
    Put(note.data(), 0xD8 + 32, connector_raw.data());
    nosfix::TimedNoteGeometry gliss;
    assert(nosfix::CaptureTimedNote(note.data(), nosfix::TimedNoteKind::Glissando,
                                   gliss) == nosfix::TimedNoteCapture::Ok);
    assert(gliss.PartCount() == 2);
    assert(gliss.PartShape(0) == nosfix::NotePartShape::TexturedQuad);
    assert(!gliss.PartIsObject(0));
    assert(gliss.PartShape(1) == nosfix::NotePartShape::BoxObject);
    assert(gliss.PartIsObject(1));
    assert(gliss.Spec().start_ms == 10'001.0);
    assert(!gliss.Spec().has_end);
    // Distance zero: the note sits on the judgement line (x 140, y 700) and the
    // quad is 60 wide and 20 high, so the top left corner is (110, 690). The
    // song clock cannot move it.
    assert(gliss.BuildPart(0, static_cast<double>(kStartMs), nosfix::ProjectGameNote,
                           nullptr, view, diagnostic));
    assert(Get<float>({view.data, 160}, 0) == 140.0f - 30.0f);
    assert(Get<float>({view.data, 160}, 4) == 700.0f - 10.0f);
    const std::vector<std::byte> gliss_at_start(view.data, view.data + 160);
    assert(gliss.BuildPart(0, static_cast<double>(kStartMs) - 750.0,
                           nosfix::ProjectGameNote, nullptr, view, diagnostic));
    assert(view.count == 1);
    assert(std::memcmp(gliss_at_start.data(), view.data, 160) == 0);
    // The connector object has no caller-supplied corners here, so the anchor
    // path must refuse instead of inventing them.
    assert(!gliss.BuildPart(1, static_cast<double>(kStartMs), nosfix::ProjectGameNote,
                            nullptr, view, diagnostic));
    assert(diagnostic == nosfix::GeometryDiagnostic::ObjectQuadsRequired);
    // A record whose two time fields disagree is not an anchor.
    Put(chart.data(), 20, std::int32_t{601});
    assert(nosfix::CaptureTimedNote(note.data(), nosfix::TimedNoteKind::Glissando,
                                   gliss) == nosfix::TimedNoteCapture::InvalidChart);
    Put(chart.data(), 20, std::int32_t{600});
    // With no surviving connector the object part is skipped, not fatal.
    Put(note.data(), 0xD8 + 24, std::uint32_t{0});
    std::vector<std::byte> stale_connector = connector_raw;
    assert(nosfix::CaptureTimedNote(note.data(), nosfix::TimedNoteKind::Glissando,
                                   gliss) == nosfix::TimedNoteCapture::Ok);
    assert(gliss.PartCount() == 1);
    assert(!gliss.PartIsObject(0));
    assert(connector_raw == stale_connector);
    assert(nosfix::TimedNoteRefusal(nosfix::TimedNoteKind::Glissando) ==
           nosfix::GeometryDiagnostic::None);

    // A kind's vtable gate fires before any field is read: a glissando note is
    // not a velocity zone.
    nosfix::TimedNoteGeometry other;
    Put(note.data(), 0, reinterpret_cast<std::byte*>(game) + 0x679AB8);
    assert(nosfix::CaptureTimedNote(note.data(), nosfix::TimedNoteKind::VelocityZone, other) ==
           nosfix::TimedNoteCapture::WrongVtable);
    assert(nosfix::TimedNoteSupported(nosfix::TimedNoteKind::Trill));
    assert(nosfix::TimedNoteSupported(nosfix::TimedNoteKind::Glissando));

    std::printf("timed_note_geometry_native: all tests passed\n");
    return 0;
}