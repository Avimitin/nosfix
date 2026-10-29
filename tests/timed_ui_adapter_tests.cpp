#include "nosfix/timed_ui_adapter.hpp"

#include "test_support.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

// The adapter is deliberately inert towards the game: it evaluates channels and
// hands them to a sink. These tests therefore prove the contract (identity,
// clocks, ordering, seek, release, no allocation) and never touch AFP.

using namespace nosfix;

namespace {

using Channels = std::size_t;
constexpr Channels kOpacity = static_cast<Channels>(UiChannel::Opacity);

UiNodeIdentity Identity(std::uintptr_t node, std::uint32_t stamp) {
    UiNodeIdentity identity;
    identity.node = node;
    identity.allocation_stamp = stamp;
    identity.parent = 0x100;
    identity.type = 3;
    identity.resource = 0x200;
    return identity;
}

const std::array<UiKeyframe, 2> kRamp{{
    {0.0, 0.0f, UiInterpolation::Linear},
    {1'000.0, 1.0f, UiInterpolation::Linear},
}};
const std::array<UiTimelineTrack, 1> kOpacityTrack{{{UiChannel::Opacity, kRamp}}};

struct Log {
    std::array<float, 320> applied{};
    std::size_t applied_count = 0;
    std::array<std::uint32_t, 320> events{};
    std::size_t event_count = 0;
    std::array<std::uintptr_t, 320> applied_nodes{};
    int order_mismatch = 0;
    int phase = 0;   // 1 after an apply, 2 after an event

    void Clear() {
        applied_count = 0;
        event_count = 0;
        order_mismatch = 0;
        phase = 0;
    }
    float Last() const { return applied_count ? applied[applied_count - 1] : -1.0f; }
};

void Apply(void* context, const UiNodeIdentity& node,
           const UiAnimationState& state) noexcept {
    auto* log = static_cast<Log*>(context);
    if (log->applied_count < log->applied.size()) {
        log->applied_nodes[log->applied_count] = node.node;
        log->applied[log->applied_count] = state.valid[kOpacity] ? state.values[kOpacity]
                                                                 : -2.0f;
        ++log->applied_count;
    }
    log->phase = 1;
}

void Event(void* context, const UiNodeIdentity& node, std::uint32_t id) noexcept {
    auto* log = static_cast<Log*>(context);
    // A callback must observe the state of the frame it belongs to.
    if (log->phase != 1) ++log->order_mismatch;
    (void)node;
    if (log->event_count < log->events.size()) log->events[log->event_count++] = id;
}

UiNodeSink SinkFor(Log& log) {
    UiNodeSink sink;
    sink.context = &log;
    sink.apply = &Apply;
    sink.event = &Event;
    return sink;
}

bool Near(float value, float expected) {
    return std::abs(value - expected) < 1.0e-5f;
}

// One frame: the phase counter proves a callback is never delivered in a frame
// that evaluated nothing.
UiAdapterStatus Frame(TimedUiAdapter& adapter, const UiNodeSink& sink, Log& log) {
    log.phase = 0;
    return adapter.Advance(sink);
}

// ---------------------------------------------------------------------------

void TestIdentityAndRecycle() {
    TimedUiAdapter adapter;
    const auto first = Identity(0x1000, 2);
    UiTimeline* timeline = adapter.Bind(first);
    CHECK(timeline != nullptr);
    CHECK(adapter.Size() == 1);
    CHECK(adapter.Bind(first) == timeline);
    CHECK(adapter.Find(first) == timeline);
    CHECK(timeline->Configured() == false);

    // Same address, new allocator stamp: the pool recycled the node.
    const auto recycled = Identity(0x1000, 3);
    UiTimeline* reused = adapter.Bind(recycled);
    CHECK(reused == timeline);
    CHECK(adapter.Find(recycled) == reused);
    CHECK(adapter.Find(first) == nullptr);
    CHECK(adapter.Size() == 1);

    // A null node is never bound.
    CHECK(adapter.Bind(Identity(0, 9)) == nullptr);

    // Fill the remaining slots; the next distinct node is refused instead of
    // evicting a live entry.
    const std::size_t bound = adapter.Size();
    for (std::uintptr_t index = 0; index + bound < adapter.Capacity(); ++index) {
        CHECK(adapter.Bind(Identity(0x20000 + index * 0x40, 1)) != nullptr);
    }
    CHECK(adapter.Size() == adapter.Capacity());
    CHECK(adapter.Bind(Identity(0x90000, 1)) == nullptr);
    CHECK(adapter.Size() == adapter.Capacity());
}

void TestBindExhaustionAndRelease() {
    TimedUiAdapter adapter;
    for (std::uintptr_t index = 0; index < adapter.Capacity(); ++index) {
        CHECK(adapter.Bind(Identity(0x30000 + index * 0x40, 1)) != nullptr);
    }
    const auto victim = Identity(0x30000, 1);
    CHECK(adapter.Find(victim) != nullptr);
    adapter.Release(victim);
    CHECK(adapter.Find(victim) == nullptr);
    CHECK(adapter.Size() == adapter.Capacity() - 1);
    CHECK(adapter.Counters().releases == 1);
    CHECK(adapter.Bind(Identity(0x91000, 4)) != nullptr);
    adapter.Release(Identity(0, 1));            // no-op, not counted
    CHECK(adapter.Counters().releases == 1);
    adapter.ReleaseNode(0x30040);
    CHECK(adapter.Counters().releases == 2);
    adapter.ReleaseNode(0);                     // no-op
    CHECK(adapter.Counters().releases == 2);
    adapter.Clear();
    CHECK(adapter.Size() == 0);
}

void TestSongDomainEvaluation() {
    TimedUiAdapter adapter;
    Log log;
    const UiNodeSink sink = SinkFor(log);
    const auto identity = Identity(0x4000, 1);
    UiTimeline* timeline = adapter.Bind(identity);
    CHECK(timeline->Configure(identity, UiTimeDomain::Song, kOpacityTrack, {}) ==
          UiTimelineDiagnostic::None);

    // No song time yet: nothing is evaluated.
    CHECK(Frame(adapter, sink, log) == UiAdapterStatus::Ok);
    CHECK(log.applied_count == 0);
    CHECK(!adapter.WallClockValid());

    double song = -1.0;
    CHECK(!adapter.Bank().DomainTime(UiTimeDomain::Song, song));

    adapter.SetSongTime(250.0, 7);
    CHECK(adapter.Bank().DomainTime(UiTimeDomain::Song, song));
    CHECK(song == 250.0);
    CHECK(adapter.Bank().DomainGeneration(UiTimeDomain::Song) == 7);
    CHECK(Frame(adapter, sink, log) == UiAdapterStatus::Ok);
    CHECK(log.applied_count == 1);
    CHECK(Near(log.Last(), 0.25f));
    CHECK(log.applied_nodes[0] == identity.node);

    adapter.SetSongTime(500.0, 7);
    CHECK(Frame(adapter, sink, log) == UiAdapterStatus::Ok);
    CHECK(log.applied_count == 2);
    CHECK(Near(log.Last(), 0.5f));

    // A paused song freezes every song-synced timeline.
    adapter.ClearSongTime();
    CHECK(Frame(adapter, sink, log) == UiAdapterStatus::Ok);
    CHECK(log.applied_count == 2);
}

void TestWallClockFromTicks() {
    TimedUiAdapter adapter;
    Log log;
    const UiNodeSink sink = SinkFor(log);
    const auto identity = Identity(0x5000, 1);
    UiTimeline* timeline = adapter.Bind(identity);
    CHECK(timeline->Configure(identity, UiTimeDomain::WallClock, kOpacityTrack, {}) ==
          UiTimelineDiagnostic::None);

    CHECK(!adapter.SetWallClockTicks(1000, 0));       // zero frequency
    CHECK(!adapter.SetWallClockTicks(-5, 1000));      // negative counter
    CHECK(!adapter.WallClockValid());
    CHECK(adapter.Counters().rejected_clock == 2);
    CHECK(Frame(adapter, sink, log) == UiAdapterStatus::Ok);
    CHECK(log.applied_count == 0);

    CHECK(adapter.SetWallClockTicks(1000, 1000));
    CHECK(Frame(adapter, sink, log) == UiAdapterStatus::Ok);
    CHECK(log.applied_count == 1);
    CHECK(Near(log.Last(), 0.0f));

    CHECK(adapter.SetWallClockTicks(1500, 1000));
    CHECK(Frame(adapter, sink, log) == UiAdapterStatus::Ok);
    CHECK(Near(log.Last(), 0.5f));

    // QPC never moves backwards; a backwards sample leaves the clock alone.
    CHECK(!adapter.SetWallClockTicks(1200, 1000));
    CHECK(Frame(adapter, sink, log) == UiAdapterStatus::Ok);
    CHECK(Near(log.Last(), 0.5f));
    CHECK(adapter.Counters().rejected_clock == 3);

    // A frequency change rebases on the published value instead of jumping.
    CHECK(adapter.SetWallClockTicks(2000, 2000));
    CHECK(Frame(adapter, sink, log) == UiAdapterStatus::Ok);
    CHECK(Near(log.Last(), 0.5f));
    CHECK(adapter.SetWallClockTicks(2500, 2000));     // +500 ticks at 2 kHz
    CHECK(Frame(adapter, sink, log) == UiAdapterStatus::Ok);
    CHECK(Near(log.Last(), 0.75f));
    CHECK(adapter.WallClockValid());
    CHECK(std::abs(adapter.WallMilliseconds() - 750.0) < 1.0e-6);
}

void TestManualWallClockRebases() {
    TimedUiAdapter adapter;
    Log log;
    const UiNodeSink sink = SinkFor(log);
    const auto identity = Identity(0x6000, 1);
    UiTimeline* timeline = adapter.Bind(identity);
    CHECK(timeline->Configure(identity, UiTimeDomain::WallClock, kOpacityTrack, {}) ==
          UiTimelineDiagnostic::None);
    CHECK(adapter.SetWallClock(0.0));
    CHECK(adapter.SetWallClock(100.0));
    CHECK(!adapter.SetWallClock(50.0));   // backwards
    CHECK(adapter.SetWallClock(200.0));
    CHECK(Frame(adapter, sink, log) == UiAdapterStatus::Ok);
    CHECK(Near(log.Last(), 0.2f));
    // A QPC sample after a manual value continues from that value.
    CHECK(adapter.SetWallClockTicks(4242, 1000));
    CHECK(Frame(adapter, sink, log) == UiAdapterStatus::Ok);
    CHECK(Near(log.Last(), 0.2f));
    CHECK(adapter.SetWallClockTicks(4342, 1000));
    CHECK(Frame(adapter, sink, log) == UiAdapterStatus::Ok);
    CHECK(Near(log.Last(), 0.3f));
}

void TestNullSinkNeverConsumes() {
    TimedUiAdapter adapter;
    Log log;
    const UiNodeSink sink = SinkFor(log);
    const auto identity = Identity(0x7000, 1);
    UiTimeline* timeline = adapter.Bind(identity);
    const std::array<UiTimelineEventSpec, 2> events{{{100.0, 1}, {300.0, 2}}};
    CHECK(timeline->Configure(identity, UiTimeDomain::Song, kOpacityTrack, events) ==
          UiTimelineDiagnostic::None);

    const UiNodeSink none;
    adapter.SetSongTime(50.0, 9);
    CHECK(adapter.Advance(none) == UiAdapterStatus::NoSink);
    CHECK(adapter.Counters().rejected_no_sink == 1);
    CHECK(log.applied_count == 0);
    CHECK(log.event_count == 0);

    CHECK(Frame(adapter, sink, log) == UiAdapterStatus::Ok);
    CHECK(log.applied_count == 1);
    CHECK(log.event_count == 0);          // 100 ms has not been crossed yet

    adapter.SetSongTime(300.0, 9);
    CHECK(Frame(adapter, sink, log) == UiAdapterStatus::Ok);
    CHECK(log.event_count == 2);
    CHECK(log.events[0] == 1);
    CHECK(log.events[1] == 2);
    CHECK(log.order_mismatch == 0);
    CHECK(adapter.Counters().events == 2);

    // A null event sink leaves crossings pending without consuming them.
    UiNodeSink apply_only = sink;
    apply_only.event = nullptr;
    adapter.SetSongTime(300.0, 9);
    CHECK(Frame(adapter, apply_only, log) == UiAdapterStatus::Ok);
    CHECK(log.event_count == 2);
}

void TestEventOrderAndOnce() {
    TimedUiAdapter adapter;
    Log log;
    const UiNodeSink sink = SinkFor(log);
    const auto identity = Identity(0x8000, 1);
    UiTimeline* timeline = adapter.Bind(identity);
    // Deliberately unsorted: the configuration sorts by time.
    const std::array<UiTimelineEventSpec, 4> events{
        {{300.0, 2}, {100.0, 1}, {400.0, 4}, {200.0, 3}}};
    CHECK(timeline->Configure(identity, UiTimeDomain::Song, kOpacityTrack, events) ==
          UiTimelineDiagnostic::None);

    adapter.SetSongTime(0.0, 3);
    CHECK(Frame(adapter, sink, log) == UiAdapterStatus::Ok);
    CHECK(log.event_count == 0);
    adapter.SetSongTime(200.0, 3);
    CHECK(Frame(adapter, sink, log) == UiAdapterStatus::Ok);
    CHECK(log.event_count == 2);
    CHECK(log.events[0] == 1);
    CHECK(log.events[1] == 3);
    // Repeating the same timestamp never re-delivers.
    CHECK(Frame(adapter, sink, log) == UiAdapterStatus::Ok);
    CHECK(log.event_count == 2);
    adapter.SetSongTime(400.0, 3);
    CHECK(Frame(adapter, sink, log) == UiAdapterStatus::Ok);
    CHECK(log.event_count == 4);
    CHECK(log.events[2] == 2);
    CHECK(log.events[3] == 4);
    // Jumping past everything at once still delivers in scheduled order.
    adapter.SetSongTime(900.0, 3);
    CHECK(Frame(adapter, sink, log) == UiAdapterStatus::Ok);
    CHECK(log.event_count == 4);
}

void TestStaleGeneration() {
    TimedUiAdapter adapter;
    Log log;
    const UiNodeSink sink = SinkFor(log);
    const auto identity = Identity(0x9000, 1);
    UiTimeline* timeline = adapter.Bind(identity);
    CHECK(timeline->Configure(identity, UiTimeDomain::Song, kOpacityTrack, {}) ==
          UiTimelineDiagnostic::None);

    adapter.SetSongTime(100.0, 8);
    double song = -1.0;
    CHECK(adapter.Bank().DomainTime(UiTimeDomain::Song, song));
    CHECK(song == 100.0);
    // An older producer must not rewind the epoch.
    adapter.SetSongTime(400.0, 7);
    CHECK(adapter.Bank().DomainTime(UiTimeDomain::Song, song));
    CHECK(song == 100.0);
    CHECK(adapter.Bank().DomainGeneration(UiTimeDomain::Song) == 8);
    CHECK(Frame(adapter, sink, log) == UiAdapterStatus::Ok);
    CHECK(Near(log.Last(), 0.1f));

    // Clearing the clock does not lower the floor.
    adapter.ClearSongTime();
    adapter.SetSongTime(900.0, 7);
    CHECK(!adapter.Bank().DomainTime(UiTimeDomain::Song, song));
    CHECK(Frame(adapter, sink, log) == UiAdapterStatus::Ok);
    CHECK(Near(log.Last(), 0.1f));
}

void TestSeekSkipsEarlierEvents() {
    TimedUiAdapter adapter;
    Log log;
    const UiNodeSink sink = SinkFor(log);
    const auto identity = Identity(0xA000, 1);
    UiTimeline* timeline = adapter.Bind(identity);
    const std::array<UiTimelineEventSpec, 3> events{{{400.0, 1}, {550.0, 2}, {700.0, 3}}};
    CHECK(timeline->Configure(identity, UiTimeDomain::Song, kOpacityTrack, events) ==
          UiTimelineDiagnostic::None);

    adapter.SetSongTime(100.0, 4);
    CHECK(Frame(adapter, sink, log) == UiAdapterStatus::Ok);
    CHECK(log.event_count == 0);

    // Seek inside a new epoch: everything before the seek point is skipped, and
    // an event exactly at the seek point is included.
    CHECK(timeline->Start(5, 550.0) == UiTimelineStatus::Ok);
    adapter.SetSongTime(600.0, 5);
    CHECK(Frame(adapter, sink, log) == UiAdapterStatus::Ok);
    CHECK(log.event_count == 1);
    CHECK(log.events[0] == 2);

    // Another seek in the same timeline never replays an older epoch.
    CHECK(timeline->Start(5, 650.0) == UiTimelineStatus::GenerationNotNew);
    CHECK(timeline->Start(6, 650.0) == UiTimelineStatus::Ok);
    adapter.SetSongTime(800.0, 6);
    CHECK(Frame(adapter, sink, log) == UiAdapterStatus::Ok);
    CHECK(log.event_count == 2);
    CHECK(log.events[1] == 3);
}

void TestReleaseStopsEvaluation() {
    TimedUiAdapter adapter;
    Log log;
    const UiNodeSink sink = SinkFor(log);
    const auto identity = Identity(0xB000, 1);
    UiTimeline* timeline = adapter.Bind(identity);
    CHECK(timeline->Configure(identity, UiTimeDomain::Song, kOpacityTrack, {}) ==
          UiTimelineDiagnostic::None);
    adapter.SetSongTime(200.0, 2);
    CHECK(Frame(adapter, sink, log) == UiAdapterStatus::Ok);
    CHECK(log.applied_count == 1);

    // Pool release drops the track so a recycled address inherits nothing.
    adapter.Release(identity);
    CHECK(Frame(adapter, sink, log) == UiAdapterStatus::Ok);
    CHECK(log.applied_count == 1);

    UiTimeline* recycled = adapter.Bind(Identity(0xB000, 7));
    CHECK(recycled != nullptr);
    CHECK(!recycled->Configured());
    CHECK(Frame(adapter, sink, log) == UiAdapterStatus::Ok);
    CHECK(log.applied_count == 1);
}

void TestDomainsStaySeparate() {
    TimedUiAdapter adapter;
    Log log;
    const UiNodeSink sink = SinkFor(log);
    const auto song = Identity(0xC000, 1);
    const auto wall = Identity(0xD000, 1);
    UiTimeline* song_timeline = adapter.Bind(song);
    UiTimeline* wall_timeline = adapter.Bind(wall);
    CHECK(song_timeline->Configure(song, UiTimeDomain::Song, kOpacityTrack, {}) ==
          UiTimelineDiagnostic::None);
    CHECK(wall_timeline->Configure(wall, UiTimeDomain::WallClock, kOpacityTrack, {}) ==
          UiTimelineDiagnostic::None);

    // Wall clock only: the song timeline must not move.
    CHECK(adapter.SetWallClock(250.0));
    CHECK(Frame(adapter, sink, log) == UiAdapterStatus::Ok);
    CHECK(log.applied_count == 1);
    CHECK(log.applied_nodes[0] == wall.node);
    CHECK(Near(log.Last(), 0.25f));

    // Song time only: the wall timeline keeps its own value.
    log.Clear();
    adapter.SetSongTime(500.0, 2);
    CHECK(Frame(adapter, sink, log) == UiAdapterStatus::Ok);
    CHECK(log.applied_count == 2);
    // Wall clock is evaluated before song time, so the wall node comes first.
    CHECK(log.applied_nodes[0] == wall.node);
    CHECK(log.applied_nodes[1] == song.node);
    CHECK(Near(log.applied[0], 0.25f));
    CHECK(Near(log.applied[1], 0.5f));
}

void TestNoAllocationPerFrame() {
    TimedUiAdapter adapter;
    Log log;
    const UiNodeSink sink = SinkFor(log);
    for (std::uintptr_t index = 0; index < 8; ++index) {
        const auto identity = Identity(0xE000 + index * 0x40, 1);
        UiTimeline* timeline = adapter.Bind(identity);
        CHECK(timeline->Configure(identity, UiTimeDomain::Song, kOpacityTrack, {}) ==
              UiTimelineDiagnostic::None);
    }
    adapter.SetSongTime(500.0, 2);
    CHECK(Frame(adapter, sink, log) == UiAdapterStatus::Ok);
    CHECK(log.applied_count == 8);

    log.Clear();
    test::BeginAllocationProbe();
    for (int frame = 0; frame < 30; ++frame) {
        adapter.SetSongTime(500.0 + frame, 2);
        CHECK(Frame(adapter, sink, log) == UiAdapterStatus::Ok);
    }
    const long long allocations = test::EndAllocationProbe();

    CHECK(allocations == 0);
    CHECK(log.applied_count == 8 * 30);
}

void TestStatusNames() {
    CHECK(std::strcmp(UiAdapterStatusName(UiAdapterStatus::Ok), "ok") == 0);
    CHECK(std::strcmp(UiAdapterStatusName(UiAdapterStatus::NoSink), "no-sink") == 0);
}

} // namespace

int main() {
    TestIdentityAndRecycle();
    TestBindExhaustionAndRelease();
    TestSongDomainEvaluation();
    TestWallClockFromTicks();
    TestManualWallClockRebases();
    TestNullSinkNeverConsumes();
    TestEventOrderAndOnce();
    TestStaleGeneration();
    TestSeekSkipsEarlierEvents();
    TestReleaseStopsEvaluation();
    TestDomainsStaySeparate();
    TestNoAllocationPerFrame();
    TestStatusNames();
    std::printf("timed_ui_adapter: all tests passed\n");
    return test::Finish("timed_ui_adapter");
}
