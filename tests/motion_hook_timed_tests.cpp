// Integration test: real module, real projection helper, synthetic note record.
// The update detours capture the note; the draw detour must then hand AFP a
// private chart-time rebuild instead of the game buffer. No game code other
// than the projection helper runs.
#ifdef NDEBUG
#undef NDEBUG
#endif
#include "nosfix/motion_hooks.hpp"
#include "nosfix/presentation.hpp"
#include "nosfix/timed_rendering.hpp"

#include <Windows.h>

#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>
#include <span>
#include <string>
#include <vector>

// Allocation probe: the caller cache must be prepared outside the hooks and
// then never allocate again, including when notes are erased and their slots
// are reused. It also proves that a failed cache preparation is contained.
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

using UpdateFn = char(__fastcall*)(void*);
using DrawFn = std::intptr_t(__fastcall*)(std::uint32_t, void*, void*, void*);

constexpr std::size_t kMusicFrameOffset = 0x70A610;
constexpr std::int32_t kStartMs = 10'001;
constexpr std::int32_t kEndMs = 11'001;
constexpr std::size_t kNoteBytes = 464;

std::vector<std::byte> g_module(kMusicFrameOffset + 8);
std::array<std::byte, kNoteBytes> g_note{};
std::array<std::byte, 104> g_chart{};
std::array<std::byte, 160> g_head{};
std::array<std::byte, 160> g_tail{};
std::array<std::byte, 120 * 12> g_ribbon{};
std::array<std::byte, 120 * 12> g_line{};
std::array<std::byte, 120 * 90> g_trill_ribbon{};
std::array<std::byte, 120 * 90> g_trill_glow{};
std::array<std::byte, 120 * 2> g_connector{};   // glissando connector object
std::array<std::array<std::byte, kNoteBytes>, 512> g_notes{};
unsigned g_update_calls = 0;
unsigned g_draw_calls = 0;
std::vector<std::byte> g_last_draw;
const std::byte* g_live_buffer = nullptr;
bool g_expect_private = false;

template <class T> void Store(void* base, std::size_t offset, const T& value) {
    std::memcpy(static_cast<std::byte*>(base) + offset, &value, sizeof(T));
}
template <class T> T Load(const void* base, std::size_t offset = 0) {
    T value{};
    std::memcpy(&value, static_cast<const std::byte*>(base) + offset, sizeof(T));
    return value;
}

char __fastcall FakeUpdate(void*) {
    ++g_update_calls;
    return 7;
}

std::intptr_t __fastcall FakeDraw(std::uint32_t type, void*, void*, void* opaque) {
    ++g_draw_calls;
    const auto count = Load<std::uint32_t>(opaque, 0);
    const auto* buffer = Load<const std::byte*>(opaque, 8);
    const std::size_t stride = ((type >> 1) & 0x3ff) == 2 ? 160u : 120u;
    g_last_draw.assign(buffer, buffer + static_cast<std::size_t>(count) * stride);
    if (g_live_buffer) {
        // A timed rebuild must be a private buffer; a fallback must be the
        // exact game-owned buffer with the exact live count.
        assert((buffer != g_live_buffer) == g_expect_private);
    }
    return 0x1234;
}

struct Hooks {
    UpdateFn long_note = nullptr;
    UpdateFn trill = nullptr;
    UpdateFn normal = nullptr;
    UpdateFn beats = nullptr;
    UpdateFn glissando = nullptr;
    UpdateFn velocity = nullptr;
    DrawFn draw = nullptr;
};

