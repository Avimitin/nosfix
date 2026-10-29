#include "nosfix/timed_rendering.hpp"
#include "nosfix/module_guard.hpp"
#include "nosfix/shadow_clock.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstring>

namespace nosfix {
namespace {
using Project = float* (__fastcall*)(int, float, int*, int*, float*, float*,
                                      float*, float*, int, int, int*, int*);
using Instance = void* (__fastcall*)();
using Elapsed = float(__fastcall*)(void*, const char*);
const std::byte* g_base = nullptr;
Project g_project = nullptr;
Instance g_instance = nullptr;
Elapsed g_elapsed = nullptr;
std::atomic<bool> g_enabled{false};
bool g_prepared = false;
ShadowClock g_shadow_clock;
thread_local std::optional<double> g_song_ms;

template<class T> T Read(const void* base, std::size_t offset) noexcept {
    T value{};
    std::memcpy(&value, static_cast<const std::byte*>(base) + offset, sizeof(T));
    return value;
}

constexpr std::array<unsigned char, 24> kProjectBytes{
    0x48,0x89,0x5C,0x24,0x08,0x57,0x48,0x81,0xEC,0x80,0,0,0,0x0F,0x29,
    0x7C,0x24,0x60,0x49,0x8B,0xD8,0x44,0x0F,0x29};
constexpr std::array<unsigned char, 24> kInstanceBytes{
    0x48,0x83,0xEC,0x28,0x8B,0x05,0xD6,0xBF,0x63,0x09,0xA8,0x01,
    0x75,0x15,0x83,0xC8,0x01,0x48,0x8D,0x0D,0xB8,0x03,0x01,0x00};
constexpr std::array<unsigned char, 24> kElapsedBytes{
    0x48,0x83,0xEC,0x28,0x83,0xB9,0x1C,0xE0,0x08,0x00,0x02,0x75,
    0x33,0x48,0x85,0xD2,0x74,0x2E,0x48,0x8B,0xCA,0xE8,0x46,0xEC};

// Note vtables located by scanning the module for the verified update entries
// (one reference each): 0x684328 normal, 0x6841D0 long, 0x679960 beats,
// 0x679AB8 glissando, 0x679C10 trill, 0x679D68 velocity zone.
constexpr std::array<std::uintptr_t, 6> kNoteVtable{
    0x684328, 0x6841D0, 0x679960, 0x679AB8, 0x679C10, 0x679D68};

struct PartSpec {
    std::size_t object_offset;
    NotePartShape shape;
    NoteAnchorRule anchors;
    std::size_t width_offset;
    std::size_t height_offset;
    bool optional;
};

// Object offsets are the verified per-type primitive fields: +128 head texture,
// +168 tail texture, +352 wide ribbon, +408 four-pixel hold line, +368 trill
// tremolo strip and +424 trill edge strip. Head/tail dimensions come from the
// exact words each update multiplies: +196/+200 normal, +240/+244 long/trill
// head, +272/+276 long/trill tail.
constexpr std::array<PartSpec, 1> kNormalParts{{
    {0x80, NotePartShape::TexturedQuad, NoteAnchorRule::Start, 196, 200, false},
}};
constexpr std::array<PartSpec, 4> kHoldParts{{
    {0x80, NotePartShape::TexturedQuad, NoteAnchorRule::Start, 240, 244, false},
    {0xA8, NotePartShape::TexturedQuad, NoteAnchorRule::End, 272, 276, true},
    {0x160, NotePartShape::BoxRibbon, NoteAnchorRule::StartToEnd, 0, 0, false},
    {0x198, NotePartShape::BoxLine, NoteAnchorRule::StartToEnd, 0, 0, false},
}};
constexpr std::array<PartSpec, 4> kTrillParts{{
    {0x80, NotePartShape::TexturedQuad, NoteAnchorRule::Start, 240, 244, false},
    {0xA8, NotePartShape::TexturedQuad, NoteAnchorRule::End, 272, 276, true},
    {0x170, NotePartShape::TrillRibbon, NoteAnchorRule::StartToEnd, 0, 0, false},
    {0x1A8, NotePartShape::TrillGlow, NoteAnchorRule::StartToEnd, 0, 0, false},
}};

// Glissando: the note's own textured quad at note+0x80, projected at distance
// zero (0x1801A6993..0x1801A6A12), plus the connector box object at note+216
// (0xD8), which receives one quad per surviving link (chart+0x40, chart+0x48).
// With neither link that object holds no record and draws nothing, so the object
// part is optional; the quad is the note itself and is not.
constexpr std::array<PartSpec, 2> kGlissParts{{
    {0x80, NotePartShape::TexturedQuad, NoteAnchorRule::Fixed, 196, 200, false},
    {0xD8, NotePartShape::BoxObject, NoteAnchorRule::None, 0, 0, true},
}};

bool ValidRate(double rate) noexcept {
    return std::isfinite(rate) && rate >= 59.0 && rate <= 61.0;
}
bool ValidKeys(int lane, int left, int right) noexcept {
    return lane >= 0 && lane < 28 && left >= 0 && right >= left && right <= 28;
}

// Shared static-layout derivation from the live note and chart records.
TimedNoteCapture BuildNoteSpec(void* note, TimedNoteKind kind,
                               NoteGeometrySpec& spec) noexcept {
    if (Read<std::uint8_t>(note, 68) != 0) return TimedNoteCapture::Judged;
    const auto* chart = Read<const std::byte*>(note, 8);
    if (!chart || Read<std::int32_t>(chart, 48) == 2) return TimedNoteCapture::Judged;

    spec = {};
    spec.start_ms = Read<std::int32_t>(chart, 0);
    if (kind == TimedNoteKind::Glissando) {
        // A glissando note's anchor is the record's own chart millisecond field,
        // and the same record stores its truncated frame at +0x14. Both must
        // agree before either can anchor a connector, so a foreign or
        // half-written record is refused instead of trusted. The loader
        // multiplies by the double 60.0 at 0x18070B2F8 and divides by 1000.0
        // (0x180687218) before truncating (0x1801A4945).
        double anchor = 0.0;
        if (!ChartRecordAnchorMilliseconds(Read<std::int32_t>(chart, 0),
                                           Read<std::int32_t>(chart, 0x14),
                                           Read<double>(g_base, 0x70B2F8), anchor)) {
            return TimedNoteCapture::InvalidChart;
        }
        spec.start_ms = anchor;
    }
    // Only long notes and trills have an evidenced second chart time. A
    // glissando uses its own record anchor above, a velocity zone has none
    // (TimedNoteRefusal), and beats never read chart time at all.
    const bool hold = kind == TimedNoteKind::Long || kind == TimedNoteKind::Trill;
    spec.has_end = hold;
    if (hold) {
        spec.end_ms = Read<std::int32_t>(chart, 4);
        if (!(spec.end_ms > spec.start_ms)) return TimedNoteCapture::InvalidChart;
    }
    spec.legacy_rate = Read<float>(g_base, 0x70B300);
    if (!ValidRate(spec.legacy_rate)) return TimedNoteCapture::InvalidChart;
    spec.lane = Read<std::int32_t>(note, 20);
    spec.left_key = Read<std::int32_t>(note, 44);
    spec.right_key = Read<std::int32_t>(note, 48);
    if (!ValidKeys(spec.lane, spec.left_key, spec.right_key)) {
        return TimedNoteCapture::InvalidChart;
    }
    spec.scroll_mode = Read<std::int32_t>(g_base, 0x13E4724);
    if (spec.scroll_mode < 0 || spec.scroll_mode > 5) {
        return TimedNoteCapture::LayoutUnsupported;
    }
    const auto timing_offset =
        static_cast<std::int64_t>(Read<std::int32_t>(g_base, 0x136690C)) +
        Read<std::int32_t>(g_base, 0x1366918);
    spec.visual_offset_ms = static_cast<float>(static_cast<double>(timing_offset) * 1000.0 /
                                               spec.legacy_rate);

    const int width_keys = Read<std::int32_t>(note, 64);
    if (width_keys < 0 || width_keys > 28) return TimedNoteCapture::InvalidChart;
    int shrink = 0;
    const int wide_offset = Read<std::int32_t>(g_base, 0x13E476C);
    if (Read<std::uint8_t>(g_base, 0x13E5721) == 0) {
        shrink = width_keys < 3 ? 0 : (width_keys == 3 ? -1 : -2);
        shrink = std::max(shrink, std::min(0, Read<std::int32_t>(g_base, 0x1B725C8)));
    }
    const int divisor = shrink < 0 && width_keys > 4 && width_keys + shrink <= 4 ? 4 : 2;
    // Width/height live at different offsets per note type: +196/+200 for
    // normal and glissando (verified in both updates), +240/+244 for long notes
    // (0x2F4E80 reads a1[60]/a1[61]); the long object's +200 word belongs to
    // the tail texture object, so it must never be read as a height.
    const std::size_t width_offset =
        (kind == TimedNoteKind::Normal || kind == TimedNoteKind::Glissando) ? 196 : 240;
    const std::size_t height_offset = width_offset + 4;
    const int width = Read<std::int32_t>(note, width_offset);
    const int height = Read<std::int32_t>(note, height_offset);
    if (width <= 0 || width > 4096 || height <= 0 || height > 4096) {
        return TimedNoteCapture::InvalidChart;
    }
    spec.quad_half_width = static_cast<float>(width / divisor);
    spec.quad_half_height = static_cast<float>(height / 2) *
        (static_cast<float>(Read<std::int32_t>(g_base, 0x1B725C4)) / 10.0f);
    spec.quad_width_divisor = divisor;
    spec.quad_height_scale =
        static_cast<float>(Read<std::int32_t>(g_base, 0x1B725C4)) / 10.0f;
    if (!(spec.quad_height_scale > 0.0f) || !std::isfinite(spec.quad_height_scale)) {
        return TimedNoteCapture::LayoutUnsupported;
    }
    if (width_keys >= 3 && shrink == -1) spec.x_shift = static_cast<float>(wide_offset / -2);
    spec.opacity_scale = Read<std::uint8_t>(chart, 97) ? 0.25f : 1.0f;

    // Native long/trill ribbon width: (shrink * wide / 2 - 10 + note+60 / 2).
    // The +60 word is the note's own pixel width; velocity zones use the same
    // raster with `note+60 / 2 - 10` and no shrink/wide term (0x1801AAF31),
    // while the glissando connectors use note+208 with a -6 margin instead.
    // RibbonHalfWidth() is the single source of truth for both rules.
    const int ribbon_width = Read<std::int32_t>(note, 60);
    if (ribbon_width < 0 || ribbon_width > 4096) return TimedNoteCapture::InvalidChart;
    const int ribbon_offset = shrink * wide_offset / 2;
    float ribbon_base = 0.0f;
    if (!RibbonHalfWidth(RibbonWidthRuleFor(kind), ribbon_width, shrink,
                         wide_offset, ribbon_base)) {
        // Normal notes have no box ribbon; keep the reviewed long/trill value
        // so the spec stays byte-identical for every supported kind.
        ribbon_base = static_cast<float>(ribbon_offset - 10 + ribbon_width / 2);
    }
    spec.ribbon_base_width = ribbon_base;
    spec.ribbon_line_half_width = 2.0f;
    spec.approach = Read<float>(g_base, 0x13668D4);
    spec.ribbon_start_bias = Read<float>(g_base, 0x13E4758);
    // The long update divides the approach distance by the constant at
    // 0x18068726C (30.0) and emits at most 37 segments; the trill path uses
    // 0x180687298 (80.0) and emits at most 90 segments per object.
    if (kind == TimedNoteKind::Trill) {
        spec.ribbon_step_divisor = 80.0f;
        spec.max_ribbon_segments = 90;
    } else {
        spec.ribbon_step_divisor = 30.0f;
        spec.max_ribbon_segments = 37;
    }
    if (!(spec.approach > 0.0f) || !std::isfinite(spec.ribbon_start_bias)) {
        return TimedNoteCapture::LayoutUnsupported;
    }
    return TimedNoteCapture::Ok;
}

bool ReadObject(const void* note, std::size_t offset, std::uint32_t& count,
                const std::byte*& buffer) noexcept {
    count = Read<std::uint32_t>(note, offset + 24);
    buffer = Read<const std::byte*>(note, offset + 32);
    return buffer != nullptr && count > 0 &&
        count <= TimedNoteGeometry::kMaximumRecords;
}

// Per-part half extents from the exact words each update multiplies. A zero
// width/height means the native update emits no quad for that part; the caller
// decides whether the part is optional.
bool ReadHalfExtents(const void* note, std::size_t width_offset,
                     std::size_t height_offset, const NoteGeometrySpec& spec,
                     float& half_width, float& half_height) noexcept {
    const int width = Read<std::int32_t>(note, width_offset);
    const int height = Read<std::int32_t>(note, height_offset);
    if (width < 0 || width > 4096 || height < 0 || height > 4096) return false;
    if (width == 0 || height == 0) {
        half_width = 0.0f;
        half_height = 0.0f;
        return true;
    }
    half_width = static_cast<float>(width / spec.quad_width_divisor);
    half_height = static_cast<float>(height / 2) * spec.quad_height_scale;
    return std::isfinite(half_width) && std::isfinite(half_height) &&
        half_width > 0.0f && half_height > 0.0f;
}

} // namespace

const char* TimedNoteCaptureName(TimedNoteCapture value) noexcept {
    switch (value) {
    case TimedNoteCapture::Ok: return "ok";
    case TimedNoteCapture::Disabled: return "disabled";
    case TimedNoteCapture::WrongVtable: return "wrong-vtable";
    case TimedNoteCapture::Judged: return "judged";
    case TimedNoteCapture::InvalidChart: return "invalid-chart";
    case TimedNoteCapture::LayoutUnsupported: return "layout-unsupported";
    case TimedNoteCapture::MissingPart: return "missing-part";
    case TimedNoteCapture::InvalidPart: return "invalid-part";
    }
    return "unknown";
}

bool TimedNoteSupported(TimedNoteKind kind) noexcept {
    // Normal, long/hold, trill and glissando are evidenced end to end.
    // Glissando's anchor is now proven: a 104-byte record stores its own
    // chart milliseconds at +0x00 and their truncated frame at +0x14, the note
    // quad is projected at distance zero, and both connectors take their far
    // ends from the link frame (GlissLinkDistance) and the +0x50 clip. A
    // velocity zone still has no chart-derived end anchor and beats take their
    // position from the live frame counter, so both keep the original path.
    return kind == TimedNoteKind::Normal || kind == TimedNoteKind::Long ||
        kind == TimedNoteKind::Trill || kind == TimedNoteKind::Glissando;
}

RibbonWidthRule RibbonWidthRuleFor(TimedNoteKind kind) noexcept {
    switch (kind) {
    case TimedNoteKind::Normal:
    case TimedNoteKind::Beats:
    case TimedNoteKind::Glissando:
        return RibbonWidthRule::None;
    case TimedNoteKind::Long:
    case TimedNoteKind::Trill:
        return RibbonWidthRule::HeadAndTail;
    case TimedNoteKind::VelocityZone:
        return RibbonWidthRule::VelocityZone;
    }
    return RibbonWidthRule::None;
}

bool RibbonHalfWidth(RibbonWidthRule rule, int note_width, int shrink,
                     int wide_offset, float& half_width) noexcept {
    if (note_width < 0 || note_width > 4096) return false;
    int value = 0;
    switch (rule) {
    case RibbonWidthRule::HeadAndTail:
        if (shrink < -2 || shrink > 0) return false;
        if (wide_offset < -65536 || wide_offset > 65536) return false;
        // Native integer arithmetic: shrink*wide/2 truncates toward zero, then
        // -10, then + note+60/2. Same expression in 0x2F4E80 / 0x1A84F0.
        value = shrink * wide_offset / 2 - 10 + note_width / 2;
        break;
    case RibbonWidthRule::VelocityZone:
        // 0x1801AAF31: the velocity-zone raster has no shrink or wide term.
        value = note_width / 2 - 10;
        break;
    case RibbonWidthRule::None:
    default:
        return false;
    }
    half_width = static_cast<float>(value);
    return std::isfinite(half_width);
}

GeometryDiagnostic TimedNoteRefusal(TimedNoteKind kind) noexcept {
    switch (kind) {
    case TimedNoteKind::Normal:
    case TimedNoteKind::Long:
    case TimedNoteKind::Trill:
        return GeometryDiagnostic::None;
    case TimedNoteKind::Glissando:
        // Closed in round 2: the record's +0x00 milliseconds and its +0x14 frame
        // agree by construction (ChartRecordAnchorMilliseconds), so the anchor is
        // exact and the kind is rebuilt. Connector corners still come from the
        // caller (BuildObjectPart), never from an invented layout.
        return GeometryDiagnostic::None;
    case TimedNoteKind::VelocityZone:
        return GeometryDiagnostic::VelocityAnchorUnproven;
    case TimedNoteKind::Beats:
        return GeometryDiagnostic::ChartTimeAbsent;
    }
    return GeometryDiagnostic::LayoutUnverified;
}

bool PrepareTimedRendering(HMODULE game, std::string& reason) {
    if (!game || TimedRenderingEnabled()) {
        reason = "timed renderer requires an inactive, verified game module";
        return false;
    }
    ModuleIdentity nostalgia, xact;
    std::wstring why;
    if (!VerifyNostalgiaModule(nostalgia, why) || nostalgia.module != game ||
        !VerifyLibxactModule(xact, why)) {
        reason.clear();
        for (const wchar_t c : why) reason.push_back(c <= 127 ? static_cast<char>(c) : '?');
        if (reason.empty()) reason = "timed renderer received a different game module";
        return false;
    }
    const auto* base = reinterpret_cast<const std::byte*>(game);
    const auto* xbase = reinterpret_cast<const std::byte*>(xact.module);
    if (std::memcmp(base + 0x305430, kProjectBytes.data(), kProjectBytes.size()) ||
        std::memcmp(xbase + 0xA970, kInstanceBytes.data(), kInstanceBytes.size()) ||
        std::memcmp(xbase + 0x7230, kElapsedBytes.data(), kElapsedBytes.size())) {
        reason = "timed renderer helper bytes do not match the analyzed build";
        return false;
    }
    g_base = base;
    g_project = reinterpret_cast<Project>(base + 0x305430);
    g_instance = reinterpret_cast<Instance>(xbase + 0xA970);
    g_elapsed = reinterpret_cast<Elapsed>(xbase + 0x7230);
    g_prepared = true;
    return true;
}

void ConfigureTimedRendering(bool enabled) noexcept {
    g_enabled.store(enabled && g_prepared && g_base && g_project && g_instance &&
                        g_elapsed,
                    std::memory_order_release);
    g_song_ms.reset();
}

bool TimedRenderingEnabled() noexcept { return g_enabled.load(std::memory_order_acquire); }

void ConfigureSongClock(bool enabled) noexcept {
    g_shadow_clock.Configure(enabled && g_prepared);
}

bool SongClockEnabled() noexcept { return g_shadow_clock.Enabled(); }

const SongClockPublication* SongClockPublicationForInput() noexcept {
    return g_shadow_clock.Publication();
}

void BeginTimedRendering(std::int64_t qpc_ticks,
                         std::int64_t qpc_frequency) noexcept {
    g_song_ms.reset();
    if (!g_prepared) return;
    // Preroll and special clock correction states are not yet migrated.
    // Do not invent a song anchor from a legacy frame in these cases.
    if (Read<std::int32_t>(g_base, 0x70A610) <= 0 ||
        Read<std::int32_t>(g_base, 0x70A750) > 0) return;
    const double rate = Read<double>(g_base, 0x70B2F8);
    if (!ValidRate(rate)) return;
    const auto* label = reinterpret_cast<const char*>(g_base + 0x70A618);
    if (!label[0] || !std::memchr(label, 0, 256)) return;
    void* instance = g_instance();
    if (!instance) return;
    // Public cue query contains the corrected QPC difference and audio check.
    // It does not dispatch notes. A missing cue yields no sample.
    const double elapsed_ms = g_elapsed(instance, label);
    if (!std::isfinite(elapsed_ms) || elapsed_ms < 0.0) return;
    const double adjustment = Read<std::int32_t>(g_base, 0x70A740) * 1000.0 / rate;
    const double song_ms = std::max(0.0, elapsed_ms + adjustment);
    if (TimedRenderingEnabled()) g_song_ms = song_ms;
    if (g_shadow_clock.Enabled() && qpc_ticks > 0 && qpc_frequency > 0) {
        (void)g_shadow_clock.Observe(song_ms * 1000.0, qpc_ticks, qpc_frequency);
    }
}

void EndTimedRendering() noexcept { g_song_ms.reset(); }
std::optional<double> RenderSongMilliseconds() noexcept { return g_song_ms; }

void SetTimedSongMillisecondsForTest(double ms) noexcept {
    if (std::isfinite(ms) && ms >= 0.0) g_song_ms = ms;
    else g_song_ms.reset();
}

bool ProjectGameNote(void*, int lane, float distance, int left, int right,
                     NoteProjection& result) noexcept {
    if (!TimedRenderingEnabled() || !ValidKeys(lane, left, right) ||
        !std::isfinite(distance) || std::abs(distance) > 10'000.0f ||
        (Read<std::uint32_t>(g_base, 0x1B72568) & 1) == 0 ||
        Read<std::int32_t>(g_base, 0x13E4724) < 0 ||
        Read<std::int32_t>(g_base, 0x13E4724) > 5) return false;
    const float approach = Read<float>(g_base, 0x13668D4);
    if (!std::isfinite(approach) || approach <= 0.0f) return false;
    int x = 0, y = 0;
    float scale = 0, opacity = 0;
    g_project(lane, distance, &x, &y, nullptr, &scale, nullptr, &opacity,
              left, right, nullptr, nullptr);
    result = {static_cast<float>(x), static_cast<float>(y), scale, opacity};
    return true;
}

bool CaptureTimedNormal(void* note, std::span<const std::byte> raw,
                        TimedNormal& target) noexcept {
    target.Reset();
    if (!TimedRenderingEnabled() || !note ||
        Read<const void*>(note, 0) != g_base + kNoteVtable[0] ||
        (Read<std::uint32_t>(g_base, 0x1B72568) & 1) == 0) return false;
    NoteGeometrySpec spec{};
    if (BuildNoteSpec(note, TimedNoteKind::Normal, spec) != TimedNoteCapture::Ok) {
        return false;
    }
    TimedNormalSpec normal;
    normal.target_ms = spec.start_ms;
    normal.visual_offset_ms = spec.visual_offset_ms;
    normal.legacy_units_per_second = spec.legacy_rate;
    normal.lane = spec.lane;
    normal.left_key = spec.left_key;
    normal.right_key = spec.right_key;
    normal.half_width = spec.quad_half_width;
    normal.half_height = spec.quad_half_height;
    normal.x_shift = spec.x_shift;
    normal.opacity = spec.opacity_scale;
    return target.Capture(normal, raw);
}

TimedNoteCapture CaptureTimedNote(void* note, TimedNoteKind kind,
                                  TimedNoteGeometry& target) noexcept {
    target.Reset();
    if (!TimedRenderingEnabled() || !note || !g_base) return TimedNoteCapture::Disabled;
    const auto index = static_cast<std::size_t>(kind);
    if (index >= kNoteVtable.size()) return TimedNoteCapture::WrongVtable;
    if (Read<const void*>(note, 0) != g_base + kNoteVtable[index]) {
        return TimedNoteCapture::WrongVtable;
    }
    if ((Read<std::uint32_t>(g_base, 0x1B72568) & 1) == 0) {
        return TimedNoteCapture::Disabled;
    }
    if (!TimedNoteSupported(kind)) return TimedNoteCapture::LayoutUnsupported;

    NoteGeometrySpec spec{};
    const TimedNoteCapture status = BuildNoteSpec(note, kind, spec);
    if (status != TimedNoteCapture::Ok) return status;

    std::span<const PartSpec> parts_spec;
    if (kind == TimedNoteKind::Normal) parts_spec = kNormalParts;
    else if (kind == TimedNoteKind::Trill) parts_spec = kTrillParts;
    else if (kind == TimedNoteKind::Glissando) parts_spec = kGlissParts;
    else parts_spec = kHoldParts;
    std::array<NotePartInput, TimedNoteGeometry::kMaxParts> parts{};
    std::size_t count = 0;
    for (const auto& entry : parts_spec) {
        std::uint32_t records = 0;
        const std::byte* buffer = nullptr;
        if (!ReadObject(note, entry.object_offset, records, buffer)) {
            // A glissando connector object with no surviving link holds no
            // record at all and draws nothing, so an optional part is skipped
            // instead of failing the whole note.
            if (entry.optional) continue;
            return TimedNoteCapture::MissingPart;
        }
        const std::size_t stride = entry.shape == NotePartShape::TexturedQuad
            ? TimedNoteGeometry::kTexturedStride
            : TimedNoteGeometry::kBoxStride;
        float half_width = 0.0f;
        float half_height = 0.0f;
        if (entry.shape == NotePartShape::TexturedQuad) {
            if (!ReadHalfExtents(note, entry.width_offset, entry.height_offset, spec,
                                 half_width, half_height)) {
                return TimedNoteCapture::InvalidChart;
            }
            if (half_width <= 0.0f || half_height <= 0.0f) {
                // The native update simply does not draw this cap.
                if (entry.optional) continue;
                return TimedNoteCapture::InvalidChart;
            }
        }
        parts[count++] = {entry.shape, entry.anchors, records,
                          {buffer, static_cast<std::size_t>(records) * stride}, buffer,
                          half_width, half_height};
    }
    if (!target.Capture(spec, {parts.data(), count})) {
        return TimedNoteCapture::InvalidPart;
    }
    return TimedNoteCapture::Ok;
}

} // namespace nosfix