#include "nosfix/motion_hooks.hpp"
#include "nosfix/presentation.hpp"

#include <Windows.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <span>
#include <string>
#include <vector>

namespace {

void CheckImpl(bool condition, const char* expression, int line) {
    if (!condition) {
        std::fprintf(stderr, "motion hook test failed at line %d: %s\n", line, expression);
        std::abort();
    }
}
#define Check(condition) CheckImpl((condition), #condition, __LINE__)

constexpr std::size_t kMusicFrameOffset = 0x70A610;
using UpdateFn = char(__fastcall*)(void*);
using DrawFn = std::intptr_t(__fastcall*)(std::uint32_t, void*, void*, void*);
using FreeFn = std::intptr_t(__fastcall*)(void*);

std::vector<std::byte> g_module(kMusicFrameOffset + sizeof(std::uint32_t));
std::vector<std::byte> g_textured(160);
std::vector<std::byte> g_ribbon(120);
std::array<std::byte, 472> g_note{};
std::array<std::byte, 472> g_reused_note{};
std::uint64_t g_tick = 0;
unsigned g_rate = 120;
unsigned g_update_calls = 0;
unsigned g_draw_calls = 0;
unsigned g_free_calls = 0;
std::vector<std::byte> g_last_draw;
const std::byte* g_expected_live_buffer = nullptr;
std::size_t g_expected_live_size = 0;
bool g_expect_predicted = false;

template<class T>
void Store(void* base, std::size_t offset, const T& value) {
    std::memcpy(static_cast<std::byte*>(base) + offset, &value, sizeof(T));
}

template<class T>
T Load(const void* base, std::size_t offset = 0) {
    T value{};
    std::memcpy(&value, static_cast<const std::byte*>(base) + offset, sizeof(T));
    return value;
}

bool Near(float actual, float expected) {
    return std::abs(actual - expected) < 0.001f;
}

char __fastcall FakeUpdate(void*) {
    ++g_update_calls;
    return 7;
}

std::intptr_t __fastcall FakeDraw(std::uint32_t type, void*, void*, void* opaque) {
    ++g_draw_calls;
    const auto count = Load<std::uint32_t>(opaque, 0);
    const auto* buffer = Load<const std::byte*>(opaque, 8);
    const auto primitive = (type >> 1) & 0x3ff;
    const std::size_t stride = primitive == 2 ? 160 : 120;
    g_last_draw.assign(buffer, buffer + static_cast<std::size_t>(count) * stride);

    if (g_expected_live_buffer) {
        Check(g_expected_live_size == static_cast<std::size_t>(count) * stride);
        // A predicted draw must receive a private buffer. A fallback must
        // receive exactly the game-owned buffer.
        Check((buffer != g_expected_live_buffer) == g_expect_predicted);
    }
    return 0x1234;
}

std::intptr_t __fastcall FakeFree(void*) {
    ++g_free_calls;
    return 0x5678;
}

void SetMusicFrame(std::int32_t frame) {
    Store(g_module.data(), kMusicFrameOffset, frame);
}

void SetPointer(void* base, std::size_t offset, const void* pointer) {
    Store(base, offset, pointer);
}

void SetCountAndBuffer(void* note, std::size_t offset, std::uint32_t count,
                       const void* buffer) {
    Store(note, offset + 24, count);
    SetPointer(note, offset + 32, buffer);
}

void SetTextured(std::vector<std::byte>& raw, float x, float y,
                 std::uint32_t flags = 0xD0000000,
                 std::uint64_t texture = 0x123456789abcdef0ull,
                 std::uint8_t uv_seed = 0x30) {
    raw.assign(160, std::byte{0});
    for (std::size_t vertex = 0; vertex < 4; ++vertex) {
        const auto offset = vertex * 24;
        Store(raw.data(), offset, x + static_cast<float>(vertex));
        Store(raw.data(), offset + 4, y + static_cast<float>(vertex * 2));
        Store(raw.data(), offset + 12, static_cast<std::uint32_t>(0x10203040u + vertex));
        for (std::size_t i = 16; i < 24; ++i)
            raw[offset + i] = static_cast<std::byte>(uv_seed + vertex + i);
    }
    Store(raw.data(), 144, flags);
    Store(raw.data(), 152, texture);
}

void SetRibbon(std::vector<std::byte>& raw, float x, float y) {
    raw.assign(120, std::byte{0});
    for (std::size_t vertex = 0; vertex < 6; ++vertex) {
        const auto offset = vertex * 20;
        Store(raw.data(), offset, x + static_cast<float>(vertex));
        Store(raw.data(), offset + 4, y - static_cast<float>(vertex));
        Store(raw.data(), offset + 8, 9.5f + static_cast<float>(vertex));
        Store(raw.data(), offset + 12, 0.75f);
        Store(raw.data(), offset + 16, static_cast<std::uint32_t>(0xff102030u + vertex));
    }
}

void InitNote(void* note, const void* chart_record, std::int32_t frame,
              std::int32_t lane = 3) {
    std::memset(note, 0, 472);
    SetPointer(note, 8, chart_record);
    Store(note, 20, lane);
    Store(note, 24, frame);
    Store(note, 44, 5);
    Store(note, 48, 6);
    Store(note, 300, 1);
}

struct Hooks {
    UpdateFn normal = nullptr;
    UpdateFn long_note = nullptr;
    UpdateFn beats = nullptr;
    UpdateFn glissando = nullptr;
    UpdateFn trill = nullptr;
    UpdateFn velocity = nullptr;
    DrawFn draw = nullptr;
    FreeFn free_buffer = nullptr;
};

Hooks InstallFakeOriginals() {
    std::vector<nosfix::HookSpec> specs;
    nosfix::AppendMotionHooks(specs);
    Hooks hooks;
    for (const auto& spec : specs) {
        void* original = nullptr;
        if (std::string(spec.name) == "normal note geometry") {
            original = reinterpret_cast<void*>(&FakeUpdate);
            hooks.normal = reinterpret_cast<UpdateFn>(spec.detour);
        } else if (std::string(spec.name) == "long note geometry") {
            original = reinterpret_cast<void*>(&FakeUpdate);
            hooks.long_note = reinterpret_cast<UpdateFn>(spec.detour);
        } else if (std::string(spec.name) == "beat line geometry") {
            original = reinterpret_cast<void*>(&FakeUpdate);
            hooks.beats = reinterpret_cast<UpdateFn>(spec.detour);
        } else if (std::string(spec.name) == "glissando geometry") {
            original = reinterpret_cast<void*>(&FakeUpdate);
            hooks.glissando = reinterpret_cast<UpdateFn>(spec.detour);
        } else if (std::string(spec.name) == "trill geometry") {
            original = reinterpret_cast<void*>(&FakeUpdate);
            hooks.trill = reinterpret_cast<UpdateFn>(spec.detour);
        } else if (std::string(spec.name) == "velocity zone geometry") {
            original = reinterpret_cast<void*>(&FakeUpdate);
            hooks.velocity = reinterpret_cast<UpdateFn>(spec.detour);
        } else if (std::string(spec.name) == "AFP primitive draw") {
            original = reinterpret_cast<void*>(&FakeDraw);
            hooks.draw = reinterpret_cast<DrawFn>(spec.detour);
        } else if (std::string(spec.name) == "BM2D buffer release") {
            original = reinterpret_cast<void*>(&FakeFree);
            hooks.free_buffer = reinterpret_cast<FreeFn>(spec.detour);
        }
        Check(original != nullptr && spec.original != nullptr);
        *spec.original = original;
    }
    Check(hooks.normal && hooks.long_note && hooks.beats && hooks.glissando &&
          hooks.trill && hooks.velocity && hooks.draw && hooks.free_buffer);
    return hooks;
}

void UpdateUsing(UpdateFn update, void* note, std::int32_t music_frame) {
    Check(update != nullptr);
    SetMusicFrame(music_frame);
    const auto before = g_update_calls;
    const auto result = update(note);
    Check(result == 7);
    Check(g_update_calls == before + 1); // Detour calls simulation exactly once.
}

void Update(const Hooks& hooks, void* note, std::int32_t music_frame,
            bool long_note = false) {
    UpdateUsing(long_note ? hooks.long_note : hooks.normal, note, music_frame);
}

void BeginDraw(double alpha) {
    nosfix::BeginMotionPresentation(alpha);
}

void Draw(const Hooks& hooks, const std::byte* buffer, std::uint32_t type,
          std::uint32_t count = 1, std::uint32_t mode = 0xA5A5) {
    struct Header { std::uint32_t count, mode; const std::byte* buffer; };
    static_assert(sizeof(Header) == 16);
    Header header{count, mode, buffer};
    g_expected_live_buffer = buffer;
    g_expected_live_size = static_cast<std::size_t>(count) *
        (((type >> 1) & 0x3ff) == 2 ? 160 : 120);
    const auto result = hooks.draw(type, nullptr, nullptr, &header);
    Check(result == 0x1234);
    g_expected_live_buffer = nullptr;
}

void Retire() {
    nosfix::RetireMotionTick();
    ++g_tick;
}

void ResetTestState(const Hooks& hooks, unsigned rate = 120) {
    (void)hooks;
    nosfix::EndMotionPresentation();
    g_rate = rate;
    nosfix::ConfigureMotion(reinterpret_cast<HMODULE>(g_module.data()), false);
    nosfix::ConfigureMotion(reinterpret_cast<HMODULE>(g_module.data()), true);
    (void)nosfix::TakeMotionMetrics();
    g_last_draw.clear();
    g_expected_live_buffer = nullptr;
    g_update_calls = g_draw_calls = g_free_calls = 0;
}

void AttachNormalTextured(void* note, std::vector<std::byte>& buffer) {
    SetCountAndBuffer(note, 128, 1, buffer.data());
}

void TestTexturedPredictionAndPreservation(const Hooks& hooks) {
    ResetTestState(hooks);
    InitNote(g_note.data(), reinterpret_cast<void*>(0x1110), 100);
    SetTextured(g_textured, 10, 20);
    AttachNormalTextured(g_note.data(), g_textured);
    const auto first_raw = g_textured;
    Update(hooks, g_note.data(), 100);
    Retire();

    SetTextured(g_textured, 20, 30, 0xD0000000, 0x123456789abcdef0ull, 0x30);
    const auto latest_raw = g_textured;
    Update(hooks, g_note.data(), 101);
    Check(g_update_calls == 2);

    BeginDraw(0.5);
    g_expect_predicted = true;
    Draw(hooks, g_textured.data(), 4);
    Check(g_last_draw.size() == 160);
    Check(Near(Load<float>(g_last_draw.data(), 0), 25));
    Check(Near(Load<float>(g_last_draw.data(), 4), 35));
    Check(Load<std::uint32_t>(g_last_draw.data(), 12) ==
          Load<std::uint32_t>(latest_raw.data(), 12));
    Check(std::memcmp(g_last_draw.data() + 16, latest_raw.data() + 16, 8) == 0);
    Check(Load<std::uint32_t>(g_last_draw.data(), 144) == 0xD0000000);
    Check(Load<std::uint64_t>(g_last_draw.data(), 152) == 0x123456789abcdef0ull);

    // Repeated renders always derive from the immutable latest capture.
    for (unsigned i = 0; i < 12; ++i) {
        Draw(hooks, g_textured.data(), 4);
        Check(Near(Load<float>(g_last_draw.data(), 0), 25));
    }
    Check(g_textured == latest_raw);
    Check(g_textured != first_raw);
    Check(g_update_calls == 2);
    nosfix::EndMotionPresentation();
    g_expect_predicted = false;
}

void TestRibbonPrediction(const Hooks& hooks) {
    ResetTestState(hooks);
    InitNote(g_note.data(), reinterpret_cast<void*>(0x2220), 200);
    SetRibbon(g_ribbon, 0, 5);
    SetCountAndBuffer(g_note.data(), 352, 1, g_ribbon.data());
    Update(hooks, g_note.data(), 200, true);
    Retire();
    SetRibbon(g_ribbon, 8, 9);
    const auto latest = g_ribbon;
    Update(hooks, g_note.data(), 201, true);
    BeginDraw(0.25);
    g_expect_predicted = true;
    Draw(hooks, g_ribbon.data(), 2);
    Check(Near(Load<float>(g_last_draw.data(), 0), 10));
    Check(Near(Load<float>(g_last_draw.data(), 4), 10));
    Check(Load<float>(g_last_draw.data(), 8) == Load<float>(latest.data(), 8));
    Check(Load<float>(g_last_draw.data(), 12) == Load<float>(latest.data(), 12));
    Check(Load<std::uint32_t>(g_last_draw.data(), 16) ==
          Load<std::uint32_t>(latest.data(), 16));
    Check(g_ribbon == latest);
    nosfix::EndMotionPresentation();
    g_expect_predicted = false;
}

void TestPauseSeekDoubleUpdateAndReuse(const Hooks& hooks) {
    ResetTestState(hooks);
    InitNote(g_note.data(), reinterpret_cast<void*>(0x3330), 300);
    SetTextured(g_textured, 1, 2);
    AttachNormalTextured(g_note.data(), g_textured);
    Update(hooks, g_note.data(), 300);
    Retire();
    SetTextured(g_textured, 2, 3);
    Update(hooks, g_note.data(), 301);
    Retire();

    // Same tick update and a seek each replace the current snapshot while
    // clearing velocity history.
    SetTextured(g_textured, 3, 4);
    Update(hooks, g_note.data(), 302);
    SetTextured(g_textured, 4, 5);
    Update(hooks, g_note.data(), 302);
    BeginDraw(0.5);
    g_expect_predicted = false;
    Draw(hooks, g_textured.data(), 4);
    Check(Near(Load<float>(g_last_draw.data(), 0), 4));
    nosfix::EndMotionPresentation();

    Retire();
    SetTextured(g_textured, 5, 6);
    Update(hooks, g_note.data(), 1000); // seek discontinuity
    BeginDraw(0.75);
    Draw(hooks, g_textured.data(), 4);
    Check(Near(Load<float>(g_last_draw.data(), 0), 5));
    nosfix::EndMotionPresentation();

    // A different note taking over the same storage cannot inherit velocity.
    Retire();
    InitNote(g_reused_note.data(), reinterpret_cast<void*>(0x4440), 1001);
    Store(g_textured.data(), 0, 40.0f);
    Store(g_textured.data(), 4, 50.0f);
    SetCountAndBuffer(g_reused_note.data(), 128, 1, g_textured.data());
    Update(hooks, g_reused_note.data(), 1001);
    BeginDraw(0.5);
    Draw(hooks, g_textured.data(), 4);
    Check(Near(Load<float>(g_last_draw.data(), 0), 40));
    nosfix::EndMotionPresentation();
    g_expect_predicted = false;
}

void TestStalePhaseAndRatePassthrough(const Hooks& hooks) {
    ResetTestState(hooks);
    InitNote(g_note.data(), reinterpret_cast<void*>(0x5550), 500);
    SetTextured(g_textured, 0, 0);
    AttachNormalTextured(g_note.data(), g_textured);
    Update(hooks, g_note.data(), 500);
    Retire();
    SetTextured(g_textured, 2, 2);
    Update(hooks, g_note.data(), 501);

    BeginDraw(0.5);
    g_expect_predicted = true;
    Draw(hooks, g_textured.data(), 4);
    nosfix::EndMotionPresentation();
    g_expect_predicted = false;
    Draw(hooks, g_textured.data(), 4); // Outside presentation scope.

    Retire(); // No capture at the new tick: old history must be stale.
    BeginDraw(0.5);
    Draw(hooks, g_textured.data(), 4);
    nosfix::EndMotionPresentation();

    ResetTestState(hooks, 60);
    InitNote(g_note.data(), reinterpret_cast<void*>(0x6660), 600);
    SetTextured(g_textured, 10, 10);
    AttachNormalTextured(g_note.data(), g_textured);
    Update(hooks, g_note.data(), 600);
    Retire();
    SetTextured(g_textured, 20, 20);
    Update(hooks, g_note.data(), 601);
    BeginDraw(0.5);
    Draw(hooks, g_textured.data(), 4);
    Check(Near(Load<float>(g_last_draw.data(), 0), 20));
    nosfix::EndMotionPresentation();
}

void TestMetadataAndFreeInvalidation(const Hooks& hooks) {
    ResetTestState(hooks);
    InitNote(g_note.data(), reinterpret_cast<void*>(0x7770), 700);
    SetTextured(g_textured, 0, 0);
    AttachNormalTextured(g_note.data(), g_textured);
    Update(hooks, g_note.data(), 700);
    Retire();
    SetTextured(g_textured, 2, 2, 0xD0000000, 0x123456789abcdef0ull, 0x30);
    Update(hooks, g_note.data(), 701);
    Retire();
    // A changed texture/UV causes the next capture to become a new baseline.
    SetTextured(g_textured, 4, 4, 0xD0000000, 0x223456789abcdef0ull, 0x30);
    Update(hooks, g_note.data(), 702);
    BeginDraw(0.5);
    Draw(hooks, g_textured.data(), 4);
    Check(Near(Load<float>(g_last_draw.data(), 0), 4));
    nosfix::EndMotionPresentation();

    const auto before = g_free_calls;
    Check(hooks.free_buffer(g_textured.data()) == 0x5678);
    Check(g_free_calls == before + 1);
    BeginDraw(0.5);
    Draw(hooks, g_textured.data(), 4);
    Check(Near(Load<float>(g_last_draw.data(), 0), 4));
    nosfix::EndMotionPresentation();
}

struct LayoutPrimitive {
    std::size_t offset;
    bool textured;
};

struct LayoutCase {
    UpdateFn Hooks::* update;
    std::array<LayoutPrimitive, 4> primitives;
    std::size_t primitive_count;
};

void CheckOnlyPositionsChanged(std::span<const std::byte> actual,
                               std::span<const std::byte> latest,
                               bool textured) {
    Check(actual.size() == latest.size());
    const std::size_t primitive_stride = textured ? 160 : 120;
    const std::size_t vertex_stride = textured ? 24 : 20;
    const std::size_t vertices = textured ? 4 : 6;
    for (std::size_t primitive = 0; primitive < actual.size() / primitive_stride; ++primitive) {
        for (std::size_t byte = 0; byte < primitive_stride; ++byte) {
            bool position_byte = false;
            for (std::size_t vertex = 0; vertex < vertices; ++vertex) {
                position_byte |= byte >= vertex * vertex_stride &&
                    byte < vertex * vertex_stride + 8;
            }
            if (!position_byte)
                Check(actual[primitive * primitive_stride + byte] ==
                      latest[primitive * primitive_stride + byte]);
        }
    }
}

void TestAllNoteLayouts(const Hooks& hooks) {
    constexpr std::array<LayoutCase, 6> layouts{{
        {&Hooks::normal, {{{128, true}, {}, {}, {}}}, 1},
        {&Hooks::long_note,
            {{{128, true}, {168, true}, {352, false}, {408, false}}}, 4},
        {&Hooks::beats, {{{128, true}, {}, {}, {}}}, 1},
        {&Hooks::glissando,
            {{{128, true}, {216, false}, {}, {}}}, 2},
        {&Hooks::trill,
            {{{128, true}, {168, true}, {368, false}, {424, false}}}, 4},
        {&Hooks::velocity,
            {{{128, true}, {168, true}, {352, false}, {}}}, 3},
    }};

    for (std::size_t layout_index = 0; layout_index < layouts.size(); ++layout_index) {
        const auto& layout = layouts[layout_index];
        ResetTestState(hooks);
        std::memset(g_note.data(), 0xA5, g_note.size());
        InitNote(g_note.data(), reinterpret_cast<void*>(0x9000 + layout_index * 0x100), 900);

        std::array<std::vector<std::byte>, 4> buffers;
        std::array<std::vector<std::byte>, 4> latest;
        for (std::size_t i = 0; i < layout.primitive_count; ++i) {
            const auto& primitive = layout.primitives[i];
            if (primitive.textured) SetTextured(buffers[i], 10.0f + i, 20.0f + i);
            else SetRibbon(buffers[i], 10.0f + i, 20.0f + i);
            SetCountAndBuffer(g_note.data(), primitive.offset, 1, buffers[i].data());
        }
        const auto original_note = g_note;
        UpdateFn update = hooks.*(layout.update);
        const auto calls_before = g_update_calls;
        UpdateUsing(update, g_note.data(), 900);
        Check(g_update_calls == calls_before + 1);
        Check(g_note == original_note); // Hook and fake simulation leave descriptors/sentinels intact.
        Retire();

        for (std::size_t i = 0; i < layout.primitive_count; ++i) {
            const auto& primitive = layout.primitives[i];
            if (primitive.textured) SetTextured(buffers[i], 18.0f + i, 24.0f + i);
            else SetRibbon(buffers[i], 18.0f + i, 24.0f + i);
            latest[i] = buffers[i];
        }
        const auto latest_note = g_note;
        const auto second_calls_before = g_update_calls;
        UpdateUsing(update, g_note.data(), 901);
        Check(g_update_calls == second_calls_before + 1);
        Check(g_note == latest_note);

        BeginDraw(0.5);
        g_expect_predicted = true;
        for (std::size_t i = 0; i < layout.primitive_count; ++i) {
            const auto& primitive = layout.primitives[i];
            Draw(hooks, buffers[i].data(), primitive.textured ? 4 : 2);
            Check(Near(Load<float>(g_last_draw.data(), 0), 22.0f + i));
            Check(Near(Load<float>(g_last_draw.data(), 4), 26.0f + i));
            CheckOnlyPositionsChanged(g_last_draw, latest[i], primitive.textured);
            Check(buffers[i] == latest[i]);
        }
        Check(g_update_calls == calls_before + 2); // Presentation drew without simulation updates.
        nosfix::EndMotionPresentation();
        g_expect_predicted = false;
    }
}

} // namespace

namespace nosfix {
unsigned PresentationRate() noexcept { return g_rate; }
} // namespace nosfix

int main() {
    const Hooks hooks = InstallFakeOriginals();
    TestTexturedPredictionAndPreservation(hooks);
    TestRibbonPrediction(hooks);
    TestPauseSeekDoubleUpdateAndReuse(hooks);
    TestStalePhaseAndRatePassthrough(hooks);
    TestMetadataAndFreeInvalidation(hooks);
    TestAllNoteLayouts(hooks);
    nosfix::ConfigureMotion(nullptr, false);
}
