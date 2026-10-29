#include "nosfix/motion_hooks.hpp"

#include "nosfix/diagnostics.hpp"
#include "nosfix/presentation.hpp"
#include "nosfix/primitive_motion.hpp"
#include "nosfix/timed_rendering.hpp"

#include <array>
#include <atomic>
#include <cstddef>
#include <cstring>
#include <memory>
#include <unordered_map>

namespace nosfix {
namespace {

using NoteUpdate = char(__fastcall*)(void*);
using PrimitiveDraw = std::intptr_t(__fastcall*)(std::uint32_t, void*, void*, void*);
using BufferFree = std::intptr_t(__fastcall*)(void*);

NoteUpdate g_normal_update = nullptr;
NoteUpdate g_long_update = nullptr;
NoteUpdate g_beats_update = nullptr;
NoteUpdate g_glissando_update = nullptr;
NoteUpdate g_trill_update = nullptr;
NoteUpdate g_velocity_update = nullptr;
PrimitiveDraw g_draw = nullptr;
BufferFree g_free = nullptr;
const std::byte* g_base = nullptr;
std::atomic<bool> g_enabled{false};
bool g_predict = true;

// The AFP callback receives a copied BM2D payload, not a C++ primitive object.
struct DrawPayload {
    std::uint32_t count;
    std::uint32_t mode;
    const std::byte* buffer;
};
static_assert(sizeof(DrawPayload) == 16);
static_assert(offsetof(DrawPayload, buffer) == 8);

template<class T>
T Read(const void* pointer, std::size_t offset) noexcept {
    T value{};
    std::memcpy(&value, static_cast<const std::byte*>(pointer) + offset, sizeof(T));
    return value;
}

struct Owner {
    const void* note;
    const void* chart_record;
    std::int32_t frame;
    std::int32_t lane;
    std::int32_t left_key;
    std::int32_t right_key;
    std::int32_t hold_state;
    friend bool operator==(const Owner&, const Owner&) = default;
};

struct Track {
    PrimitiveMotion geometry;
    Owner owner{};
    GeometryKey key{};
    std::uint64_t tick = UINT64_MAX;
    std::int32_t music_frame = 0;
    // Set when this buffer belongs to a note with an evidenced chart-time
    // rebuild. `timed_note` is a map key only and is never dereferenced.
    const void* timed_note = nullptr;
    std::uint32_t timed_part = 0;
    NotePartShape timed_shape = NotePartShape::TexturedQuad;
};

// One captured note geometry per note object. Rebuilt from chart milliseconds
// at draw time; the source buffers are private copies, never game pointers.
struct TimedNote {
    TimedNoteGeometry geometry;
    std::uint64_t tick = UINT64_MAX;
};

// Bounded, preallocated note cache. Storage is acquired once in
// ConfigureMotion (outside every game callback) and then only ever reused:
// erase clears the key, insert takes the first free slot, and neither the hash
// bookkeeping nor the geometry buffers allocate. A failed prepare leaves the
// cache null, so the timed path is skipped instead of throwing through a hook.
constexpr std::size_t kMaximumTimedNotes = 512;

struct TimedNoteSlot {
    const void* note = nullptr;
    TimedNote timed;
};

struct TimedNoteCache {
    std::array<TimedNoteSlot, kMaximumTimedNotes> slots{};
};

// The analyzed update and AFP draw callback run synchronously on the game
// thread. Another thread gets an empty cache and passes straight through.
// Only a currently live draw payload is dereferenced; cached game addresses
// are lookup keys, never pointers that we later walk or write through.
struct MotionState {
    std::unordered_map<const void*, Track> tracks;
    std::unique_ptr<TimedNoteCache> timed_cache;
    std::uint64_t tick = 0;
    double phase = 0;
    bool presenting = false;
    bool drawing_copy = false;
    MotionMetrics metrics;
};
thread_local MotionState g_motion;
constexpr std::size_t kMaximumTracks = 4096;

struct NoteLayout {
    std::array<std::size_t, 2> textured;
    std::array<std::size_t, 2> ribbons;
    std::size_t hold_state;
};
constexpr NoteLayout kNormal{{128, 0}, {0, 0}, 0};
constexpr NoteLayout kLong{{128, 168}, {352, 408}, 300};
constexpr NoteLayout kBeats{{128, 0}, {0, 0}, 0};
constexpr NoteLayout kGlissando{{128, 0}, {216, 0}, 0};
constexpr NoteLayout kTrill{{128, 168}, {368, 424}, 316};
constexpr NoteLayout kVelocity{{128, 168}, {352, 0}, 300};

bool Active() noexcept {
    return g_enabled.load(std::memory_order_acquire) &&
        (TimedRenderingEnabled() || PresentationRate() > 60);
}

TimedNoteKind KindOf(const NoteLayout& layout) noexcept {
    if (&layout == &kLong) return TimedNoteKind::Long;
    if (&layout == &kVelocity) return TimedNoteKind::VelocityZone;
    if (&layout == &kGlissando) return TimedNoteKind::Glissando;
    if (&layout == &kTrill) return TimedNoteKind::Trill;
    if (&layout == &kBeats) return TimedNoteKind::Beats;
    return TimedNoteKind::Normal;
}

// All cache operations are fixed-capacity scans. They neither allocate nor
// depend on any unordered_map node, so a failed or retired note can never
// make a later capture allocate on the native update thread.
TimedNote* FindTimedNote(const void* note) noexcept {
    if (!g_motion.timed_cache || !note) return nullptr;
    for (auto& slot : g_motion.timed_cache->slots) {
        if (slot.note == note) return &slot.timed;
    }
    return nullptr;
}

TimedNote* InsertTimedNote(const void* note) noexcept {
    if (!g_motion.timed_cache || !note) return nullptr;
    for (auto& slot : g_motion.timed_cache->slots) {
        if (slot.note) continue;
        slot.note = note;
        slot.timed.geometry.Reset();
        slot.timed.tick = UINT64_MAX;
        return &slot.timed;
    }
    return nullptr;
}

void EraseTimedNote(const void* note) noexcept {
    if (!g_motion.timed_cache || !note) return;
    for (auto& slot : g_motion.timed_cache->slots) {
        if (slot.note != note) continue;
        slot.note = nullptr;
        slot.timed.geometry.Reset();
        slot.timed.tick = UINT64_MAX;
        return;
    }
}

// Explicit initialization outside every hook. The 23 MiB cache is allocated
// once by ConfigureMotion; a failure is contained and permanently disables the
// timed path for this thread rather than risking an exception in a detour.
bool PrepareTimedNoteCache() noexcept {
    if (g_motion.timed_cache) return true;
    try {
        g_motion.timed_cache = std::make_unique<TimedNoteCache>();
    } catch (...) {
        g_motion.timed_cache.reset();
        return false;
    }
    return true;
}

// Captures the whole note once and binds each of its sub-buffers to the matching
// part index. Unsupported kinds keep the original path and are counted.
void CaptureTimedNoteFor(void* note, const NoteLayout& layout) noexcept {
    if (!TimedRenderingEnabled()) return;   // timed path off: no bookkeeping
    const TimedNoteKind kind = KindOf(layout);
    if (!TimedNoteSupported(kind)) {
        // Count the kind by name and record the exact missing evidence, so a
        // fallback is never anonymous.
        ++g_motion.metrics.unsupported_notes;
        switch (kind) {
        case TimedNoteKind::Glissando: ++g_motion.metrics.unsupported_glissando; break;
        case TimedNoteKind::VelocityZone:
            ++g_motion.metrics.unsupported_velocity_zones;
            break;
        case TimedNoteKind::Beats: ++g_motion.metrics.unsupported_beats; break;
        default: break;
        }
        const auto refusal = static_cast<std::size_t>(TimedNoteRefusal(kind));
        if (refusal < std::size(g_motion.metrics.geometry_diagnostics)) {
            ++g_motion.metrics.geometry_diagnostics[refusal];
        }
        EraseTimedNote(note);
        return;
    }
    TimedNote* timed = FindTimedNote(note);
    if (!timed) {
        timed = InsertTimedNote(note);
        if (!timed) {
            ++g_motion.metrics.timed_note_failures;
            return;
        }
    }
    const TimedNoteCapture status = CaptureTimedNote(note, kind, timed->geometry);
    if (status != TimedNoteCapture::Ok) {
        ++g_motion.metrics.timed_note_failures;
        EraseTimedNote(note);
        return;
    }
    ++g_motion.metrics.timed_note_captures;
    timed->tick = g_motion.tick;
    for (std::size_t part = 0; part < timed->geometry.PartCount(); ++part) {
        const void* source = timed->geometry.PartSource(part);
        if (!source) continue;
        const auto track = g_motion.tracks.find(source);
        if (track == g_motion.tracks.end()) continue;
        track->second.timed_note = note;
        track->second.timed_part = static_cast<std::uint32_t>(part);
        track->second.timed_shape = timed->geometry.PartShape(part);
    }
}

void CapturePrimitive(const Owner& owner, std::size_t offset,
                      PrimitiveKind kind, std::int32_t music_frame) {
    const auto* object = static_cast<const std::byte*>(owner.note) + offset;
    const auto count = Read<std::uint32_t>(object, 24);
    const auto* buffer = Read<const std::byte*>(object, 32);
    if (!buffer) return;
    if (count == 0 || count > TimedNoteGeometry::kMaximumRecords) {
        g_motion.tracks.erase(buffer);
        return;
    }
    auto it = g_motion.tracks.find(buffer);
    if (it == g_motion.tracks.end()) {
        if (g_motion.tracks.size() >= kMaximumTracks) return;
        it = g_motion.tracks.try_emplace(buffer).first;
    }
    auto& track = it->second;
    // One chart-frame step per legacy iteration is the only velocity sample
    // we accept. Audio holds, seeks, double updates and note reuse reset it.
    const bool discontinuity = track.owner != owner || track.tick == g_motion.tick ||
        static_cast<std::int64_t>(music_frame) - track.music_frame != 1;
    const GeometryKey key{reinterpret_cast<std::uintptr_t>(owner.note),
                          static_cast<std::uint32_t>(offset)};
    const auto stride = kind == PrimitiveKind::Textured ? 160u : 120u;
    if (track.geometry.Capture(key, g_motion.tick, kind, count,
            {buffer, static_cast<std::size_t>(count) * stride}, discontinuity)) {
        track.owner = owner;
        track.key = key;
        track.tick = g_motion.tick;
        track.music_frame = music_frame;
        ++g_motion.metrics.captures;
    } else {
        g_motion.tracks.erase(it);
    }
}

void CaptureNote(void* note, const NoteLayout& layout, std::int32_t music_frame) noexcept {
    try {
        const Owner owner{note, Read<const void*>(note, 8),
            Read<std::int32_t>(note, 24), Read<std::int32_t>(note, 20),
            Read<std::int32_t>(note, 44), Read<std::int32_t>(note, 48),
            layout.hold_state ? Read<std::int32_t>(note, layout.hold_state) : 0};
        for (const auto offset : layout.textured)
            if (offset) CapturePrimitive(owner, offset, PrimitiveKind::Textured, music_frame);
        for (const auto offset : layout.ribbons)
            if (offset) CapturePrimitive(owner, offset, PrimitiveKind::Ribbon, music_frame);
        CaptureTimedNoteFor(note, layout);
    } catch (...) {
        // Allocation failure in the legacy prediction map only disables
        // prediction for this iteration. No exceptions may escape a game
        // callback. The preallocated timed cache keeps its buffers; only the
        // keys are dropped so no stale binding survives a partial capture.
        g_motion.tracks.clear();
        if (g_motion.timed_cache) {
            for (auto& slot : g_motion.timed_cache->slots) slot.note = nullptr;
        }
    }
}

char UpdateNote(void* note, NoteUpdate original, const NoteLayout& layout) {
    const auto music_frame = Read<std::int32_t>(g_base, 0x70A610);
    if (!Active()) {
        const char result = original(note);
        if (DiagnosticProbeEnabled()) {
            ProbeMotionNote(note, static_cast<int>(KindOf(layout)), music_frame);
        }
        return result;
    }
    const char result = original(note); // Exactly one original update.
    CaptureNote(note, layout, music_frame);
    if (DiagnosticProbeEnabled()) {
        ProbeMotionNote(note, static_cast<int>(KindOf(layout)), music_frame);
    }
    return result;
}

char __fastcall NormalUpdate(void* note) { return UpdateNote(note, g_normal_update, kNormal); }
char __fastcall LongUpdate(void* note) { return UpdateNote(note, g_long_update, kLong); }
char __fastcall BeatsUpdate(void* note) { return UpdateNote(note, g_beats_update, kBeats); }
char __fastcall GlissandoUpdate(void* note) { return UpdateNote(note, g_glissando_update, kGlissando); }
char __fastcall TrillUpdate(void* note) { return UpdateNote(note, g_trill_update, kTrill); }
char __fastcall VelocityUpdate(void* note) { return UpdateNote(note, g_velocity_update, kVelocity); }

std::intptr_t __fastcall DrawPrimitive(std::uint32_t type, void* transform,
                                     void* state, void* opaque) {
    if (!Active() || !g_motion.presenting || g_motion.drawing_copy || !opaque)
        return g_draw(type, transform, state, opaque);
    const auto payload = Read<DrawPayload>(opaque, 0);
    const auto it = g_motion.tracks.find(payload.buffer);
    if (it == g_motion.tracks.end()) return g_draw(type, transform, state, opaque);
    const auto primitive_type = (type >> 1) & 0x3ff;
    if ((primitive_type != 1 && primitive_type != 2) ||
        payload.count == 0 || payload.count > TimedNoteGeometry::kMaximumRecords)
        return g_draw(type, transform, state, opaque);

    auto& track = it->second;
    const auto kind = static_cast<PrimitiveKind>(primitive_type);
    const auto stride = kind == PrimitiveKind::Textured ? 160u : 120u;
    const std::span<const std::byte> live{payload.buffer,
                                        static_cast<std::size_t>(payload.count) * stride};
    std::span<const std::byte> copy;
    bool timed = false;
    bool allow_prediction = true;
    std::uint32_t replacement_count = payload.count;
    if (track.timed_note && TimedRenderingEnabled() &&
        g_motion.tick - track.tick <= 2) {
        // Chart-time rebuild for the evidenced layouts. The part is rebuilt
        // from song milliseconds only; nothing is extrapolated from pixels.
        const auto song_ms = RenderSongMilliseconds();
        const bool shape_ok =
            (kind == PrimitiveKind::Textured) ==
            (track.timed_shape == NotePartShape::TexturedQuad);
        TimedNote* owner = FindTimedNote(track.timed_note);
        if (!shape_ok) {
            ++g_motion.metrics.geometry_diagnostics[
                static_cast<std::size_t>(GeometryDiagnostic::LayoutUnverified)];
        } else if (song_ms && owner) {
            NotePartView view{};
            GeometryDiagnostic diagnostic = GeometryDiagnostic::None;
            if (owner->geometry.BuildPart(track.timed_part, *song_ms,
                                          ProjectGameNote, nullptr, view,
                                          diagnostic) &&
                view.data && view.count > 0) {
                const std::size_t copy_stride = stride;
                copy = {view.data, static_cast<std::size_t>(view.count) * copy_stride};
                timed = true;
                replacement_count = view.count;
            } else {
                const auto index = static_cast<std::size_t>(diagnostic);
                if (index < std::size(g_motion.metrics.geometry_diagnostics)) {
                    ++g_motion.metrics.geometry_diagnostics[index];
                }
                // A whole-object part owns a record count that two independent
                // connectors produce; a predicted payload would substitute a
                // different count, so fall back to the original draw instead.
                if (diagnostic == GeometryDiagnostic::ObjectQuadsRequired ||
                    diagnostic == GeometryDiagnostic::EmptyObject) {
                    allow_prediction = false;
                }
            }
        }
    }
    if (copy.empty() && g_predict && allow_prediction) {
        copy = track.geometry.DrawCopy(track.key, g_motion.tick, g_motion.phase,
                                       kind, payload.count, live);
    }
    if (copy.empty()) {
        ++g_motion.metrics.fallback_draws;
        return g_draw(type, transform, state, opaque);
    }
    DrawPayload replacement{replacement_count, payload.mode, copy.data()};
    struct DrawScope {
        DrawScope() { g_motion.drawing_copy = true; }
        ~DrawScope() { g_motion.drawing_copy = false; }
    } scope;
    if (timed) ++g_motion.metrics.timed_draws;
    else ++g_motion.metrics.predicted_draws;
    return g_draw(type, transform, state, &replacement);
}

std::intptr_t __fastcall FreeBuffer(void* buffer) {
    // The original is a tail call to a virtual allocator, so callers already
    // obey the full Win64 volatile-register contract at this boundary.
    if (g_enabled.load(std::memory_order_acquire)) g_motion.tracks.erase(buffer);
    return g_free(buffer);
}

constexpr std::array<unsigned char, 24> kNormalBytes{
    0x48,0x89,0x5c,0x24,0x20,0x55,0x56,0x41,0x54,0x41,0x56,0x41,
    0x57,0x48,0x8d,0x6c,0x24,0xe0,0x48,0x81,0xec,0x20,0x01,0x00};
constexpr std::array<unsigned char, 24> kLongBytes{
    0x40,0x55,0x56,0x41,0x54,0x41,0x57,0x48,0x8d,0xac,0x24,0xd8,
    0xfe,0xff,0xff,0x48,0x81,0xec,0x28,0x02,0x00,0x00,0x48,0x8b};
constexpr std::array<unsigned char, 24> kDrawBytes{
    0x48,0x89,0x5c,0x24,0x10,0x48,0x89,0x74,0x24,0x18,0x57,0x48,
    0x83,0xec,0x20,0x48,0x8b,0xf2,0x89,0x4c,0x24,0x30,0xba,0x01};
constexpr std::array<unsigned char, 24> kBeatsBytes{
    0x48,0x8b,0xc4,0x48,0x89,0x78,0x20,0x55,0x41,0x56,0x41,0x57,
    0x48,0x8d,0x68,0xb8,0x48,0x81,0xec,0x30,0x01,0x00,0x00,0x0f};
constexpr std::array<unsigned char, 24> kGlissandoBytes{
    0x48,0x89,0x5c,0x24,0x20,0x55,0x56,0x41,0x54,0x41,0x56,0x41,
    0x57,0x48,0x8d,0xac,0x24,0x70,0xff,0xff,0xff,0x48,0x81,0xec};
constexpr std::array<unsigned char, 24> kTrillBytes{
    0x40,0x55,0x56,0x41,0x54,0x41,0x56,0x48,0x8d,0xac,0x24,0x98,
    0xfe,0xff,0xff,0x48,0x81,0xec,0x68,0x02,0x00,0x00,0x48,0x8b};
constexpr std::array<unsigned char, 24> kVelocityBytes{
    0x4c,0x8b,0xdc,0x55,0x56,0x49,0x8d,0xab,0x68,0xfe,0xff,0xff,
    0x48,0x81,0xec,0x88,0x02,0x00,0x00,0x48,0x8b,0x05,0x46,0x0d};
constexpr std::array<unsigned char, 17> kFreeBytes{
    0x48,0x8b,0xd1,0x48,0x8b,0x0d,0xb6,0x07,0x67,0x00,0x48,0x8b,
    0x01,0x48,0xff,0x60,0x10};

} // namespace

void AppendMotionHooks(std::vector<HookSpec>& specs) {
    specs.push_back({"normal note geometry", 0x2F6DF0, kNormalBytes,
        reinterpret_cast<void*>(&NormalUpdate), reinterpret_cast<void**>(&g_normal_update)});
    specs.push_back({"long note geometry", 0x2F4E80, kLongBytes,
        reinterpret_cast<void*>(&LongUpdate), reinterpret_cast<void**>(&g_long_update)});
    specs.push_back({"beat line geometry", 0x1A5990, kBeatsBytes,
        reinterpret_cast<void*>(&BeatsUpdate), reinterpret_cast<void**>(&g_beats_update)});
    specs.push_back({"glissando geometry", 0x1A6820, kGlissandoBytes,
        reinterpret_cast<void*>(&GlissandoUpdate), reinterpret_cast<void**>(&g_glissando_update)});
    specs.push_back({"trill geometry", 0x1A84F0, kTrillBytes,
        reinterpret_cast<void*>(&TrillUpdate), reinterpret_cast<void**>(&g_trill_update)});
    specs.push_back({"velocity zone geometry", 0x1AA4D0, kVelocityBytes,
        reinterpret_cast<void*>(&VelocityUpdate), reinterpret_cast<void**>(&g_velocity_update)});
    specs.push_back({"AFP primitive draw", 0xDE7C0, kDrawBytes,
        reinterpret_cast<void*>(&DrawPrimitive), reinterpret_cast<void**>(&g_draw)});
    specs.push_back({"BM2D buffer release", 0xD61C0, kFreeBytes,
        reinterpret_cast<void*>(&FreeBuffer), reinterpret_cast<void**>(&g_free)});
}

void ConfigureMotion(HMODULE module, bool enabled) noexcept {
    g_base = reinterpret_cast<const std::byte*>(module);
    g_predict = enabled;
    g_enabled.store(enabled || TimedRenderingEnabled(), std::memory_order_release);
    if (TimedRenderingEnabled()) {
        PrepareTimedNoteCache();   // explicit allocation outside every hook
    } else {
        g_motion.timed_cache.reset();
    }
    if (!enabled) g_motion.tracks.clear();
}

void BeginMotionPresentation(double phase) noexcept {
    g_motion.phase = phase;
    g_motion.presenting = true;
}

void EndMotionPresentation() noexcept { g_motion.presenting = false; }

void RetireMotionTick() noexcept {
    ++g_motion.tick;
    for (auto it = g_motion.tracks.begin(); it != g_motion.tracks.end();) {
        if (g_motion.tick - it->second.tick > 2) it = g_motion.tracks.erase(it);
        else ++it;
    }
    if (g_motion.timed_cache) {
        for (auto& slot : g_motion.timed_cache->slots) {
            if (slot.note && g_motion.tick - slot.timed.tick > 2) {
                // Free the key only. The preallocated buffers stay in the slot
                // for the next note; no destructor or deallocation runs here.
                slot.note = nullptr;
                slot.timed.geometry.Reset();
                slot.timed.tick = UINT64_MAX;
            }
        }
    }
}

MotionMetrics TakeMotionMetrics() noexcept {
    const auto result = g_motion.metrics;
    g_motion.metrics = {};
    return result;
}

} // namespace nosfix
