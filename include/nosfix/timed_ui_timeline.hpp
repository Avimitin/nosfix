#pragma once

// Node-bound animation timeline for AFP UI transforms and colours.
//
// This is an evaluator plus a callback contract, NOT an AFP adapter. It holds
// no game pointers, knows nothing about AFP node fields, and must not be wired
// to a live node: the afp-core evidence for colour/opacity offsets, the matrix
// convention, the evaluator time source, the user callback ABI and the node-pool
// free entry is still missing (requests/render-ui-002.md, answers/render-ui-002.md
// items 1, 2, 4, 5 and 6 are blocked). Enabling it early can only produce
// invented animation. timed_ui_adapter.hpp may evaluate these timelines per
// frame, but it hands the result to a caller-supplied sink and writes nothing.
//
// Two clocks are kept strictly apart: menu animation runs on wall-clock time,
// music-synchronised animation runs only on an explicit song time with a
// playback generation. A timeline belongs to exactly one of them.
//
// Delivery model follows ChartEventCursor: Configure() copies and validates the
// table (the only call that may allocate), Start() opens a numbered epoch, and
// Advance() emits the crossings it passes, in scheduled order, at most once per
// epoch. Every rejection is a complete no-op: state, floor and pending events
// are unchanged. A null sink never consumes an event.

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace nosfix {

enum class UiTimeDomain : std::uint8_t {
    WallClock = 0,
    Song = 1,
};

enum class UiChannel : std::uint8_t {
    TranslateX = 0,
    TranslateY,
    TranslateZ,
    ScaleX,
    ScaleY,
    RotationDegrees,
    Opacity,
    ColorR,
    ColorG,
    ColorB,
    ColorA,
    Count,
};

enum class UiInterpolation : std::uint8_t {
    Step = 0,
    Linear = 1,
    SmoothStep = 2,
};

struct UiKeyframe final {
    double time_ms = 0.0;
    float value = 0.0f;
    UiInterpolation interpolation = UiInterpolation::Linear;
};

struct UiTimelineTrack final {
    UiChannel channel = UiChannel::Opacity;
    std::span<const UiKeyframe> keys;
};

struct UiTimelineEventSpec final {
    double time_ms = 0.0;
    std::uint32_t id = 0;
};

// Pool identity of an AFP display node. A recycled node keeps the address but
// changes the allocator stamp, so a track must never survive that change.
struct UiNodeIdentity final {
    std::uintptr_t node = 0;
    std::uint32_t allocation_stamp = 0;
    std::uintptr_t parent = 0;
    std::uint16_t type = 0;
    std::uintptr_t resource = 0;

    friend constexpr bool operator==(const UiNodeIdentity&, const UiNodeIdentity&) = default;
};

// Configure() outcome. Any value other than `None` leaves the previous
// configuration and playback state untouched.
enum class UiTimelineDiagnostic : std::uint8_t {
    None = 0,
    NotConfigured,
    NoTracks,
    EmptyTrack,
    UnsortedKeys,
    InvalidKey,
    UnsupportedChannel,
    TooManyKeys,
    TooManyEvents,
    DuplicateEventId,
    InvalidEvent,
};

[[nodiscard]] const char* UiTimelineDiagnosticName(UiTimelineDiagnostic value) noexcept;

// Status of Start()/Advance(). Rejections are complete no-ops.
enum class UiTimelineStatus : std::uint8_t {
    Ok = 0,
    NotConfigured,
    NotStarted,
    NonFiniteTime,
    StaleGeneration,
    GenerationZero,
    GenerationNotNew,
    BackwardTime,
    NoSink,
};

[[nodiscard]] const char* UiTimelineStatusName(UiTimelineStatus value) noexcept;

// Evaluated animation state for one node. `valid` marks channels the timeline
// provides; everything else must keep the node's own value.
struct UiAnimationState final {
    static constexpr std::size_t kChannelCount = static_cast<std::size_t>(UiChannel::Count);

    std::array<float, kChannelCount> values{};
    std::array<bool, kChannelCount> valid{};
};

struct UiAdvanceOutcome final {
    std::size_t delivered = 0;
    UiTimelineStatus status = UiTimelineStatus::Ok;

    [[nodiscard]] bool ok() const noexcept { return status == UiTimelineStatus::Ok; }
};

struct UiTimelineCounters final {
    std::uint64_t advances = 0;
    std::uint64_t events_delivered = 0;
    std::uint64_t starts = 0;
    std::uint64_t rejected_backward = 0;
    std::uint64_t rejected_stale_generation = 0;
    std::uint64_t rejected_no_sink = 0;
    std::uint64_t evaluate_failures = 0;
};

using UiEventSink = void (*)(void* context, const UiNodeIdentity& node,
                             std::uint32_t event_id) noexcept;

// Fractional channel state for one node, handed to a caller-supplied sink. The
// evaluator never writes a game field itself; see timed_ui_adapter.hpp.
using UiStateSink = void (*)(void* context, const UiNodeIdentity& node,
                             const UiAnimationState& state) noexcept;

// One node-bound timeline. Evaluation is fractional and stateless; callbacks
// for a crossing are delivered exactly once per epoch, even if frames jitter,
// stall, or repeat the same timestamp.
class UiTimeline final {
public:
    static constexpr std::size_t kMaxKeys = 64;
    static constexpr std::size_t kMaxEvents = 64;
    static constexpr double kMaximumTimeMs = 1.0e12;

    // Copies, validates and stable-sorts the tracks and events. On rejection
    // the previous configuration and playback state are untouched. Success
    // invalidates playback (the timeline becomes unstarted) and keeps the
    // generation floor and the counters.
    [[nodiscard]] UiTimelineDiagnostic Configure(
        UiNodeIdentity node, UiTimeDomain domain,
        std::span<const UiTimelineTrack> tracks,
        std::span<const UiTimelineEventSpec> events) noexcept;

    // Opens an epoch, or seeks within a new one. Requires a configuration, a
    // finite start and a nonzero generation above the lifetime floor. An event
    // exactly at `start_ms` is included; earlier events are skipped by this
    // explicit policy only.
    [[nodiscard]] UiTimelineStatus Start(std::uint64_t generation,
                                         double start_ms) noexcept;

    // Advances to `now_ms` inside the started epoch. Emits the events crossed
    // in (previous, now] -- or [start, now] for the first call after Start --
    // in scheduled order. A null sink, a stale generation, a backwards time or
    // an unstarted timeline changes nothing at all.
    [[nodiscard]] UiAdvanceOutcome Advance(double now_ms, std::uint64_t generation,
                                           UiEventSink sink, void* context) noexcept;

    // Fractional evaluation for a caller-chosen time. Never advances the
    // timeline, never fires callbacks.
    [[nodiscard]] bool Evaluate(double now_ms, UiAnimationState& state) const noexcept;

    // True only for the exact identity this timeline was configured with.
    [[nodiscard]] bool Matches(const UiNodeIdentity& identity) const noexcept {
        return configured_ && identity == node_;
    }
    [[nodiscard]] bool Configured() const noexcept { return configured_; }
    [[nodiscard]] bool Started() const noexcept { return started_; }
    [[nodiscard]] UiTimeDomain Domain() const noexcept { return domain_; }
    [[nodiscard]] const UiNodeIdentity& Node() const noexcept { return node_; }
    [[nodiscard]] std::uint64_t Generation() const noexcept { return generation_; }
    // Highest generation ever accepted; survives Reset() and Configure().
    [[nodiscard]] std::uint64_t GenerationFloor() const noexcept { return floor_generation_; }
    [[nodiscard]] double TimeMs() const noexcept { return time_ms_; }
    [[nodiscard]] const UiTimelineCounters& Counters() const noexcept { return counters_; }

    // Drops the configuration and playback, keeping the generation floor and
    // the counters so a later epoch can never replay an older one.
    void Reset() noexcept;

private:
    struct Event final {
        double time_ms = 0.0;
        std::uint32_t id = 0;
        bool delivered = false;
    };

    [[nodiscard]] static bool EvaluateTrack(const UiKeyframe* keys, std::size_t count,
                                            double now_ms, float& value) noexcept;
    void ClearDelivery() noexcept;

    UiNodeIdentity node_{};
    UiTimeDomain domain_ = UiTimeDomain::WallClock;
    std::array<UiKeyframe, kMaxKeys> keys_{};
    std::array<std::uint32_t, static_cast<std::size_t>(UiChannel::Count)> track_offset_{};
    std::array<std::uint32_t, static_cast<std::size_t>(UiChannel::Count)> track_count_{};
    std::array<Event, kMaxEvents> events_{};
    std::size_t event_count_ = 0;
    double time_ms_ = 0.0;
    std::uint64_t generation_ = 0;
    std::uint64_t floor_generation_ = 0;
    bool configured_ = false;
    bool started_ = false;
    bool include_start_ = false;
    UiTimelineCounters counters_{};
};

// Fixed-capacity registry of node timelines, one clock per domain. Song-synced
// timelines are frozen, never advanced by wall-clock time, while the song clock
// has no valid value (paused, seeking, not playing).
class UiTimelineBank final {
public:
    static constexpr std::size_t kCapacity = 16;
    // Wall-clock timelines use this fixed, nonzero epoch.
    static constexpr std::uint64_t kWallGeneration = 1;

    // Returns the timeline bound to this identity, recycling a stale entry when
    // the pool stamp changed. Null when the bank is full of live entries.
    UiTimeline* Acquire(UiNodeIdentity identity) noexcept;
    [[nodiscard]] UiTimeline* Find(const UiNodeIdentity& identity) noexcept;

    // Node pool release path: drop the entry immediately so a recycled address
    // can never inherit geometry or event history.
    void Release(const UiNodeIdentity& identity) noexcept;
    void ReleaseNode(std::uintptr_t node) noexcept;
    void Clear() noexcept;

    // Wall time never moves backwards. Song time requires a nonzero generation;
    // a generation below the accepted one is ignored so a stale producer cannot
    // rewind the epoch.
    void SetWallClock(double ms) noexcept;
    void SetSongTime(double ms, std::uint64_t generation) noexcept;
    void ClearSongTime() noexcept;

    // Delivers every pending crossing for the requested domain. Wall-clock and
    // song timelines are advanced separately by design.
    std::size_t AdvanceDomain(UiTimeDomain domain, UiEventSink sink, void* context) noexcept;

    // Evaluates every live timeline of `domain` at the domain time and hands its
    // fractional state to `sink`. The start (or seek) of a not-yet-started
    // entry is identical to AdvanceDomain's, so evaluation and event delivery
    // always agree on the epoch. Returns the number of states delivered.
    std::size_t EvaluateDomain(UiTimeDomain domain, UiStateSink sink,
                               void* context) noexcept;

    // Published time for one domain; false while that clock has no valid value
    // (paused, seeking, not started).
    [[nodiscard]] bool DomainTime(UiTimeDomain domain, double& ms) const noexcept;
    [[nodiscard]] std::uint64_t DomainGeneration(UiTimeDomain domain) const noexcept;

    [[nodiscard]] std::size_t Size() const noexcept;
    [[nodiscard]] std::size_t Capacity() const noexcept { return kCapacity; }
    [[nodiscard]] bool SongTimeValid() const noexcept { return song_valid_; }
    [[nodiscard]] std::uint64_t SongGeneration() const noexcept { return song_generation_; }

private:
    struct Entry final {
        UiTimeline timeline;
        // Identity recorded at Acquire time. Kept here rather than in the
        // timeline so an entry is addressable before Configure: a caller may
        // bind a node, read its tracks, and configure it later.
        UiNodeIdentity identity{};
        bool used = false;
    };

    std::array<Entry, kCapacity> entries_{};
    double wall_ms_ = 0.0;
    double song_ms_ = 0.0;
    std::uint64_t song_generation_ = 0;
    bool wall_valid_ = false;
    bool song_valid_ = false;
};

} // namespace nosfix