Hooks InstallFakeOriginals() {
    std::vector<nosfix::HookSpec> specs;
    nosfix::AppendMotionHooks(specs);
    Hooks hooks;
    for (const auto& spec : specs) {
        void* original = nullptr;
        const std::string name(spec.name);
        if (name == "long note geometry") {
            original = reinterpret_cast<void*>(&FakeUpdate);
            hooks.long_note = reinterpret_cast<UpdateFn>(spec.detour);
        } else if (name == "trill geometry") {
            original = reinterpret_cast<void*>(&FakeUpdate);
            hooks.trill = reinterpret_cast<UpdateFn>(spec.detour);
        } else if (name == "normal note geometry") {
            original = reinterpret_cast<void*>(&FakeUpdate);
            hooks.normal = reinterpret_cast<UpdateFn>(spec.detour);
        } else if (name == "beat line geometry") {
            original = reinterpret_cast<void*>(&FakeUpdate);
            hooks.beats = reinterpret_cast<UpdateFn>(spec.detour);
        } else if (name == "glissando geometry") {
            original = reinterpret_cast<void*>(&FakeUpdate);
            hooks.glissando = reinterpret_cast<UpdateFn>(spec.detour);
        } else if (name == "velocity zone geometry") {
            original = reinterpret_cast<void*>(&FakeUpdate);
            hooks.velocity = reinterpret_cast<UpdateFn>(spec.detour);
        } else if (name == "AFP primitive draw") {
            original = reinterpret_cast<void*>(&FakeDraw);
            hooks.draw = reinterpret_cast<DrawFn>(spec.detour);
        } else {
            original = reinterpret_cast<void*>(&FakeUpdate);
        }
        *spec.original = original;
    }
    assert(hooks.long_note && hooks.trill && hooks.normal && hooks.beats &&
           hooks.glissando && hooks.draw);
    return hooks;
}

} // namespace

// The motion hooks query the presentation rate; this test owns the value.
namespace nosfix {
unsigned PresentationRate() noexcept { return 120; }
} // namespace nosfix

