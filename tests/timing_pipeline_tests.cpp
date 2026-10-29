#ifdef NDEBUG
#undef NDEBUG
#endif

#include "nosfix/chart_event_cursor.hpp"
#include "nosfix/input_normalizer.hpp"
#include "nosfix/normal_judgment.hpp"
#include "nosfix/song_clock.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <vector>

namespace {
using namespace nosfix;

constexpr std::int64_t kAnchor = 100'000'000'000'000;
constexpr std::int64_t kFrequency = 10'000'000;
constexpr double kObservationUs = 2'000'000.0;
constexpr NormalWindows kWindows{1500.0, 2500.0, 5000.0, 7000.0, 10000.0, 15000.0};

std::int64_t Qpc(double wall_us) {
    return kAnchor + static_cast<std::int64_t>(std::llround(wall_us * 10.0));
}

struct Capture {
    PhysicalControlState state;
    InputStamp stamp;
    SongClockSegment segment;
    std::size_t note;
    double wall_us;
};

struct Fixture {
    SongClock clock;
    SongClockSegment before;
    SongClockSegment after;
    std::vector<Capture> reports;
    std::vector<double> notes;
    std::vector<NormalRank> expected;
    std::vector<ChartEvent> chart;

    Fixture() {
        assert(clock.Start(kAnchor, kFrequency));
        before = clock.Snapshot();
        // An audio observation made while input may be queued. It must not
        // retime earlier captures, including ones consumed after this update.
        assert(clock.Observe(Qpc(kObservationUs), kFrequency,
                             kObservationUs + 100.0, 100.0));
        after = clock.Snapshot();
        constexpr std::array<double, 6> deltas{0.0, 1250.0, -2250.0, 4500.0, 9500.0, -14500.0};
        constexpr std::array<NormalRank, 6> ranks{
            NormalRank::Just, NormalRank::Just, NormalRank::Just,
            NormalRank::Great, NormalRank::Good, NormalRank::Good};
        for (std::size_t i = 0; i < 120; ++i) {
            const double press_us = 200'000.0 + static_cast<double>(i / 2) * 100'000.0;
            const auto captured = Segment(press_us).Map(Qpc(press_us));
            assert(captured);
            notes.push_back(captured->microseconds + deltas[i % deltas.size()]);
            expected.push_back(ranks[i % ranks.size()]);
            for (unsigned edge = 0; edge < 3; ++edge) {
                const double wall = press_us + (edge == 2 ? 700.0 : edge * 100.0);
                reports.push_back({{1, 1, static_cast<std::uint32_t>(i % 28 + 1),
                                    edge != 2, edge == 0 ? 0.8f : 0.0f},
                                   {Qpc(wall), 0, 1, 0}, Segment(wall), i, wall});
            }
            // Equal-time events must keep source order; payload identifies the
            // note but the cursor never interprets it as a game ABI address.
            for (unsigned kind = 0; kind < 3; ++kind) {
                chart.push_back({i * 3 + kind + 1, notes.back() - 100'000.0,
                                 kind, static_cast<std::uint32_t>(i)});
            }
        }
        std::stable_sort(reports.begin(), reports.end(), [](const Capture& a, const Capture& b) {
            return a.wall_us < b.wall_us;
        });
        for (std::size_t i = 0; i < reports.size(); ++i) reports[i].stamp.sequence = i + 1;
    }

