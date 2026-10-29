#pragma once

#include "nosfix/song_clock.hpp"
#include "nosfix/song_clock_publication.hpp"

#include <atomic>
#include <cstdint>

namespace nosfix {

enum class ShadowClockStatus : std::uint8_t {
    Disabled = 0,
    Started,
    Observed,
    Seeked,   // discontinuity or out-of-budget correction; new generation
    Rejected, // invalid observation; nothing changed
};

// Single-writer diagnostic clock owner for the shadow integration.
//
// The render thread is the only writer. Each explicit audio observation either
// updates the existing SongClock within a bounded correction budget or starts a
// new generation with Seek. The immutable snapshot is published for the input
// producer, which takes it before reading a packet QPC. This is a diagnostic
// path, not a judgement clock: no game path reads it back, and the render loop
// never advances song time by a frame count.
class ShadowClock final {
public:
    ShadowClock() noexcept = default;
    ~ShadowClock() = default;
    ShadowClock(const ShadowClock&) = delete;
    ShadowClock& operator=(const ShadowClock&) = delete;

    // Enabling arms the next observation. Disabling stops the clock and
    // advances its generation so queued input from the old segment is stale.
    void Configure(bool enabled) noexcept;

    [[nodiscard]] bool Enabled() const noexcept {
        return enabled_.load(std::memory_order_acquire);
    }
    [[nodiscard]] ShadowClockStatus Observe(double song_microseconds,
                                            std::int64_t qpc_ticks,
                                            std::int64_t qpc_frequency) noexcept;
    void Stop() noexcept;

    // Always non-null; callers must check Enabled() before passing it to the
    // input capture. The publication is written only by Observe().
    [[nodiscard]] const SongClockPublication* Publication() const noexcept {
        return &publication_;
    }
    [[nodiscard]] SongClockSegment Snapshot() const noexcept { return clock_.Snapshot(); }
    [[nodiscard]] std::uint64_t Generation() const noexcept { return clock_.Generation(); }
    [[nodiscard]] bool HasPublished() const noexcept { return has_published_; }

    // Maximum accepted correction before a new generation is required. This is
    // a diagnostic bound, not a claim about audible drift.
    static constexpr double kMaxCorrectionUs = 50'000.0;

private:
    SongClock clock_{};
    SongClockPublication publication_{};
    std::atomic<bool> enabled_{false};
    bool has_published_ = false;
};

} // namespace nosfix
