#ifdef NDEBUG
#undef NDEBUG
#endif
#include "nosfix/native_note_state.hpp"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <limits>
#include <vector>

namespace {

int g_failures = 0;

void Check(bool condition, const char* text, int line) {
    if (!condition) {
        std::printf("FAIL line %d: %s\n", line, text);
        ++g_failures;
    }
}

#define CHECK(expr) Check((expr), #expr, __LINE__)

using namespace nosfix;

constexpr double kJust = 33'334.0;
constexpr double kGreat = 66'667.0;
constexpr double kGood = 133'334.0;

NormalWindows Windows() {
    NormalWindows windows;
    windows.just_early_us = kJust;
    windows.just_late_us = kJust;
    windows.great_early_us = kGreat;
    windows.great_late_us = kGreat;
    windows.good_early_us = kGood;
    windows.good_late_us = kGood;
    return windows;
}

NativeNoteConfig Config() {
    NativeNoteConfig config;
    config.windows = Windows();
    config.judgment_offset_us = 0.0;
    config.eligibility_total_offset_us = 0.0;
    config.units_per_second = 60.0;
    config.windows_sampled = true;
    return config;
}

NativeNoteSpec Note(std::uint32_t chart, std::uint8_t lane, double start_us,
                    std::uint32_t sound, bool auto_play = false) {
    NativeNoteSpec spec;
    spec.chart_index = chart;
    spec.lane = lane;
    spec.start_us = start_us;
    spec.end_us = start_us;
    spec.sound_id = sound;
    spec.auto_play = auto_play;
    return spec;
}

struct Fixture final {
    NativeNoteState state;
    std::vector<NativeNoteOutcome> out;
    std::uint64_t sequence = 1;

    bool Configure(const std::vector<NativeNoteSpec>& notes,
                   const NativeNoteConfig& config = Config()) {
        const NoteConfigStatus status = state.Configure(notes, config);
        out.assign(state.NoteCount(), NativeNoteOutcome{});
        return status == NoteConfigStatus::Accepted;
    }

    TimedInputEvent Event(std::uint8_t lane, double us, InputEdge edge,
                          std::uint64_t generation) {
        TimedInputEvent event;
        event.input.lane = lane;
        event.input.edge = edge;
        event.input.stamp.sequence = sequence++;
        event.input.stamp.stream_epoch = 7;
        event.song_time.microseconds = us;
        event.song_time.generation = generation;
        return event;
    }

    NoteRunStatus Press(std::uint8_t lane, double us, std::uint64_t generation,
                        double decision, NoteAdvance& counts) {
        return state.Press(Event(lane, us, InputEdge::Press, generation), decision, out,
                           counts);
    }

    NoteRunStatus Release(std::uint8_t lane, double us, std::uint64_t generation,
                          NoteAdvance& counts) {
        return state.Release(Event(lane, us, InputEdge::Release, generation), out, counts);
    }

