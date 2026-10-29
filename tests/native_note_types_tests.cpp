// Non-normal note models: long/hold 0x2F4220, glissando 0x1A60C0, trill
// 0x1A7AF0. Builds with /O2 /DNDEBUG, so every check is a counting one.

#ifdef NDEBUG
#undef NDEBUG
#endif
#include "nosfix/native_input_bridge.hpp"
#include "nosfix/native_note_types.hpp"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <string_view>

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
constexpr double kUnit60 = 1'000'000.0 / 60.0; // 16666.67 us per song unit
constexpr double kTolerance60 = 15.0 * kUnit60;

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

NativeTypeConfig Config() {
    NativeTypeConfig config;
    config.windows = Windows();
    config.windows_sampled = true;
    config.judgment_offset_us = 0.0;
    config.units_per_second = 60.0;
    return config;
}

NativeTypeConfig ProvenConfig() {
    NativeTypeConfig config = Config();
    config.release_gate_proven = true;
    return config;
}

NativeNoteEvent Event(std::uint8_t lane, double at_us, NativeEventKind kind = NativeEventKind::Press,
                      std::uint32_t event_id = 0, std::uint32_t group_id = 0,
                      std::uint64_t generation = 7) {
    NativeNoteEvent event;
    event.lane = lane;
    event.kind = kind;
    event.event_id = event_id;
    event.group_id = group_id;
    event.song_time.generation = generation;
    event.song_time.microseconds = at_us;
    return event;
}

NativeLongSpec LongSpec(std::uint8_t lane = 2, double start = 1'000'000.0,
                        double end = 2'000'000.0, bool auto_play = false) {
    NativeLongSpec spec;
    spec.lane = lane;
    spec.start_us = start;
    spec.end_us = end;
    spec.chart_index = 11;
    spec.auto_play = auto_play;
    return spec;
}

NativeGlissSpec GlissSpec(std::uint8_t connectors = 0, bool auto_play = false,
                          std::uint32_t group_id = 1) {
    NativeGlissSpec spec;
    spec.lane = 5;
    spec.start_us = 1'000'000.0;
    spec.chart_index = 12;
    spec.auto_play = auto_play;
    spec.group_id = group_id;
    spec.connector_count = connectors;
    return spec;
}

NativeTrillSpec TrillSpec(std::uint8_t lane = 9, double start = 1'000'000.0,
                          double end = 2'000'000.0, bool auto_play = false) {
    NativeTrillSpec spec;
    spec.lane = lane;
    spec.start_us = start;
    spec.end_us = end;
    spec.chart_index = 13;
    spec.auto_play = auto_play;
    return spec;
}

// Builds the outcome kind sequence as a string so a whole call can be compared.
struct Sequence final {
    char text[256]{};
    std::size_t length = 0;

    void Append(const char* name) {
        for (const char* p = name; *p != '\0' && length + 1 < sizeof(text); ++p) {
            text[length++] = *p;
        }
        if (length + 1 < sizeof(text)) text[length++] = ',';
        text[length] = '\0';
    }
};

const char* KindName(const NoteTypeResult& result, std::size_t index) {
    return index < result.count ? NoteTypeOutcomeKindName(result.outcomes[index].kind) : "none";
}

bool OnlyOutcome(const NoteTypeResult& result, NoteTypeOutcomeKind kind) {
    return result.count == 1 && result.outcomes[0].kind == kind;
}

