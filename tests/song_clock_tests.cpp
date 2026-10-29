#ifdef NDEBUG
#undef NDEBUG
#endif
#include "nosfix/song_clock.hpp"

#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <limits>
#include <vector>

namespace {

constexpr std::int64_t kFrequency = 10'000'000;
constexpr std::int64_t kEpoch = 7'000'000'000'000'000'000LL;

struct Event final {
    std::int64_t qpc;
    std::size_t id;
};

std::array<double, 64> ReplayAtRate(const nosfix::SongClockSegment& segment,
                                   unsigned rate,
                                   const std::array<Event, 64>& events) {
    // This is a deterministic core test: render schedules only decide when
    // queued events are drained. Each event is mapped at its captured QPC,
    // independent of render cadence, jitter, stalls, or drain order.
    std::vector<std::int64_t> renders;
    renders.reserve(static_cast<std::size_t>(rate) * 4 + 8);
    std::int64_t tick = kEpoch;
    std::int64_t remainder = 0;
    for (std::size_t frame = 0; tick < kEpoch + 40'000'000; ++frame) {
        remainder += kFrequency;
        const auto step = remainder / static_cast<std::int64_t>(rate);
        remainder %= static_cast<std::int64_t>(rate);
        tick += step;

        // Bounded deterministic jitter; inject one 230 ms presentation stall.
        const auto jitter = static_cast<std::int64_t>((frame * 7919) % 2001) - 1000;
        if (frame == 90) tick += 2'300'000;
        renders.push_back(tick + jitter);
    }

    std::vector<std::vector<Event>> batches(renders.size());
    for (const auto& event : events) {
        std::size_t frame = 0;
        while (frame < renders.size() && renders[frame] < event.qpc) ++frame;
        assert(frame < renders.size());
        batches[frame].push_back(event);
    }

    std::array<double, 64> result{};
    for (std::size_t frame = 0; frame < batches.size(); ++frame) {
        auto& batch = batches[frame];
        if ((frame & 1U) != 0) {
            for (auto it = batch.rbegin(); it != batch.rend(); ++it) {
                const auto mapped = segment.Map(it->qpc);
                assert(mapped);
                result[it->id] = mapped->microseconds;
            }
        } else {
            for (const auto& event : batch) {
                const auto mapped = segment.Map(event.qpc);
                assert(mapped);
                result[event.id] = mapped->microseconds;
            }
        }
    }
    return result;
}

} // namespace