    NoteRunStatus Advance(double up_to, double decision, NoteAdvance& counts) {
        return state.Advance(up_to, decision, out, counts);
    }
};

bool Nearly(double a, double b) { return std::abs(a - b) < 1e-6; }

void TestLifecycle() {
    Fixture fixture;
    std::vector<NativeNoteSpec> notes{
        Note(0, 0, 1'000'000.0, 10),
        Note(1, 0, 1'500'000.0, 11),
        Note(2, 1, 2'000'000.0, 12, true),
        Note(3, 2, 1'200'000.0, 13),
        Note(4, 0, 3'000'000.0, 14),
        Note(5, 3, 4'000'000.0, 15),
        Note(6, 4, 5'000'000.0, 16),
        Note(7, 5, 6'000'000.0, 17),
        Note(8, 6, 7'000'000.0, 18),
    };
    CHECK(fixture.Configure(notes));
    CHECK(fixture.state.Begin(1, 0.0) == NoteRunStatus::Ok);
    CHECK(fixture.state.PendingCount() == notes.size());

    // Capacity is part of the contract and is a complete no-op.
    {
        NoteAdvance counts;
        std::vector<NativeNoteOutcome> tiny(1);
        CHECK(fixture.state.Advance(0.0, 0.0, tiny, counts) ==
              NoteRunStatus::InsufficientCapacity);
        CHECK(counts.written == 0);
    }

    // Exact note time is Just, effect grade 0, one sound, one completion.
    {
        NoteAdvance counts;
        CHECK(fixture.Press(0, 1'000'000.0, 1, 1'000'000.0, counts) == NoteRunStatus::Ok);
        CHECK(counts.written == 1 && counts.scored == 1);
        CHECK(fixture.out[0].kind == NoteOutcomeKind::Scored);
        CHECK(fixture.out[0].rank == NormalRank::Just);
        CHECK(fixture.out[0].effect_grade == 0);
        CHECK(fixture.out[0].sound_id == 10);
        CHECK(Nearly(fixture.out[0].at_us, 1'000'000.0));
        CHECK(Nearly(fixture.out[0].input_us, 1'000'000.0));
        CHECK(fixture.state.Phase(0) == NativeNotePhase::Completed);
        CHECK(fixture.state.ActiveSound(0) == 10);
    }

    // A repeat press cannot re-judge the completed note; lane 0's next eligible
    // note is far outside its window, so the press is ignored.
    {
        NoteAdvance counts;
        CHECK(fixture.Press(0, 1'000'500.0, 1, 1'000'500.0, counts) == NoteRunStatus::Ok);
        CHECK(counts.written == 0 && counts.ignored_presses == 1);
    }

    // Release cuts the active sound exactly once and never scores.
    {
        NoteAdvance counts;
        CHECK(fixture.Release(0, 1'001'000.0, 1, counts) == NoteRunStatus::Ok);
        CHECK(counts.cuts == 1 && counts.written == 1);
        CHECK(fixture.out[0].kind == NoteOutcomeKind::KeysoundCut);
        CHECK(fixture.out[0].sound_id == 10);
        CHECK(fixture.state.ActiveSound(0) == 0);
        NoteAdvance again;
        CHECK(fixture.Release(0, 1'001'500.0, 1, again) == NoteRunStatus::Ok);
        CHECK(again.cuts == 0 && again.written == 0 && again.ignored_releases == 1);
    }

    // A Good press earlier than the Great window scores immediately at Good.
    {
        NoteAdvance counts;
        CHECK(fixture.Press(2, 1'080'000.0, 1, 1'080'000.0, counts) == NoteRunStatus::Ok);
        CHECK(counts.written == 1 && counts.scored == 1);
        CHECK(fixture.out[0].rank == NormalRank::Good);
        CHECK(fixture.out[0].effect_grade == 1);
        CHECK(fixture.out[0].note_index == 3);
    }

    // Early Great press is deferred; resolution at the note time records Great
    // (not the original rank) and starts the sound once.
    {
        NoteAdvance counts;
        CHECK(fixture.Press(0, 1'455'000.0, 1, 1'455'000.0, counts) == NoteRunStatus::Ok);
        CHECK(counts.written == 0 && counts.early_deferred == 1);
        CHECK(fixture.state.Phase(1) == NativeNotePhase::PendingEarly);
        NoteAdvance before;
        CHECK(fixture.Advance(1'499'999.0, 1'499'999.0, before) == NoteRunStatus::Ok);
        CHECK(before.written == 0);
        CHECK(fixture.state.Phase(1) == NativeNotePhase::PendingEarly);
    }
    {
        NoteAdvance counts;
        CHECK(fixture.Advance(1'500'000.0, 1'500'000.0, counts) == NoteRunStatus::Ok);
        CHECK(counts.written == 1 && counts.scored == 1);
        CHECK(fixture.out[0].note_index == 1);
        CHECK(fixture.out[0].rank == NormalRank::Great);
        CHECK(fixture.out[0].effect_grade == 1);
        CHECK(Nearly(fixture.out[0].at_us, 1'500'000.0));
        CHECK(Nearly(fixture.out[0].input_us, 1'455'000.0));
        CHECK(fixture.state.Phase(1) == NativeNotePhase::Completed);
    }

    // Auto completion is evaluated on song time with no press, is Just, and
    // happens once. Its crossing is one legacy unit before the note time.
    {
        NoteAdvance counts;
        CHECK(fixture.Advance(2'000'000.0, 2'000'000.0, counts) == NoteRunStatus::Ok);
        CHECK(counts.auto_completed == 1 && counts.written == 1);
        CHECK(fixture.out[0].kind == NoteOutcomeKind::Auto);
        CHECK(fixture.out[0].note_index == 2);
        CHECK(fixture.out[0].rank == NormalRank::Just);
        CHECK(fixture.out[0].sound_id == 12);
        CHECK(Nearly(fixture.out[0].at_us, 1'983'333.3333333333));
        NoteAdvance again;
        CHECK(fixture.Advance(2'000'000.0, 2'000'000.0, again) == NoteRunStatus::Ok);
        CHECK(again.written == 0);
    }

    // Two early presses overwrite the pending decision; only one resolution and
    // one sound result. The last recorded rank wins (Great -> Just here).
    {
        NoteAdvance first;
        CHECK(fixture.Press(0, 2'940'000.0, 1, 2'940'000.0, first) == NoteRunStatus::Ok);
        CHECK(first.early_deferred == 1 && first.written == 0);
        CHECK(fixture.state.Phase(4) == NativeNotePhase::PendingEarly);
        NoteAdvance second;
        CHECK(fixture.Press(0, 2'980'000.0, 1, 2'980'000.0, second) == NoteRunStatus::Ok);
        CHECK(second.early_deferred == 1 && second.early_overwritten == 1);
        CHECK(second.written == 0);
        NoteAdvance resolution;
        CHECK(fixture.Advance(3'000'000.0, 3'000'000.0, resolution) == NoteRunStatus::Ok);
        CHECK(resolution.written == 1 && resolution.scored == 1);
        CHECK(fixture.out[0].note_index == 4);
        CHECK(fixture.out[0].rank == NormalRank::Just);
        CHECK(fixture.out[0].effect_grade == 0);
        CHECK(fixture.state.ActiveSound(0) == 14);
    }

    // The lane-3 note expires strictly after its inclusive deadline.
    {
        NoteAdvance counts;
        CHECK(fixture.Advance(4'133'334.0, 4'133'334.0, counts) == NoteRunStatus::Ok);
        CHECK(counts.missed == 0);
        CHECK(fixture.state.Phase(5) == NativeNotePhase::Pending);
        NoteAdvance miss;
        CHECK(fixture.Advance(4'133'335.0, 4'133'335.0, miss) == NoteRunStatus::Ok);
        CHECK(miss.written == 1 && miss.missed == 1);
        CHECK(fixture.out[0].kind == NoteOutcomeKind::Missed);
        CHECK(fixture.out[0].rank == NormalRank::Missed);
        CHECK(fixture.out[0].sound_id == 0);
        CHECK(Nearly(fixture.out[0].at_us, 4'133'334.0));
    }

    // The inclusive Good-late deadline is the last possible hit.
    {
        NoteAdvance counts;
        CHECK(fixture.Press(4, 5'133'334.0, 1, 5'133'334.0, counts) == NoteRunStatus::Ok);
        CHECK(counts.written == 1 && counts.scored == 1);
        CHECK(fixture.out[0].rank == NormalRank::Good);
        CHECK(fixture.out[0].effect_grade == 2);
    }

    // One microsecond after the deadline the note is missed, and the press is
    // not turned into a hit. The miss happens exactly once.
    {
        NoteAdvance counts;
        CHECK(fixture.Press(5, 6'133'335.0, 1, 6'133'335.0, counts) == NoteRunStatus::Ok);
        CHECK(counts.written == 1 && counts.missed == 1);
        CHECK(counts.ignored_presses == 1);
        CHECK(fixture.out[0].kind == NoteOutcomeKind::Missed);
        NoteAdvance repeat;
        CHECK(fixture.Advance(6'500'000.0, 6'500'000.0, repeat) == NoteRunStatus::Ok);
        CHECK(repeat.written == 0);
    }

    // The state horizon (certified watermark) is behind the deadline, so the
    // note stays pending. A press captured in-window then still scores.
    {
        NoteAdvance counts;
        CHECK(fixture.Advance(7'000'000.0, 7'000'000.0, counts) == NoteRunStatus::Ok);
        CHECK(counts.missed == 0);
        CHECK(fixture.state.Phase(8) == NativeNotePhase::Pending);
        NoteAdvance press;
        CHECK(fixture.Press(6, 7'100'000.0, 1, 7'500'000.0, press) == NoteRunStatus::Ok);
        CHECK(press.written == 1 && press.scored == 1);
        CHECK(fixture.out[0].rank == NormalRank::Good);
        CHECK(fixture.state.Phase(8) == NativeNotePhase::Completed);
    }

    // Unknown lane press is ignored without touching any note.
    {
        NoteAdvance counts;
        CHECK(fixture.Press(20, 7'600'000.0, 1, 7'600'000.0, counts) == NoteRunStatus::Ok);
        CHECK(counts.ignored_presses == 1 && counts.written == 0);
    }

    // Stale generation and a non-new generation are rejected.
    {
        NoteAdvance counts;
        CHECK(fixture.Press(0, 3'500'000.0, 2, 3'500'000.0, counts) ==
              NoteRunStatus::GenerationMismatch);
        CHECK(fixture.state.Begin(1, 0.0) == NoteRunStatus::GenerationNotNew);
        CHECK(fixture.state.Begin(0, 0.0) == NoteRunStatus::GenerationZero);
    }

    // Retry/seek: a new generation skips notes before the start and never
    // scores them as misses.
    {
        CHECK(fixture.state.Begin(2, 3'500'000.0) == NoteRunStatus::Ok);
        CHECK(fixture.state.SkippedCount() == 5);
        CHECK(fixture.state.PendingCount() == 4);
        NoteAdvance counts;
        CHECK(fixture.Advance(3'600'000.0, 3'600'000.0, counts) == NoteRunStatus::Ok);
        CHECK(counts.missed == 0);
        CHECK(fixture.state.MissedCount() == 0);
    }
}

void TestNullUpdates() {
    Fixture fixture;
    std::vector<NativeNoteSpec> notes{
        Note(0, 0, 2'000'000.0, 20),
        Note(1, 1, 1'500'000.0, 21, true),
    };
    CHECK(fixture.Configure(notes));
    CHECK(fixture.state.Begin(1, 0.0) == NoteRunStatus::Ok);

    // An auto note is not press-judgeable: the native auto branch ignores the
    // event and completes on song time without a press.
    {
        NoteAdvance counts;
        CHECK(fixture.Press(1, 1'400'000.0, 1, 1'400'000.0, counts) == NoteRunStatus::Ok);
        CHECK(counts.written == 0 && counts.ignored_presses == 1);
        CHECK(fixture.state.Phase(1) == NativeNotePhase::Pending);
    }

    // At exactly the note time the non-auto note is still pending; only the
    // auto note completes. A null update never fabricates a press.
    {
        NoteAdvance counts;
        CHECK(fixture.Advance(2'000'000.0, 2'000'000.0, counts) == NoteRunStatus::Ok);
        CHECK(counts.auto_completed == 1 && counts.written == 1);
        CHECK(fixture.out[0].note_index == 1);
        CHECK(fixture.state.Phase(0) == NativeNotePhase::Pending);
        CHECK(fixture.state.Phase(1) == NativeNotePhase::Completed);
    }
    {
        NoteAdvance counts;
        CHECK(fixture.Advance(2'133'334.0, 2'133'334.0, counts) == NoteRunStatus::Ok);
        CHECK(counts.missed == 0);
        CHECK(fixture.state.Phase(0) == NativeNotePhase::Pending);
        NoteAdvance miss;
        CHECK(fixture.Advance(2'133'335.0, 2'133'335.0, miss) == NoteRunStatus::Ok);
        CHECK(miss.missed == 1);
        CHECK(fixture.state.Phase(0) == NativeNotePhase::Missed);
    }
}

void TestOrdering() {
    Fixture fixture;
    std::vector<NativeNoteSpec> notes{
        Note(0, 0, 2'000'000.0, 30),
        Note(1, 1, 1'000'000.0, 31),
        Note(2, 2, 1'500'000.0, 32, true),
        Note(3, 3, 1'800'000.0, 33),
    };
    CHECK(fixture.Configure(notes));
    CHECK(fixture.state.Begin(1, 0.0) == NoteRunStatus::Ok);

    NoteAdvance counts;
    CHECK(fixture.Advance(3'000'000.0, 3'000'000.0, counts) == NoteRunStatus::Ok);
    CHECK(counts.written == 4);
    CHECK(counts.missed == 3);
    CHECK(counts.auto_completed == 1);
    // Sorted by decision time, not chart order.
    CHECK(fixture.out[0].note_index == 1);
    CHECK(fixture.out[0].kind == NoteOutcomeKind::Missed);
    CHECK(fixture.out[1].note_index == 2);
    CHECK(fixture.out[1].kind == NoteOutcomeKind::Auto);
    CHECK(fixture.out[2].note_index == 3);
    CHECK(fixture.out[2].kind == NoteOutcomeKind::Missed);
    CHECK(fixture.out[3].note_index == 0);
    CHECK(fixture.out[3].kind == NoteOutcomeKind::Missed);
    for (std::size_t i = 1; i < counts.written; ++i) {
        CHECK(fixture.out[i - 1].at_us <= fixture.out[i].at_us);
    }
}

void TestPressOrdering() {
    Fixture fixture;
    std::vector<NativeNoteSpec> notes{
        Note(0, 0, 1'000'000.0, 40),
        Note(1, 1, 1'010'000.0, 41),
    };
    CHECK(fixture.Configure(notes));
    CHECK(fixture.state.Begin(1, 0.0) == NoteRunStatus::Ok);

    // An early press on note 0 is pending; a later press on note 1 first
    // resolves note 0 at its note time, so outcomes stay in song-time order.
    NoteAdvance counts;
    CHECK(fixture.Press(0, 990'000.0, 1, 990'000.0, counts) == NoteRunStatus::Ok);
    CHECK(counts.early_deferred == 1);
    NoteAdvance second;
    CHECK(fixture.Press(1, 1'012'000.0, 1, 1'012'000.0, second) == NoteRunStatus::Ok);
    CHECK(second.written == 2);
    CHECK(fixture.out[0].note_index == 0);
    CHECK(fixture.out[0].rank == NormalRank::Just);
    CHECK(Nearly(fixture.out[0].at_us, 1'000'000.0));
    CHECK(fixture.out[1].note_index == 1);
    CHECK(fixture.out[1].rank == NormalRank::Just);
    CHECK(Nearly(fixture.out[1].at_us, 1'012'000.0));
}

void TestEligibilityOffset() {
    // The eligibility helper subtracts judgment + visual, separately from the
    // judgment offset. A press can therefore be inside the judgment window yet
    // outside the selectable eligibility union.
    const std::vector<NativeNoteSpec> notes{Note(0, 0, 1'000'000.0, 50)};

    Fixture shifted;
    NativeNoteConfig shifted_config = Config();
    shifted_config.eligibility_total_offset_us = 50'000.0;
    CHECK(shifted.Configure(notes, shifted_config));
    CHECK(shifted.state.Begin(1, 0.0) == NoteRunStatus::Ok);
    {
        NoteAdvance counts;
        CHECK(shifted.Press(0, 1'100'000.0, 1, 1'100'000.0, counts) == NoteRunStatus::Ok);
        CHECK(counts.written == 0 && counts.ignored_presses == 1);
        CHECK(shifted.state.Phase(0) == NativeNotePhase::Pending);
    }
    {
        NoteAdvance counts;
        CHECK(shifted.Advance(1'200'000.0, 1'200'000.0, counts) == NoteRunStatus::Ok);
        CHECK(counts.missed == 1);
        CHECK(shifted.state.Phase(0) == NativeNotePhase::Missed);
    }

    Fixture aligned;
    NativeNoteConfig aligned_config = Config();
    aligned_config.eligibility_total_offset_us = 0.0;
    CHECK(aligned.Configure(notes, aligned_config));
    CHECK(aligned.state.Begin(1, 0.0) == NoteRunStatus::Ok);
    {
        NoteAdvance counts;
        CHECK(aligned.Press(0, 1'100'000.0, 1, 1'100'000.0, counts) == NoteRunStatus::Ok);
        CHECK(counts.written == 1 && counts.scored == 1);
        CHECK(aligned.out[0].rank == NormalRank::Good);
    }
}

void TestWindowSample() {
    const auto sampled = native_layout::ValidateNormalWindowUnits(
        native_layout::kInitialWindowSeeds, 60.0);
    CHECK(sampled.status == native_layout::WindowSampleStatus::Ok);
    CHECK(sampled.matches_initial_seed);
    CHECK(Nearly(sampled.windows.just_early_us, 2.0 * 1'000'000.0 / 60.0));
    CHECK(Nearly(sampled.windows.just_late_us, 2.0 * 1'000'000.0 / 60.0));
    CHECK(Nearly(sampled.windows.great_early_us, 4.0 * 1'000'000.0 / 60.0));
    CHECK(Nearly(sampled.windows.great_late_us, 4.0 * 1'000'000.0 / 60.0));
    CHECK(Nearly(sampled.windows.good_early_us, 7.0 * 1'000'000.0 / 60.0));
    CHECK(Nearly(sampled.windows.good_late_us, 6.0 * 1'000'000.0 / 60.0));

    CHECK(native_layout::ValidateNormalWindowUnits(
              native_layout::kInitialWindowSeeds, 0.0).status ==
          native_layout::WindowSampleStatus::InvalidRate);

    native_layout::NativeWindowUnits bad_sign = native_layout::kInitialWindowSeeds;
    bad_sign.just_earliest = -2;
    CHECK(native_layout::ValidateNormalWindowUnits(bad_sign, 60.0).status ==
          native_layout::WindowSampleStatus::InvalidUnits);

    native_layout::NativeWindowUnits bad_band = native_layout::kInitialWindowSeeds;
    bad_band.effect_early = 1;
    CHECK(native_layout::ValidateNormalWindowUnits(bad_band, 60.0).status ==
          native_layout::WindowSampleStatus::InconsistentEffectBand);

    // The runtime reader takes a caller-owned image; the 8 globals are read
    // through the proven RVAs. No live pointer is dereferenced here.
    constexpr std::size_t kImageSize = 0x1366900;
    std::vector<std::byte> image(kImageSize, std::byte{0});
    const auto write = [&](std::size_t offset, std::int32_t value) {
        std::memcpy(image.data() + offset, &value, sizeof(value));
    };
    const std::array<std::int32_t, 8> seeds{9, -11, 7, -6, 4, -4, 2, -2};
    for (std::size_t i = 0; i < seeds.size(); ++i) {
        write(native_layout::kEffectBandEarlyRva + i * 4, seeds[i]);
    }
    const auto read = native_layout::ReadNormalWindowUnits(image, 60.0);
    CHECK(read.status == native_layout::WindowSampleStatus::Ok);
    CHECK(read.matches_initial_seed);
    CHECK(Nearly(read.windows.just_early_us, 2.0 * 1'000'000.0 / 60.0));

    std::vector<std::byte> too_small(1024);
    CHECK(native_layout::ReadNormalWindowUnits(too_small, 60.0).status ==
          native_layout::WindowSampleStatus::Unavailable);
}

void TestConfigValidation() {
    Fixture fixture;
    const auto status = [&](std::vector<NativeNoteSpec> notes, NativeNoteConfig config) {
        return fixture.state.Configure(notes, config);
    };
    {
        std::vector<NativeNoteSpec> notes{Note(0, 28, 1'000'000.0, 1)};
        CHECK(status(notes, Config()) == NoteConfigStatus::InvalidLane);
    }
    {
        std::vector<NativeNoteSpec> notes{Note(0, 0, 1'000'000.0, 1)};
        notes[0].kind = NoteKind::Long;
        CHECK(status(notes, Config()) == NoteConfigStatus::UnsupportedKind);
    }
    {
        std::vector<NativeNoteSpec> notes{Note(0, 0, 1'000'000.0, 1)};
        notes[0].start_us = std::numeric_limits<double>::quiet_NaN();
        CHECK(status(notes, Config()) == NoteConfigStatus::NonFiniteTime);
    }
    {
        std::vector<NativeNoteSpec> notes{Note(0, 0, 2'000'000.0, 1)};
        notes[0].end_us = 1'000'000.0;
        CHECK(status(notes, Config()) == NoteConfigStatus::InvalidTimeRange);
    }
    {
        std::vector<NativeNoteSpec> notes{Note(0, 0, 1'000'000.0, 1),
                                          Note(0, 1, 2'000'000.0, 2)};
        CHECK(status(notes, Config()) == NoteConfigStatus::DuplicateChartIndex);
    }
    {
        std::vector<NativeNoteSpec> notes{Note(0, 0, 1'000'000.0, 1)};
        NativeNoteConfig config = Config();
        config.windows.great_early_us = kJust - 1.0;
        CHECK(status(notes, config) == NoteConfigStatus::InvalidWindows);
    }
    {
        std::vector<NativeNoteSpec> notes{Note(0, 0, 1'000'000.0, 1)};
        NativeNoteConfig config = Config();
        config.units_per_second = 0.0;
        CHECK(status(notes, config) == NoteConfigStatus::InvalidRate);
    }
    {
        std::vector<NativeNoteSpec> notes{Note(0, 0, 1'000'000.0, 1)};
        NativeNoteConfig config = Config();
        config.judgment_offset_us = std::numeric_limits<double>::infinity();
        CHECK(status(notes, config) == NoteConfigStatus::InvalidOffset);
    }
    {
        std::vector<NativeNoteSpec> notes{Note(0, 0, 1'000'000.0, 1)};
        NativeNoteConfig config = Config();
        config.windows_sampled = false;
        CHECK(status(notes, config) == NoteConfigStatus::WindowsNotSampled);
    }
    {
        std::vector<NativeNoteSpec> notes{Note(0, 0, 1'000'000.0, 1)};
        NativeNoteConfig config = Config();
        config.windows = NormalWindows{};
        CHECK(status(notes, config) == NoteConfigStatus::DegenerateWindows);
    }
    {
        std::vector<NativeNoteSpec> notes{Note(0, 0, 1'000'000.0, 1)};
        NativeNoteConfig config = Config();
        config.selection = NoteSelectionPolicy::Native2F7C10;
        CHECK(status(notes, config) == NoteConfigStatus::UnsupportedSelection);
    }
    CHECK(!fixture.state.IsConfigured());
}

void TestChartRecordDecode() {
    std::vector<std::byte> record(native_layout::kChartNoteBytes, std::byte{0});
    const auto write32 = [&](std::size_t offset, std::int32_t value) {
        std::memcpy(record.data() + offset, &value, sizeof(value));
    };
    const auto write16 = [&](std::size_t offset, std::int16_t value) {
        std::memcpy(record.data() + offset, &value, sizeof(value));
    };
    write32(native_layout::kChartNoteStartMs, 1234);
    write32(native_layout::kChartNoteEndMs, 5678);
    write32(native_layout::kChartNoteStartFrame, 74);
    write32(native_layout::kChartNoteEndFrame, 340);
    write16(native_layout::kChartNoteLane, 3);
    write32(native_layout::kChartNoteLeftKey, 5);
    write32(native_layout::kChartNoteRightKey, 7);
    write32(native_layout::kChartNoteMode, 1);
    write32(native_layout::kChartNoteEffectKind, 6);
    record[native_layout::kChartNoteAutoByte] = std::byte{1};

    native_layout::ChartNoteView view;
    CHECK(native_layout::DecodeChartNote(record, view));
    CHECK(view.start_ms == 1234 && view.end_ms == 5678);
    CHECK(view.start_frame == 74 && view.end_frame == 340);
    CHECK(view.lane == 3);
    CHECK(view.left_key == 5 && view.right_key == 7);
    CHECK(view.mode == 1 && view.effect_kind == 6 && view.auto_flag);
    CHECK(native_layout::ChartMsToUs(1234) == 1'234'000.0);

    std::vector<std::byte> short_record(103);
    CHECK(!native_layout::DecodeChartNote(short_record, view));

    // The field tables are the explicit guard documentation; verify the values
    // the migration relies on are present and have the stated widths.
    CHECK(native_layout::kNormalNoteFields.size() == 24);
    CHECK(native_layout::kChartNoteFields.size() == 14);
    CHECK(native_layout::kChartNoteBytes == 104);
    CHECK(native_layout::kChartNoteStartMs == 0);
    CHECK(native_layout::kChartNoteEndMs == 4);
    CHECK(native_layout::kChartNoteLane == 16);
    CHECK(native_layout::kChartNoteStartFrame == 20);
    CHECK(native_layout::kChartNoteEndFrame == 24);
    CHECK(native_layout::kChartNoteLeftKey == 88);
    CHECK(native_layout::kChartNoteRightKey == 92);
    CHECK(native_layout::kNormalNotesVtableRva == 0x684328);
    CHECK(native_layout::kNormalVtableRvas.size() == 32);
    CHECK(native_layout::kNormalVtableRvas[4] == 0x2F6800);
    CHECK(native_layout::kNormalVtableSlots.size() == 12);
    CHECK(native_layout::kNormalObjectStride == 216);
    CHECK(native_layout::kNormalVtableBytes == 256);
    CHECK(native_layout::kTargetPrefixes.size() == native_layout::kTimingTargets.size());
    for (const auto& prefix : native_layout::kTargetPrefixes) {
        CHECK(prefix.size() == 32);
        CHECK(prefix[0] != 0 || prefix[1] != 0);
    }
    CHECK(!native_layout::kAbiAndMatchingVerified);
}

} // namespace

int main() {
    TestLifecycle();
    TestNullUpdates();
    TestOrdering();
    TestPressOrdering();
    TestEligibilityOffset();
    TestWindowSample();
    TestConfigValidation();
    TestChartRecordDecode();

    if (g_failures == 0) {
        std::printf("native_note_state: all checks passed\n");
        return 0;
    }
    std::printf("native_note_state: %d failure(s)\n", g_failures);
    return 1;
}
