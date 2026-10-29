#pragma once

#include <cstdint>
#include <optional>

namespace nosfix {

struct SongTime final {
    double microseconds = 0.0;
    std::uint64_t generation = 0;
};

// Immutable-by-convention clock snapshot. Copy this when an input event is
// captured so later audio observations cannot retime that event. `rate` is the
// playback slope in song microseconds per wall-clock microsecond (1.0 = normal).
// A segment with a rate is continuous at its anchor; Map() never extrapolates
// before the anchor.
class SongClockSegment final {
public:
    [[nodiscard]] std::optional<SongTime> Map(std::int64_t qpc_ticks) const noexcept;
    [[nodiscard]] bool IsValid() const noexcept { return valid_; }
    [[nodiscard]] bool IsRunning() const noexcept { return valid_ && running_; }
    [[nodiscard]] std::uint64_t Generation() const noexcept { return generation_; }
    [[nodiscard]] double Rate() const noexcept { return rate_; }
    [[nodiscard]] std::int64_t Frequency() const noexcept { return qpc_frequency_; }

private:
    friend class SongClock;

    std::int64_t qpc_anchor_ = 0;
    std::int64_t qpc_frequency_ = 0;
    double song_anchor_us_ = 0.0;
    double rate_ = 1.0;
    std::uint64_t generation_ = 0;
    bool valid_ = false;
    bool running_ = false;
};

// Maps QPC capture times to the song position supplied by explicit audio
// observations. Render/update calls do not advance or alter this clock.
// One owner mutates this value. Cross-thread publication of snapshots requires
// synchronization supplied by the integration; this class is not atomic.
class SongClock final {
public:
    // Starts a new playback generation, including restart or seek-to-start.
    [[nodiscard]] bool Start(std::int64_t qpc_ticks, std::int64_t qpc_frequency,
                             double song_microseconds = 0.0) noexcept;

    // Reanchors only for known-running playback, and only when the caller's
    // explicit, finite, nonnegative correction budget covers the difference
    // from the current prediction. Song position must be monotonic and QPC
    // anchors cannot move backwards. Larger corrections require Seek(), which
    // starts a new generation. The caller must Pause() when audio stops; this
    // method does not infer pause or run a drift controller.
    [[nodiscard]] bool Observe(std::int64_t qpc_ticks,
                               std::int64_t qpc_frequency,
                               double song_microseconds,
                               double max_correction_microseconds) noexcept;

    // Continuous rate change: the segment is re-anchored at the current mapped
    // value at qpc_ticks and the new slope applies only after that QPC. Song
    // position at qpc_ticks is unchanged, and Map() remains monotonic for later
    // QPC values. rate must be finite, positive and <= 4. A discontinuity
    // (audio lost/seek/device reset) is a Seek/new generation, not a rate change.
    // Generation is unchanged. Returns false without changing anything on
    // invalid rate, invalid QPC, or a non-running segment.
    [[nodiscard]] bool ChangeRate(std::int64_t qpc_ticks, double rate) noexcept;

    // Seek is a discontinuity: it starts a new generation at the given song
    // position. Consumers can reject events carrying older generation IDs.
    [[nodiscard]] bool Seek(std::int64_t qpc_ticks, double song_microseconds) noexcept;

    // Pause invalidates mapping while stopped. Resume holds the paused song
    // position fixed across the wall-clock pause and keeps the generation.
    // Invalid calls leave the clock unchanged.
    [[nodiscard]] bool Pause(std::int64_t qpc_ticks) noexcept;
    [[nodiscard]] bool Resume(std::int64_t qpc_ticks) noexcept;

    // Stop invalidates the current mapping and advances generation.
    [[nodiscard]] bool Stop() noexcept;

    [[nodiscard]] SongClockSegment Snapshot() const noexcept { return current_; }
    [[nodiscard]] std::uint64_t Generation() const noexcept { return generation_; }
    [[nodiscard]] bool IsRunning() const noexcept { return current_.IsRunning(); }
    [[nodiscard]] std::int64_t Frequency() const noexcept { return current_.Frequency(); }

private:
    [[nodiscard]] bool AdvanceGeneration() noexcept;

    SongClockSegment current_{};
    std::uint64_t generation_ = 0;
};

} // namespace nosfix
