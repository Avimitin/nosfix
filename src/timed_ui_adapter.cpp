#include "nosfix/timed_ui_adapter.hpp"

#include <cmath>
#include <cstdint>

#include <Windows.h>

namespace nosfix {
namespace {

bool FiniteTime(double value) noexcept {
    return std::isfinite(value) && std::abs(value) <= UiTimeline::kMaximumTimeMs;
}

} // namespace

const char* UiAdapterStatusName(UiAdapterStatus value) noexcept {
    switch (value) {
    case UiAdapterStatus::Ok: return "ok";
    case UiAdapterStatus::NoSink: return "no-sink";
    }
    return "unknown";
}

UiTimeline* TimedUiAdapter::Bind(UiNodeIdentity identity) noexcept {
    return bank_.Acquire(identity);
}

UiTimeline* TimedUiAdapter::Find(const UiNodeIdentity& identity) noexcept {
    return bank_.Find(identity);
}

void TimedUiAdapter::Release(const UiNodeIdentity& identity) noexcept {
    if (identity.node == 0) return;
    bank_.Release(identity);
    ++counters_.releases;
}

void TimedUiAdapter::ReleaseNode(std::uintptr_t node) noexcept {
    if (node == 0) return;
    bank_.ReleaseNode(node);
    ++counters_.releases;
}

void TimedUiAdapter::Clear() noexcept {
    bank_.Clear();
    wall_ms_ = 0.0;
    wall_origin_ms_ = 0.0;
    wall_origin_ticks_ = 0;
    wall_frequency_ = 0;
    wall_valid_ = false;
}

void TimedUiAdapter::SetSongTime(double ms, std::uint64_t generation) noexcept {
    // The bank applies its own generation floor and finite check; a rejected
    // sample leaves the previously published song time untouched.
    bank_.SetSongTime(ms, generation);
}

void TimedUiAdapter::ClearSongTime() noexcept {
    bank_.ClearSongTime();
}

bool TimedUiAdapter::SetWallClockTicks(std::int64_t ticks,
                                       std::uint64_t frequency) noexcept {
    if (frequency == 0 || ticks < 0) {
        ++counters_.rejected_clock;
        return false;
    }
    // A frequency change (or the first sample) rebases the origin at the value
    // already published, so the wall clock never jumps.
    if (!wall_valid_ || frequency != wall_frequency_ || wall_origin_ticks_ == 0) {
        wall_frequency_ = frequency;
        wall_origin_ticks_ = ticks == 0 ? 1 : ticks;   // 0 marks "unset"
        wall_origin_ms_ = wall_valid_ ? wall_ms_ : 0.0;
    }
    if (ticks < wall_origin_ticks_) {
        ++counters_.rejected_clock;
        return false;
    }
    const double delta_ticks =
        static_cast<double>(ticks - wall_origin_ticks_);
    const double ms = wall_origin_ms_ +
        delta_ticks * 1000.0 / static_cast<double>(frequency);
    if (!FiniteTime(ms) || (wall_valid_ && ms < wall_ms_)) {
        ++counters_.rejected_clock;
        return false;
    }
    wall_ms_ = ms;
    wall_valid_ = true;
    bank_.SetWallClock(ms);
    return true;
}

bool TimedUiAdapter::SetWallClock(double ms) noexcept {
    if (!FiniteTime(ms) || ms < 0.0 || (wall_valid_ && ms < wall_ms_)) {
        ++counters_.rejected_clock;
        return false;
    }
    wall_ms_ = ms;
    // Force the next tick sample to rebase on this value.
    wall_origin_ticks_ = 0;
    wall_valid_ = true;
    bank_.SetWallClock(ms);
    return true;
}

bool TimedUiAdapter::PollWallClock() noexcept {
    LARGE_INTEGER counter{};
    LARGE_INTEGER frequency{};
    if (!QueryPerformanceCounter(&counter) || !QueryPerformanceFrequency(&frequency)) {
        ++counters_.rejected_clock;
        return false;
    }
    if (frequency.QuadPart <= 0) {
        ++counters_.rejected_clock;
        return false;
    }
    return SetWallClockTicks(counter.QuadPart,
                             static_cast<std::uint64_t>(frequency.QuadPart));
}

UiAdapterStatus TimedUiAdapter::Advance(const UiNodeSink& sink) noexcept {
    ++counters_.frames;
    if (sink.apply == nullptr) {
        // Nothing may be evaluated without a consumer: a solution that
        // evaluates and then drops the state would still cost a frame, and a
        // callback with no sink must never consume an event.
        ++counters_.rejected_no_sink;
        return UiAdapterStatus::NoSink;
    }
    // Evaluate first, then deliver crossings, so a callback always observes the
    // state of the frame it belongs to. Domains are advanced separately: a
    // song-synced node never moves on wall-clock time and vice versa. Each step
    // is sequenced explicitly; `a + b` has unspecified operand order.
    std::size_t evaluated =
        bank_.EvaluateDomain(UiTimeDomain::WallClock, sink.apply, sink.context);
    evaluated += bank_.EvaluateDomain(UiTimeDomain::Song, sink.apply, sink.context);
    counters_.applies += evaluated;

    if (sink.event != nullptr) {
        std::size_t events =
            bank_.AdvanceDomain(UiTimeDomain::WallClock, sink.event, sink.context);
        events += bank_.AdvanceDomain(UiTimeDomain::Song, sink.event, sink.context);
        counters_.events += events;
    }
    return UiAdapterStatus::Ok;
}

} // namespace nosfix
