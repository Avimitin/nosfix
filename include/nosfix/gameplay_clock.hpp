#pragma once

// Guarded audio-observation adapter around the reviewed SongClock.
//
// The audio backend remains libxact. A libxact BGM cue label reports engine or
// source progress for that cue; it is NOT a measurement of the sample currently
// audible at the DAC, and no output-latency term has been verified. The adapter
// therefore keeps the observation source and its quality explicit instead of
// pretending every position is a device clock.
//
// Rules enforced here:
//   * Time comes only from an explicit observation or from the last accepted
//     anchor plus QPC. No render/update call advances song position.
//   * Every observation is bounded by the caller-supplied correction budget for
//     its source. Larger disagreements require an explicit Seek/new generation.
//   * With rate control (default), a PI controller adjusts the segment slope
//     continuously at its QPC: proportional error/horizon plus an integral
//     bias that estimates the rate mismatch. It never snaps the anchor and
//     never maps song time backwards; a persistent drift converges with a
//     near-zero residual instead of drift_rate * horizon.
//   * The legacy step policy is available for kernel compatibility; negative
//     errors are reported as RejectedNegativeDrift and retained.
//   * An observation may not move QPC backwards, may not come from another
//     playback generation, and may not come from a source of lower quality than
//     one already accepted (unless policy allows regression).
//   * A late observation never retimes already captured input; input mapping
//     uses SongClockSegment snapshots taken at capture time. A segment captured
//     before a rate change maps later QPCs differently from the new segment:
//     the input producer must acquire snapshot and QPC under one serialization
//     (see INTEGRATION.txt), not copy a snapshot on another thread.
//
// This class is single-owner and not atomic. It never blocks or allocates.

#include "nosfix/song_clock.hpp"

#include <cstdint>
#include <optional>

namespace nosfix {

// Ordered by increasing trust. BgmCueLabel is source/engine progress only.
enum class AudioObservationSource : std::uint8_t {
    Unknown = 0,
    BgmCueLabel = 1, // libxact GetSoundLabelLapse for the BGM cue
    VoiceSamples = 2, // XAudio2 voice SamplesPlayed converted by the producer
    DeviceRender = 3, // output-device consumed position, caller compensated
};

[[nodiscard]] const char* AudioObservationSourceName(AudioObservationSource source) noexcept;

// Unknown=0, cue=1, voice=2, device=3.
[[nodiscard]] constexpr std::uint8_t AudioObservationQuality(
    AudioObservationSource source) noexcept {
    return static_cast<std::uint8_t>(source);
}

struct AudioObservation final {
    std::int64_t qpc = 0;
    std::int64_t qpc_frequency = 0;
    // Song position the observation refers to, already latency-compensated by
    // its producer when the producer has a measured term. The adapter adds no
    // latency of its own and never derives one from a frame count. Negative
    // positions are legitimate preroll (Nostalgia starts at legacy -200).
    double song_us = 0.0;
    std::uint64_t generation = 0;
    AudioObservationSource source = AudioObservationSource::Unknown;
};

struct AudioObservationPolicy final {
    // Per-source maximum |observation - prediction| accepted as a correction.
    // The cue-label budget is not smaller than the device budget because a cue
    // can carry a larger engine offset; it is lower quality and is still marked
    // as such. These are policy inputs: measure the source before tuning them.
    double cue_label_correction_us = 2000.0;
    double voice_samples_correction_us = 1500.0;
    double device_correction_us = 1000.0;

    bool accept_cue_label = true;
    bool accept_voice_samples = true;
    bool accept_device = true;

    // Once a DeviceRender anchor has been accepted, a cue-label observation
    // must not pull the clock back onto a lower-quality source. The same rule
    // applies between voice and cue quality.
    bool reject_quality_regression = true;

    // Continuous synchronization. When enabled, a PI controller adjusts the
    // segment slope instead of snapping the anchor:
    //   proportional: rate_p = clamp(1 + error / horizon, 1 +/- max_delta)
    //   integral:     bias += (error / horizon) * (dt / horizon)
    //   rate = clamp(1 + bias + error / horizon, 1 +/- max_delta)
    // The integral term estimates the true rate mismatch, so a persistent drift
    // converges with near-zero residual instead of a steady error of
    // drift_rate * horizon. Rate changes are continuous at the observation QPC
    // and never map song time backwards. The supported envelope is a drift rate
    // within max_rate_delta and an offset transient within the source budget;
    // larger discontinuities require Seek.
    bool use_rate_control = true;
    double control_horizon_us = 2'000'000.0;
    double max_rate_delta = 0.02;

