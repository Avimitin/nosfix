#include "nosfix/native_timeline.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

namespace {

struct Tempo {
    float bpm;
    std::int32_t time;
};

struct Timeline {
    alignas(8) std::array<std::byte, 0x200> bytes{};
    std::array<std::byte, 0x100> tempo_storage{};
    std::vector<Tempo> tempo;
};

template <class T>
T Load(const void* base, std::size_t offset) {
    T value{};
    std::memcpy(&value, static_cast<const std::byte*>(base) + offset, sizeof(T));
    return value;
}

template <class T>
void Store(void* base, std::size_t offset, T value) {
    std::memcpy(static_cast<std::byte*>(base) + offset, &value, sizeof(T));
}

void Configure(Timeline& timeline, std::vector<Tempo> tempos, bool microsecond_times) {
    timeline.tempo = std::move(tempos);
    Store<std::int32_t>(timeline.bytes.data(), nosfix::kTimelineCounter,
                        microsecond_times ? nosfix::kTimelinePrerollUs : -200);
    Store<float>(timeline.bytes.data(), nosfix::kTimelineBpm, 120.0f);
    Store<float>(timeline.bytes.data(), nosfix::kTimelineBeat, 0.0f);
    Store<float>(timeline.bytes.data(), nosfix::kTimelinePreviousBeat, 0.0f);
    Store<float>(timeline.bytes.data(), nosfix::kTimelineBeatBase, 0.0f);
    Store<std::int32_t>(timeline.bytes.data(), nosfix::kTimelineLastTempoUs, 0);
    Store<std::int32_t>(timeline.bytes.data(), nosfix::kTimelineTempoCount,
                        static_cast<std::int32_t>(timeline.tempo.size()));
    Store<std::int32_t>(timeline.bytes.data(), nosfix::kTimelineTempoIndex, 0);
    Store<std::uint8_t>(timeline.bytes.data(), nosfix::kTimelineBeatFlag, 0);
    Store<float>(timeline.bytes.data(), nosfix::kTimelinePhase, 1.0f);
    Store<std::uint32_t>(timeline.bytes.data(), nosfix::kTimelinePreviousPhase, 0);
    for (std::size_t i = 0; i < timeline.tempo.size(); ++i) {
        const auto offset = nosfix::kTempoArrayHeader + i * nosfix::kTempoEntrySize;
        Store<float>(timeline.tempo_storage.data(), offset + nosfix::kTempoEntryBpm,
                     timeline.tempo[i].bpm);
        Store<std::int32_t>(timeline.tempo_storage.data(),
                            offset + nosfix::kTempoEntryTime,
                            timeline.tempo[i].time);
    }
    Store<std::uintptr_t>(timeline.bytes.data(), nosfix::kTimelineTempoArray,
                          reinterpret_cast<std::uintptr_t>(timeline.tempo_storage.data()));
}

// Literal model of the current 60 Hz sub_1802FC9D0, including its exact
// equality tempo check and the rate*rate denominator.
std::int32_t FrameTick(Timeline& timeline) {
    auto* base = timeline.bytes.data();
    const std::int32_t previous = Load<std::int32_t>(base, nosfix::kTimelineCounter);
    std::int32_t counter = previous + 1;
    Store<std::int32_t>(base, nosfix::kTimelineCounter, counter);

    std::int32_t index = Load<std::int32_t>(base, nosfix::kTimelineTempoIndex);
    const std::int32_t count = Load<std::int32_t>(base, nosfix::kTimelineTempoCount);
    if (index < count) {
        const auto entry = timeline.tempo_storage.data() + nosfix::kTempoArrayHeader +
                           index * nosfix::kTempoEntrySize;
        const std::int32_t event = Load<std::int32_t>(entry, nosfix::kTempoEntryTime);
        if (counter == event) {
            Store<float>(base, nosfix::kTimelineBpm,
                         Load<float>(entry, nosfix::kTempoEntryBpm));
            Store<float>(base, nosfix::kTimelineBeatBase,
                         Load<float>(base, nosfix::kTimelineBeat));
            Store<std::int32_t>(base, nosfix::kTimelineLastTempoUs, event);
            Store<std::int32_t>(base, nosfix::kTimelineTempoIndex, index + 1);
        }
    }

    const float old_beat = Load<float>(base, nosfix::kTimelineBeat);
    Store<float>(base, nosfix::kTimelinePreviousBeat, old_beat);
    const std::int32_t delta = counter - Load<std::int32_t>(base, nosfix::kTimelineLastTempoUs);
    const float bpm = Load<float>(base, nosfix::kTimelineBpm);
    const float beat = static_cast<float>(delta) / (3600.0f / bpm) +
                       Load<float>(base, nosfix::kTimelineBeatBase);
    Store<float>(base, nosfix::kTimelineBeat, beat);

    const bool was_unset = Load<std::uint8_t>(base, nosfix::kTimelineBeatFlag) == 0;
    if (was_unset) {
        if (counter >= 0 &&
            ((old_beat - static_cast<float>(static_cast<std::int32_t>(old_beat))) >
                 (beat - static_cast<float>(static_cast<std::int32_t>(beat))) ||
             counter == 0)) {
            Store<std::uint8_t>(base, nosfix::kTimelineBeatFlag, 1);
        }
    } else {
        Store<std::uint8_t>(base, nosfix::kTimelineBeatFlag, 0);
    }

    const std::uint32_t previous_phase = Load<std::uint32_t>(base, nosfix::kTimelinePhase);
    Store<std::uint32_t>(base, nosfix::kTimelinePreviousPhase, previous_phase);
    if (previous >= 0) {
        Store<float>(base, nosfix::kTimelinePhase,
                     1.0f - (beat - static_cast<float>(static_cast<std::int32_t>(beat))));
        return static_cast<std::int32_t>(beat);
    }
    Store<float>(base, nosfix::kTimelinePhase, 1.0f);
    return static_cast<std::int32_t>(previous_phase);
}

void Require(bool condition, const char* text) {
    if (condition) return;
    std::fprintf(stderr, "%s\n", text);
    std::exit(1);
}

bool Near(float a, float b, float tolerance = 2.0e-3f) {
    return std::fabs(a - b) <= tolerance;
}

// Exact musical beat for the test's tempo map. The native frame model applies
// a tempo event one 60 Hz frame late; the microsecond tick intentionally uses
// the exact event position, so this is the reference for the new path.
float AnalyticBeat(std::int32_t frame) {
    if (frame < 0) return static_cast<float>(frame) * 120.0f / 3600.0f;
    if (frame < 300) return static_cast<float>(frame) * 120.0f / 3600.0f;
    if (frame < 600) return 10.0f + static_cast<float>(frame - 300) * 180.0f / 3600.0f;
    return 25.0f + static_cast<float>(frame - 600) * 240.0f / 3600.0f;
}

} // namespace

