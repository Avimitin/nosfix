#ifdef NDEBUG
#undef NDEBUG
#endif
#include "nosfix/gameplay_clock.hpp"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <limits>

namespace {

int g_failures = 0;

void Check(bool condition, const char* text, int line) {
    if (!condition) {
        std::printf("FAIL line %d: %s\n", line, text);
        ++g_failures;
    }
}

#define CHECK(expr) Check((expr), #expr, __LINE__)

bool Close(double a, double b) { return std::abs(a - b) < 1e-6; }

using nosfix::AudioObservation;
using nosfix::AudioObservationPolicy;
using nosfix::AudioObservationSource;
using nosfix::AudioObservationStatus;
using nosfix::GameplayClock;

constexpr std::int64_t kFrequency = 10'000'000;
constexpr std::int64_t kEpoch = 7'000'000'000'000'000'000LL;

AudioObservation Observation(std::int64_t qpc, double song_us,
                             std::uint64_t generation,
                             AudioObservationSource source) {
    AudioObservation observation;
    observation.qpc = qpc;
    observation.qpc_frequency = kFrequency;
    observation.song_us = song_us;
    observation.generation = generation;
    observation.source = source;
    return observation;
}

void TestBasicAndRateControl() {
    GameplayClock clock;
    CHECK(!clock.Now(kEpoch));
    CHECK(clock.Observe(Observation(kEpoch, 0.0, 1,
                                    AudioObservationSource::BgmCueLabel)) ==
          AudioObservationStatus::RejectedNotRunning);

    CHECK(clock.Start(kEpoch, kFrequency, 100'000.0));
    CHECK(clock.Generation() == 1);
    CHECK(clock.IsRunning());
    CHECK(clock.Rate() == 1.0);
    CHECK(clock.AnchorSource() == AudioObservationSource::Unknown);

    const auto initial = clock.Now(kEpoch + 2'500'000);
    CHECK(initial.has_value());
    CHECK(Close(initial->microseconds, 350'000.0));
    CHECK(initial->generation == 1);

    const auto captured_segment = clock.Snapshot();
    const auto captured_before = captured_segment.Map(kEpoch + 2'500'000);
    CHECK(captured_before.has_value());

    // A cue-label observation 500 us ahead of prediction sets a continuous
    // rate: the map at the observation QPC is unchanged, and later time runs
    // faster. It is engine/source progress, recorded as the anchor source.
    const double at_ten = 1'100'000.0;
    CHECK(clock.Observe(Observation(kEpoch + 10'000'000, at_ten + 500.0, 1,
                                    AudioObservationSource::BgmCueLabel)) ==
          AudioObservationStatus::AcceptedSlew);
    CHECK(clock.AnchorSource() == AudioObservationSource::BgmCueLabel);
    CHECK(clock.AcceptedObservations() == 1);
    CHECK(Close(clock.LastErrorUs(), 500.0));
    CHECK(Close(clock.LastRate(), 1.00025));
    CHECK(!clock.RateClamped());
    const auto at_anchor = clock.Now(kEpoch + 10'000'000);
    CHECK(Close(at_anchor->microseconds, 1'100'000.0));
    const auto after_cue = clock.Now(kEpoch + 20'000'000);
    CHECK(after_cue.has_value());
    CHECK(Close(after_cue->microseconds, 2'100'250.0));
    CHECK(Close(captured_segment.Map(kEpoch + 2'500'000)->microseconds,
                captured_before->microseconds));

    // A device observation is a better anchor and adjusts the slope again.
    CHECK(clock.Observe(Observation(kEpoch + 30'000'000, 3'100'600.0, 1,
                                    AudioObservationSource::DeviceRender)) ==
          AudioObservationStatus::AcceptedSlew);
    CHECK(clock.AnchorSource() == AudioObservationSource::DeviceRender);
    CHECK(Close(clock.Now(kEpoch + 30'000'000)->microseconds, 3'100'500.0));
    CHECK(Close(clock.LastRate(), 1.0001));

    // Device quality is now the floor; a cue observation cannot replace it.
    CHECK(clock.Observe(Observation(kEpoch + 40'000'000, 4'100'700.0, 1,
                                    AudioObservationSource::BgmCueLabel)) ==
          AudioObservationStatus::RejectedQualityRegression);

    // Budget and finite-value rejections leave the mapping.
    const auto before_rejects = clock.Now(kEpoch + 40'000'000);
    CHECK(clock.Observe(Observation(kEpoch + 40'000'000, 4'101'700.0, 1,
                                    AudioObservationSource::DeviceRender)) ==
          AudioObservationStatus::RejectedOutOfBudget);
    // A small audio-behind error is corrected through a slower rate, not a
    // backwards snap.
    CHECK(clock.Observe(Observation(kEpoch + 40'000'000, 4'100'500.0, 1,
                                    AudioObservationSource::DeviceRender)) ==
          AudioObservationStatus::AcceptedSlew);
    CHECK(Close(clock.LastErrorUs(), -100.0));
    CHECK(clock.Rate() < 1.0);
    CHECK(Close(clock.PendingNegativeErrorUs(), 0.0));
    CHECK(clock.Observe(Observation(kEpoch + 40'000'000, 4'098'000.0, 1,
                                    AudioObservationSource::DeviceRender)) ==
          AudioObservationStatus::RejectedOutOfBudget);
    CHECK(clock.Observe(Observation(kEpoch + 40'000'000,
                                    std::numeric_limits<double>::quiet_NaN(), 1,
                                    AudioObservationSource::DeviceRender)) ==
          AudioObservationStatus::RejectedNonFinite);
    CHECK(clock.Observe(Observation(kEpoch + 40'000'000, 4'100'450.0, 0,
                                    AudioObservationSource::DeviceRender)) ==
          AudioObservationStatus::RejectedGeneration);
    CHECK(clock.Observe(Observation(kEpoch + 40'000'000, 4'100'450.0, 2,
                                    AudioObservationSource::DeviceRender)) ==
          AudioObservationStatus::RejectedGeneration);
    CHECK(clock.Observe(Observation(kEpoch + 40'000'000, 4'100'450.0, 1,
                                    AudioObservationSource::Unknown)) ==
          AudioObservationStatus::RejectedSource);
    {
        AudioObservation wrong_frequency = Observation(
            kEpoch + 40'000'000, 4'100'500.0, 1,
            AudioObservationSource::DeviceRender);
        wrong_frequency.qpc_frequency += 1;
        CHECK(clock.Observe(wrong_frequency) ==
              AudioObservationStatus::RejectedFrequency);
    }
    CHECK(clock.Observe(Observation(-1, 4'100'450.0, 1,
                                    AudioObservationSource::DeviceRender)) ==
          AudioObservationStatus::RejectedNonFinite);
    CHECK(clock.Now(kEpoch + 40'000'000)->microseconds ==
          before_rejects->microseconds);

    // A backward QPC after an accepted observation is rejected.
    CHECK(clock.Observe(Observation(kEpoch + 20'000'000, 2'100'000.0, 1,
                                    AudioObservationSource::DeviceRender)) ==
          AudioObservationStatus::RejectedBackward);

    // Pause freezes at the mapped value; no render call advances it.
    const auto paused_position = clock.Now(kEpoch + 50'000'000);
    CHECK(paused_position.has_value());
    CHECK(clock.Pause(kEpoch + 50'000'000));
    CHECK(!clock.Now(kEpoch + 50'000'000).has_value());
    CHECK(clock.Observe(Observation(kEpoch + 60'000'000, 5'100'000.0, 1,
                                    AudioObservationSource::DeviceRender)) ==
          AudioObservationStatus::RejectedNotRunning);
    CHECK(clock.Resume(kEpoch + 60'000'000));
    CHECK(Close(clock.Now(kEpoch + 60'000'000)->microseconds,
                paused_position->microseconds));

    // Seek starts a new generation at rate 1 and invalidates old observations.
    CHECK(clock.Seek(kEpoch + 70'000'000, 50'000.0));
    CHECK(clock.Generation() == 2);
    CHECK(clock.Rate() == 1.0);
    CHECK(clock.AnchorSource() == AudioObservationSource::Unknown);
    CHECK(Close(clock.Now(kEpoch + 72'000'000)->microseconds, 250'000.0));
    CHECK(clock.Observe(Observation(kEpoch + 75'000'000, 550'000.0, 1,
                                    AudioObservationSource::DeviceRender)) ==
          AudioObservationStatus::RejectedGeneration);
    CHECK(clock.Observe(Observation(kEpoch + 75'000'000, 550'100.0, 2,
                                    AudioObservationSource::DeviceRender)) ==
          AudioObservationStatus::AcceptedSlew);
    CHECK(Close(clock.Now(kEpoch + 75'000'000)->microseconds, 550'000.0));

    // A disabled source is rejected explicitly rather than silently used.
    AudioObservationPolicy no_cue;
    no_cue.accept_cue_label = false;
    GameplayClock cue_disabled(no_cue);
    CHECK(cue_disabled.Start(kEpoch, kFrequency, 0.0));
    CHECK(cue_disabled.Observe(Observation(kEpoch + 10'000'000, 1'000'000.0,
                                           cue_disabled.Generation(),
                                           AudioObservationSource::BgmCueLabel)) ==
          AudioObservationStatus::RejectedSource);

    // Invalid policy fails closed (rate bounds and budgets are validated).
    AudioObservationPolicy invalid;
    invalid.max_rate_delta = 0.0;
    GameplayClock bad_policy(invalid);
    CHECK(bad_policy.Start(kEpoch, kFrequency, 0.0));
    CHECK(bad_policy.Observe(Observation(kEpoch + 10'000'000, 1'000'000.0,
                                         bad_policy.Generation(),
                                         AudioObservationSource::DeviceRender)) ==
          AudioObservationStatus::RejectedPolicy);
}

void TestLegacyStepPolicyAndPreroll() {
    AudioObservationPolicy legacy;
    legacy.use_rate_control = false;
    legacy.device_correction_us = 1000.0;
    GameplayClock clock(legacy);
    CHECK(clock.Start(kEpoch, kFrequency, 0.0));
    CHECK(clock.Observe(Observation(kEpoch + 10'000'000, 999'500.0, 1,
                                    AudioObservationSource::DeviceRender)) ==
          AudioObservationStatus::RejectedNegativeDrift);
    CHECK(Close(clock.PendingNegativeErrorUs(), 500.0));
    CHECK(clock.Observe(Observation(kEpoch + 10'000'000, 1'000'300.0, 1,
                                    AudioObservationSource::DeviceRender)) ==
          AudioObservationStatus::Accepted);
    CHECK(Close(clock.Now(kEpoch + 10'000'000)->microseconds, 1'000'300.0));

    // Negative song positions are legitimate preroll, and Seek may target one.
    GameplayClock preroll;
    CHECK(preroll.Start(kEpoch, kFrequency, -3'333.333333));
    CHECK(Close(preroll.Now(kEpoch)->microseconds, -3'333.333333));
    CHECK(Close(preroll.Now(kEpoch + 1'000'000)->microseconds, 96'666.666667));
    CHECK(preroll.Seek(kEpoch + 2'000'000, -5'000.0));
    CHECK(Close(preroll.Now(kEpoch + 2'500'000)->microseconds, 45'000.0));
}

void TestDriftControl() {
    const auto run = [](double drift, bool skip_middle) {
        GameplayClock clock;
        CHECK(clock.Start(kEpoch, kFrequency, 0.0));
        double last_now = -1.0;
        double last_observed = 0.0;
        bool all_accepted = true;
        bool monotonic = true;
        for (int i = 1; i <= 600; ++i) {
            if (skip_middle && i >= 300 && i <= 310) continue;
            const double seconds = static_cast<double>(i) * 0.1;
            const auto qpc = kEpoch + static_cast<std::int64_t>(seconds * 1.0e7);
            const double noise = static_cast<double>((i * 7919) % 201 - 100);
            const double observed = seconds * 1.0e6 * (1.0 + drift) + noise;
            const auto status = clock.Observe(
                Observation(qpc, observed, 1, AudioObservationSource::DeviceRender));
            if (status != AudioObservationStatus::AcceptedSlew) all_accepted = false;
            const auto now = clock.Now(qpc);
            if (!now || now->microseconds < last_now - 1e-9) monotonic = false;
            if (now) last_now = now->microseconds;
            last_observed = observed;
        }
        CHECK(all_accepted);
        CHECK(monotonic);
        CHECK(std::abs(last_observed - last_now) < 500.0);
        CHECK(clock.Rate() > 0.98 && clock.Rate() < 1.02);
        if (drift < 0.0) CHECK(clock.Rate() < 1.0);
        if (drift > 0.0) CHECK(clock.Rate() > 1.0);
    };
    run(-0.0005, false);
    run(0.0005, false);
    run(-0.0005, true);
    run(0.0005, true);
    run(-0.00005, false);
    run(0.00005, false);
}

void TestSnapshotInterleaving() {
    GameplayClock clock;
    CHECK(clock.Start(kEpoch, kFrequency, 0.0));
    const auto old_segment = clock.Snapshot();
    CHECK(Close(old_segment.Map(kEpoch + 2'000'000)->microseconds, 200'000.0));

    // A rate change at 0.1 s joins continuously at that QPC, but an old
    // snapshot captured before the change maps a later QPC differently. This is
    // the producer-ordering hazard: the snapshot and its QPC must be acquired
    // under one serialization owned by the producer, not copied separately.
    CHECK(clock.Observe(Observation(kEpoch + 1'000'000, 100'500.0, 1,
                                    AudioObservationSource::DeviceRender)) ==
          AudioObservationStatus::AcceptedSlew);
    const auto new_segment = clock.Snapshot();
    CHECK(Close(old_segment.Map(kEpoch + 1'000'000)->microseconds, 100'000.0));
    CHECK(Close(new_segment.Map(kEpoch + 1'000'000)->microseconds, 100'000.0));
    CHECK(Close(old_segment.Map(kEpoch + 2'000'000)->microseconds, 200'000.0));
    CHECK(Close(new_segment.Map(kEpoch + 2'000'000)->microseconds, 200'025.0));
    CHECK(!Close(old_segment.Map(kEpoch + 2'000'000)->microseconds,
                 new_segment.Map(kEpoch + 2'000'000)->microseconds));

    // The old snapshot stays exactly immutable after further observations.
    CHECK(clock.Observe(Observation(kEpoch + 3'000'000, 300'150.0, 1,
                                    AudioObservationSource::DeviceRender)) ==
          AudioObservationStatus::AcceptedSlew);
    CHECK(Close(old_segment.Map(kEpoch + 2'000'000)->microseconds, 200'000.0));
    CHECK(clock.Rate() > 1.0);
}

} // namespace

int main() {
    TestBasicAndRateControl();
    TestLegacyStepPolicyAndPreroll();
    TestDriftControl();
    TestSnapshotInterleaving();

    if (g_failures == 0) {
        std::printf("gameplay_clock: all checks passed\n");
        return 0;
    }
    std::printf("gameplay_clock: %d failure(s)\n", g_failures);
    return 1;
}