    // Hard bounds keep a malformed policy from disabling the checks.
    static constexpr double kMaximumBudgetUs = 1'000'000.0;
    static constexpr double kMaximumHorizonUs = 60'000'000.0;
    static constexpr double kMaximumRateDelta = 0.5;
};

enum class AudioObservationStatus : std::uint8_t {
    Accepted = 0,        // legacy step re-anchor
    AcceptedSlew = 1,    // continuous rate adjustment
    RejectedNotRunning,   // no valid running clock segment
    RejectedGeneration,   // zero or mismatched playback generation
    RejectedSource,       // Unknown or disabled source
    RejectedNonFinite,    // NaN/inf/absurd song position or invalid QPC rate
    RejectedFrequency,    // observation frequency differs from the segment
    RejectedBackward,     // QPC regressed
    RejectedNegativeDrift, // legacy policy: audio behind prediction
    RejectedOutOfBudget,  // correction larger than the source budget
    RejectedQualityRegression, // lower-quality source after a better anchor
    RejectedPolicy,       // invalid policy value
};

[[nodiscard]] const char* AudioObservationStatusName(AudioObservationStatus status) noexcept;

class GameplayClock final {
public:
    GameplayClock() = default;
    explicit GameplayClock(const AudioObservationPolicy& policy) noexcept;

    // Starts a new playback generation. Rejects a non-finite position or an
    // invalid QPC rate. The anchor quality returns to Unknown.
    [[nodiscard]] bool Start(std::int64_t qpc_ticks, std::int64_t qpc_frequency,
                             double song_microseconds = 0.0) noexcept;

    // Seek is a discontinuity and advances the generation. Consumers must drop
    // input captured under an older generation.
    [[nodiscard]] bool Seek(std::int64_t qpc_ticks, double song_microseconds) noexcept;

    [[nodiscard]] bool Pause(std::int64_t qpc_ticks) noexcept;
    [[nodiscard]] bool Resume(std::int64_t qpc_ticks) noexcept;
    [[nodiscard]] bool Stop() noexcept;

    // Current song time for the supplied host QPC, or empty when invalid or
    // paused. Pure query: does not advance, correct or re-anchor anything.
    [[nodiscard]] std::optional<SongTime> Now(std::int64_t qpc_ticks) const noexcept;

    // Immutable capture-time snapshot. Copy this when an input event is
    // timestamped; later observations must not retime that event.
    [[nodiscard]] SongClockSegment Snapshot() const noexcept { return clock_.Snapshot(); }

    // Applies one bounded audio observation. A rejected observation changes
    // neither the clock nor any accounting except the rejected counter and the
    // last status. See AudioObservationStatus for precedence.
    [[nodiscard]] AudioObservationStatus Observe(const AudioObservation& observation) noexcept;

    [[nodiscard]] const AudioObservationPolicy& Policy() const noexcept { return policy_; }
    [[nodiscard]] std::uint64_t Generation() const noexcept { return clock_.Generation(); }
    [[nodiscard]] bool IsRunning() const noexcept { return clock_.IsRunning(); }
    [[nodiscard]] AudioObservationSource AnchorSource() const noexcept { return anchor_source_; }
    [[nodiscard]] AudioObservationStatus LastObservationStatus() const noexcept {
        return last_status_;
    }
    [[nodiscard]] std::uint64_t AcceptedObservations() const noexcept { return accepted_; }
    [[nodiscard]] std::uint64_t RejectedObservations() const noexcept { return rejected_; }

    // Last audio-behind-prediction error that could not be applied. It is
    // retained for the legacy step policy; with rate control a negative error is
    // corrected through LastRate()/LastErrorUs() instead.
    [[nodiscard]] double PendingNegativeErrorUs() const noexcept {
        return pending_negative_error_us_;
    }

    // Diagnostics of the last applied correction. LastErrorUs is the signed
    // observation minus prediction at its QPC (positive = audio ahead).
    [[nodiscard]] double LastErrorUs() const noexcept { return last_error_us_; }
    [[nodiscard]] double LastRate() const noexcept { return last_rate_; }
    [[nodiscard]] bool RateClamped() const noexcept { return rate_clamped_; }
    [[nodiscard]] double Rate() const noexcept { return clock_.Snapshot().Rate(); }

private:
    [[nodiscard]] bool ValidPolicy() const noexcept;
    [[nodiscard]] double BudgetFor(AudioObservationSource source) const noexcept;
    [[nodiscard]] bool SourceEnabled(AudioObservationSource source) const noexcept;
    [[nodiscard]] AudioObservationStatus Reject(AudioObservationStatus status) noexcept;

    SongClock clock_{};
    AudioObservationPolicy policy_{};
    AudioObservationSource anchor_source_ = AudioObservationSource::Unknown;
    AudioObservationStatus last_status_ = AudioObservationStatus::RejectedNotRunning;
    std::int64_t last_observation_qpc_ = 0;
    bool has_observation_qpc_ = false;
    double pending_negative_error_us_ = 0.0;
    double last_error_us_ = 0.0;
    double last_rate_ = 1.0;
    double rate_bias_ = 0.0;
    std::int64_t last_correction_qpc_ = 0;
    bool has_correction_qpc_ = false;
    bool rate_clamped_ = false;
    std::uint64_t accepted_ = 0;
    std::uint64_t rejected_ = 0;
};

} // namespace nosfix