namespace {

void FillTextured(std::array<std::byte, 160>& raw) {
    raw.fill(std::byte{0});
    for (std::size_t vertex = 0; vertex < 4; ++vertex) {
        Store(raw.data(), vertex * 24, static_cast<float>(vertex));
        Store(raw.data(), vertex * 24 + 4, static_cast<float>(vertex));
        Store(raw.data(), vertex * 24 + 12, 0xFF102030U);
    }
    Store(raw.data(), 144, std::uint32_t{0xD0000000});
}

void FillBox(std::span<std::byte> raw, std::uint32_t records) {
    std::memset(raw.data(), 0, static_cast<std::size_t>(records) * 120);
    for (std::uint32_t record = 0; record < records; ++record) {
        for (std::size_t vertex = 0; vertex < 6; ++vertex) {
            const std::size_t offset = record * 120 + vertex * 20;
            Store(raw.data(), offset, 0.0f);
            Store(raw.data(), offset + 4, 0.0f);
            Store(raw.data(), offset + 8, 0.0f);
            Store(raw.data(), offset + 12, 1.0f);
            Store(raw.data(), offset + 16, 0xFF00FF00U);
        }
    }
}

void AttachObject(void* note, std::size_t offset, std::uint32_t count,
                  const void* buffer) {
    Store(note, offset + 24, count);
    Store(note, offset + 32, buffer);
}

void Draw(const Hooks& hooks, const std::byte* buffer, std::uint32_t type,
          std::uint32_t count, bool expect_private) {
    struct Header {
        std::uint32_t count;
        std::uint32_t mode;
        const std::byte* buffer;
    };
    static_assert(sizeof(Header) == 16);
    Header header{count, 0xA5A5, buffer};
    g_live_buffer = buffer;
    g_expect_private = expect_private;
    assert(hooks.draw(type, nullptr, nullptr, &header) == 0x1234);
    g_live_buffer = nullptr;
}

void SetLongNoteHeader(std::byte* note, HMODULE game) {
    Store(note, 0, reinterpret_cast<std::byte*>(game) + 0x6841D0);
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

    Store(game, 0x1B72568, std::uint32_t{1});
    Store(game, 0x1B725AC, std::int32_t{20});
    Store(game, 0x1B725C4, std::int32_t{10});
    Store(game, 0x1B725C8, std::int32_t{0});
    Store(game, 0x13E4724, std::int32_t{3});
    Store(game, 0x13668D4, 2.0f);
    Store(game, 0x13E4758, 0.0f);
    Store(game, 0x13E476C, std::int32_t{40});
    Store(game, 0x70B300, 60.0f);
    Store(game, 0x70B2F8, 60.0);
    Store(game, 0x136690C, std::int32_t{0});
    Store(game, 0x1366918, std::int32_t{0});
    Store(game, 0x13668B0 + 0x7DEC0 + 4, std::int32_t{100});
    Store(game, 0x13668B0 + 0x7DEC0 + 8, std::int32_t{140});

    const Hooks hooks = InstallFakeOriginals();

    // A failed cache preparation must not throw through ConfigureMotion. The
    // update still runs the original and is counted as a timed-note failure.
    {
        const std::size_t before = g_allocations;
        g_fail_allocations = true;
        nosfix::ConfigureMotion(reinterpret_cast<HMODULE>(g_module.data()), true);
        g_fail_allocations = false;
        assert(g_allocations == before);
    }

    Store(g_chart.data(), 0, kStartMs);
    Store(g_chart.data(), 4, kEndMs);
    Store(g_chart.data(), 48, std::int32_t{0});
    Store(g_chart.data(), 97, std::uint8_t{0});
    Store(g_note.data(), 8, g_chart.data());
    Store(g_note.data(), 20, std::int32_t{0});
    Store(g_note.data(), 24, std::int32_t{600});
    Store(g_note.data(), 44, std::int32_t{1});
    Store(g_note.data(), 48, std::int32_t{2});
    Store(g_note.data(), 60, std::int32_t{60});
    Store(g_note.data(), 64, std::int32_t{1});
    Store(g_note.data(), 68, std::uint8_t{0});
    Store(g_note.data(), 240, std::int32_t{60});
    Store(g_note.data(), 244, std::int32_t{20});
    Store(g_note.data(), 272, std::int32_t{80});
    Store(g_note.data(), 276, std::int32_t{30});
    FillTextured(g_head);
    FillTextured(g_tail);
    FillBox(g_ribbon, 12);
    FillBox(g_line, 12);
    FillBox(g_trill_ribbon, 90);
    FillBox(g_trill_glow, 90);
    FillBox(g_connector, 2);
    AttachObject(g_note.data(), 0x80, 1, g_head.data());
    AttachObject(g_note.data(), 0xA8, 1, g_tail.data());
    AttachObject(g_note.data(), 0x160, 12, g_ribbon.data());
    AttachObject(g_note.data(), 0x198, 12, g_line.data());
    SetLongNoteHeader(g_note.data(), game);

    // Cache absent: fallback is counted, not thrown. Then prepare for real.
    Store(g_module.data(), kMusicFrameOffset, std::int32_t{600});
    assert(hooks.long_note(g_note.data()) == 7);
    auto metrics = nosfix::TakeMotionMetrics();
    assert(metrics.timed_note_failures >= 1);
    assert(metrics.timed_note_captures == 0);

    const std::size_t prepare_before = g_allocations;
    nosfix::ConfigureMotion(reinterpret_cast<HMODULE>(g_module.data()), true);
    assert(g_allocations > prepare_before);   // one explicit cache allocation
    assert(hooks.long_note(g_note.data()) == 7);
    metrics = nosfix::TakeMotionMetrics();
    assert(metrics.timed_note_captures == 1);

    const auto head_before = g_head;
    const auto ribbon_before = g_ribbon;
    const auto note_before = g_note;

    // Head quad at the exact chart millisecond: distance 0, centre (140, 700).
    nosfix::BeginMotionPresentation(0.0);
    nosfix::SetTimedSongMillisecondsForTest(static_cast<double>(kStartMs));
    Draw(hooks, g_head.data(), 4, 1, true);
    assert(g_last_draw.size() == 160);
    assert(Load<float>(g_last_draw.data(), 0) == 140.0f - 30.0f);
    assert(Load<float>(g_last_draw.data(), 4) == 700.0f - 10.0f);

    // The long tail cap uses its own +272/+276 extents (half width 40,
    // half height 15), not the head's +240/+244 words.
    Draw(hooks, g_tail.data(), 4, 1, true);
    assert(Load<float>(g_last_draw.data(), 0) == 140.0f - 40.0f);
    assert(Load<float>(g_last_draw.data(), 4) < 690.0f);

    nosfix::NoteProjection head_note{};
    assert(nosfix::ProjectGameNote(nullptr, 0, 0.0f, 1, 2, head_note));
    assert(head_note.x == 140.0f && head_note.y == 700.0f);

    Draw(hooks, g_ribbon.data(), 2, 12, true);
    assert(g_last_draw.size() >= 120);
    const auto ribbon_count = g_last_draw.size() / 120;
    assert(ribbon_count >= 1 && ribbon_count <= 37);
    assert(ribbon_count != 12);  // rebuilt, never an echo of the live records
    // Far end of the strip: x = lane centre 140 + shift 0 - half width 20.
    assert(Load<float>(g_last_draw.data(), 0) == 120.0f);
    assert(Load<float>(g_last_draw.data(), 4) >= -50.0f &&
           Load<float>(g_last_draw.data(), 4) <= -20.0f);

    Draw(hooks, g_line.data(), 2, 12, true);
    const auto line_count = g_last_draw.size() / 120;
    assert(line_count == ribbon_count);
    const float left = Load<float>(g_last_draw.data(), 0);
    const float right = Load<float>(g_last_draw.data(), 20);
    assert(std::abs((right - left) - 4.0f) < 1e-3f);

    // A different fractional song time must move the private geometry, while
    // the game buffers stay exactly as the simulation left them.
    std::vector<std::byte> at_start = g_last_draw;
    nosfix::SetTimedSongMillisecondsForTest(static_cast<double>(kStartMs) - 10.0);
    Draw(hooks, g_line.data(), 2, 12, true);
    assert(g_last_draw != at_start);
    assert(g_head == head_before);
    assert(g_ribbon == ribbon_before);
    assert(g_note == note_before);

    // Caller cache: fill every slot under a failing allocator, observe the
    // capacity failure, erase half through the unsupported beats path and
    // reuse those slots without touching the heap.
    for (auto& note : g_notes) note = g_note;
    nosfix::SetTimedSongMillisecondsForTest(static_cast<double>(kStartMs));
    (void)nosfix::TakeMotionMetrics();
    {
        const std::size_t before = g_allocations;
        g_fail_allocations = true;
        for (auto& note : g_notes) {
            assert(hooks.long_note(note.data()) == 7);
        }
        std::array<std::byte, kNoteBytes> overflow = g_note;
        assert(hooks.long_note(overflow.data()) == 7);
        g_fail_allocations = false;
        assert(g_allocations == before);
    }
    metrics = nosfix::TakeMotionMetrics();
    // g_note already occupied one slot; 511 of the 512 new keys fit and the
    // 512th plus the overflow probe are counted as capacity failures.
    assert(metrics.timed_note_captures == 511);
    assert(metrics.timed_note_failures == 2);

    (void)nosfix::TakeMotionMetrics();
    {
        const std::size_t before = g_allocations;
        g_fail_allocations = true;
        for (std::size_t index = 0; index < 256; ++index) {
            Store(g_notes[index].data(), 0,
                  reinterpret_cast<std::byte*>(game) + 0x679960); // BeatsNotes
            assert(hooks.beats(g_notes[index].data()) == 7);
        }
        for (std::size_t index = 0; index < 256; ++index) {
            SetLongNoteHeader(g_notes[index].data(), game);
            assert(hooks.long_note(g_notes[index].data()) == 7);
        }
        g_fail_allocations = false;
        assert(g_allocations == before);
    }
    metrics = nosfix::TakeMotionMetrics();
    assert(metrics.timed_note_captures == 256);
    assert(metrics.timed_note_failures == 0);
    assert(metrics.unsupported_notes >= 256);

    // Retire path: three idle ticks clear every key but free no storage; the
    // cleared slots are then reused by a new burst with a failing allocator.
    (void)nosfix::TakeMotionMetrics();
    for (int tick = 0; tick < 3; ++tick) nosfix::RetireMotionTick();
    assert(hooks.long_note(g_note.data()) == 7);   // re-warm the track map
    const std::size_t retire_before = g_allocations;
    g_fail_allocations = true;
    for (std::size_t index = 0; index < 100; ++index) {
        assert(hooks.long_note(g_notes[index].data()) == 7);
    }
    g_fail_allocations = false;
    assert(g_allocations == retire_before);
    metrics = nosfix::TakeMotionMetrics();
    assert(metrics.timed_note_captures == 101);
    assert(metrics.timed_note_failures == 0);

    // Trill: verified 0x1A84F0 layout. Two 90-record strips, step approach/80,
    // per-part tail extents +272/+276 and a ten-pixel glow edge.
    (void)nosfix::TakeMotionMetrics();
    Store(g_note.data(), 0, reinterpret_cast<std::byte*>(game) + 0x679C10); // TrillNotes
    AttachObject(g_note.data(), 0x170, 90, g_trill_ribbon.data());
    AttachObject(g_note.data(), 0x1A8, 90, g_trill_glow.data());
    assert(hooks.trill(g_note.data()) == 7);

    Draw(hooks, g_head.data(), 4, 1, true);
    assert(Load<float>(g_last_draw.data(), 0) == 140.0f - 30.0f);
    assert(Load<float>(g_last_draw.data(), 4) == 690.0f);
    Draw(hooks, g_tail.data(), 4, 1, true);
    assert(Load<float>(g_last_draw.data(), 0) == 140.0f - 40.0f);
    assert(Load<float>(g_last_draw.data(), 4) == -65.0f);

    const auto trill_head_before = g_head;
    const auto trill_ribbon_before = g_trill_ribbon;
    const auto trill_glow_before = g_trill_glow;
    Draw(hooks, g_trill_ribbon.data(), 2, 12, true);
    assert(g_last_draw.size() == 80 * 120);
    assert(Load<float>(g_last_draw.data(), 0) == 140.0f - 15.0f);
    assert(Load<float>(g_last_draw.data(), 20) == 140.0f + 15.0f);
    assert(Load<float>(g_last_draw.data(), 40) == 140.0f - 20.0f);

    Draw(hooks, g_trill_glow.data(), 2, 12, true);
    assert(g_last_draw.size() == 80 * 120);
    assert(Load<float>(g_last_draw.data(), 0) == 140.0f - 15.0f);
    assert(Load<float>(g_last_draw.data(), 20) == 140.0f - 5.0f);
    assert(Load<float>(g_last_draw.data(), 40) == 140.0f - 20.0f);

    std::vector<std::byte> trill_at_start = g_last_draw;
    nosfix::SetTimedSongMillisecondsForTest(static_cast<double>(kStartMs) - 0.25);
    Draw(hooks, g_trill_glow.data(), 2, 12, true);
    assert(g_last_draw != trill_at_start);
    assert(g_head == trill_head_before);
    assert(g_trill_ribbon == trill_ribbon_before);
    assert(g_trill_glow == trill_glow_before);

    // Glissando is captured now: the record's own anchor, the distance-zero quad
    // and the connector object as a whole-object part. The connector has no
    // corners here, so its draw keeps the original payload, not a prediction.
    nosfix::SetTimedSongMillisecondsForTest(static_cast<double>(kStartMs));
    Store(g_chart.data(), 20, std::int32_t{600});   // frame of kStartMs
    Store(g_note.data(), 196, std::int32_t{60});
    Store(g_note.data(), 200, std::int32_t{20});
    AttachObject(g_note.data(), 0xD8, 2, g_connector.data());
    Store(g_note.data(), 0, reinterpret_cast<std::byte*>(game) + 0x679AB8);
    (void)nosfix::TakeMotionMetrics();   // count only this kind's own work
    assert(hooks.glissando(g_note.data()) == 7);
    metrics = nosfix::TakeMotionMetrics();
    assert(metrics.timed_note_captures == 1);
    assert(metrics.unsupported_notes == 0);
    assert(metrics.unsupported_glissando == 0);
    assert(metrics.unsupported_beats == 0);
    assert(metrics.unsupported_velocity_zones == 0);
    assert(metrics.object_part_builds == 0);

    // The quad sits on the judgement line (x 140, y 700) at 60 x 20, so its top
    // left corner is (110, 690) whatever the clock says.
    Draw(hooks, g_head.data(), 4, 1, true);
    assert(g_last_draw.size() == 160);
    assert(Load<float>(g_last_draw.data(), 0) == 140.0f - 30.0f);
    assert(Load<float>(g_last_draw.data(), 4) == 700.0f - 10.0f);
    const auto gliss_quad = g_last_draw;
    nosfix::SetTimedSongMillisecondsForTest(static_cast<double>(kStartMs) - 250.0);
    Draw(hooks, g_head.data(), 4, 1, true);
    assert(g_last_draw == gliss_quad);
    nosfix::SetTimedSongMillisecondsForTest(static_cast<double>(kStartMs));

    // The connector object must never be predicted: it falls back to the exact
    // game buffer with its live count, and the refusal is counted by name.
    Draw(hooks, g_connector.data(), 2, 2, false);
    assert(g_last_draw.size() == 2 * 120);
    metrics = nosfix::TakeMotionMetrics();
    assert(metrics.fallback_draws == 1);
    assert(metrics.timed_draws == 2);
    assert(metrics.predicted_draws == 0);
    assert(metrics.geometry_diagnostics[static_cast<std::size_t>(
               nosfix::GeometryDiagnostic::ObjectQuadsRequired)] == 1);
    assert(nosfix::TimedNoteRefusal(nosfix::TimedNoteKind::Glissando) ==
           nosfix::GeometryDiagnostic::None);
    assert(nosfix::TimedNoteRefusal(nosfix::TimedNoteKind::VelocityZone) ==
           nosfix::GeometryDiagnostic::VelocityAnchorUnproven);
    assert(nosfix::TimedNoteRefusal(nosfix::TimedNoteKind::Beats) ==
           nosfix::GeometryDiagnostic::ChartTimeAbsent);
    assert(nosfix::TimedNoteRefusal(nosfix::TimedNoteKind::Long) ==
           nosfix::GeometryDiagnostic::None);
    assert(nosfix::TimedNoteSupported(nosfix::TimedNoteKind::Glissando));
    assert(!nosfix::TimedNoteSupported(nosfix::TimedNoteKind::Beats));
    assert(!nosfix::TimedNoteSupported(nosfix::TimedNoteKind::VelocityZone));
    // A record whose two time fields disagree is refused, not anchored.
    Store(g_chart.data(), 20, std::int32_t{601});
    assert(hooks.glissando(g_note.data()) == 7);
    metrics = nosfix::TakeMotionMetrics();
    assert(metrics.timed_note_captures == 0);
    assert(metrics.timed_note_failures == 1);
    Store(g_chart.data(), 20, std::int32_t{600});
    // note+196/+200 are the glissando quad extents, and in this synthetic note
    // they overlap the long-note tail object header at 0xA8 (its buffer pointer
    // lives at 0xC8). The native note has no tail object there, so put that
    // object back before the next kind reads it.
    AttachObject(g_note.data(), 0xA8, 1, g_tail.data());

    // A beats note is counted by name as well.
    Store(g_note.data(), 0, reinterpret_cast<std::byte*>(game) + 0x679960);
    assert(hooks.beats(g_note.data()) == 7);
    Store(g_note.data(), 0, reinterpret_cast<std::byte*>(game) + 0x679D68);
    assert(hooks.velocity(g_note.data()) == 7);
    metrics = nosfix::TakeMotionMetrics();
    assert(metrics.unsupported_beats == 1);
    assert(metrics.unsupported_velocity_zones == 1);
    assert(metrics.unsupported_notes == 2);
    assert(metrics.geometry_diagnostics[static_cast<std::size_t>(
               nosfix::GeometryDiagnostic::ChartTimeAbsent)] == 1);
    assert(metrics.geometry_diagnostics[static_cast<std::size_t>(
               nosfix::GeometryDiagnostic::VelocityAnchorUnproven)] == 1);

    std::printf("motion_hook_timed: all tests passed\n");
    return 0;
}
