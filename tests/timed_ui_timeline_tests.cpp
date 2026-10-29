#include "nosfix/timed_ui_timeline.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <vector>

namespace {

void CheckImpl(bool condition, const char* expression, int line) {
    if (!condition) {
        std::fprintf(stderr, "timed-ui-timeline test failed at line %d: %s\n",
                     line, expression);
        std::abort();
    }
}
#define Check(condition) CheckImpl((condition), #condition, __LINE__)

using nosfix::UiChannel;
using nosfix::UiInterpolation;
using nosfix::UiKeyframe;
using nosfix::UiNodeIdentity;
using nosfix::UiTimelineStatus;

constexpr std::size_t kChannel = static_cast<std::size_t>(UiChannel::Count);

UiNodeIdentity Identity(std::uintptr_t node, std::uint32_t stamp) {
    UiNodeIdentity identity;
    identity.node = node;
    identity.allocation_stamp = stamp;
    identity.parent = 0x100;
    identity.type = 3;
    identity.resource = 0x200;
    return identity;
}

struct EventLog {
    std::vector<std::uint32_t> ids;
    const UiNodeIdentity* expected_node = nullptr;
    bool wrong_node = false;
};

void Sink(void* context, const UiNodeIdentity& node, std::uint32_t id) noexcept {
    auto* log = static_cast<EventLog*>(context);
    log->ids.push_back(id);
    if (log->expected_node && !(node == *log->expected_node)) log->wrong_node = true;
}

const std::array<UiKeyframe, 1> kFlat{{{0.0, 1.0f, UiInterpolation::Step}}};
const std::array<nosfix::UiTimelineTrack, 1> kOpacityTrack{{{UiChannel::Opacity, kFlat}}};

void TestInterpolation() {
    const std::array<UiKeyframe, 2> linear{{{0.0, 0.0f, UiInterpolation::Linear},
                                            {1'000.0, 100.0f, UiInterpolation::Linear}}};
    const std::array<UiKeyframe, 2> step{{{0.0, 0.0f, UiInterpolation::Step},
                                          {1'000.0, 100.0f, UiInterpolation::Step}}};
    const std::array<UiKeyframe, 2> smooth{{{0.0, 0.0f, UiInterpolation::SmoothStep},
                                            {1'000.0, 100.0f, UiInterpolation::SmoothStep}}};
    const std::array<UiKeyframe, 3> opacity{{{0.0, 0.2f, UiInterpolation::Linear},
                                             {500.0, 1.5f, UiInterpolation::Linear},
                                             {1'000.0, -1.0f, UiInterpolation::Linear}}};

    const std::array<nosfix::UiTimelineTrack, 2> tracks{{
        {UiChannel::TranslateX, linear},
        {UiChannel::Opacity, opacity},
    }};

    nosfix::UiTimeline timeline;
    Check(timeline.Configure(Identity(0x1000, 1), nosfix::UiTimeDomain::WallClock,
                             tracks, {}) == nosfix::UiTimelineDiagnostic::None);
    nosfix::UiAnimationState state;
    Check(timeline.Evaluate(250.0, state));
    Check(state.valid[static_cast<std::size_t>(UiChannel::TranslateX)]);
    Check(std::abs(state.values[static_cast<std::size_t>(UiChannel::TranslateX)] - 25.0f) < 1e-5f);
    Check(std::abs(state.values[static_cast<std::size_t>(UiChannel::Opacity)] - 0.85f) < 1e-5f);
    Check(!state.valid[static_cast<std::size_t>(UiChannel::ColorR)]);

    Check(timeline.Evaluate(750.0, state));
    Check(std::abs(state.values[static_cast<std::size_t>(UiChannel::Opacity)] - 0.25f) < 1e-5f);
    Check(std::abs(state.values[static_cast<std::size_t>(UiChannel::TranslateX)] - 75.0f) < 1e-5f);

    Check(timeline.Evaluate(2'000.0, state));
    Check(state.values[static_cast<std::size_t>(UiChannel::TranslateX)] == 100.0f);
    Check(state.values[static_cast<std::size_t>(UiChannel::Opacity)] == 0.0f);
    Check(timeline.Evaluate(-5.0, state));
    Check(state.values[static_cast<std::size_t>(UiChannel::TranslateX)] == 0.0f);
    Check(std::abs(state.values[static_cast<std::size_t>(UiChannel::Opacity)] - 0.2f) < 1e-5f);

    const std::array<nosfix::UiTimelineTrack, 1> step_tracks{{
        {UiChannel::ScaleX, step},
    }};
    nosfix::UiTimeline step_timeline;
    Check(step_timeline.Configure(Identity(0x1001, 1), nosfix::UiTimeDomain::WallClock,
                                  step_tracks, {}) == nosfix::UiTimelineDiagnostic::None);
    Check(step_timeline.Evaluate(999.0, state));
    Check(state.values[static_cast<std::size_t>(UiChannel::ScaleX)] == 0.0f);
    Check(step_timeline.Evaluate(1'000.0, state));
    Check(state.values[static_cast<std::size_t>(UiChannel::ScaleX)] == 100.0f);

    const std::array<nosfix::UiTimelineTrack, 1> smooth_tracks{{
        {UiChannel::ScaleX, smooth},
    }};
    nosfix::UiTimeline smooth_timeline;
    Check(smooth_timeline.Configure(Identity(0x1002, 1), nosfix::UiTimeDomain::WallClock,
                                    smooth_tracks, {}) == nosfix::UiTimelineDiagnostic::None);
    Check(smooth_timeline.Evaluate(500.0, state));
    Check(std::abs(state.values[static_cast<std::size_t>(UiChannel::ScaleX)] - 50.0f) < 1e-4f);
    Check(smooth_timeline.Evaluate(250.0, state));
    const float quarter = state.values[static_cast<std::size_t>(UiChannel::ScaleX)];
    Check(quarter > 0.0f && quarter < 25.0f);
    Check(smooth_timeline.Evaluate(750.0, state));
    Check(std::abs(state.values[static_cast<std::size_t>(UiChannel::ScaleX)] -
                   (100.0f - quarter)) < 1e-4f);
    Check(!timeline.Evaluate(std::numeric_limits<double>::infinity(), state));
}

void TestInclusiveStartAndFirstObservation() {
    const std::array<nosfix::UiTimelineEventSpec, 3> events{{
        {500.0, 5}, {1'000.0, 10}, {1'500.0, 15},
    }};
    const auto identity = Identity(0x2000, 1);
    nosfix::UiTimeline timeline;
    Check(timeline.Configure(identity, nosfix::UiTimeDomain::Song, kOpacityTrack, events) ==
          nosfix::UiTimelineDiagnostic::None);
    EventLog log;
    log.expected_node = &identity;

    // Nothing is delivered before an explicit epoch.
    Check(timeline.Advance(2'000.0, 1, Sink, &log).status == UiTimelineStatus::NotStarted);
    Check(log.ids.empty());

    // An epoch starting exactly on an event includes it.
    Check(timeline.Start(1, 1'000.0) == UiTimelineStatus::Ok);
    Check(timeline.Advance(1'000.0, 1, Sink, &log).delivered == 1);
    Check(log.ids.size() == 1 && log.ids[0] == 10);
    // Repeated observation of the same instant delivers nothing more.
    Check(timeline.Advance(1'000.0, 1, Sink, &log).delivered == 0);
    Check(timeline.Advance(1'000.0, 1, Sink, &log).delivered == 0);
    Check(log.ids.size() == 1);
    // Events before the start are skipped by the explicit start policy.
    Check(timeline.Advance(1'400.0, 1, Sink, &log).delivered == 0);
    Check(timeline.Advance(1'500.0, 1, Sink, &log).delivered == 1);
    Check(log.ids.size() == 2 && log.ids[1] == 15);
    Check(!log.wrong_node);
}

void TestRejectionsAreNoOps() {
    const std::array<nosfix::UiTimelineEventSpec, 3> events{{
        {100.0, 1}, {200.0, 2}, {300.0, 3},
    }};
    nosfix::UiTimeline timeline;
    EventLog log;

    // Not configured.
    Check(timeline.Advance(0.0, 1, Sink, &log).status == UiTimelineStatus::NotConfigured);
    Check(timeline.Start(1, 0.0) == UiTimelineStatus::NotConfigured);

    Check(timeline.Configure(Identity(0x3000, 7), nosfix::UiTimeDomain::Song,
                             kOpacityTrack, events) == nosfix::UiTimelineDiagnostic::None);
    // Generation rules.
    Check(timeline.Start(0, 0.0) == UiTimelineStatus::GenerationZero);
    Check(timeline.Start(1, std::numeric_limits<double>::quiet_NaN()) ==
          UiTimelineStatus::NonFiniteTime);
    Check(timeline.Start(4, 0.0) == UiTimelineStatus::Ok);

    Check(timeline.Start(4, 0.0) == UiTimelineStatus::GenerationNotNew);
    Check(timeline.Start(3, 0.0) == UiTimelineStatus::GenerationNotNew);
    Check(timeline.GenerationFloor() == 4);

    // Priming advance, then a stall crossing every event.
    Check(timeline.Advance(0.0, 4, Sink, &log).delivered == 0);
    const auto stall = timeline.Advance(1'000.0, 4, Sink, &log);
    Check(stall.status == UiTimelineStatus::Ok && stall.delivered == 3);
    Check(log.ids.size() == 3 && log.ids[0] == 1 && log.ids[1] == 2 && log.ids[2] == 3);

    // Stale generation and backwards time change nothing, including pending
    // events: a later epoch still has to restart explicitly.
    const double before = timeline.TimeMs();
    Check(timeline.Advance(1'100.0, 3, Sink, &log).status == UiTimelineStatus::StaleGeneration);
    Check(timeline.TimeMs() == before);
    Check(timeline.Advance(500.0, 4, Sink, &log).status == UiTimelineStatus::BackwardTime);
    Check(timeline.TimeMs() == before);
    Check(log.ids.size() == 3);
    Check(timeline.Counters().rejected_stale_generation == 1);
    Check(timeline.Counters().rejected_backward == 1);
}

void TestNoSinkNeverConsumes() {
    const std::array<nosfix::UiTimelineEventSpec, 2> events{{{100.0, 1}, {200.0, 2}}};
    nosfix::UiTimeline timeline;
    Check(timeline.Configure(Identity(0x4000, 1), nosfix::UiTimeDomain::Song,
                             kOpacityTrack, events) == nosfix::UiTimelineDiagnostic::None);
    Check(timeline.Start(1, 0.0) == UiTimelineStatus::Ok);
    EventLog log;

    Check(timeline.Advance(50.0, 1, nullptr, nullptr).status == UiTimelineStatus::NoSink);
    Check(timeline.TimeMs() == 0.0);   // time is not advanced either
    Check(timeline.Counters().rejected_no_sink == 1);

    // Supplying a sink later recovers every crossing.
    const auto result = timeline.Advance(500.0, 1, Sink, &log);
    Check(result.delivered == 2);
    Check(log.ids.size() == 2 && log.ids[0] == 1 && log.ids[1] == 2);
}

void TestEventOrderUnderStall() {
    const std::array<nosfix::UiTimelineEventSpec, 8> events{{
        {700.0, 70}, {100.0, 10}, {500.0, 50}, {300.0, 30},
        {200.0, 20}, {600.0, 60}, {400.0, 40}, {800.0, 80},
    }};
    nosfix::UiTimeline timeline;
    Check(timeline.Configure(Identity(0x5001, 1), nosfix::UiTimeDomain::WallClock,
                             kOpacityTrack, events) == nosfix::UiTimelineDiagnostic::None);
    Check(timeline.Start(1, 0.0) == UiTimelineStatus::Ok);
    EventLog log;
    Check(timeline.Advance(50.0, 1, Sink, &log).delivered == 0);
    Check(timeline.Advance(2'000.0, 1, Sink, &log).delivered == 8);
    Check(log.ids.size() == 8);
    for (std::size_t index = 0; index < log.ids.size(); ++index) {
        Check(log.ids[index] == static_cast<std::uint32_t>(10 * (index + 1)));
    }
    Check(timeline.Advance(2'000.0, 1, Sink, &log).delivered == 0);
}

void TestCadenceEventInvariance() {
    const std::array<double, 7> rates{60.0, 90.0, 120.0, 144.0, 165.0, 240.0, 360.0};
    std::array<nosfix::UiTimelineEventSpec, 12> events{};
    for (std::size_t index = 0; index < events.size(); ++index) {
        events[index] = {static_cast<double>(index + 1) * 37.0,
                         static_cast<std::uint32_t>(index + 1)};
    }
    std::vector<std::uint32_t> reference;
    for (const double rate : rates) {
        nosfix::UiTimeline timeline;
        Check(timeline.Configure(Identity(0x6000, 1), nosfix::UiTimeDomain::Song,
                                 kOpacityTrack, events) == nosfix::UiTimelineDiagnostic::None);
        Check(timeline.Start(1, 0.0) == UiTimelineStatus::Ok);
        EventLog log;
        std::uint64_t frame = 0;
        while (true) {
            const double now = (static_cast<double>(frame) * 1000.0) / rate;
            if (now > 1'000.0) break;
            (void)timeline.Advance(now, 1, Sink, &log);
            // Extra renders of the same instant must not repeat a callback.
            (void)timeline.Advance(now, 1, Sink, &log);
            ++frame;
        }
        Check(log.ids.size() == events.size());
        for (std::size_t index = 0; index < log.ids.size(); ++index) {
            Check(log.ids[index] == static_cast<std::uint32_t>(index + 1));
        }
        if (reference.empty()) reference = log.ids;
        else Check(reference == log.ids);
    }
}

void TestConfigureIsTransactional() {
    const std::array<UiKeyframe, 2> keys{{{0.0, 0.0f, UiInterpolation::Linear},
                                          {1'000.0, 1.0f, UiInterpolation::Linear}}};
    const std::array<nosfix::UiTimelineTrack, 1> good{{{UiChannel::Opacity, keys}}};
    const std::array<nosfix::UiTimelineEventSpec, 1> events{{{500.0, 42}}};
    const auto identity = Identity(0x7000, 1);

    nosfix::UiTimeline timeline;
    Check(timeline.Configure(identity, nosfix::UiTimeDomain::Song, good, events) ==
          nosfix::UiTimelineDiagnostic::None);
    Check(timeline.Start(3, 0.0) == UiTimelineStatus::Ok);
    EventLog log;
    Check(timeline.Advance(600.0, 3, Sink, &log).delivered == 1);
    Check(timeline.GenerationFloor() == 3);

    const std::array<UiKeyframe, 2> unsorted{{{500.0, 0.0f, UiInterpolation::Linear},
                                              {100.0, 1.0f, UiInterpolation::Linear}}};
    const std::array<nosfix::UiTimelineTrack, 1> bad{{{UiChannel::Opacity, unsorted}}};
    Check(timeline.Configure(identity, nosfix::UiTimeDomain::Song, bad, events) ==
          nosfix::UiTimelineDiagnostic::UnsortedKeys);
    // The previous configuration and playback survived the rejection.
    Check(timeline.Configured());
    Check(timeline.Started());
    Check(timeline.Generation() == 3);
    Check(timeline.TimeMs() == 600.0);
    Check(timeline.Advance(700.0, 3, Sink, &log).delivered == 0);

    // A successful reconfiguration restarts playback but keeps the floor, so an
    // older epoch can never replay.
    Check(timeline.Configure(identity, nosfix::UiTimeDomain::Song, good, events) ==
          nosfix::UiTimelineDiagnostic::None);
    Check(!timeline.Started());
    Check(timeline.GenerationFloor() == 3);
    Check(timeline.Start(3, 0.0) == UiTimelineStatus::GenerationNotNew);
    Check(timeline.Start(4, 600.0) == UiTimelineStatus::Ok);
    Check(timeline.Advance(600.0, 4, Sink, &log).delivered == 0); // event already behind
}

void TestWallAndSongSeparation() {
    const std::array<UiKeyframe, 2> keys{{{0.0, 0.0f, UiInterpolation::Linear},
                                          {2'000.0, 1.0f, UiInterpolation::Linear}}};
    const std::array<nosfix::UiTimelineTrack, 1> tracks{{{UiChannel::TranslateY, keys}}};
    const std::array<nosfix::UiTimelineEventSpec, 2> events{{{250.0, 1}, {1'400.0, 2}}};
    nosfix::UiTimelineBank bank;
    auto* wall = bank.Acquire(Identity(0x8000, 1));
    Check(wall != nullptr);
    Check(wall->Configure(Identity(0x8000, 1), nosfix::UiTimeDomain::WallClock,
                          tracks, events) == nosfix::UiTimelineDiagnostic::None);
    auto* song = bank.Acquire(Identity(0x8001, 1));
    Check(song != nullptr);
    Check(song->Configure(Identity(0x8001, 1), nosfix::UiTimeDomain::Song,
                          tracks, events) == nosfix::UiTimelineDiagnostic::None);

    EventLog log;
    bank.SetWallClock(250.0);
    bank.SetSongTime(250.0, 4);
    Check(bank.AdvanceDomain(nosfix::UiTimeDomain::WallClock, Sink, &log) == 1);
    Check(log.ids.size() == 1 && log.ids[0] == 1);   // wall timeline starts at 250
    Check(wall->TimeMs() == 250.0);
    Check(!song->Started());

    Check(bank.AdvanceDomain(nosfix::UiTimeDomain::Song, Sink, &log) == 1);
    Check(log.ids.size() == 2 && log.ids[1] == 1);   // inclusive start at 250
    Check(song->TimeMs() == 250.0);
    Check(song->Generation() == 4);

    // Clearing the song clock freezes song timelines; the wall clock keeps
    // running on its own values.
    bank.ClearSongTime();
    bank.SetWallClock(1'500.0);
    Check(bank.AdvanceDomain(nosfix::UiTimeDomain::Song, Sink, &log) == 0);
    Check(song->TimeMs() == 250.0);
    Check(bank.AdvanceDomain(nosfix::UiTimeDomain::WallClock, Sink, &log) == 1);
    Check(log.ids.size() == 3 && log.ids[2] == 2);
    Check(wall->TimeMs() == 1'500.0);
    Check(!bank.SongTimeValid());

    // A stale song generation is ignored; a newer one re-arms the timeline at
    // the seek point, so events behind that point do not fire again.
    bank.SetSongTime(300.0, 2);
    Check(bank.SongGeneration() == 4);
    bank.SetSongTime(400.0, 5);
    Check(bank.AdvanceDomain(nosfix::UiTimeDomain::Song, Sink, &log) == 0);
    Check(song->Generation() == 5);
    bank.SetSongTime(1'500.0, 5);
    Check(bank.AdvanceDomain(nosfix::UiTimeDomain::Song, Sink, &log) == 1);
    Check(log.ids.size() == 4 && log.ids[3] == 2);
}

void TestNodeIdentityLifetime() {
    nosfix::UiTimelineBank bank;
    const auto first = Identity(0x9000, 11);
    auto* timeline = bank.Acquire(first);
    Check(timeline != nullptr);
    Check(timeline->Configure(first, nosfix::UiTimeDomain::WallClock,
                              kOpacityTrack, {}) == nosfix::UiTimelineDiagnostic::None);
    Check(bank.Acquire(first) == timeline);
    Check(bank.Find(first) == timeline);
    Check(bank.Size() == 1);

    // Same address with a new allocator stamp: the node was recycled, so the
    // entry starts from scratch instead of inheriting the old timeline.
    const auto recycled = Identity(0x9000, 12);
    auto* fresh = bank.Acquire(recycled);
    Check(fresh == timeline);
    Check(!fresh->Configured());
    Check(bank.Find(first) == nullptr);
    Check(fresh->Configure(recycled, nosfix::UiTimeDomain::Song,
                           kOpacityTrack, {}) == nosfix::UiTimelineDiagnostic::None);
    Check(bank.Find(recycled) == fresh);

    bank.Release(recycled);
    Check(bank.Find(recycled) == nullptr);
    Check(bank.Size() == 0);

    std::array<nosfix::UiTimeline*, nosfix::UiTimelineBank::kCapacity> slots{};
    for (std::size_t index = 0; index < slots.size(); ++index) {
        slots[index] = bank.Acquire(Identity(0xA000 + index, 1));
        Check(slots[index] != nullptr);
        Check(slots[index]->Configure(Identity(0xA000 + index, 1),
                                      nosfix::UiTimeDomain::WallClock,
                                      kOpacityTrack, {}) == nosfix::UiTimelineDiagnostic::None);
    }
    Check(bank.Size() == nosfix::UiTimelineBank::kCapacity);
    Check(bank.Acquire(Identity(0xB000, 1)) == nullptr);

    bank.ReleaseNode(0xA003);
    Check(bank.Size() == nosfix::UiTimelineBank::kCapacity - 1);
    bank.Clear();
    Check(bank.Size() == 0);
}

void TestConfigureDiagnostics() {
    nosfix::UiTimeline timeline;
    const std::array<UiKeyframe, 2> unsorted{{{500.0, 0.0f, UiInterpolation::Linear},
                                              {100.0, 1.0f, UiInterpolation::Linear}}};
    const std::array<UiKeyframe, 2> sorted{{{100.0, 0.0f, UiInterpolation::Linear},
                                            {500.0, 1.0f, UiInterpolation::Linear}}};
    const std::array<nosfix::UiTimelineTrack, 1> bad{{{UiChannel::Opacity, unsorted}}};
    const std::array<nosfix::UiTimelineTrack, 1> good{{{UiChannel::Opacity, sorted}}};
    const std::array<nosfix::UiTimelineTrack, 0> none{};
    const std::array<nosfix::UiTimelineTrack, 2> same_channel{{
        {UiChannel::Opacity, sorted},
        {UiChannel::Opacity, sorted},
    }};

    Check(timeline.Configure({}, nosfix::UiTimeDomain::Song, good, {}) ==
          nosfix::UiTimelineDiagnostic::InvalidKey);
    Check(timeline.Configure(Identity(1, 1), nosfix::UiTimeDomain::Song, none, {}) ==
          nosfix::UiTimelineDiagnostic::NoTracks);
    Check(timeline.Configure(Identity(1, 1), nosfix::UiTimeDomain::Song, bad, {}) ==
          nosfix::UiTimelineDiagnostic::UnsortedKeys);
    Check(timeline.Configure(Identity(1, 1), nosfix::UiTimeDomain::Song, same_channel, {}) ==
          nosfix::UiTimelineDiagnostic::UnsupportedChannel);

    const std::array<nosfix::UiKeyframe, 0> empty_keys{};
    const std::array<nosfix::UiTimelineTrack, 1> empty_track{{ {UiChannel::Opacity, empty_keys} }};
    Check(timeline.Configure(Identity(1, 1), nosfix::UiTimeDomain::Song, empty_track, {}) ==
          nosfix::UiTimelineDiagnostic::EmptyTrack);

    const std::array<UiKeyframe, 1> nan_key{{
        {0.0, std::numeric_limits<float>::quiet_NaN(), UiInterpolation::Step}}};
    const std::array<nosfix::UiTimelineTrack, 1> nan_track{{ {UiChannel::Opacity, nan_key} }};
    Check(timeline.Configure(Identity(1, 1), nosfix::UiTimeDomain::Song, nan_track, {}) ==
          nosfix::UiTimelineDiagnostic::InvalidKey);

    const std::array<nosfix::UiTimelineEventSpec, 2> duplicate_events{{{100.0, 5}, {200.0, 5}}};
    Check(timeline.Configure(Identity(1, 1), nosfix::UiTimeDomain::Song, good,
                             duplicate_events) == nosfix::UiTimelineDiagnostic::DuplicateEventId);
    const std::array<nosfix::UiTimelineEventSpec, 1> zero_event{{{100.0, 0}}};
    Check(timeline.Configure(Identity(1, 1), nosfix::UiTimeDomain::Song, good, zero_event) ==
          nosfix::UiTimelineDiagnostic::InvalidEvent);

    std::array<nosfix::UiTimelineEventSpec, nosfix::UiTimeline::kMaxEvents + 1> many{};
    for (std::size_t index = 0; index < many.size(); ++index) {
        many[index] = {static_cast<double>(index), static_cast<std::uint32_t>(index + 1)};
    }
    Check(timeline.Configure(Identity(1, 1), nosfix::UiTimeDomain::Song, good, many) ==
          nosfix::UiTimelineDiagnostic::TooManyEvents);

    Check(timeline.Configure(Identity(1, 1), nosfix::UiTimeDomain::Song, good, {}) ==
          nosfix::UiTimelineDiagnostic::None);
    Check(timeline.Configured());
    Check(timeline.Matches(Identity(1, 1)));
    Check(!timeline.Matches(Identity(1, 2)));
    timeline.Reset();
    Check(!timeline.Configured());
    Check(!timeline.Matches(Identity(1, 1)));
}

void TestEvaluateIsStateless() {
    const std::array<UiKeyframe, 2> keys{{{0.0, 0.0f, UiInterpolation::Linear},
                                          {1'000.0, 10.0f, UiInterpolation::Linear}}};
    const std::array<nosfix::UiTimelineTrack, 1> tracks{{{UiChannel::TranslateZ, keys}}};
    const std::array<nosfix::UiTimelineEventSpec, 1> events{{{500.0, 9}}};
    nosfix::UiTimeline timeline;
    Check(timeline.Configure(Identity(0xC000, 1), nosfix::UiTimeDomain::WallClock,
                             tracks, events) == nosfix::UiTimelineDiagnostic::None);
    nosfix::UiAnimationState before;
    nosfix::UiAnimationState after;
    Check(timeline.Evaluate(250.0, before));
    Check(!timeline.Started());
    Check(timeline.Start(1, 0.0) == UiTimelineStatus::Ok);
    EventLog log;
    Check(timeline.Advance(250.0, 1, Sink, &log).delivered == 0);
    Check(timeline.Evaluate(250.0, after));
    Check(before.values == after.values);
    Check(before.valid == after.valid);
    Check(timeline.Advance(600.0, 1, Sink, &log).delivered == 1);
    Check(timeline.Evaluate(250.0, after));
    Check(before.values == after.values);
}

void TestDiagnosticNames() {
    using nosfix::UiTimelineDiagnosticName;
    using nosfix::UiTimelineStatusName;
    Check(std::strcmp(UiTimelineDiagnosticName(nosfix::UiTimelineDiagnostic::UnsortedKeys),
                      "unsorted-keys") == 0);
    Check(std::strcmp(UiTimelineStatusName(UiTimelineStatus::StaleGeneration),
                      "stale-generation") == 0);
    Check(std::strcmp(UiTimelineStatusName(UiTimelineStatus::Ok), "ok") == 0);
}

} // namespace

int main() {
    TestInterpolation();
    TestInclusiveStartAndFirstObservation();
    TestRejectionsAreNoOps();
    TestNoSinkNeverConsumes();
    TestEventOrderUnderStall();
    TestCadenceEventInvariance();
    TestConfigureIsTransactional();
    TestWallAndSongSeparation();
    TestNodeIdentityLifetime();
    TestConfigureDiagnostics();
    TestEvaluateIsStateless();
    TestDiagnosticNames();
    std::printf("timed_ui_timeline: all tests passed\n");
    return 0;
}