// ---------------------------------------------------------------------------
// Configuration and status paths
// ---------------------------------------------------------------------------
void TestConfigurationAndStatus() {
    NativeLongState state;
    NativeLongSpec spec = LongSpec();
    CHECK(state.Phase() == LongPhase::Idle);
    CHECK(state.Press(Event(2, 1'000'000.0), 1'000'000.0).status ==
          NoteTypeStatus::NotConfigured);

    NativeTypeConfig config = Config();
    config.windows_sampled = false;
    CHECK(state.Configure(spec, config) == NoteTypeStatus::WindowsNotSampled);

    config = Config();
    config.windows.just_early_us = -1.0;
    CHECK(state.Configure(spec, config) == NoteTypeStatus::InvalidWindows);

    config = Config();
    config.judgment_offset_us = std::numeric_limits<double>::quiet_NaN();
    CHECK(state.Configure(spec, config) == NoteTypeStatus::InvalidOffset);

    config = Config();
    config.units_per_second = 0.0;
    CHECK(state.Configure(spec, config) == NoteTypeStatus::InvalidRate);

    config = Config();
    config.sound_serial_start = 0;
    CHECK(state.Configure(spec, config) == NoteTypeStatus::InvalidSpec);

    NativeLongSpec bad = LongSpec();
    bad.lane = 28;
    CHECK(state.Configure(bad, Config()) == NoteTypeStatus::InvalidSpec);
    bad = LongSpec();
    bad.end_us = bad.start_us - 1.0;
    CHECK(state.Configure(bad, Config()) == NoteTypeStatus::InvalidSpec);
    bad = LongSpec();
    bad.start_us = std::numeric_limits<double>::infinity();
    CHECK(state.Configure(bad, Config()) == NoteTypeStatus::NonFiniteTime);

    CHECK(state.Configure(spec, Config()) == NoteTypeStatus::Ok);
    CHECK(state.Begin(0, 0.0) == NoteTypeStatus::GenerationZero);
    CHECK(state.Begin(3, 0.0) == NoteTypeStatus::Ok);
    CHECK(state.Begin(3, 0.0) == NoteTypeStatus::GenerationNotNew);
    CHECK(state.Begin(2, 0.0) == NoteTypeStatus::GenerationNotNew);
    CHECK(state.Begin(4, std::numeric_limits<double>::quiet_NaN()) ==
          NoteTypeStatus::NonFiniteTime);
    CHECK(state.Begin(4, 0.0) == NoteTypeStatus::Ok);

    // A press from another generation, one certified before it was captured and
    // a decision time later than the update time are all rejected.
    NativeNoteEvent foreign = Event(2, 1'000'000.0, NativeEventKind::Press, 0, 0, 3);
    CHECK(state.Press(foreign, 1'000'000.0).status == NoteTypeStatus::GenerationMismatch);
    CHECK(state.Press(Event(2, 1'000'000.0, NativeEventKind::Press, 0, 0, 4), 999'000.0)
              .status == NoteTypeStatus::WatermarkBehindCapture);
    CHECK(state.Advance(1'000'000.0, 1'000'001.0).status == NoteTypeStatus::WatermarkAhead);
    CHECK(state.Advance(1'000'000.0, 1'000'000.0).status == NoteTypeStatus::Ok);
    const NoteTypeResult backwards = state.Advance(900'000.0, 900'000.0);
    CHECK(backwards.status == NoteTypeStatus::BackwardTime);
    CHECK(state.Advance(1'000'000.0, std::numeric_limits<double>::infinity()).status ==
          NoteTypeStatus::NonFiniteTime);
    const NoteTypeResult press =
        state.Press(Event(2, 1'000'000.0, NativeEventKind::Press, 0, 0, 4), 1'000'000.0);
    CHECK(press.status == NoteTypeStatus::Ok);
    CHECK(press.count == 1); // Just at delta 0

    // Skipped when the note time is already past the segment start.
    CHECK(state.Begin(5, 1'500'000.0) == NoteTypeStatus::Ok);
    CHECK(state.Phase() == LongPhase::Skipped);
    const NoteTypeResult skipped =
        state.Press(Event(2, 1'500'000.0, NativeEventKind::Press, 0, 0, 5), 1'500'000.0);
    CHECK(skipped.count == 0 && skipped.ignored == 1);
}

void TestChartTypeAndEligibility() {
    using namespace nonnormal_layout;
    CHECK(ChartNoteTypeFromFlags(0) == ChartNoteType::Normal);
    CHECK(ChartNoteTypeFromFlags(kChartFlagLong) == ChartNoteType::Long);
    CHECK(ChartNoteTypeFromFlags(kChartFlagGliss) == ChartNoteType::Gliss);
    CHECK(ChartNoteTypeFromFlags(kChartFlagTrill) == ChartNoteType::Trill);
    // 1A4C01 precedence: gliss bit wins over trill and long, trill over long.
    CHECK(ChartNoteTypeFromFlags(kChartFlagLong | kChartFlagTrill) == ChartNoteType::Trill);
    CHECK(ChartNoteTypeFromFlags(kChartFlagLong | kChartFlagGliss) == ChartNoteType::Gliss);
    CHECK(ChartNoteTypeFromFlags(kChartFlagLong | kChartFlagGliss | kChartFlagTrill) ==
          ChartNoteType::Gliss);
    CHECK(IsChartNoteType(0) && IsChartNoteType(1) && IsChartNoteType(2) &&
          IsChartNoteType(4));
    CHECK(!IsChartNoteType(3));

    const NormalWindows windows = Windows();
    // Held/tail short-circuit 2F4E29 / 1A8499.
    CHECK(LongEligible(true, 1.0e9, windows));
    CHECK(TrillEligible(true, -1.0e9, windows));
    // Inclusive union of the three windows, inclusive on both ends.
    CHECK(LongEligible(false, kGood, windows));
    CHECK(LongEligible(false, -kGood, windows));
    CHECK(LongEligible(false, kGood + 0.5, windows) == false);
    CHECK(TrillEligible(false, kGreat, windows));
    CHECK(TrillEligible(false, -kGreat, windows));
    CHECK(TrillEligible(false, kGood + 1.0, windows) == false);
    CHECK(LongEligible(false, std::numeric_limits<double>::quiet_NaN(), windows) == false);
}

// ---------------------------------------------------------------------------
// Long / hold
// ---------------------------------------------------------------------------
void TestLongPressHoldFinish() {
    NativeLongState state;
    CHECK(state.Configure(LongSpec(), Config()) == NoteTypeStatus::Ok);
    CHECK(state.Begin(7, 0.0) == NoteTypeStatus::Ok);

    const NoteTypeResult press = state.Press(Event(2, 1'000'000.0), 1'000'000.0);
    CHECK(press.status == NoteTypeStatus::Ok);
    CHECK(OnlyOutcome(press, NoteTypeOutcomeKind::HeadCommitted));
    CHECK(press.outcomes[0].rank == NormalRank::Just);
    CHECK(press.outcomes[0].sound_started);
    CHECK(press.outcomes[0].sound_id == 10'001); // dword_1813E56E0 starts at 10000
    CHECK(press.outcomes[0].judged == false);
    CHECK(state.Phase() == LongPhase::Held);
    CHECK(state.ActiveSound() == 10'001);
    CHECK(state.RecordedDeltaUs() == 0.0);

    // Before the end nothing happens.
    const NoteTypeResult idle = state.Advance(1'500'000.0, 1'500'000.0);
    CHECK(idle.count == 0);

    const NoteTypeResult finish = state.Advance(2'000'000.0, 2'000'000.0);
    CHECK(OnlyOutcome(finish, NoteTypeOutcomeKind::Scored));
    CHECK(finish.outcomes[0].rank == NormalRank::Just);
    CHECK(finish.outcomes[0].scale == 100);
    CHECK(finish.outcomes[0].judged);
    CHECK(finish.outcomes[0].sound_cut == false); // rank >= Great keeps the sound
    CHECK(state.Phase() == LongPhase::Finished);
    CHECK(state.FinalRank() == NormalRank::Just);
    CHECK(state.Done());
    // The sound stays active for a later release cut.
    CHECK(state.ActiveSound() == 10'001);

    // A press or update after completion changes nothing.
    const NoteTypeResult after = state.Press(Event(2, 2'100'000.0), 2'100'000.0);
    CHECK(after.count == 0 && after.ignored == 1);
    CHECK(state.FinalRank() == NormalRank::Just);
}

void TestLongPendingEarlyAndResolution() {
    NativeLongState state;
    CHECK(state.Configure(LongSpec(), Config()) == NoteTypeStatus::Ok);
    CHECK(state.Begin(7, 0.0) == NoteTypeStatus::Ok);

    // 0 < delta <= great_early defers with the recorded rank Great(3).
    const NoteTypeResult pending = state.Press(Event(2, 950'000.0), 950'000.0);
    CHECK(OnlyOutcome(pending, NoteTypeOutcomeKind::PendingEarly));
    CHECK(pending.outcomes[0].rank == NormalRank::Great);
    CHECK(state.Phase() == LongPhase::PendingEarly);
    CHECK(state.ActiveSound() == 0); // no sound until the start is committed

    // A second in-window press overwrites the pending state, still as Great.
    const NoteTypeResult again = state.Press(Event(2, 960'000.0), 960'000.0);
    CHECK(OnlyOutcome(again, NoteTypeOutcomeKind::PendingEarly));
    CHECK(again.outcomes[0].rank == NormalRank::Great);

    // Song time reaching note - judgment offset commits the start, no sound.
    const NoteTypeResult resolve = state.Advance(1'000'000.0, 1'000'000.0);
    CHECK(OnlyOutcome(resolve, NoteTypeOutcomeKind::HeadCommitted));
    CHECK(resolve.outcomes[0].rank == NormalRank::Great);
    CHECK(resolve.outcomes[0].sound_started == false);
    CHECK(state.Phase() == LongPhase::Held);

    const NoteTypeResult finish = state.Advance(2'000'000.0, 2'000'000.0);
    CHECK(finish.count == 1 && finish.outcomes[0].kind == NoteTypeOutcomeKind::Scored);
    CHECK(finish.outcomes[0].rank == NormalRank::Great);
    CHECK(finish.outcomes[0].scale == 100);
    CHECK(state.PenaltyCount() == 0);
}

void TestLongPendingResolvedByPress() {
    NativeLongState state;
    CHECK(state.Configure(LongSpec(), Config()) == NoteTypeStatus::Ok);
    CHECK(state.Begin(7, 0.0) == NoteTypeStatus::Ok);
    CHECK(state.Press(Event(2, 950'000.0), 950'000.0).outcomes[0].rank == NormalRank::Great);
    // A press at the note time resolves the pending start and is consumed by it.
    const NoteTypeResult resolve = state.Press(Event(2, 1'000'000.0), 1'000'000.0);
    CHECK(OnlyOutcome(resolve, NoteTypeOutcomeKind::HeadCommitted));
    CHECK(resolve.outcomes[0].rank == NormalRank::Great);
    CHECK(resolve.outcomes[0].sound_started); // committed by a real event -> serial
    CHECK(resolve.outcomes[0].sound_id == 10'001);
}

void TestLongGoodHeadAndPenalty() {
    NativeLongState state;
    CHECK(state.Configure(LongSpec(), Config()) == NoteTypeStatus::Ok);
    CHECK(state.Begin(7, 0.0) == NoteTypeStatus::Ok);
    // Great_early < delta <= good_early: accepted immediately as Good(2).
    const NoteTypeResult press = state.Press(Event(2, 900'000.0), 900'000.0);
    CHECK(OnlyOutcome(press, NoteTypeOutcomeKind::HeadCommitted));
    CHECK(press.outcomes[0].rank == NormalRank::Good);
    const NoteTypeResult finish = state.Advance(2'000'000.0, 2'000'000.0);
    CHECK(finish.count == 2);
    CHECK(finish.outcomes[0].kind == NoteTypeOutcomeKind::Scored);
    CHECK(finish.outcomes[0].rank == NormalRank::Good);
    CHECK(finish.outcomes[0].scale == 100);
    CHECK(finish.outcomes[0].sound_cut); // rank < Great fades the sound
    CHECK(finish.outcomes[1].kind == NoteTypeOutcomeKind::KeysoundCut);
    CHECK(finish.penalties == 1); // native counter follows the assigned rank
    CHECK(state.PenaltyCount() == 1);
    CHECK(state.ActiveSound() == 0);
}

void TestLongLatePressIsMissed() {
    NativeLongState state;
    CHECK(state.Configure(LongSpec(), Config()) == NoteTypeStatus::Ok);
    CHECK(state.Begin(7, 0.0) == NoteTypeStatus::Ok);
    // Past the inclusive Good-late window by 1 us.
    const NoteTypeResult press = state.Press(Event(2, 1'000'000.0 + kGood + 1.0),
                                            1'000'000.0 + kGood + 1.0);
    CHECK(press.count == 3);
    CHECK(press.outcomes[0].kind == NoteTypeOutcomeKind::HeadCommitted);
    CHECK(press.outcomes[0].rank == NormalRank::Missed);
    CHECK(press.outcomes[0].sound_started);
    CHECK(press.outcomes[1].kind == NoteTypeOutcomeKind::Missed);
    CHECK(press.outcomes[1].rank == NormalRank::Missed);
    CHECK(press.outcomes[1].scale == 100);
    CHECK(press.outcomes[1].sound_cut);
    CHECK(press.outcomes[2].kind == NoteTypeOutcomeKind::KeysoundCut);
    CHECK(state.Phase() == LongPhase::Missed);
    CHECK(state.FinalRank() == NormalRank::Missed);
}

void TestLongTooEarlyPressIgnored() {
    NativeLongState state;
    CHECK(state.Configure(LongSpec(), Config()) == NoteTypeStatus::Ok);
    CHECK(state.Begin(7, 0.0) == NoteTypeStatus::Ok);
    const NoteTypeResult press = state.Press(Event(2, 800'000.0), 800'000.0);
    CHECK(press.count == 0 && press.ignored == 1);
    CHECK(state.Phase() == LongPhase::Waiting);
}

void TestLongReleaseGateAndPenalties() {
    // Gate not proven: the release is refused and nothing changes.
    {
        NativeLongState state;
        CHECK(state.Configure(LongSpec(), Config()) == NoteTypeStatus::Ok);
        CHECK(state.Begin(7, 0.0) == NoteTypeStatus::Ok);
        CHECK(state.Press(Event(2, 1'000'000.0), 1'000'000.0).count == 1);
        const NoteTypeResult release =
            state.Release(Event(2, 1'500'000.0, NativeEventKind::Release), 1'500'000.0);
        CHECK(OnlyOutcome(release, NoteTypeOutcomeKind::Unsupported));
        CHECK(release.outcomes[0].reason != nullptr);
        CHECK(release.unsupported == 1);
        CHECK(state.Phase() == LongPhase::Held);
        CHECK(state.ActiveSound() == 10'001);
    }
    // Gate proven: a release more than 15 units before the end is Good/50.
    {
        NativeLongState state;
        CHECK(state.Configure(LongSpec(), ProvenConfig()) == NoteTypeStatus::Ok);
        CHECK(state.Begin(7, 0.0) == NoteTypeStatus::Ok);
        CHECK(state.Press(Event(2, 1'000'000.0), 1'000'000.0).count == 1);
        const double at = 2'000'000.0 - kTolerance60 - 1'000.0;
        const NoteTypeResult release =
            state.Release(Event(2, at, NativeEventKind::Release), at);
        CHECK(release.count == 2);
        CHECK(release.outcomes[0].kind == NoteTypeOutcomeKind::KeysoundCut);
        CHECK(release.outcomes[0].sound_cut);
        CHECK(release.outcomes[1].kind == NoteTypeOutcomeKind::Scored);
        CHECK(release.outcomes[1].rank == NormalRank::Good);
        CHECK(release.outcomes[1].scale == 50);
        CHECK(release.outcomes[1].effect_grade == 1);
        CHECK(release.penalties == 1);
        CHECK(state.Phase() == LongPhase::Finished);
        CHECK(state.ActiveSound() == 0);
    }
    // Inside the tolerance band the head rank and scale 100 survive.
    {
        NativeLongState state;
        CHECK(state.Configure(LongSpec(), ProvenConfig()) == NoteTypeStatus::Ok);
        CHECK(state.Begin(7, 0.0) == NoteTypeStatus::Ok);
        CHECK(state.Press(Event(2, 1'000'000.0), 1'000'000.0).count == 1);
        const double at = 2'000'000.0 - kTolerance60 + 1'000.0;
        const NoteTypeResult release =
            state.Release(Event(2, at, NativeEventKind::Release), at);
        CHECK(release.count == 2);
        CHECK(release.outcomes[1].kind == NoteTypeOutcomeKind::Scored);
        CHECK(release.outcomes[1].rank == NormalRank::Just);
        CHECK(release.outcomes[1].scale == 100);
        CHECK(release.penalties == 0);
    }
    // A release after the end still completes with the head rank and 100.
    {
        NativeLongState state;
        CHECK(state.Configure(LongSpec(), ProvenConfig()) == NoteTypeStatus::Ok);
        CHECK(state.Begin(7, 0.0) == NoteTypeStatus::Ok);
        CHECK(state.Press(Event(2, 1'000'000.0), 1'000'000.0).count == 1);
        const NoteTypeResult release = state.Release(
            Event(2, 2'000'100.0, NativeEventKind::Release), 2'000'100.0);
        CHECK(release.count == 2);
        CHECK(release.outcomes[1].rank == NormalRank::Just);
        CHECK(release.outcomes[1].scale == 100);
        CHECK(release.outcomes[1].remaining_us < 0.0);
    }
}

void TestLongHoldToEndIsAMissedRelease() {
    // Releasing never: the note finishes at its end with the head rank. This is
    // the native LABEL_36 fall-through, distinct from an early release.
    NativeLongState state;
    CHECK(state.Configure(LongSpec(), Config()) == NoteTypeStatus::Ok);
    CHECK(state.Begin(7, 0.0) == NoteTypeStatus::Ok);
    CHECK(state.Press(Event(2, 1'000'000.0), 1'000'000.0).count == 1);
    const NoteTypeResult finish = state.Advance(2'010'000.0, 2'010'000.0);
    CHECK(finish.count == 1);
    CHECK(finish.outcomes[0].kind == NoteTypeOutcomeKind::Scored);
    CHECK(finish.outcomes[0].rank == NormalRank::Just);
    CHECK(finish.outcomes[0].scale == 100);
    CHECK(finish.outcomes[0].remaining_us < 0.0);
    // A release afterwards is inert.
    const NoteTypeResult release =
        state.Release(Event(2, 2'100'000.0, NativeEventKind::Release), 2'100'000.0);
    CHECK(release.count == 0 && release.ignored == 1);
}

void TestLongStaleHoldArm() {
    NativeLongState state;
    CHECK(state.Configure(LongSpec(), Config()) == NoteTypeStatus::Ok);
    CHECK(state.Begin(7, 0.0) == NoteTypeStatus::Ok);
    CHECK(state.Press(Event(2, 1'000'000.0), 1'000'000.0).count == 1);
    // +313 armed by the update while the hold is far from its end.
    const NoteTypeResult stale = state.Advance(1'100'000.0, 1'100'000.0, true);
    CHECK(stale.count == 2);
    CHECK(stale.outcomes[0].kind == NoteTypeOutcomeKind::Scored);
    CHECK(stale.outcomes[0].rank == NormalRank::Good);
    CHECK(stale.outcomes[0].scale == 50);
    CHECK(stale.outcomes[1].kind == NoteTypeOutcomeKind::KeysoundCut);
    CHECK(state.Phase() == LongPhase::Finished);
}

void TestLongAutoAndRepeatAndExpiry() {
    {
        NativeLongState state;
        CHECK(state.Configure(LongSpec(2, 1'000'000.0, 2'000'000.0, true), Config()) ==
              NoteTypeStatus::Ok);
        CHECK(state.Begin(7, 0.0) == NoteTypeStatus::Ok);
        // An auto note ignores a press before its note time entirely.
        CHECK(state.Press(Event(2, 900'000.0), 900'000.0).ignored == 1);
        CHECK(state.Phase() == LongPhase::Waiting);
        const NoteTypeResult press = state.Press(Event(2, 1'000'000.0), 1'000'000.0);
        CHECK(press.count == 1);
        CHECK(press.outcomes[0].sound_started == false); // auto notes start no sound
        const NoteTypeResult finish = state.Advance(2'000'000.0, 2'000'000.0);
        CHECK(finish.count == 1);
        CHECK(finish.outcomes[0].kind == NoteTypeOutcomeKind::Auto);
        CHECK(finish.outcomes[0].rank == NormalRank::Just);
        CHECK(finish.outcomes[0].scale == 100);
    }
    {
        // The same auto head is committed by the null update when no press comes.
        NativeLongState state;
        CHECK(state.Configure(LongSpec(2, 1'000'000.0, 2'000'000.0, true), Config()) ==
              NoteTypeStatus::Ok);
        CHECK(state.Begin(7, 0.0) == NoteTypeStatus::Ok);
        const NoteTypeResult commit = state.Advance(1'000'000.0, 1'000'000.0);
        CHECK(OnlyOutcome(commit, NoteTypeOutcomeKind::HeadCommitted));
        CHECK(commit.outcomes[0].rank == NormalRank::Just);
        const NoteTypeResult finish = state.Advance(2'000'000.0, 2'000'000.0);
        CHECK(OnlyOutcome(finish, NoteTypeOutcomeKind::Auto));
    }
    {
        NativeLongState state;
        CHECK(state.Configure(LongSpec(), Config()) == NoteTypeStatus::Ok);
        CHECK(state.Begin(7, 0.0) == NoteTypeStatus::Ok);
        CHECK(state.Press(Event(2, 1'000'000.0), 1'000'000.0).count == 1);
        const NoteTypeResult repeat =
            state.Press(Event(2, 1'300'000.0, NativeEventKind::Repeat, 42), 1'300'000.0);
        CHECK(OnlyOutcome(repeat, NoteTypeOutcomeKind::RepeatIgnored));
        CHECK(repeat.repeats == 1);
        CHECK(state.Phase() == LongPhase::Held);
        CHECK(state.ActiveSound() == 10'001);
    }
    {
        NativeLongState state;
        CHECK(state.Configure(LongSpec(), Config()) == NoteTypeStatus::Ok);
        CHECK(state.Begin(7, 0.0) == NoteTypeStatus::Ok);
        // Unpressed head: the adapter reports the drop only after the inclusive
        // Good-late deadline, judged false, and only if both clocks are past it.
        const double deadline = 1'000'000.0 + kGood;
        CHECK(state.Advance(deadline, deadline).count == 0);
        const NoteTypeResult expired = state.Advance(deadline + 1.0, deadline + 1.0);
        CHECK(OnlyOutcome(expired, NoteTypeOutcomeKind::Missed));
        CHECK(expired.outcomes[0].judged == false);
        CHECK(expired.outcomes[0].reason != nullptr);
        CHECK(state.Phase() == LongPhase::Missed);
    }
}

void TestLongSoundSerialWrap() {
    NativeLongSpec spec = LongSpec(2, 1'000'000.0, 1'000'001.0);
    NativeTypeConfig config = Config();
    config.sound_serial_start = 10'000;
    config.sound_serial_limit = 10'001;
    NativeLongState state;
    CHECK(state.Configure(spec, config) == NoteTypeStatus::Ok);
    // Three segments in a row: 10001, then the wrap to 10000, then 10001.
    CHECK(state.Begin(1, 0.0) == NoteTypeStatus::Ok);
    CHECK(state.Press(Event(2, 1'000'000.0, NativeEventKind::Press, 0, 0, 1), 1'000'000.0)
              .outcomes[0].sound_id == 10'001);
    CHECK(state.Begin(2, 0.0) == NoteTypeStatus::Ok);
    CHECK(state.Press(Event(2, 1'000'000.0, NativeEventKind::Press, 0, 0, 2), 1'000'000.0)
              .outcomes[0].sound_id == 10'000);
    CHECK(state.Begin(3, 0.0) == NoteTypeStatus::Ok);
    CHECK(state.Press(Event(2, 1'000'000.0, NativeEventKind::Press, 0, 0, 3), 1'000'000.0)
              .outcomes[0].sound_id == 10'001);
}

// ---------------------------------------------------------------------------
// Glissando
// ---------------------------------------------------------------------------
void TestGlissPressAtNote() {
    NativeGlissState state;
    CHECK(state.Configure(GlissSpec(2), Config()) == NoteTypeStatus::Ok);
    CHECK(state.Begin(7, 0.0) == NoteTypeStatus::Ok);
    const NoteTypeResult press = state.Press(Event(5, 1'000'000.0, NativeEventKind::Press, 77), 1'000'000.0);
    CHECK(press.count == 2);
    CHECK(press.outcomes[0].kind == NoteTypeOutcomeKind::Scored);
    CHECK(press.outcomes[0].rank == NormalRank::Just);
    CHECK(press.outcomes[0].scale == 100);
    CHECK(press.outcomes[0].judged);
    CHECK(press.outcomes[0].sound_started == false); // the judge starts no sound
    CHECK(press.outcomes[1].kind == NoteTypeOutcomeKind::ConnectorReleased);
    CHECK(press.outcomes[1].connectors == 2);
    CHECK(state.Phase() == GlissPhase::Finished);
    CHECK(state.Done());
    CHECK(state.FinalRank() == NormalRank::Just);
    // A second press cannot produce a second judgement.
    const NoteTypeResult again = state.Press(Event(5, 1'000'000.0), 1'000'000.0);
    CHECK(again.count == 0 && again.ignored == 1);
}

void TestGlissPendingAndConnectorVariants() {
    {
        NativeGlissState state;
        CHECK(state.Configure(GlissSpec(1), Config()) == NoteTypeStatus::Ok);
        CHECK(state.Begin(7, 0.0) == NoteTypeStatus::Ok);
        const NoteTypeResult press = state.Press(Event(5, 960'000.0, NativeEventKind::Press, 5), 960'000.0);
        CHECK(OnlyOutcome(press, NoteTypeOutcomeKind::PendingEarly));
        CHECK(press.outcomes[0].rank == NormalRank::Just); // always recorded as 4
        CHECK(state.Phase() == GlissPhase::PendingEarly);
        const NoteTypeResult resolve = state.Advance(1'000'000.0, 1'000'000.0);
        CHECK(resolve.count == 2);
        CHECK(resolve.outcomes[0].kind == NoteTypeOutcomeKind::Scored);
        CHECK(resolve.outcomes[0].rank == NormalRank::Just);
        CHECK(resolve.outcomes[1].connectors == 1);
    }
    {
        NativeGlissState state;
        CHECK(state.Configure(GlissSpec(0), Config()) == NoteTypeStatus::Ok);
        CHECK(state.Begin(7, 0.0) == NoteTypeStatus::Ok);
        const NoteTypeResult press = state.Press(Event(5, 1'000'000.0), 1'000'000.0);
        CHECK(OnlyOutcome(press, NoteTypeOutcomeKind::Scored));
    }
    {
        NativeGlissState state;
        CHECK(state.Configure(GlissSpec(2), Config()) == NoteTypeStatus::Ok);
        CHECK(state.Begin(7, 0.0) == NoteTypeStatus::Ok);
        const NoteTypeResult press =
            state.Press(Event(5, 1'000'000.0 + kGood + 1.0), 1'000'000.0 + kGood + 1.0);
        CHECK(press.count == 2);
        CHECK(press.outcomes[0].kind == NoteTypeOutcomeKind::Missed);
        CHECK(press.outcomes[0].rank == NormalRank::Missed);
        CHECK(press.outcomes[0].scale == 100);
        CHECK(press.outcomes[1].kind == NoteTypeOutcomeKind::ConnectorReleased);
    }
    {
        NativeGlissState state;
        CHECK(state.Configure(GlissSpec(0, true), Config()) == NoteTypeStatus::Ok);
        CHECK(state.Begin(7, 0.0) == NoteTypeStatus::Ok);
        // 1A60E7: an auto glissando ignores every event and completes at its
        // note time as Just(4)/100.
        const NoteTypeResult press = state.Press(Event(5, 1'000'000.0), 1'000'000.0);
        CHECK(press.count == 0 && press.ignored == 1);
        CHECK(state.Phase() == GlissPhase::Waiting);
        const NoteTypeResult auto_finish = state.Advance(1'000'000.0, 1'000'000.0);
        CHECK(OnlyOutcome(auto_finish, NoteTypeOutcomeKind::Auto));
        CHECK(auto_finish.outcomes[0].rank == NormalRank::Just);
        CHECK(state.Phase() == GlissPhase::Finished);
    }
    {
        NativeGlissState state;
        CHECK(state.Configure(GlissSpec(0, true), Config()) == NoteTypeStatus::Ok);
        CHECK(state.Begin(7, 0.0) == NoteTypeStatus::Ok);
        CHECK(state.Advance(900'000.0, 900'000.0).count == 0); // still early: inert
        const NoteTypeResult auto_finish = state.Advance(1'000'000.0, 1'000'000.0);
        CHECK(OnlyOutcome(auto_finish, NoteTypeOutcomeKind::Auto));
    }
}

void TestGlissRefusedBranches() {
    // Event kind 2 (repeat) needs the shared id list: refused by default.
    {
        NativeGlissState state;
        CHECK(state.Configure(GlissSpec(0), Config()) == NoteTypeStatus::Ok);
        CHECK(state.Begin(7, 0.0) == NoteTypeStatus::Ok);
        const NoteTypeResult repeat =
            state.Press(Event(5, 1'000'000.0, NativeEventKind::Repeat, 9), 1'000'000.0);
        CHECK(OnlyOutcome(repeat, NoteTypeOutcomeKind::Unsupported));
        CHECK(repeat.outcomes[0].reason != nullptr);
        CHECK(repeat.unsupported == 1 && repeat.repeats == 1);
        CHECK(state.Phase() == GlissPhase::Waiting);
    }
    // The caller owns the list: kind 2 then behaves like a press.
    {
        NativeTypeConfig config = Config();
        config.gliss_repeat_as_press = true;
        NativeGlissState state;
        CHECK(state.Configure(GlissSpec(0), config) == NoteTypeStatus::Ok);
        CHECK(state.Begin(7, 0.0) == NoteTypeStatus::Ok);
        const NoteTypeResult repeat =
            state.Press(Event(5, 1'000'000.0, NativeEventKind::Repeat, 9), 1'000'000.0);
        CHECK(OnlyOutcome(repeat, NoteTypeOutcomeKind::Scored));
        CHECK(repeat.outcomes[0].rank == NormalRank::Just);
    }
    // The connector table scan is refused once, then silent.
    {
        NativeGlissState state;
        CHECK(state.Configure(GlissSpec(0), Config()) == NoteTypeStatus::Ok);
        CHECK(state.Begin(7, 0.0) == NoteTypeStatus::Ok);
        const NoteTypeResult first = state.Advance(1'000'000.0, 1'000'000.0);
        CHECK(OnlyOutcome(first, NoteTypeOutcomeKind::Unsupported));
        CHECK(first.outcomes[0].reason != nullptr);
        const NoteTypeResult second = state.Advance(1'010'000.0, 1'010'000.0);
        CHECK(second.count == 0 && second.ignored == 1);
        CHECK(state.Phase() == GlissPhase::Waiting);
    }
    // With the scan proven, the caller's hit decides the Just judgement.
    {
        NativeTypeConfig config = Config();
        config.gliss_connector_scan_proven = true;
        NativeGlissState state;
        CHECK(state.Configure(GlissSpec(0), config) == NoteTypeStatus::Ok);
        CHECK(state.Begin(7, 0.0) == NoteTypeStatus::Ok);
        CHECK(state.Advance(1'000'000.0, 1'000'000.0, false).count == 0);
        const NoteTypeResult hit = state.Advance(1'010'000.0, 1'010'000.0, true);
        CHECK(OnlyOutcome(hit, NoteTypeOutcomeKind::Scored));
        CHECK(hit.outcomes[0].rank == NormalRank::Just);
    }
    // An unpressed glissando is dropped by the adapter after the inclusive
    // Good-late deadline, and only once both clocks are past it.
    {
        NativeGlissState state;
        CHECK(state.Configure(GlissSpec(0), Config()) == NoteTypeStatus::Ok);
        CHECK(state.Begin(7, 0.0) == NoteTypeStatus::Ok);
        const double deadline = 1'000'000.0 + kGood;
        // At the inclusive deadline the note is not yet dropped; the post-note
        // connector scan is what the model refuses first.
        const NoteTypeResult scan = state.Advance(deadline, deadline);
        CHECK(OnlyOutcome(scan, NoteTypeOutcomeKind::Unsupported));
        CHECK(state.Phase() == GlissPhase::Waiting);
        const NoteTypeResult expired = state.Advance(deadline + 1.0, deadline + 1.0);
        CHECK(OnlyOutcome(expired, NoteTypeOutcomeKind::Missed));
        CHECK(expired.outcomes[0].judged == false);
        CHECK(state.Phase() == GlissPhase::Missed);
    }
    // Group id 1 is the wildcard; any other id must match event+5.
    {
        NativeGlissState state;
        CHECK(state.Configure(GlissSpec(0, false, 5), Config()) == NoteTypeStatus::Ok);
        CHECK(state.Begin(7, 0.0) == NoteTypeStatus::Ok);
        const NoteTypeResult mismatch = state.Press(Event(5, 1'000'000.0, NativeEventKind::Press, 1, 7), 1'000'000.0);
        CHECK(mismatch.count == 0 && mismatch.ignored == 1);
        const NoteTypeResult match = state.Press(Event(5, 1'000'000.0, NativeEventKind::Press, 1, 5), 1'000'000.0);
        CHECK(OnlyOutcome(match, NoteTypeOutcomeKind::Scored));
    }
    // A release fades an event-owned sound and never judges.
    {
        NativeGlissState state;
        CHECK(state.Configure(GlissSpec(0), Config()) == NoteTypeStatus::Ok);
        CHECK(state.Begin(7, 0.0) == NoteTypeStatus::Ok);
        const NoteTypeResult release =
            state.Release(Event(5, 1'050'000.0, NativeEventKind::Release, 321), 1'050'000.0);
        CHECK(OnlyOutcome(release, NoteTypeOutcomeKind::KeysoundCut));
        CHECK(release.outcomes[0].sound_id == 321);
        CHECK(state.Phase() == GlissPhase::Waiting);
        CHECK(state.Release(Event(5, 1'060'000.0, NativeEventKind::Release, 0), 1'060'000.0).count == 0);
    }
}

// ---------------------------------------------------------------------------
// Trill
// ---------------------------------------------------------------------------
void TestTrillHeadTail() {
    NativeTrillState state;
    CHECK(state.Configure(TrillSpec(), Config()) == NoteTypeStatus::Ok);
    CHECK(state.Begin(7, 0.0) == NoteTypeStatus::Ok);
    const NoteTypeResult head = state.Press(Event(9, 1'000'000.0), 1'000'000.0);
    CHECK(OnlyOutcome(head, NoteTypeOutcomeKind::HeadCommitted));
    CHECK(head.outcomes[0].rank == NormalRank::Just);
    CHECK(head.outcomes[0].sound_started);
    CHECK(state.Phase() == TrillPhase::Tail);
    CHECK(state.AlternationCounter() == 99999);
    const NoteTypeResult tail = state.Advance(2'000'000.0, 2'000'000.0);
    CHECK(OnlyOutcome(tail, NoteTypeOutcomeKind::Scored));
    CHECK(tail.outcomes[0].rank == NormalRank::Just);
    CHECK(tail.outcomes[0].scale == 100);
    CHECK(tail.outcomes[0].sound_cut == false);
    CHECK(state.Phase() == TrillPhase::Finished);
    CHECK(state.FinalRank() == NormalRank::Just);
}

void TestTrillAlternationCounter() {
    NativeTrillState state;
    CHECK(state.Configure(TrillSpec(), Config()) == NoteTypeStatus::Ok);
    CHECK(state.Begin(7, 0.0) == NoteTypeStatus::Ok);
    CHECK(state.Press(Event(9, 1'000'000.0), 1'000'000.0).count == 1);
    state.SetAlternationCounter(0);
    const NoteTypeResult tail = state.Advance(2'000'000.0, 2'000'000.0);
    CHECK(tail.count == 2);
    CHECK(tail.outcomes[0].kind == NoteTypeOutcomeKind::Scored);
    CHECK(tail.outcomes[0].rank == NormalRank::Good);
    CHECK(tail.outcomes[0].scale == 40);
    CHECK(tail.outcomes[0].effect_grade == 1);
    CHECK(tail.outcomes[0].sound_cut);
    CHECK(tail.outcomes[1].kind == NoteTypeOutcomeKind::KeysoundCut);
}

void TestTrillPendingAndLate() {
    {
        NativeTrillState state;
        CHECK(state.Configure(TrillSpec(), Config()) == NoteTypeStatus::Ok);
        CHECK(state.Begin(7, 0.0) == NoteTypeStatus::Ok);
        const NoteTypeResult pending = state.Press(Event(9, 950'000.0), 950'000.0);
        CHECK(OnlyOutcome(pending, NoteTypeOutcomeKind::PendingEarly));
        CHECK(pending.outcomes[0].rank == NormalRank::Great);
        CHECK(state.Phase() == TrillPhase::PendingEarly);
        const NoteTypeResult resolve = state.Advance(1'000'000.0, 1'000'000.0);
        CHECK(OnlyOutcome(resolve, NoteTypeOutcomeKind::HeadCommitted));
        CHECK(resolve.outcomes[0].rank == NormalRank::Great);
        CHECK(resolve.outcomes[0].sound_started == false);
        const NoteTypeResult tail = state.Advance(2'000'000.0, 2'000'000.0);
        CHECK(tail.outcomes[0].rank == NormalRank::Great);
        CHECK(tail.outcomes[0].scale == 100);
    }
    {
        NativeTrillState state;
        CHECK(state.Configure(TrillSpec(), Config()) == NoteTypeStatus::Ok);
        CHECK(state.Begin(7, 0.0) == NoteTypeStatus::Ok);
        const NoteTypeResult late_delta = state.Press(Event(9, 800'000.0), 800'000.0);
        CHECK(late_delta.count == 0 && late_delta.ignored == 1); // too early: inert
        const NoteTypeResult late =
            state.Press(Event(9, 1'000'000.0 + kGood + 1.0), 1'000'000.0 + kGood + 1.0);
        CHECK(late.count == 3);
        CHECK(late.outcomes[0].kind == NoteTypeOutcomeKind::HeadCommitted);
        CHECK(late.outcomes[0].rank == NormalRank::Missed);
        CHECK(late.outcomes[1].kind == NoteTypeOutcomeKind::Missed);
        CHECK(late.outcomes[1].scale == 100);
        CHECK(late.outcomes[2].kind == NoteTypeOutcomeKind::KeysoundCut);
        CHECK(state.FinalRank() == NormalRank::Missed);
    }
}

void TestTrillTailEvents() {
    // A tail press is refused without the undecoded 0x1A7910 handler.
    {
        NativeTrillState state;
        CHECK(state.Configure(TrillSpec(), Config()) == NoteTypeStatus::Ok);
        CHECK(state.Begin(7, 0.0) == NoteTypeStatus::Ok);
        CHECK(state.Press(Event(9, 1'000'000.0), 1'000'000.0).count == 1);
        const NoteTypeResult tail_press = state.Press(Event(9, 1'500'000.0), 1'500'000.0);
        CHECK(OnlyOutcome(tail_press, NoteTypeOutcomeKind::Unsupported));
        CHECK(tail_press.outcomes[0].reason != nullptr);
        CHECK(tail_press.unsupported == 1);
        CHECK(state.Phase() == TrillPhase::Tail);
        CHECK(state.ActiveSound() == 10'001);
        const NoteTypeResult tail = state.Advance(2'000'000.0, 2'000'000.0);
        CHECK(tail.outcomes[0].rank == NormalRank::Just);
    }
    // A tail release cuts the sound, counts a penalty and does not finish early.
    {
        NativeTrillState state;
        CHECK(state.Configure(TrillSpec(), Config()) == NoteTypeStatus::Ok);
        CHECK(state.Begin(7, 0.0) == NoteTypeStatus::Ok);
        CHECK(state.Press(Event(9, 1'000'000.0), 1'000'000.0).count == 1);
        const NoteTypeResult release =
            state.Release(Event(9, 1'500'000.0, NativeEventKind::Release), 1'500'000.0);
        CHECK(OnlyOutcome(release, NoteTypeOutcomeKind::KeysoundCut));
        CHECK(release.penalties == 1);
        CHECK(state.Phase() == TrillPhase::Tail);
        CHECK(state.ActiveSound() == 0);
        const NoteTypeResult tail = state.Advance(2'000'000.0, 2'000'000.0);
        CHECK(tail.outcomes[0].kind == NoteTypeOutcomeKind::Scored);
        CHECK(tail.outcomes[0].scale == 100);
    }
    // A release at the end finishes the tail.
    {
        NativeTrillState state;
        CHECK(state.Configure(TrillSpec(), Config()) == NoteTypeStatus::Ok);
        CHECK(state.Begin(7, 0.0) == NoteTypeStatus::Ok);
        CHECK(state.Press(Event(9, 1'000'000.0), 1'000'000.0).count == 1);
        const NoteTypeResult release =
            state.Release(Event(9, 2'000'000.0, NativeEventKind::Release), 2'000'000.0);
        CHECK(release.count == 2);
        CHECK(release.outcomes[0].kind == NoteTypeOutcomeKind::KeysoundCut);
        CHECK(release.outcomes[1].kind == NoteTypeOutcomeKind::Scored);
        CHECK(state.Phase() == TrillPhase::Finished);
    }
    // A kind-2 event is inert for a trill.
    {
        NativeTrillState state;
        CHECK(state.Configure(TrillSpec(), Config()) == NoteTypeStatus::Ok);
        CHECK(state.Begin(7, 0.0) == NoteTypeStatus::Ok);
        CHECK(state.Press(Event(9, 1'000'000.0), 1'000'000.0).count == 1);
        const NoteTypeResult repeat =
            state.Press(Event(9, 1'200'000.0, NativeEventKind::Repeat, 3), 1'200'000.0);
        CHECK(OnlyOutcome(repeat, NoteTypeOutcomeKind::RepeatIgnored));
        CHECK(repeat.repeats == 1);
        CHECK(state.Phase() == TrillPhase::Tail);
    }
    // Head expiry, and the auto tail.
    {
        NativeTrillState state;
        CHECK(state.Configure(TrillSpec(), Config()) == NoteTypeStatus::Ok);
        CHECK(state.Begin(7, 0.0) == NoteTypeStatus::Ok);
        const double deadline = 1'000'000.0 + kGood;
        CHECK(state.Advance(deadline, deadline).count == 0);
        const NoteTypeResult expired = state.Advance(deadline + 1.0, deadline + 1.0);
        CHECK(OnlyOutcome(expired, NoteTypeOutcomeKind::Missed));
        CHECK(expired.outcomes[0].judged == false);
    }
    {
        NativeTrillState state;
        CHECK(state.Configure(TrillSpec(9, 1'000'000.0, 2'000'000.0, true), Config()) ==
              NoteTypeStatus::Ok);
        CHECK(state.Begin(7, 0.0) == NoteTypeStatus::Ok);
        const NoteTypeResult head = state.Press(Event(9, 1'000'000.0), 1'000'000.0);
        CHECK(head.outcomes[0].sound_started == false);
        const NoteTypeResult tail = state.Advance(2'000'000.0, 2'000'000.0);
        CHECK(OnlyOutcome(tail, NoteTypeOutcomeKind::Auto));
        CHECK(tail.outcomes[0].rank == NormalRank::Just);
        CHECK(tail.outcomes[0].scale == 100);
    }
}

// ---------------------------------------------------------------------------
// Chords, rapid repeats and render-cadence invariance
// ---------------------------------------------------------------------------
void TestChordsAcrossLanes() {
    NativeLongState low;
    NativeLongState high;
    CHECK(low.Configure(LongSpec(3, 1'000'000.0, 2'000'000.0), Config()) == NoteTypeStatus::Ok);
    CHECK(high.Configure(LongSpec(4, 1'000'000.0, 2'000'000.0), Config()) == NoteTypeStatus::Ok);
    CHECK(low.Begin(7, 0.0) == NoteTypeStatus::Ok);
    CHECK(high.Begin(7, 0.0) == NoteTypeStatus::Ok);
    const NoteTypeResult a = low.Press(Event(3, 1'000'000.0), 1'000'000.0);
    const NoteTypeResult b = high.Press(Event(4, 1'000'000.0), 1'000'000.0);
    CHECK(a.count == 1 && b.count == 1);
    CHECK(a.outcomes[0].lane == 3 && b.outcomes[0].lane == 4);
    CHECK(a.outcomes[0].sound_id == 10'001 && b.outcomes[0].sound_id == 10'001);
    CHECK(low.Advance(2'000'000.0, 2'000'000.0).outcomes[0].rank == NormalRank::Just);
    CHECK(high.Advance(2'000'000.0, 2'000'000.0).outcomes[0].rank == NormalRank::Just);
}

// Drives one long note with a fixed press/release capture but a render cadence
// that varies from 60 to 360 Hz, with jitter and a 700 ms stall, and requires
// the same outcome sequence every time.
Sequence RunLongCadence(double hz, bool jitter, bool stall) {
    NativeLongState state;
    (void)state.Configure(LongSpec(2, 1'000'000.0, 3'000'000.0), ProvenConfig());
    (void)state.Begin(7, 0.0);
    const double period = 1'000'000.0 / hz;
    Sequence sequence;
    double now = 0.0;
    std::uint64_t step = 1;
    auto advance_to = [&](double target) {
        while (now + period <= target) {
            double next = now + period;
            if (jitter) {
                const double wobble = (static_cast<double>((step * 37u) % 11u) / 11.0 - 0.5) *
                                      period * 0.6;
                next = now + period + wobble;
                if (next <= now) next = now + period * 0.25;
            }
            if (stall && now < 1'200'000.0 && next > 1'200'000.0) {
                next = 1'900'000.0 + period; // one 700 ms stall
            }
            if (next >= target) break;
            const NoteTypeResult r = state.Advance(next, next);
            for (std::size_t i = 0; i < r.count; ++i) sequence.Append(KindName(r, i));
            now = next;
            ++step;
        }
    };
    advance_to(1'000'000.0);
    const NoteTypeResult press = state.Press(Event(2, 1'000'000.0), 1'000'000.0);
    for (std::size_t i = 0; i < press.count; ++i) sequence.Append(KindName(press, i));
    advance_to(2'800'000.0);
    const NoteTypeResult release =
        state.Release(Event(2, 2'800'000.0, NativeEventKind::Release), 2'800'000.0);
    for (std::size_t i = 0; i < release.count; ++i) sequence.Append(KindName(release, i));
    advance_to(3'100'000.0);
    return sequence;
}

void TestCadenceInvariance() {
    const double rates[] = {60.0, 90.0, 120.0, 144.0, 165.0, 240.0, 360.0};
    const Sequence expected = RunLongCadence(60.0, false, false);
    CHECK(expected.length > 0);
    for (double hz : rates) {
        const Sequence plain = RunLongCadence(hz, false, false);
        CHECK(plain.length == expected.length);
        for (std::size_t i = 0; i < plain.length && i < expected.length; ++i) {
            CHECK(plain.text[i] == expected.text[i]);
        }
        const Sequence shaky = RunLongCadence(hz, true, true);
        CHECK(shaky.length == expected.length);
        for (std::size_t i = 0; i < shaky.length && i < expected.length; ++i) {
            CHECK(shaky.text[i] == expected.text[i]);
        }
    }
    // The sequence is exactly: start commit, release cut, judged finish.
    CHECK(std::string_view(expected.text) == "head-committed,keysound-cut,scored,");
}

void TestOutcomeCapacityBound() {
    NativeLongState state;
    CHECK(state.Configure(LongSpec(), ProvenConfig()) == NoteTypeStatus::Ok);
    CHECK(state.Begin(7, 0.0) == NoteTypeStatus::Ok);
    const NoteTypeResult press = state.Press(Event(2, 1'000'000.0), 1'000'000.0);
    CHECK(press.count <= kNoteTypeMaxOutcomes);
    const NoteTypeResult release =
        state.Release(Event(2, 1'100'000.0, NativeEventKind::Release), 1'100'000.0);
    CHECK(release.count <= kNoteTypeMaxOutcomes);
}

// ---------------------------------------------------------------------------
// Bridge
// ---------------------------------------------------------------------------
void TestBridgeMapping() {
    TimedInputEvent timed;
    timed.input.lane = 17;
    timed.input.edge = InputEdge::Press;
    timed.input.velocity = 0.75f;
    timed.input.stamp.sequence = 12;
    timed.song_time.generation = 4;
    timed.song_time.microseconds = 500.0;

    NativeNoteEvent event;
    CHECK(MakeNativeNoteEvent(timed, 99, 3, event));
    CHECK(event.lane == 17);
    CHECK(event.kind == NativeEventKind::Press);
    CHECK(event.event_id == 99 && event.group_id == 3);
    CHECK(event.song_time.generation == 4);
    CHECK(event.song_time.microseconds == 500.0);
    CHECK(event.stamp.sequence == 12);
    CHECK(event.velocity == 0.75f);
    CHECK(event.panb_level == PanbLevelFromNormalized(0.75f));

    timed.input.edge = InputEdge::Release;
    timed.input.velocity = 0.0f;
    CHECK(MakeNativeNoteEvent(timed, 0, 0, event));
    CHECK(event.kind == NativeEventKind::Release);
    CHECK(event.panb_level == 0);

    timed.input.lane = 28;
    CHECK(MakeNativeNoteEvent(timed, 0, 0, event) == false);

    timed.input.lane = 0;
    timed.song_time.microseconds = std::numeric_limits<double>::quiet_NaN();
    CHECK(MakeNativeNoteEvent(timed, 0, 0, event) == false);
}

} // namespace

int main() {
    TestConfigurationAndStatus();
    TestChartTypeAndEligibility();
    TestLongPressHoldFinish();
    TestLongPendingEarlyAndResolution();
    TestLongPendingResolvedByPress();
    TestLongGoodHeadAndPenalty();
    TestLongLatePressIsMissed();
    TestLongTooEarlyPressIgnored();
    TestLongReleaseGateAndPenalties();
    TestLongHoldToEndIsAMissedRelease();
    TestLongStaleHoldArm();
    TestLongAutoAndRepeatAndExpiry();
    TestLongSoundSerialWrap();
    TestGlissPressAtNote();
    TestGlissPendingAndConnectorVariants();
    TestGlissRefusedBranches();
    TestTrillHeadTail();
    TestTrillAlternationCounter();
    TestTrillPendingAndLate();
    TestTrillTailEvents();
    TestChordsAcrossLanes();
    TestCadenceInvariance();
    TestOutcomeCapacityBound();
    TestBridgeMapping();

    if (g_failures == 0) {
        std::printf("PASS native_note_types_tests\n");
        return 0;
    }
    std::printf("FAILED native_note_types_tests (%d checks)\n", g_failures);
    return 1;
}