    const SongClockSegment& Segment(double wall_us) const {
        return wall_us < kObservationUs ? before : after;
    }
    SongTime At(double wall_us) const {
        const auto mapped = Segment(wall_us).Map(Qpc(wall_us));
        assert(mapped);
        return *mapped;
    }
};

struct Result {
    std::vector<LogicalInputEvent> edges;
    std::vector<NormalRank> ranks;
    std::vector<double> deltas;
    std::vector<std::uint64_t> events;
};

Result Replay(const Fixture& fixture, double hz, bool stall) {
    InputNormalizer input;
    std::array<InputBinding, 28> bindings{};
    for (std::uint8_t lane = 0; lane < bindings.size(); ++lane) {
        bindings[lane] = {1, 1, static_cast<std::uint32_t>(lane + 1), lane};
    }
    assert(input.Configure(bindings) == InputConfigStatus::Accepted);
    ChartEventCursor cursor;
    assert(cursor.Configure(fixture.chart) == ChartTableStatus::accepted);
    assert(cursor.Reset(1, 0.0) == ChartCursorStatus::ok);

    Result result;
    std::vector<bool> judged(fixture.notes.size());
    std::size_t read = 0;
    SongTime input_through{0.0, 1};
    std::size_t delayed_presses = 0;
    std::array<LogicalInputEvent, 28> edges{};
    std::array<ChartEvent, 2> events{};
    const double period = 1'000'000.0 / hz;
    double now = 0.0;
    for (unsigned frame = 0; now < 10'000'000.0; ++frame) {
        now += period * (frame % 7 == 0 ? 1.17 : 0.97);
        if (stall && now >= 900'000.0 && now < 2'430'000.0) now = 2'430'000.0;
        const SongTime current = fixture.At(now);

        // Only the producer/drainer can certify input completion. A render
        // time or chart-cursor watermark is not an input watermark.
        for (std::size_t i = read; i < fixture.reports.size(); ++i) {
            const auto& report = fixture.reports[i];
            if (!report.state.down || judged[report.note]) continue;
            const auto miss = DecideNormalMiss(fixture.notes[report.note], current,
                                               input_through, 1, 0.0, kWindows);
            assert(miss.ok() && !miss.finalize_miss);
        }

        // Deliberately bounded consumption leaves backlog after the stall.
        std::size_t capacity = 3;
        while (read < fixture.reports.size() && fixture.reports[read].wall_us <= now && capacity--) {
            const auto& report = fixture.reports[read];
            const auto applied = input.Apply(report.state, report.stamp, edges);
            assert(applied.Consumed());
            const auto captured = report.segment.Map(report.stamp.qpc);
            assert(captured);
            for (std::size_t i = 0; i < applied.event_count; ++i) {
                result.edges.push_back(edges[i]);
                const TimedInputEvent event{edges[i], *captured};
                if (event.input.edge != InputEdge::Press || event.input.synthetic) continue;
                assert(!judged[report.note]);
                const auto press = EvaluateNormalPress(fixture.notes[report.note],
                                                       event.song_time, 1, 0.0, kWindows);
                assert(press.ok() && press.eligible);
                assert(press.rank == fixture.expected[report.note]);
                result.ranks.push_back(press.rank);
                result.deltas.push_back(press.delta_us);
                judged[report.note] = true;
                if (current.microseconds > fixture.notes[report.note] + kWindows.good_late_us) {
                    ++delayed_presses;
                }
            }
            input_through = *captured;
            ++read;
        }
        // The fixture's producer guarantees no uncaptured reports through now.
        // This is a simulation contract, not something a consumer may infer
        // from an empty concurrent queue in the eventual live integration.
        if (read == fixture.reports.size() || fixture.reports[read].wall_us > now) {
            input_through = current;
        }
        assert(input_through.microseconds <= current.microseconds);

        const auto drained = cursor.Drain(current, events);
        assert(drained.ok());
        for (std::size_t i = 0; i < drained.written; ++i) result.events.push_back(events[i].id);
        if (const auto watermark = cursor.CompletedThrough(); watermark && cursor.NextEvent()) {
            assert(watermark->microseconds < cursor.NextEvent()->song_us);
        }
        if (read == fixture.reports.size() && cursor.PendingCount() == 0) break;
    }
    assert(read == fixture.reports.size());
    assert(input.HeldLaneMask() == 0);
    assert(result.edges.size() == fixture.notes.size() * 2);
    assert(result.ranks.size() == fixture.notes.size());
    assert(cursor.PendingCount() == 0);
    if (stall) assert(delayed_presses > 0);
    return result;
}

void SameResults(const Result& a, const Result& b) {
    assert(a.ranks == b.ranks && a.deltas == b.deltas && a.events == b.events);
    assert(a.edges.size() == b.edges.size());
    for (std::size_t i = 0; i < a.edges.size(); ++i) {
        const auto& x = a.edges[i];
        const auto& y = b.edges[i];
        assert(x.stamp.qpc == y.stamp.qpc && x.stamp.sequence == y.stamp.sequence);
        assert(x.stamp.ordinal == y.stamp.ordinal && x.stamp.stream_epoch == y.stamp.stream_epoch);
        assert(x.source_id == y.source_id && x.device_generation == y.device_generation);
        assert(x.lane == y.lane && x.edge == y.edge && x.velocity == y.velocity && x.synthetic == y.synthetic);
    }
}

void SeekRejectsQueuedInput(Fixture fixture) {
    const auto old_time = fixture.before.Map(Qpc(500'000.0));
    assert(old_time);
    assert(fixture.clock.Seek(Qpc(8'000'000.0), 0.0));
    const auto generation = fixture.clock.Generation();
    const auto press = EvaluateNormalPress(old_time->microseconds, *old_time, generation, 0.0, kWindows);
    assert(!press.ok() && !press.eligible);
    const auto current = fixture.clock.Snapshot().Map(Qpc(8'100'000.0));
    assert(current);
    const auto miss = DecideNormalMiss(0.0, *current, *old_time, generation, 0.0, kWindows);
    assert(!miss.ok() && !miss.finalize_miss);
}
} // namespace

int main() {
    const Fixture fixture;
    const auto reference = Replay(fixture, 1000.0, false);
    for (const double hz : {60.0, 90.0, 120.0, 144.0, 165.0, 240.0, 360.0}) {
        SameResults(reference, Replay(fixture, hz, false));
        SameResults(reference, Replay(fixture, hz, true));
    }
    SeekRejectsQueuedInput(fixture);
    std::puts("timing pipeline: identical edges, judgments and chart events at all seven cadences");
}
