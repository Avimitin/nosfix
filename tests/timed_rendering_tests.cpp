#ifdef NDEBUG
#undef NDEBUG
#endif
#include "nosfix/timed_rendering.hpp"
#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <cstdio>

template<class T> void Put(void* base, std::size_t offset, T value) {
    std::memcpy(static_cast<std::byte*>(base) + offset, &value, sizeof(value));
}
template<class T> T Get(std::span<const std::byte> raw, std::size_t offset) {
    T result{};
    std::memcpy(&result, raw.data() + offset, sizeof(result));
    return result;
}

int wmain(int argc, wchar_t** argv) {
    assert(argc == 3);
    const auto game = LoadLibraryExW(argv[1], nullptr, DONT_RESOLVE_DLL_REFERENCES);
    const auto xact = LoadLibraryExW(argv[2], nullptr, DONT_RESOLVE_DLL_REFERENCES);
    assert(game && xact);
    std::string reason;
    assert(nosfix::PrepareTimedRendering(game, reason));
    nosfix::ConfigureTimedRendering(true);
    assert(nosfix::TimedRenderingEnabled());

    // This test executes the isolated original projection in linear scroll
    // mode. Its only helper is the already-initialized PlayerSettingData getter;
    // no game loop, graphics, imports, audio, input or scoring function runs.
    Put(game, 0x1B72568, std::uint32_t{1});
    Put(game, 0x1B725AC, std::int32_t{20}); // player+60, judgment-line offset
    Put(game, 0x1B725C4, std::int32_t{10}); // player+84, note height /10
    Put(game, 0x1B725C8, std::int32_t{0});
    Put(game, 0x13E4724, std::int32_t{3}); // linear projection (no trig calls)
    Put(game, 0x13668D4, 2.0f);           // approach distance
    Put(game, 0x13E476C, std::int32_t{40});
    Put(game, 0x70B300, 60.0f);
    Put(game, 0x70B2F8, 60.0);
    Put(game, 0x136690C, std::int32_t{0});
    Put(game, 0x1366918, std::int32_t{0});
    Put(game, 0x13668B0 + 0x7DEC0 + 4, std::int32_t{100});
    Put(game, 0x13668B0 + 0x7DEC0 + 8, std::int32_t{140});

    nosfix::NoteProjection projected;
    assert(nosfix::ProjectGameNote(nullptr, 0, 1.0f, 1, 2, projected));
    assert(projected.x == 140.0f && projected.y == 325.0f);
    assert(projected.scale == 1.0f && projected.opacity == 1.0f);
    assert(!nosfix::ProjectGameNote(nullptr, 99, 1.0f, 1, 2, projected));

    std::array<std::byte, 216> note{};
    std::array<std::byte, 104> chart{};
    std::array<std::byte, 160> raw{};
    Put(chart.data(), 0, std::int32_t{10'001}); // preserved ms, not frame-rounded 10,000
    Put(chart.data(), 20, std::int32_t{600});
    Put(note.data(), 0, reinterpret_cast<std::byte*>(game) + 0x684328);
    Put(note.data(), 8, chart.data());
    Put(note.data(), 20, std::int32_t{0});
    Put(note.data(), 24, std::int32_t{600});
    Put(note.data(), 44, std::int32_t{1});
    Put(note.data(), 48, std::int32_t{2});
    Put(note.data(), 64, std::int32_t{1});
    Put(note.data(), 196, std::int32_t{60});
    Put(note.data(), 200, std::int32_t{20});
    Put(raw.data(), 144, std::uint32_t{0xD0000000});
    const auto original_note = note;
    const auto original_chart = chart;
    const auto original_raw = raw;
    nosfix::TimedNormal geometry;
    assert(nosfix::CaptureTimedNormal(note.data(), raw, geometry));
    // At the exact stored millisecond timestamp, distance=0 and center y=700.
    const auto first = geometry.Draw(10'001.0, raw, nosfix::ProjectGameNote, nullptr);
    assert(!first.empty());
    assert(Get<float>(first, 4) == 690.0f);
    const auto quarter_ms_earlier = geometry.Draw(10'000.75, raw, nosfix::ProjectGameNote, nullptr);
    assert(!quarter_ms_earlier.empty());
    assert(Get<float>(quarter_ms_earlier, 4) == 689.0f); // integer XY projection retained
    assert(note == original_note && chart == original_chart && raw == original_raw);
    nosfix::ConfigureTimedRendering(false);
    assert(!nosfix::ProjectGameNote(nullptr, 0, 1.0f, 1, 2, projected));
    assert(FreeLibrary(xact) && FreeLibrary(game));
    std::puts("Original projection ABI, precise chart-time capture and private geometry passed.");
}