int main() {
    using namespace nosfix;

    Require(kTimelinePrerollUs == FrameBoundaryToTimelineUs(-200),
            "preroll conversion is not the rounded -200 frame boundary");
    Require(ChartMsToTimelineUs(1234) == 1'234'000,
            "chart milliseconds are not exact microseconds");
    Require(FrameBoundaryToTimelineUs(9) == 150'000,
            "9-frame window is not 150000 us");
    Require(FrameBoundaryToTimelineUs(-11) == -183'333,
            "negative window rounding changed");
    Require(FrameBoundaryToTimelineUs(7) == 116'667,
            "7-frame window is not rounded to nearest microsecond");

    const std::vector<Tempo> frame_tempos{{120.0f, 0}, {180.0f, 300}, {240.0f, 600}};
    const std::vector<Tempo> us_tempos{
        {120.0f, FrameBoundaryToTimelineUs(0)},
        {180.0f, FrameBoundaryToTimelineUs(300)},
        {240.0f, FrameBoundaryToTimelineUs(600)}};

    Timeline frame;
    Timeline microseconds;
    Configure(frame, frame_tempos, false);
    Configure(microseconds, us_tempos, true);

    for (std::int32_t f = -199; f <= 900; ++f) {
        FrameTick(frame);
        const auto expected = FrameBoundaryToTimelineUs(f);
        (void)AdvanceTimelineUs(microseconds.bytes.data(), expected);
        const float frame_beat = Load<float>(frame.bytes.data(), kTimelineBeat);
        const float us_beat = Load<float>(microseconds.bytes.data(), kTimelineBeat);
        const float analytic = AnalyticBeat(f);
        Require(Near(us_beat, analytic),
                "microsecond beat diverged from the analytic tempo map");
        const float expected_phase = f <= 0
            ? 1.0f
            : 1.0f - (us_beat - static_cast<float>(static_cast<std::int32_t>(us_beat)));
        const float actual_phase = Load<float>(microseconds.bytes.data(), kTimelinePhase);
        if (!Near(actual_phase, expected_phase)) {
            std::fprintf(stderr, "phase divergence f=%d beat=%.9f phase=%.9f expected=%.9f\n",
                         f, us_beat, actual_phase, expected_phase);
        }
        Require(Near(actual_phase, expected_phase),
                "microsecond phase diverged");
        if (f < 0) {
            // Before the first tempo event the native frame model and the
            // microsecond path are the same musical time.
            Require(Near(frame_beat, us_beat),
                    "pre-event frame and microsecond beat diverged");
        }
        const float expected_bpm = f < 300 ? 120.0f : (f < 600 ? 180.0f : 240.0f);
        Require(Load<float>(microseconds.bytes.data(), kTimelineBpm) == expected_bpm,
                "tempo event was not applied at the expected segment");
    }

    // A single late update crossing several tempo events must produce the same
    // state as the stepped sequence, not skip events as exact equality would.
    Timeline late_frame;
    Timeline late_us;
    Configure(late_frame, frame_tempos, false);
    Configure(late_us, us_tempos, true);
    for (std::int32_t f = -199; f <= 900; ++f) (void)FrameTick(late_frame);
    (void)AdvanceTimelineUs(late_us.bytes.data(), FrameBoundaryToTimelineUs(900));
    Require(Near(Load<float>(late_us.bytes.data(), kTimelineBeat),
                 AnalyticBeat(900), 1.0e-2f),
            "late microsecond update did not produce the analytic beat");
    Require(Load<std::int32_t>(late_us.bytes.data(), kTimelineTempoIndex) ==
                static_cast<std::int32_t>(us_tempos.size()),
            "late microsecond update did not drain all tempo events");

    std::puts("microsecond timeline tick matches the analytic tempo map and pre-event native parity");
    return 0;
}
