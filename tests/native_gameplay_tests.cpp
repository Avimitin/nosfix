#ifdef NDEBUG
#undef NDEBUG
#endif
#include "nosfix/gameplay_clock.hpp"
#include "nosfix/input_normalizer.hpp"
#include "nosfix/native_gameplay.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <tuple>
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

constexpr std::int64_t kFrequency = 10'000'000;
constexpr std::int64_t kEpoch = 7'000'000'000'000'000'000LL;
constexpr std::uint64_t kGeneration = 1;
constexpr double kJudgment = 10'000.0;
constexpr double kEligibility = 10'000.0;

NormalWindows Windows() {
    NormalWindows windows;
    windows.just_early_us = 33'334.0;
    windows.just_late_us = 33'334.0;
    windows.great_early_us = 66'667.0;
    windows.great_late_us = 66'667.0;
    windows.good_early_us = 133'334.0;
    windows.good_late_us = 133'334.0;
    return windows;
}

NativeNoteConfig NoteConfig() {
    NativeNoteConfig config;
    config.windows = Windows();
    config.judgment_offset_us = kJudgment;
    config.eligibility_total_offset_us = kEligibility;
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

std::vector<NativeNoteSpec> Notes() {
    return {
        Note(0, 0, 1'000'000.0, 100),
        Note(1, 1, 1'300'000.0, 101),
        Note(2, 2, 1'600'000.0, 102),
        Note(3, 3, 1'600'000.0, 103),
        Note(4, 4, 2'000'000.0, 104),
        Note(5, 5, 2'400'000.0, 105),
        Note(6, 6, 2'800'000.0, 106),
        Note(7, 7, 3'200'000.0, 107),
        Note(8, 8, 3'600'000.0, 108, true),
        Note(9, 9, 4'000'000.0, 109),
    };
}

std::vector<ChartEvent> Chart() {
    std::vector<ChartEvent> events;
    for (std::uint32_t i = 0; i < 10; ++i) {
        ChartEvent event;
        event.id = 100 + i;
        event.song_us = 500'000.0 + static_cast<double>(i) * 400'000.0;
        event.kind = 1 + (i % 3);
        event.payload = i;
        events.push_back(event);
    }
    return events;
}

std::vector<ScheduledSound> Sounds() {
    ScheduledSound first;
    first.has_start = true;
    first.start_us = 800'000.0;
    first.sound_id = 200;
    first.velocity = 1.0f;
    first.has_release = true;
    first.release_us = 900'000.0;
    first.release_target_id = 200;

    ScheduledSound second;
    second.has_start = true;
    second.start_us = 2'000'000.0;
    second.sound_id = 201;
    second.velocity = 0.8f;

    ScheduledSound third;
    third.has_start = true;
    third.start_us = 3'300'000.0;
    third.sound_id = 202;
    third.velocity = 0.6f;
    third.has_release = true;
    third.release_us = 3'400'000.0;
    third.release_target_id = 202;
    return {first, second, third};
}

struct ScriptedInput final {
    double at_us = 0.0;
    std::uint8_t lane = 0;
    InputEdge edge = InputEdge::Press;
};

std::vector<ScriptedInput> Script() {
    return {
        {985'000.0, 0, InputEdge::Press},
        {1'250'000.0, 1, InputEdge::Press},
        {1'400'000.0, 1, InputEdge::Release},
        {1'600'000.0, 2, InputEdge::Press},
        {1'600'000.0, 3, InputEdge::Press},
        {1'950'000.0, 4, InputEdge::Press},
        {1'960'000.0, 4, InputEdge::Press},
        {2'523'334.0, 5, InputEdge::Press},
        {2'933'335.0, 6, InputEdge::Press},
        {3'190'000.0, 7, InputEdge::Press},
        {3'300'000.0, 0, InputEdge::Release},
        {3'500'000.0, 20, InputEdge::Press},
        {3'700'000.0, 2, InputEdge::Release},
    };
}

TimedInputEvent MakeEvent(const ScriptedInput& input, SongTime time,
                          std::uint64_t sequence) {
    TimedInputEvent event;
    event.input.lane = input.lane;
    event.input.edge = input.edge;
    event.input.stamp.sequence = sequence;
    event.input.stamp.stream_epoch = 11;
    event.input.stamp.ordinal = 0;
    event.song_time = time;
    return event;
}

struct OutcomeKey final {
    double at_us = 0.0;
    std::uint32_t note = 0;
    std::uint8_t kind = 0;
    std::uint8_t rank = 0;
    std::uint8_t grade = 0;
    std::uint32_t sound = 0;
    double input_us = 0.0;
};

struct NoteFinal final {
    std::uint8_t phase = 0;
    std::uint8_t rank = 0;
    int starts = 0;
    int cuts = 0;
};

struct SoundKey final {
    std::uint8_t action = 0;
    std::uint32_t sound = 0;
    double at_us = 0.0;
};

struct RunResult final {
    std::array<NoteFinal, 10> notes{};
    std::vector<std::uint64_t> chart_ids;
    std::vector<SoundKey> sounds;
    std::vector<OutcomeKey> outcomes;
    std::size_t scored = 0;
    std::size_t auto_completed = 0;
    std::size_t missed = 0;
    std::size_t cuts = 0;
    std::size_t ignored = 0;
    bool ok = true;
    bool monotonic = true;
};

void AppendRun(const GameplayAdvance& advance,
               const std::vector<NativeNoteOutcome>& note_buffer,
               const std::vector<ChartEvent>& chart_buffer,
               const std::vector<SoundCommand>& sound_buffer, RunResult& run) {
    for (std::size_t i = 0; i < advance.note_counts.written; ++i) {
        const NativeNoteOutcome& outcome = note_buffer[i];
        if (!run.outcomes.empty() &&
            outcome.at_us < run.outcomes.back().at_us - 1e-9) {
            run.monotonic = false;
        }
        run.outcomes.push_back(OutcomeKey{outcome.at_us, outcome.note_index,
                                          static_cast<std::uint8_t>(outcome.kind),
                                          static_cast<std::uint8_t>(outcome.rank),
                                          outcome.effect_grade, outcome.sound_id,
                                          outcome.input_us});
    }
    for (std::size_t i = 0; i < advance.chart_written; ++i) {
        run.chart_ids.push_back(chart_buffer[i].id);
    }
    for (std::size_t i = 0; i < advance.sounds_written; ++i) {
        run.sounds.push_back(SoundKey{static_cast<std::uint8_t>(sound_buffer[i].action),
                                      sound_buffer[i].sound_id, sound_buffer[i].at_us});
    }
    run.scored += advance.note_counts.scored;
    run.auto_completed += advance.note_counts.auto_completed;
    run.missed += advance.note_counts.missed;
    run.cuts += advance.note_counts.cuts;
    run.ignored += advance.note_counts.ignored_presses +
                   advance.note_counts.ignored_releases;
}

RunResult RunCadence(unsigned hertz) {
    RunResult run;
    NativeGameplay game;
    const std::vector<NativeNoteSpec> notes = Notes();
    const std::vector<ChartEvent> chart = Chart();
    const std::vector<ScheduledSound> sounds = Sounds();
    if (game.Configure(notes, chart, sounds, NoteConfig()) != GameplayStatus::Ok) {
        run.ok = false;
        return run;
    }

    GameplayClock clock;
    if (!clock.Start(kEpoch, kFrequency, 0.0)) {
        run.ok = false;
        return run;
    }
    const SongClockSegment segment = clock.Snapshot();
    if (game.Begin(kGeneration, 0.0) != GameplayStatus::Ok) {
        run.ok = false;
        return run;
    }

    const std::vector<ScriptedInput> script = Script();
    std::size_t next_input = 0;
    std::uint64_t sequence = 1;
    std::vector<NativeNoteOutcome> note_buffer(game.MaxOutcomesPerAdvance());
    std::vector<ChartEvent> chart_buffer(8);
    std::vector<SoundCommand> sound_buffer(4);

    double base = 0.0;
    double scaled = 0.0;
    std::uint64_t frame = 0;
    while (base <= 4'600'000.0) {
        const double jitter = static_cast<double>((frame * 7919U) % 2001U) * 0.5;
        const double now_us = base + jitter;
        const auto tick_qpc = kEpoch +
            static_cast<std::int64_t>(std::llround(now_us * 10.0));
        const auto now = clock.Now(tick_qpc);
        if (!now) {
            run.ok = false;
            return run;
        }

        while (next_input < script.size() &&
               script[next_input].at_us <= now_us + 1e-9) {
            const auto capture_qpc = kEpoch +
                static_cast<std::int64_t>(std::llround(script[next_input].at_us * 10.0));
            const auto captured = segment.Map(capture_qpc);
            if (!captured) {
                run.ok = false;
                return run;
            }
            const GameplayStatus submitted = game.SubmitInput(
                MakeEvent(script[next_input], *captured, sequence++));
            if (submitted != GameplayStatus::Ok) {
                run.ok = false;
                std::printf("cadence %u: submit failed: %s\n", hertz,
                            GameplayStatusName(submitted));
                return run;
            }
            ++next_input;
        }
        if (game.SetInputWatermark(*now) != GameplayStatus::Ok) {
            run.ok = false;
            return run;
        }
        const GameplayOutputs outputs{note_buffer, chart_buffer, sound_buffer};
        const GameplayAdvance advance = game.Advance(*now, outputs);
        if (!advance.ok()) {
            run.ok = false;
            std::printf("cadence %u: advance failed: %s\n", hertz,
                        GameplayStatusName(advance.status));
            return run;
        }
        AppendRun(advance, note_buffer, chart_buffer, sound_buffer, run);

        scaled += 1'000'000.0;
        const auto step = static_cast<std::int64_t>(scaled / hertz);
        scaled -= static_cast<double>(step * hertz);
        base += static_cast<double>(step);
        ++frame;
        if (frame == 40) base += 200'000.0; // one 200 ms presentation stall
    }

    // Drain any chart/sound backlog without adding input.
    for (int i = 0; i < 4; ++i) {
        const SongTime tail{base, kGeneration};
        if (game.SetInputWatermark(tail) != GameplayStatus::Ok) break;
        const GameplayOutputs outputs{note_buffer, chart_buffer, sound_buffer};
        const GameplayAdvance advance = game.Advance(tail, outputs);
        if (!advance.ok()) {
            run.ok = false;
            break;
        }
        AppendRun(advance, note_buffer, chart_buffer, sound_buffer, run);
        if (!advance.chart_backlog && !advance.sound_backlog) break;
    }

    for (std::size_t i = 0; i < notes.size(); ++i) {
        run.notes[i].phase = static_cast<std::uint8_t>(game.Notes().Phase(i));
        run.notes[i].rank = static_cast<std::uint8_t>(game.Notes().FinalRank(i));
    }
    for (const OutcomeKey& key : run.outcomes) {
        if (key.note < run.notes.size()) {
            if (key.kind == static_cast<std::uint8_t>(NoteOutcomeKind::KeysoundCut)) {
                ++run.notes[key.note].cuts;
            } else if (key.sound != 0) {
                ++run.notes[key.note].starts;
            }
        }
    }
    return run;
}

bool SameDouble(double a, double b) { return std::abs(a - b) < 1e-6; }

bool SameRun(const RunResult& a, const RunResult& b) {
    if (!a.ok || !b.ok) return false;
    if (a.chart_ids != b.chart_ids) return false;
    if (a.sounds.size() != b.sounds.size()) return false;
    for (std::size_t i = 0; i < a.sounds.size(); ++i) {
        if (a.sounds[i].action != b.sounds[i].action ||
            a.sounds[i].sound != b.sounds[i].sound ||
            !SameDouble(a.sounds[i].at_us, b.sounds[i].at_us)) {
            return false;
        }
    }
    if (a.outcomes.size() != b.outcomes.size()) return false;
    for (std::size_t i = 0; i < a.outcomes.size(); ++i) {
        const OutcomeKey& x = a.outcomes[i];
        const OutcomeKey& y = b.outcomes[i];
        if (!SameDouble(x.at_us, y.at_us) || x.note != y.note ||
            x.kind != y.kind || x.rank != y.rank || x.grade != y.grade ||
            x.sound != y.sound || !SameDouble(x.input_us, y.input_us)) {
            return false;
        }
    }
    for (std::size_t i = 0; i < a.notes.size(); ++i) {
        if (a.notes[i].phase != b.notes[i].phase ||
            a.notes[i].rank != b.notes[i].rank ||
            a.notes[i].starts != b.notes[i].starts ||
            a.notes[i].cuts != b.notes[i].cuts) {
            return false;
        }
    }
    return a.scored == b.scored && a.auto_completed == b.auto_completed &&
           a.missed == b.missed && a.cuts == b.cuts && a.ignored == b.ignored;
}

void TestCadenceInvariance() {
    const std::array<unsigned, 7> rates{60, 90, 120, 144, 165, 240, 360};
    const RunResult baseline = RunCadence(60);
    CHECK(baseline.ok);
    CHECK(baseline.monotonic);

    // Deterministic expected effects, not fabricated score points.
    CHECK(baseline.chart_ids.size() == 10);
    for (std::size_t i = 0; i < baseline.chart_ids.size(); ++i) {
        CHECK(baseline.chart_ids[i] == 100 + i);
    }
    CHECK(baseline.sounds.size() == 5);
    CHECK(baseline.scored == 7);
    CHECK(baseline.auto_completed == 1);
    CHECK(baseline.missed == 2);
    CHECK(baseline.cuts == 3);
    CHECK(baseline.outcomes.size() == 13);

    const std::array<std::pair<std::uint8_t, std::uint8_t>, 10> expected{{
        {static_cast<std::uint8_t>(NativeNotePhase::Completed),
         static_cast<std::uint8_t>(NormalRank::Just)},
        {static_cast<std::uint8_t>(NativeNotePhase::Completed),
         static_cast<std::uint8_t>(NormalRank::Great)},
        {static_cast<std::uint8_t>(NativeNotePhase::Completed),
         static_cast<std::uint8_t>(NormalRank::Just)},
        {static_cast<std::uint8_t>(NativeNotePhase::Completed),
         static_cast<std::uint8_t>(NormalRank::Just)},
        {static_cast<std::uint8_t>(NativeNotePhase::Completed),
         static_cast<std::uint8_t>(NormalRank::Just)},
        {static_cast<std::uint8_t>(NativeNotePhase::Completed),
         static_cast<std::uint8_t>(NormalRank::Good)},
        {static_cast<std::uint8_t>(NativeNotePhase::Missed),
         static_cast<std::uint8_t>(NormalRank::Missed)},
        {static_cast<std::uint8_t>(NativeNotePhase::Completed),
         static_cast<std::uint8_t>(NormalRank::Just)},
        {static_cast<std::uint8_t>(NativeNotePhase::Completed),
         static_cast<std::uint8_t>(NormalRank::Just)},
        {static_cast<std::uint8_t>(NativeNotePhase::Missed),
         static_cast<std::uint8_t>(NormalRank::Missed)},
    }};
    for (std::size_t i = 0; i < expected.size(); ++i) {
        CHECK(baseline.notes[i].phase == expected[i].first);
        CHECK(baseline.notes[i].rank == expected[i].second);
    }

    // Exact per-note effect times: pending resolution, auto crossing and miss
    // deadline, never a render tick.
    CHECK(SameDouble(baseline.outcomes[0].at_us, 990'000.0));
    CHECK(SameDouble(baseline.outcomes[1].at_us, 1'290'000.0));
    CHECK(SameDouble(baseline.outcomes[2].at_us, 1'400'000.0));
    CHECK(SameDouble(baseline.outcomes[3].at_us, 1'600'000.0));
    CHECK(SameDouble(baseline.outcomes[4].at_us, 1'600'000.0));
    CHECK(SameDouble(baseline.outcomes[5].at_us, 1'990'000.0));
    CHECK(SameDouble(baseline.outcomes[6].at_us, 2'523'334.0));
    CHECK(SameDouble(baseline.outcomes[7].at_us, 2'923'334.0));
    CHECK(SameDouble(baseline.outcomes[8].at_us, 3'190'000.0));
    CHECK(SameDouble(baseline.outcomes[10].at_us, 3'573'333.333333333));
    CHECK(SameDouble(baseline.outcomes[12].at_us, 4'123'334.0));

    for (const unsigned rate : rates) {
        const RunResult run = RunCadence(rate);
        CHECK(run.ok);
        CHECK(run.monotonic);
        CHECK(SameRun(baseline, run));
        if (!SameRun(baseline, run)) {
            std::printf("cadence %u diverged from 60 Hz\n", rate);
        }
    }
}

void TestDelayedInWindowInput() {
    NativeGameplay game;
    const std::vector<NativeNoteSpec> notes{Note(0, 0, 1'000'000.0, 300)};
    const std::vector<ChartEvent> chart;
    const std::vector<ScheduledSound> sounds;
    CHECK(game.Configure(notes, chart, sounds, NoteConfig()) == GameplayStatus::Ok);
    CHECK(game.Begin(1, 0.0) == GameplayStatus::Ok);

    std::vector<NativeNoteOutcome> out(game.MaxOutcomesPerAdvance());
    std::vector<ChartEvent> chart_out(1);
    std::vector<SoundCommand> sound_out(1);
    const GameplayOutputs outputs{out, chart_out, sound_out};

    // The consumer is far past the deadline, but the producer has certified
    // input only through 1.0 s, so the note must remain pending.
    CHECK(game.SetInputWatermark(SongTime{1'000'000.0, 1}) == GameplayStatus::Ok);
    GameplayAdvance advance = game.Advance(SongTime{1'200'000.0, 1}, outputs);
    CHECK(advance.ok());
    CHECK(advance.note_counts.missed == 0);
    CHECK(game.Notes().Phase(0) == NativeNotePhase::Pending);

    // The in-window press is captured at 1.1 s but arrives after the stall.
    const TimedInputEvent press = MakeEvent(
        ScriptedInput{1'100'000.0, 0, InputEdge::Press}, SongTime{1'100'000.0, 1}, 1);
    CHECK(game.SubmitInput(press) == GameplayStatus::Ok);
    CHECK(game.SetInputWatermark(SongTime{1'100'000.0, 1}) == GameplayStatus::Ok);
    advance = game.Advance(SongTime{1'200'000.0, 1}, outputs);
    CHECK(advance.ok());
    CHECK(advance.note_counts.written == 1);
    CHECK(out[0].rank == NormalRank::Good);
    CHECK(SameDouble(out[0].at_us, 1'100'000.0));
    CHECK(game.Notes().Phase(0) == NativeNotePhase::Completed);

    // Once the input is certified past the deadline, nothing is left to miss.
    CHECK(game.SetInputWatermark(SongTime{1'200'000.0, 1}) == GameplayStatus::Ok);
    advance = game.Advance(SongTime{1'200'000.0, 1}, outputs);
    CHECK(advance.ok());
    CHECK(advance.note_counts.missed == 0);

    // The same watermark progression without the press does miss exactly once.
    NativeGameplay other;
    CHECK(other.Configure(notes, chart, sounds, NoteConfig()) == GameplayStatus::Ok);
    CHECK(other.Begin(1, 0.0) == GameplayStatus::Ok);
    CHECK(other.SetInputWatermark(SongTime{1'200'000.0, 1}) == GameplayStatus::Ok);
    const GameplayAdvance missed = other.Advance(SongTime{1'200'000.0, 1}, outputs);
    CHECK(missed.ok());
    CHECK(missed.note_counts.missed == 1);
    CHECK(SameDouble(out[0].at_us, 1'123'334.0));
}

void TestPauseSeekAndReset() {
    NativeGameplay game;
    const std::vector<NativeNoteSpec> notes = Notes();
    const std::vector<ChartEvent> chart = Chart();
    const std::vector<ScheduledSound> sounds = Sounds();
    CHECK(game.Configure(notes, chart, sounds, NoteConfig()) == GameplayStatus::Ok);
    CHECK(game.Begin(1, 0.0) == GameplayStatus::Ok);

    std::vector<NativeNoteOutcome> out(game.MaxOutcomesPerAdvance());
    std::vector<ChartEvent> chart_out(8);
    std::vector<SoundCommand> sound_out(4);
    const GameplayOutputs outputs{out, chart_out, sound_out};

    CHECK(game.SetInputWatermark(SongTime{1'000'000.0, 1}) == GameplayStatus::Ok);
    GameplayAdvance advance = game.Advance(SongTime{1'000'000.0, 1}, outputs);
    CHECK(advance.ok());
    const std::size_t chart_after_first = advance.chart_written;
    CHECK(chart_after_first > 0);

    // A paused/duplicate update at the same time is a no-op.
    advance = game.Advance(SongTime{1'000'000.0, 1}, outputs);
    CHECK(advance.ok());
    CHECK(advance.chart_written == 0);
    CHECK(advance.sounds_written == 0);
    CHECK(advance.note_counts.written == 0);

    // Seek to 2 s: earlier notes are skipped, earlier chart events and sounds
    // are not replayed, and stale generation input is rejected.
    CHECK(game.Begin(2, 2'000'000.0) == GameplayStatus::Ok);
    CHECK(game.Notes().SkippedCount() == 4);
    CHECK(game.Notes().PendingCount() == 6);
    CHECK(game.Notes().MissedCount() == 0);

    const TimedInputEvent stale = MakeEvent(
        ScriptedInput{2'100'000.0, 0, InputEdge::Press}, SongTime{2'100'000.0, 1}, 1);
    CHECK(game.SubmitInput(stale) == GameplayStatus::StaleGeneration);
    CHECK(game.SubmitInput(MakeEvent(
              ScriptedInput{2'400'000.0, 5, InputEdge::Press},
              SongTime{2'400'000.0, 2}, 2)) == GameplayStatus::Ok);

    CHECK(game.SetInputWatermark(SongTime{4'600'000.0, 2}) == GameplayStatus::Ok);
    std::vector<std::uint64_t> delivered;
    std::vector<SoundKey> sound_log;
    for (int i = 0; i < 4; ++i) {
        const GameplayAdvance step = game.Advance(SongTime{4'600'000.0, 2}, outputs);
        CHECK(step.ok());
        for (std::size_t j = 0; j < step.chart_written; ++j) {
            delivered.push_back(chart_out[j].id);
        }
        for (std::size_t j = 0; j < step.sounds_written; ++j) {
            sound_log.push_back(SoundKey{static_cast<std::uint8_t>(sound_out[j].action),
                                         sound_out[j].sound_id, sound_out[j].at_us});
        }
        if (!step.chart_backlog && !step.sound_backlog) break;
    }
    CHECK((delivered == std::vector<std::uint64_t>{104, 105, 106, 107, 108, 109}));
    CHECK(sound_log.size() == 3);
    CHECK(sound_log[0].sound == 201);
    CHECK(sound_log[0].action == static_cast<std::uint8_t>(SoundAction::Start));
    CHECK(sound_log[1].sound == 202);
    CHECK(sound_log[2].sound == 202);
    CHECK(sound_log[2].action == static_cast<std::uint8_t>(SoundAction::Cut));

    // The note at 2.4 s was captured at its note time and scored Just; notes
    // before the seek were never missed or scored.
    CHECK(game.Notes().Phase(5) == NativeNotePhase::Completed);
    CHECK(game.Notes().FinalRank(5) == NormalRank::Just);
    for (std::size_t i = 0; i < 4; ++i) {
        CHECK(game.Notes().Phase(i) == NativeNotePhase::Skipped);
    }
}

void TestInputBridgeLimits() {
    NativeGameplay game;
    std::vector<NativeNoteSpec> notes{Note(0, 0, 10'000'000.0, 1)};
    CHECK(game.Configure(notes, {}, {}, NoteConfig(), 2) == GameplayStatus::Ok);
    CHECK(game.Begin(1, 0.0) == GameplayStatus::Ok);

    std::vector<NativeNoteOutcome> out(game.MaxOutcomesPerAdvance());
    const GameplayOutputs outputs{out, {}, {}};

    const TimedInputEvent first = MakeEvent(
        ScriptedInput{100'000.0, 0, InputEdge::Press}, SongTime{100'000.0, 1}, 1);
    const TimedInputEvent second = MakeEvent(
        ScriptedInput{200'000.0, 1, InputEdge::Press}, SongTime{200'000.0, 1}, 2);
    const TimedInputEvent third = MakeEvent(
        ScriptedInput{300'000.0, 2, InputEdge::Press}, SongTime{300'000.0, 1}, 3);
    CHECK(game.SubmitInput(first) == GameplayStatus::Ok);
    CHECK(game.SubmitInput(second) == GameplayStatus::Ok);
    CHECK(game.SubmitInput(third) == GameplayStatus::InputQueueFull);
    CHECK(game.QueueDepth() == 2);

    // Malformed identities and a synthetic press are rejected, not judged.
    {
        TimedInputEvent zero = MakeEvent(
            ScriptedInput{210'000.0, 4, InputEdge::Press}, SongTime{210'000.0, 1}, 0);
        zero.input.stamp.sequence = 0;
        CHECK(game.SubmitInput(zero) == GameplayStatus::InvalidInput);
        TimedInputEvent synthetic = MakeEvent(
            ScriptedInput{220'000.0, 4, InputEdge::Press}, SongTime{220'000.0, 1}, 9);
        synthetic.input.synthetic = true;
        CHECK(game.SubmitInput(synthetic) == GameplayStatus::InvalidInput);
    }

    // A resubmission of the last accepted identity is rejected, and an older
    // capture time is rejected rather than reordered.
    CHECK(game.SubmitInput(second) == GameplayStatus::DuplicateInput);
    CHECK(game.SubmitInput(MakeEvent(
              ScriptedInput{50'000.0, 3, InputEdge::Press},
              SongTime{50'000.0, 1}, 4)) == GameplayStatus::InputNotChronological);

    CHECK(game.SetInputWatermark(SongTime{200'000.0, 1}) == GameplayStatus::Ok);
    GameplayAdvance advance = game.Advance(SongTime{200'000.0, 1}, outputs);
    CHECK(advance.ok());
    CHECK(advance.inputs_applied == 2);
    CHECK(game.QueueDepth() == 0);

    // The rejected record can be retried after capacity is freed.
    CHECK(game.SubmitInput(third) == GameplayStatus::Ok);
    CHECK(game.SetInputWatermark(SongTime{300'000.0, 1}) == GameplayStatus::Ok);
    advance = game.Advance(SongTime{300'000.0, 1}, outputs);
    CHECK(advance.ok());
    CHECK(advance.inputs_applied == 1);

    // Watermark ahead of current song time is a complete no-op.
    const double before = game.LastNow();
    CHECK(game.SetInputWatermark(SongTime{800'000.0, 1}) == GameplayStatus::Ok);
    advance = game.Advance(SongTime{700'000.0, 1}, outputs);
    CHECK(advance.status == GameplayStatus::WatermarkAhead);
    CHECK(game.LastNow() == before);
    CHECK(game.SetInputWatermark(SongTime{800'000.0, 1}) == GameplayStatus::Ok);
    CHECK(game.Advance(SongTime{800'000.0, 1}, outputs).ok());

    // Regressing the watermark is rejected.
    CHECK(game.SetInputWatermark(SongTime{700'000.0, 1}) ==
          GameplayStatus::WatermarkRegressed);

    // Backward song time is rejected.
    CHECK(game.Advance(SongTime{799'000.0, 1}, outputs).status ==
          GameplayStatus::BackwardTime);

    // Note outcome span smaller than the note count is a no-op.
    std::vector<NativeNoteOutcome> too_small;
    const GameplayOutputs small{too_small, {}, {}};
    CHECK(game.Advance(SongTime{900'000.0, 1}, small).status ==
          GameplayStatus::InsufficientCapacity);
}

void TestMultiLaneIdentity() {
    NativeGameplay game;
    CHECK(game.Configure({}, {}, {}, NoteConfig(), 8) == GameplayStatus::Ok);
    CHECK(game.Begin(1, 0.0) == GameplayStatus::Ok);

    TimedInputEvent press = MakeEvent(
        ScriptedInput{100'000.0, 0, InputEdge::Press}, SongTime{100'000.0, 1}, 100);
    press.input.stamp.stream_epoch = 12;
    TimedInputEvent chord = press;
    chord.input.lane = 1;
    CHECK(game.SubmitInput(press) == GameplayStatus::Ok);
    CHECK(game.SubmitInput(chord) == GameplayStatus::Ok);
    CHECK(game.QueueDepth() == 2);
    CHECK(game.SubmitInput(press) == GameplayStatus::DuplicateInput);
    CHECK(game.SubmitInput(chord) == GameplayStatus::DuplicateInput);

    TimedInputEvent reset = press;
    reset.input.lane = 0;
    reset.input.edge = InputEdge::Release;
    reset.input.synthetic = true;
    reset.input.stamp.stream_epoch = 13;
    reset.input.stamp.sequence = 101;
    TimedInputEvent reset_other = reset;
    reset_other.input.lane = 1;
    CHECK(game.SubmitInput(reset) == GameplayStatus::Ok);
    CHECK(game.SubmitInput(reset_other) == GameplayStatus::Ok);
    CHECK(game.SubmitInput(reset) == GameplayStatus::DuplicateInput);
    CHECK(game.QueueDepth() == 4);
}

void TestNormalizerIntegration() {
    // One physical control bound to two lanes emits two LogicalInputEvents with
    // the exact same capture stamp. The bridge must keep both, and a synthetic
    // two-lane reset must also keep both, without changing ordinals.
    InputNormalizer normalizer;
    const std::array<InputBinding, 2> bindings{{
        InputBinding{1, 1, 1, 0},
        InputBinding{1, 1, 1, 1},
    }};
    CHECK(normalizer.Configure(bindings) == InputConfigStatus::Accepted);

    std::array<LogicalInputEvent, kInputLaneCount> edges{};
    InputStamp stamp;
    stamp.qpc = 7'000'000'000'000'001'000LL;
    stamp.sequence = 1;
    stamp.stream_epoch = 21;
    stamp.ordinal = 0;
    PhysicalControlState control;
    control.source_id = 1;
    control.device_generation = 1;
    control.control_id = 1;
    control.down = true;
    control.velocity = 1.0f;
    const InputApplyResult applied = normalizer.Apply(control, stamp, edges);
    CHECK(applied.status == InputApplyStatus::Applied);
    CHECK(applied.event_count == 2);
    CHECK(edges[0].stamp.sequence == edges[1].stamp.sequence);
    CHECK(edges[0].stamp.ordinal == edges[1].stamp.ordinal);
    CHECK(edges[0].stamp.qpc == edges[1].stamp.qpc);
    CHECK(edges[0].lane == 0 && edges[1].lane == 1);
    CHECK(edges[0].edge == InputEdge::Press && edges[1].edge == InputEdge::Press);

    NativeGameplay game;
    CHECK(game.Configure({}, {}, {}, NoteConfig(), 8) == GameplayStatus::Ok);
    CHECK(game.Begin(1, 0.0) == GameplayStatus::Ok);
    for (std::size_t i = 0; i < applied.event_count; ++i) {
        TimedInputEvent event;
        event.input = edges[i];
        event.song_time = SongTime{200'000.0, 1};
        CHECK(game.SubmitInput(event) == GameplayStatus::Ok);
    }
    CHECK(game.QueueDepth() == 2);

    std::array<LogicalInputEvent, kInputLaneCount> releases{};
    InputStamp reset_stamp;
    reset_stamp.qpc = stamp.qpc + 1'000;
    reset_stamp.sequence = 2;
    reset_stamp.stream_epoch = 22;
    reset_stamp.ordinal = 0;
    const InputResetResult reset = normalizer.Reset(reset_stamp, releases);
    CHECK(reset.Applied());
    CHECK(reset.event_count == 2);
    CHECK(releases[0].stamp.sequence == releases[1].stamp.sequence);
    CHECK(releases[0].stamp.qpc == releases[1].stamp.qpc);
    CHECK(releases[0].synthetic && releases[1].synthetic);
    CHECK(releases[0].lane == 0 && releases[1].lane == 1);
    for (std::size_t i = 0; i < reset.event_count; ++i) {
        TimedInputEvent event;
        event.input = releases[i];
        event.song_time = SongTime{201'000.0, 1};
        CHECK(game.SubmitInput(event) == GameplayStatus::Ok);
    }
    CHECK(game.QueueDepth() == 4);

    // Replaying the first lane edge is still a duplicate; the sibling lane is
    // a distinct logical event and was already accepted above.
    TimedInputEvent replay;
    replay.input = edges[0];
    replay.song_time = SongTime{200'000.0, 1};
    CHECK(game.SubmitInput(replay) == GameplayStatus::DuplicateInput);
}

void TestConfigureInvalidation() {
    NativeGameplay game;
    const std::vector<NativeNoteSpec> notes{Note(0, 0, 1'000'000.0, 1)};
    CHECK(game.Configure(notes, {}, {}, NoteConfig()) == GameplayStatus::Ok);
    CHECK(game.Begin(1, 0.0) == GameplayStatus::Ok);

    // A rejected reconfigure makes the instance explicitly unusable before any
    // partial change; it never leaves a partially running object.
    ChartEvent duplicate_a;
    duplicate_a.id = 5;
    duplicate_a.song_us = 100'000.0;
    ChartEvent duplicate_b = duplicate_a;
    duplicate_b.song_us = 200'000.0;
    const std::vector<ChartEvent> duplicates{duplicate_a, duplicate_b};
    CHECK(game.Configure(notes, duplicates, {}, NoteConfig()) ==
          GameplayStatus::InvalidConfig);
    CHECK(!game.IsConfigured());
    CHECK(!game.IsStarted());
    CHECK(game.Begin(2, 0.0) == GameplayStatus::NotConfigured);

    // Lifetime generation floors survive the invalidation and reconfiguration.
    CHECK(game.Configure(notes, {}, {}, NoteConfig()) == GameplayStatus::Ok);
    CHECK(game.Begin(1, 0.0) == GameplayStatus::GenerationNotNew);
    CHECK(game.Begin(2, 0.0) == GameplayStatus::Ok);
}

void TestConfigureStageFailures() {
    const std::vector<NativeNoteSpec> notes{Note(0, 0, 1'000'000.0, 1)};
    const std::array<ConfigureFailpoint, 5> stages{{
        ConfigureFailpoint::BeforeNotes,
        ConfigureFailpoint::AfterNotes,
        ConfigureFailpoint::AfterChart,
        ConfigureFailpoint::AfterSounds,
        ConfigureFailpoint::AfterQueue,
    }};
    for (const ConfigureFailpoint stage : stages) {
        NativeGameplay game;
        CHECK(game.Configure(notes, {}, {}, NoteConfig()) == GameplayStatus::Ok);
        CHECK(game.Begin(1, 0.0) == GameplayStatus::Ok);
        CHECK(game.Configure(notes, {}, {}, NoteConfig(),
                             kDefaultInputQueueCapacity, stage) ==
              GameplayStatus::InvalidConfig);
        CHECK(!game.IsConfigured());
        CHECK(!game.IsStarted());
        CHECK(game.Begin(2, 0.0) == GameplayStatus::NotConfigured);
        CHECK(game.Configure(notes, {}, {}, NoteConfig()) == GameplayStatus::Ok);
        CHECK(game.Begin(1, 0.0) == GameplayStatus::GenerationNotNew);
        CHECK(game.Begin(2, 0.0) == GameplayStatus::Ok);
    }
}

void TestOutcomeCapacityCanary() {
    NativeGameplay game;
    const std::vector<NativeNoteSpec> notes{Note(0, 0, 1'000'000.0, 1)};
    CHECK(game.Configure(notes, {}, {}, NoteConfig(), 2) == GameplayStatus::Ok);
    CHECK(game.Begin(1, 0.0) == GameplayStatus::Ok);
    CHECK(game.SubmitInput(MakeEvent(
              ScriptedInput{900'000.0, 0, InputEdge::Press},
              SongTime{900'000.0, 1}, 1)) == GameplayStatus::Ok);
    CHECK(game.SubmitInput(MakeEvent(
              ScriptedInput{950'000.0, 0, InputEdge::Release},
              SongTime{950'000.0, 1}, 2)) == GameplayStatus::Ok);
    CHECK(game.SetInputWatermark(SongTime{2'000'000.0, 1}) == GameplayStatus::Ok);

    std::array<NativeNoteOutcome, 4> canary{};
    for (NativeNoteOutcome& outcome : canary) outcome.note_index = 0xDEADBEEFu;
    const GameplayOutputs too_small{std::span(canary.data(), 1), {}, {}};
    const GameplayAdvance refused = game.Advance(SongTime{2'000'000.0, 1}, too_small);
    CHECK(refused.status == GameplayStatus::InsufficientCapacity);
    CHECK(game.QueueDepth() == 2);
    CHECK(game.Notes().Phase(0) == NativeNotePhase::Pending);
    for (std::size_t i = 1; i < canary.size(); ++i) {
        CHECK(canary[i].note_index == 0xDEADBEEFu);
    }

    std::vector<NativeNoteOutcome> out(game.MaxOutcomesPerAdvance());
    const GameplayOutputs outputs{out, {}, {}};
    const GameplayAdvance advance = game.Advance(SongTime{2'000'000.0, 1}, outputs);
    CHECK(advance.ok());
    CHECK(advance.note_counts.written == 2);
    CHECK(advance.note_counts.scored == 1);
    CHECK(advance.note_counts.cuts == 1);
    CHECK(out[0].kind == NoteOutcomeKind::Scored);
    CHECK(out[1].kind == NoteOutcomeKind::KeysoundCut);
    CHECK(game.QueueDepth() == 0);
}

void TestWatermarkStateOrdering() {
    // A delayed second early press must still overwrite the pending decision;
    // state may only advance through the certified watermark, not render now.
    {
        NativeGameplay game;
        const std::vector<NativeNoteSpec> notes{Note(0, 0, 1'000'000.0, 1)};
        CHECK(game.Configure(notes, {}, {}, NoteConfig(), 8) == GameplayStatus::Ok);
        CHECK(game.Begin(1, 0.0) == GameplayStatus::Ok);
        std::vector<NativeNoteOutcome> out(game.MaxOutcomesPerAdvance());
        const GameplayOutputs outputs{out, {}, {}};

        CHECK(game.SubmitInput(MakeEvent(
                  ScriptedInput{960'000.0, 0, InputEdge::Press},
                  SongTime{960'000.0, 1}, 1)) == GameplayStatus::Ok);
        CHECK(game.SetInputWatermark(SongTime{960'000.0, 1}) == GameplayStatus::Ok);
        CHECK(game.Advance(SongTime{1'200'000.0, 1}, outputs).ok());
        CHECK(game.Notes().Phase(0) == NativeNotePhase::PendingEarly);

        CHECK(game.SubmitInput(MakeEvent(
                  ScriptedInput{970'000.0, 0, InputEdge::Press},
                  SongTime{970'000.0, 1}, 2)) == GameplayStatus::Ok);
        CHECK(game.SetInputWatermark(SongTime{970'000.0, 1}) == GameplayStatus::Ok);
        CHECK(game.Advance(SongTime{1'200'000.0, 1}, outputs).ok());
        CHECK(game.Notes().Phase(0) == NativeNotePhase::PendingEarly);

        CHECK(game.SetInputWatermark(SongTime{1'000'000.0, 1}) == GameplayStatus::Ok);
        const GameplayAdvance resolved = game.Advance(SongTime{1'200'000.0, 1}, outputs);
        CHECK(resolved.ok());
        CHECK(resolved.note_counts.written == 1);
        CHECK(out[0].rank == NormalRank::Just);
        CHECK(SameDouble(out[0].at_us, 990'000.0));
    }

    // A delayed release captured before a later auto note must cut the sound
    // that was active at its capture time, not the auto note's newer sound.
    {
        NativeGameplay game;
        std::vector<NativeNoteSpec> notes{Note(0, 0, 500'000.0, 10)};
        notes.push_back(Note(1, 0, 1'100'000.0, 11, true));
        CHECK(game.Configure(notes, {}, {}, NoteConfig(), 8) == GameplayStatus::Ok);
        CHECK(game.Begin(1, 0.0) == GameplayStatus::Ok);
        std::vector<NativeNoteOutcome> out(game.MaxOutcomesPerAdvance());
        const GameplayOutputs outputs{out, {}, {}};

        CHECK(game.SubmitInput(MakeEvent(
                  ScriptedInput{500'000.0, 0, InputEdge::Press},
                  SongTime{500'000.0, 1}, 1)) == GameplayStatus::Ok);
        CHECK(game.SetInputWatermark(SongTime{500'000.0, 1}) == GameplayStatus::Ok);
        CHECK(game.Advance(SongTime{600'000.0, 1}, outputs).ok());
        CHECK(game.Notes().ActiveSound(0) == 10);

        CHECK(game.SubmitInput(MakeEvent(
                  ScriptedInput{1'000'000.0, 0, InputEdge::Release},
                  SongTime{1'000'000.0, 1}, 2)) == GameplayStatus::Ok);
        CHECK(game.Advance(SongTime{1'200'000.0, 1}, outputs).ok());
        CHECK(game.Notes().ActiveSound(0) == 10); // auto not certified yet

        CHECK(game.SetInputWatermark(SongTime{1'000'000.0, 1}) == GameplayStatus::Ok);
        const GameplayAdvance cut = game.Advance(SongTime{1'200'000.0, 1}, outputs);
        CHECK(cut.ok());
        CHECK(cut.note_counts.cuts == 1);
        CHECK(out[0].kind == NoteOutcomeKind::KeysoundCut);
        CHECK(out[0].sound_id == 10);
        CHECK(game.Notes().ActiveSound(0) == 0);

        CHECK(game.SetInputWatermark(SongTime{1'200'000.0, 1}) == GameplayStatus::Ok);
        const GameplayAdvance auto_done = game.Advance(SongTime{1'200'000.0, 1}, outputs);
        CHECK(auto_done.ok());
        CHECK(auto_done.note_counts.auto_completed == 1);
        CHECK(out[0].kind == NoteOutcomeKind::Auto);
        CHECK(out[0].sound_id == 11);
        CHECK(game.Notes().ActiveSound(0) == 11);
    }
}

void TestChartAndSoundBacklog() {
    NativeGameplay game;
    const std::vector<NativeNoteSpec> notes = Notes();
    const std::vector<ChartEvent> chart = Chart();
    const std::vector<ScheduledSound> sounds = Sounds();
    CHECK(game.Configure(notes, chart, sounds, NoteConfig()) == GameplayStatus::Ok);
    CHECK(game.Begin(1, 0.0) == GameplayStatus::Ok);

    std::vector<NativeNoteOutcome> out(game.MaxOutcomesPerAdvance());
    std::array<ChartEvent, 1> chart_one{};
    std::array<SoundCommand, 1> sound_one{};
    const GameplayOutputs outputs{out, chart_one, sound_one};

    CHECK(game.SetInputWatermark(SongTime{4'600'000.0, 1}) == GameplayStatus::Ok);
    std::vector<std::uint64_t> ids;
    std::vector<std::uint32_t> sound_ids;
    for (int i = 0; i < 12; ++i) {
        const GameplayAdvance advance = game.Advance(SongTime{4'600'000.0, 1}, outputs);
        CHECK(advance.ok());
        CHECK(advance.chart_written <= 1);
        CHECK(advance.sounds_written <= 1);
        for (std::size_t j = 0; j < advance.chart_written; ++j) {
            ids.push_back(chart_one[j].id);
        }
        for (std::size_t j = 0; j < advance.sounds_written; ++j) {
            sound_ids.push_back(sound_one[j].sound_id);
        }
        if (!advance.chart_backlog && !advance.sound_backlog) break;
    }
    CHECK(ids.size() == 10);
    for (std::size_t i = 0; i < ids.size(); ++i) CHECK(ids[i] == 100 + i);
    CHECK((sound_ids == std::vector<std::uint32_t>{200, 200, 201, 202, 202}));
    CHECK(game.ChartPending() == 0);
}

void TestSoundCrossingOrder() {
    const auto make = [](bool start, double start_us, std::uint32_t id,
                         bool release, double release_us,
                         std::uint32_t target) {
        ScheduledSound sound;
        sound.has_start = start;
        sound.start_us = start_us;
        sound.sound_id = id;
        sound.velocity = 1.0f;
        sound.has_release = release;
        sound.release_us = release_us;
        sound.release_target_id = target;
        return sound;
    };
    // A: start 10, release 100. B: start 20, release 30. A stalled Advance must
    // emit start A, start B, release B, release A - not start A, release A, ...
    const std::vector<ScheduledSound> overlapping{
        make(true, 10.0, 10, true, 100.0, 10),
        make(true, 20.0, 11, true, 30.0, 11),
    };
    const auto collect = [](NativeGameplay& game, SongTime now,
                            std::span<SoundCommand> buffer) {
        std::vector<SoundKey> log;
        std::vector<NativeNoteOutcome> notes(game.MaxOutcomesPerAdvance());
        const GameplayOutputs outputs{notes, {}, buffer};
        for (int i = 0; i < 8; ++i) {
            const GameplayAdvance advance = game.Advance(now, outputs);
            CHECK(advance.ok());
            for (std::size_t j = 0; j < advance.sounds_written; ++j) {
                log.push_back(SoundKey{static_cast<std::uint8_t>(buffer[j].action),
                                       buffer[j].sound_id, buffer[j].at_us});
            }
            if (!advance.sound_backlog) break;
        }
        return log;
    };

    {
        NativeGameplay game;
        CHECK(game.Configure({}, {}, overlapping, NoteConfig()) == GameplayStatus::Ok);
        CHECK(game.Begin(1, 0.0) == GameplayStatus::Ok);
        std::array<SoundCommand, 8> buffer{};
        const auto log = collect(game, SongTime{200.0, 1}, buffer);
        CHECK(log.size() == 4);
        CHECK(log[0].action == static_cast<std::uint8_t>(SoundAction::Start));
        CHECK(log[0].sound == 10 && SameDouble(log[0].at_us, 10.0));
        CHECK(log[1].sound == 11 && SameDouble(log[1].at_us, 20.0));
        CHECK(log[2].action == static_cast<std::uint8_t>(SoundAction::Cut));
        CHECK(log[2].sound == 11 && SameDouble(log[2].at_us, 30.0));
        CHECK(log[3].sound == 10 && SameDouble(log[3].at_us, 100.0));
    }

    // Fine cadence produces the same order.
    {
        NativeGameplay game;
        CHECK(game.Configure({}, {}, overlapping, NoteConfig()) == GameplayStatus::Ok);
        CHECK(game.Begin(1, 0.0) == GameplayStatus::Ok);
        std::array<SoundCommand, 8> buffer{};
        std::vector<SoundKey> log;
        for (const double at : {15.0, 35.0, 105.0}) {
            const auto part = collect(game, SongTime{at, 1}, buffer);
            log.insert(log.end(), part.begin(), part.end());
        }
        CHECK(log.size() == 4);
        CHECK(log[0].sound == 10 && SameDouble(log[0].at_us, 10.0));
        CHECK(log[1].sound == 11 && SameDouble(log[1].at_us, 20.0));
        CHECK(log[2].sound == 11 && SameDouble(log[2].at_us, 30.0));
        CHECK(log[3].sound == 10 && SameDouble(log[3].at_us, 100.0));
    }

    // Capacity 1 drains the same sequence without loss or duplication.
    {
        NativeGameplay game;
        CHECK(game.Configure({}, {}, overlapping, NoteConfig()) == GameplayStatus::Ok);
        CHECK(game.Begin(1, 0.0) == GameplayStatus::Ok);
        std::array<SoundCommand, 1> one{};
        std::vector<SoundKey> log;
        for (int i = 0; i < 6; ++i) {
            const auto part = collect(game, SongTime{200.0, 1}, one);
            log.insert(log.end(), part.begin(), part.end());
        }
        CHECK(log.size() == 4);
        CHECK(log[0].sound == 10 && SameDouble(log[0].at_us, 10.0));
        CHECK(log[1].sound == 11 && SameDouble(log[1].at_us, 20.0));
        CHECK(log[2].sound == 11 && SameDouble(log[2].at_us, 30.0));
        CHECK(log[3].sound == 10 && SameDouble(log[3].at_us, 100.0));
    }

    // Equal times: Start before Cut, then schedule index.
    {
        const std::vector<ScheduledSound> equal{
            make(true, 50.0, 20, true, 60.0, 20),
            make(true, 50.0, 21, true, 60.0, 21),
        };
        NativeGameplay game;
        CHECK(game.Configure({}, {}, equal, NoteConfig()) == GameplayStatus::Ok);
        CHECK(game.Begin(1, 0.0) == GameplayStatus::Ok);
        std::array<SoundCommand, 8> buffer{};
        const auto log = collect(game, SongTime{100.0, 1}, buffer);
        CHECK(log.size() == 4);
        CHECK(log[0].action == static_cast<std::uint8_t>(SoundAction::Start));
        CHECK(log[0].sound == 20);
        CHECK(log[1].action == static_cast<std::uint8_t>(SoundAction::Start));
        CHECK(log[1].sound == 21);
        CHECK(log[2].action == static_cast<std::uint8_t>(SoundAction::Cut));
        CHECK(log[2].sound == 20);
        CHECK(log[3].action == static_cast<std::uint8_t>(SoundAction::Cut));
        CHECK(log[3].sound == 21);
    }

    // Seek past both starts still delivers the future release of A exactly once.
    {
        NativeGameplay game;
        CHECK(game.Configure({}, {}, overlapping, NoteConfig()) == GameplayStatus::Ok);
        CHECK(game.Begin(1, 0.0) == GameplayStatus::Ok);
        CHECK(game.Begin(2, 50.0) == GameplayStatus::Ok);
        std::array<SoundCommand, 8> buffer{};
        const auto log = collect(game, SongTime{200.0, 2}, buffer);
        CHECK(log.size() == 1);
        CHECK(log[0].action == static_cast<std::uint8_t>(SoundAction::Cut));
        CHECK(log[0].sound == 10 && SameDouble(log[0].at_us, 100.0));
    }
}

void TestChartOffset() {
    // 0x2FF090 dispatches when record_time - music_frame - visual - judgment
    // <= 0. A record just after the seek start can therefore already be in the
    // past once the judgment offset is applied, and must not be replayed.
    NativeGameplay game;
    ChartEvent early;
    early.id = 1;
    early.song_us = 2'005'000.0; // effective 1'995'000 at judgment offset 10 ms
    ChartEvent late;
    late.id = 2;
    late.song_us = 2'020'000.0; // effective 2'010'000
    const std::vector<ChartEvent> chart{early, late};
    CHECK(game.Configure({}, chart, {}, NoteConfig()) == GameplayStatus::Ok);
    CHECK(game.Begin(1, 2'000'000.0) == GameplayStatus::Ok);
    CHECK(game.SetInputWatermark(SongTime{3'000'000.0, 1}) == GameplayStatus::Ok);
    std::array<ChartEvent, 4> buffer{};
    std::vector<NativeNoteOutcome> note_buffer(game.MaxOutcomesPerAdvance());
    const GameplayOutputs outputs{note_buffer, buffer, {}};
    const GameplayAdvance advance = game.Advance(SongTime{3'000'000.0, 1}, outputs);
    CHECK(advance.ok());
    CHECK(advance.chart_written == 1);
    CHECK(buffer[0].id == 2);
}

void TestReleaseOnlySound() {
    NativeGameplay game;
    ScheduledSound release_only;
    release_only.has_start = false;
    release_only.has_release = true;
    release_only.release_us = 1'000.0;
    release_only.release_target_id = 77;
    const std::vector<ScheduledSound> sounds{release_only};
    CHECK(game.Configure({}, {}, sounds, NoteConfig()) == GameplayStatus::Ok);
    CHECK(game.Begin(1, 0.0) == GameplayStatus::Ok);
    std::vector<NativeNoteOutcome> out(game.MaxOutcomesPerAdvance());
    std::array<SoundCommand, 4> sound_out{};
    const GameplayOutputs outputs{out, {}, sound_out};
    CHECK(game.SetInputWatermark(SongTime{2'000.0, 1}) == GameplayStatus::Ok);
    GameplayAdvance advance = game.Advance(SongTime{2'000.0, 1}, outputs);
    CHECK(advance.ok());
    CHECK(advance.sounds_written == 1);
    CHECK(sound_out[0].action == SoundAction::Cut);
    CHECK(sound_out[0].sound_id == 77);
    CHECK(SameDouble(sound_out[0].at_us, 1'000.0));
    advance = game.Advance(SongTime{2'000.0, 1}, outputs);
    CHECK(advance.ok());
    CHECK(advance.sounds_written == 0);
}

void TestSeam() {
    NativeSeamReport report = EvaluateNativeSeam(false, false);
    CHECK(report.state == NativeSeamState::DisabledByDefault);
    report = EvaluateNativeSeam(true, false);
    CHECK(report.state == NativeSeamState::WaitingForEvidence);
    report = EvaluateNativeSeam(true, true);
    CHECK(report.state == NativeSeamState::WaitingForEvidence);
    const auto targets = NativeSeamTargets();
    CHECK(targets.size() == 13);
    for (const native_layout::Target& target : targets) {
        CHECK(target.signature_known);
    }
    CHECK(native_layout::kTargetPrefixes.size() == targets.size());
    CHECK(!native_layout::kAbiAndMatchingVerified);
}

} // namespace

int main() {
    TestCadenceInvariance();
    TestDelayedInWindowInput();
    TestPauseSeekAndReset();
    TestInputBridgeLimits();
    TestMultiLaneIdentity();
    TestNormalizerIntegration();
    TestConfigureInvalidation();
    TestConfigureStageFailures();
    TestOutcomeCapacityCanary();
    TestWatermarkStateOrdering();
    TestChartAndSoundBacklog();
    TestSoundCrossingOrder();
    TestChartOffset();
    TestReleaseOnlySound();
    TestSeam();

    if (g_failures == 0) {
        std::printf("native_gameplay: all checks passed\n");
        return 0;
    }
    std::printf("native_gameplay: %d failure(s)\n", g_failures);
    return 1;
}