int main() {
    using nosfix::SongClock;
    using nosfix::SongClockSegment;

    SongClock clock;
    assert(clock.Start(kEpoch, kFrequency, 123'000.0));
    const auto generation = clock.Generation();
    const auto initial = clock.Snapshot();

    // Large QPC uptime is subtracted as integers first; sub-millisecond event
    // timestamps remain fractional in song microseconds.
    const auto fractional = initial.Map(kEpoch + 2'500);
    assert(fractional && fractional->generation == generation);
    assert(std::abs(fractional->microseconds - 123'250.0) < 1e-7);

    // A bounded observation may correct a small source discrepancy. The
    // caller explicitly supplies its uncertainty budget (10 us here).
    assert(clock.Observe(kEpoch + 10'000'000, kFrequency, 1'123'010.0, 10.0));
    const auto observed = clock.Snapshot();
    const auto at_observation = observed.Map(kEpoch + 10'000'000);
    assert(at_observation && at_observation->microseconds == 1'123'010.0);
    assert(observed.Generation() == generation);

    // Identical same-QPC observations are harmless; conflicting same-QPC
    // values are ambiguous and rejected without changing the mapping.
    assert(clock.Observe(kEpoch + 10'000'000, kFrequency, 1'123'010.0, 10.0));
    assert(!clock.Observe(kEpoch + 10'000'000, kFrequency, 1'123'011.0, 10.0));
    assert(!clock.Observe(kEpoch + 15'000'000, kFrequency + 1, 1'623'010.0, 10.0));
    assert(!clock.Observe(kEpoch + 15'000'000, kFrequency, 1'623'010.0, -1.0));
    assert(!clock.Observe(kEpoch + 15'000'000, kFrequency,
                          std::numeric_limits<double>::quiet_NaN(), 10.0));

    // Small bounded correction is accepted at a later anchor. A backwards
    // song position or over-budget correction is rejected transactionally.
    assert(clock.Observe(kEpoch + 20'000'000, kFrequency, 2'123'005.0, 10.0));
    const auto before_bad_observations = clock.Snapshot();
    assert(!clock.Observe(kEpoch + 30'000'000, kFrequency, 2'500.0, 10'000'000.0));
    assert(!clock.Observe(kEpoch + 30'000'000, kFrequency, 4'123'005.0, 100.0));
    assert(!clock.Observe(kEpoch + 30'000'000, kFrequency, 3'123'005.0,
                          std::numeric_limits<double>::infinity()));
    assert(clock.Snapshot().Map(kEpoch + 30'000'000)->microseconds ==
           before_bad_observations.Map(kEpoch + 30'000'000)->microseconds);

    // Captured event snapshots retain their original mapping after later
    // observations, and a backwards observation cannot retime future events.
    assert(initial.Map(kEpoch + 2'500)->microseconds == fractional->microseconds);
    const auto stable_before_seek = clock.Snapshot();
    const auto stable_prediction = stable_before_seek.Map(kEpoch + 35'000'000);
    assert(stable_prediction);

    // Pause freezes at the explicit QPC. Resume rejects a backwards counter
    // value, then resumes from the held position without counting pause time.
    assert(clock.Pause(kEpoch + 30'000'000));
    const auto paused = clock.Snapshot();
    assert(paused.IsValid() && !paused.IsRunning());
    assert(!paused.Map(kEpoch + 40'000'000));
    assert(!clock.Pause(kEpoch + 40'000'000));
    assert(!clock.Resume(kEpoch + 29'000'000));
    assert(!clock.Resume(kEpoch + 29'999'999));
    assert(clock.Resume(kEpoch + 100'000'000));
    const auto resumed = clock.Snapshot().Map(kEpoch + 102'500'000);
    assert(resumed && resumed->generation == generation);
    assert(std::abs(resumed->microseconds - 3'373'005.0) < 1e-6);

    // Seek is explicit and creates a new generation. It may move song time
    // backwards, but its QPC anchor must remain monotonic.
    const auto pre_seek = clock.Snapshot();
    assert(!clock.Seek(kEpoch + 99'000'000, 50'000.0));
    assert(clock.Generation() == generation && clock.IsRunning());
    assert(clock.Seek(kEpoch + 110'000'000, 50'000.0));
    assert(clock.Generation() == generation + 1);
    const auto post_seek = clock.Snapshot();
    assert(pre_seek.Map(kEpoch + 112'500'000)->generation == generation);
    assert(post_seek.Map(kEpoch + 112'500'000)->generation == generation + 1);
    assert(std::abs(post_seek.Map(kEpoch + 112'500'000)->microseconds - 300'000.0) < 1e-7);
    assert(!clock.Start(kEpoch + 109'000'000, kFrequency, 0.0));
    assert(clock.Generation() == generation + 1);

    // Invalid start inputs and generation changes are transactional.
    SongClock invalid;
    assert(!invalid.Start(100, 0, 0.0));
    assert(!invalid.Snapshot().Map(100));
    assert(!invalid.Start(100, -1, 0.0));
    const auto before_bad_start = clock.Snapshot();
    assert(!clock.Start(kEpoch + 120'000'000, 0, 0.0));
    assert(clock.Generation() == generation + 1);
    assert(clock.Snapshot().Map(kEpoch + 120'000'000)->microseconds ==
           before_bad_start.Map(kEpoch + 120'000'000)->microseconds);

    // Delta overflow is rejected before conversion to floating point.
    SongClock near_limit;
    assert(near_limit.Start(std::numeric_limits<std::int64_t>::min(),
                            std::numeric_limits<std::int64_t>::max(), 0.0));
    assert(!near_limit.Snapshot().Map(std::numeric_limits<std::int64_t>::max()));

    // Song timestamps are stable under 60..360 Hz render schedules, jitter,
    // a long render stall, and different event-batch drain orders. This tests
    // the clock core only; it does not prove game-hook integration.
    const auto replay_segment = initial;
    std::array<Event, 64> events{};
    for (std::size_t i = 0; i < events.size(); ++i) {
        events[i] = Event{kEpoch + static_cast<std::int64_t>(i) * 491'357, i};
    }
    const auto baseline = ReplayAtRate(replay_segment, 60, events);
    for (const unsigned rate : {90U, 120U, 144U, 165U, 240U, 360U}) {
        const auto replay = ReplayAtRate(replay_segment, rate, events);
        for (std::size_t i = 0; i < events.size(); ++i) {
            assert(replay[i] == baseline[i]);
        }
    }

    // Continuous rate adjustment: the segment joins at the current mapped value
    // at the requested QPC, later time uses the new slope, and snapshots taken
    // before the change retain their original mapping.
    {
        SongClock rate_clock;
        assert(rate_clock.Start(kEpoch, kFrequency, 0.0));
        const auto before = rate_clock.Snapshot();
        assert(before.Rate() == 1.0);
        assert(rate_clock.ChangeRate(kEpoch + 10'000'000, 1.01));
        const auto after = rate_clock.Snapshot();
        assert(after.Rate() == 1.01);
        assert(after.Map(kEpoch + 10'000'000)->microseconds ==
               before.Map(kEpoch + 10'000'000)->microseconds);
        assert(std::abs(after.Map(kEpoch + 20'000'000)->microseconds -
                        2'010'000.0) < 1e-6);
        assert(std::abs(before.Map(kEpoch + 20'000'000)->microseconds -
                        2'000'000.0) < 1e-6);
        assert(!rate_clock.ChangeRate(kEpoch + 9'000'000, 1.0));
        assert(!rate_clock.ChangeRate(kEpoch + 20'000'000, 0.0));
        assert(!rate_clock.ChangeRate(kEpoch + 20'000'000,
                                      std::numeric_limits<double>::quiet_NaN()));
        assert(rate_clock.ChangeRate(kEpoch + 20'000'000, 0.99));
        assert(std::abs(rate_clock.Snapshot().Map(kEpoch + 20'000'000)->microseconds -
                        2'010'000.0) < 1e-6);
        assert(rate_clock.Seek(kEpoch + 30'000'000, 0.0));
        assert(rate_clock.Snapshot().Rate() == 1.0);
        assert(std::abs(rate_clock.Snapshot().Map(kEpoch + 30'000'000)->microseconds) <
               1e-6);
    }

    const auto old_snapshot = clock.Snapshot();
    assert(clock.Stop());
    assert(clock.Generation() == generation + 2);
    assert(!clock.IsRunning() && !clock.Snapshot().Map(kEpoch + 130'000'000));
    assert(old_snapshot.Map(kEpoch + 120'000'000));
    assert(!clock.Resume(kEpoch + 130'000'000));
}
