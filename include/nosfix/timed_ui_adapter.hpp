#pragma once

// Fail-closed adapter between chart-time UI animation and any display node.
//
// This file writes nothing into a game module. It sweeps the timelines in a
// UiTimelineBank, evaluates their channels fractionally, and hands the result
// to a caller-supplied sink. That boundary is deliberate: afp-core's colour and
// opacity property offsets, the keyframe/timeline record layout, the user
// callback ABI and the node-pool free entry point are still unproven (see
// requests/render-ui-002.md), so no offset is claimed here and no node is
// mutated. A sink that does write AFP fields must cite its own evidence.
//
// Two clocks are kept strictly apart, as in UiTimeline: a Song timeline is only
// advanced by an explicit song time with a playback generation, and a
// WallClock timeline by a monotonic QPC-derived millisecond count. Neither can
// advance from the other domain's source.
//
// Advance() is allocation free, takes no lock and performs no file or OS I/O in
// its steady path (PollWallClock is the only OS call and is optional), so it is
// safe to call from a per-frame render callback.

#include "nosfix/timed_ui_timeline.hpp"

#include <cstdint>

namespace nosfix {

// Everything the adapter is allowed to do to a node, as function pointers. A
// default-initialised sink performs no write at all.
struct UiNodeSink final {
    void* context = nullptr;
    // Fractional channel values for one node. Called once per live timeline per
    // frame, before that frame's callbacks. `valid` marks the channels the
    // timeline provides; every other channel keeps the node's own value.
    void (*apply)(void* context, const UiNodeIdentity& node,
                  const UiAnimationState& state) noexcept = nullptr;
    // A scheduled crossing, delivered exactly once per epoch and in scheduled
    // order. May be null, in which case crossings stay pending.
    UiEventSink event = nullptr;
};

enum class UiAdapterStatus : std::uint8_t {
    Ok = 0,
    // The sink cannot receive state; nothing was evaluated or delivered.
    NoSink,
};

[[nodiscard]] const char* UiAdapterStatusName(UiAdapterStatus value) noexcept;

struct UiAdapterCounters final {
    std::uint64_t frames = 0;
    std::uint64_t applies = 0;
    std::uint64_t events = 0;
    std::uint64_t releases = 0;
    std::uint64_t rejected_no_sink = 0;
    std::uint64_t rejected_clock = 0;
};

// One node-bound timeline adapter. Bound timelines are addressed by the full
// UiNodeIdentity, so a recycled pool slot (same address, different allocator
// stamp) never inherits a track.
class TimedUiAdapter final {
public:
    // Wall-clock timelines use this fixed, nonzero epoch (see UiTimelineBank).
    static constexpr std::uint64_t kWallGeneration = UiTimelineBank::kWallGeneration;

    // Returns the timeline bound to this identity, recycling a stale entry when
    // the pool stamp changed or the bank is otherwise full of dead entries.
    UiTimeline* Bind(UiNodeIdentity identity) noexcept;
    [[nodiscard]] UiTimeline* Find(const UiNodeIdentity& identity) noexcept;

    // Node pool release: drop the entry so a recycled address cannot inherit
    // geometry or event history. Counted.
    void Release(const UiNodeIdentity& identity) noexcept;
    void ReleaseNode(std::uintptr_t node) noexcept;
    void Clear() noexcept;

    void SetSongTime(double ms, std::uint64_t generation) noexcept;
    void ClearSongTime() noexcept;
    // Absolute QPC counter and its frequency. Rejected (no state change) when
    // the frequency is zero, the tick count is negative, or the derived
    // millisecond count would move backwards.
    [[nodiscard]] bool SetWallClockTicks(std::int64_t ticks,
                                         std::uint64_t frequency) noexcept;
    // Test and manual seam: the same monotonic filter without QPC. Returns
    // false (no state change) for a non-finite, negative or backwards value.
    [[nodiscard]] bool SetWallClock(double ms) noexcept;
    // Optional real clock. Returns false when QueryPerformanceCounter fails.
    [[nodiscard]] bool PollWallClock() noexcept;

    // Evaluates every live timeline of both domains, then delivers the
    // crossings of each domain. A null `apply` sink is a complete no-op.
    [[nodiscard]] UiAdapterStatus Advance(const UiNodeSink& sink) noexcept;

    [[nodiscard]] std::size_t Size() const noexcept { return bank_.Size(); }
    [[nodiscard]] std::size_t Capacity() const noexcept { return bank_.Capacity(); }
    [[nodiscard]] const UiAdapterCounters& Counters() const noexcept { return counters_; }
    [[nodiscard]] double WallMilliseconds() const noexcept { return wall_ms_; }
    [[nodiscard]] bool WallClockValid() const noexcept { return wall_valid_; }
    [[nodiscard]] const UiTimelineBank& Bank() const noexcept { return bank_; }

private:
    UiTimelineBank bank_;
    UiAdapterCounters counters_{};
    double wall_ms_ = 0.0;
    double wall_origin_ms_ = 0.0;
    std::int64_t wall_origin_ticks_ = 0;
    std::uint64_t wall_frequency_ = 0;
    bool wall_valid_ = false;
};

} // namespace nosfix
