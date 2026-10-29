#include "nosfix/gameplay_clock.hpp"

#include <algorithm>
#include <cmath>

namespace nosfix {
namespace {

constexpr double kMaximumSongUs = 1.0e15;

bool FinitePosition(double song_us) noexcept {
    return std::isfinite(song_us) && std::abs(song_us) <= kMaximumSongUs;
}

} // namespace

const char* AudioObservationSourceName(AudioObservationSource source) noexcept {
    switch (source) {
    case AudioObservationSource::Unknown: return "unknown";
    case AudioObservationSource::BgmCueLabel: return "libxact-bgm-cue-label";
    case AudioObservationSource::VoiceSamples: return "xaudio2-voice-samples";
    case AudioObservationSource::DeviceRender: return "device-render-position";
    }
    return "invalid";
}

const char* AudioObservationStatusName(AudioObservationStatus status) noexcept {
    switch (status) {
    case AudioObservationStatus::Accepted: return "accepted";
    case AudioObservationStatus::AcceptedSlew: return "accepted-slew";
    case AudioObservationStatus::RejectedNotRunning: return "rejected-not-running";
    case AudioObservationStatus::RejectedGeneration: return "rejected-generation";
    case AudioObservationStatus::RejectedSource: return "rejected-source";
    case AudioObservationStatus::RejectedNonFinite: return "rejected-non-finite";
    case AudioObservationStatus::RejectedFrequency: return "rejected-frequency";
    case AudioObservationStatus::RejectedBackward: return "rejected-backward";
    case AudioObservationStatus::RejectedNegativeDrift: return "rejected-negative-drift";
    case AudioObservationStatus::RejectedOutOfBudget: return "rejected-out-of-budget";
    case AudioObservationStatus::RejectedQualityRegression: return "rejected-quality-regression";
    case AudioObservationStatus::RejectedPolicy: return "rejected-policy";
    }
    return "invalid";
}

GameplayClock::GameplayClock(const AudioObservationPolicy& policy) noexcept : policy_(policy) {}

bool GameplayClock::ValidPolicy() const noexcept {
    const auto valid_budget = [](double value) {
        return std::isfinite(value) && value >= 0.0 &&
               value <= AudioObservationPolicy::kMaximumBudgetUs;
    };
    const bool control_valid =
        std::isfinite(policy_.control_horizon_us) &&
        policy_.control_horizon_us > 0.0 &&
        policy_.control_horizon_us <= AudioObservationPolicy::kMaximumHorizonUs &&
        std::isfinite(policy_.max_rate_delta) &&
        policy_.max_rate_delta > 0.0 &&
        policy_.max_rate_delta <= AudioObservationPolicy::kMaximumRateDelta;
    return valid_budget(policy_.cue_label_correction_us) &&
           valid_budget(policy_.voice_samples_correction_us) &&
           valid_budget(policy_.device_correction_us) && control_valid;
}

double GameplayClock::BudgetFor(AudioObservationSource source) const noexcept {
    switch (source) {
    case AudioObservationSource::BgmCueLabel: return policy_.cue_label_correction_us;
    case AudioObservationSource::VoiceSamples: return policy_.voice_samples_correction_us;
    case AudioObservationSource::DeviceRender: return policy_.device_correction_us;
    case AudioObservationSource::Unknown: break;
    }
    return -1.0;
}

bool GameplayClock::SourceEnabled(AudioObservationSource source) const noexcept {
    switch (source) {
    case AudioObservationSource::BgmCueLabel: return policy_.accept_cue_label;
    case AudioObservationSource::VoiceSamples: return policy_.accept_voice_samples;
    case AudioObservationSource::DeviceRender: return policy_.accept_device;
    case AudioObservationSource::Unknown: break;
    }
    return false;
}

AudioObservationStatus GameplayClock::Reject(AudioObservationStatus status) noexcept {
    last_status_ = status;
    ++rejected_;
    return status;
}

bool GameplayClock::Start(std::int64_t qpc_ticks, std::int64_t qpc_frequency,
                          double song_microseconds) noexcept {
    if (!FinitePosition(song_microseconds) || qpc_ticks < 0) return false;
    if (!clock_.Start(qpc_ticks, qpc_frequency, song_microseconds)) return false;
    anchor_source_ = AudioObservationSource::Unknown;
    has_observation_qpc_ = false;
    last_observation_qpc_ = 0;
    pending_negative_error_us_ = 0.0;
    last_error_us_ = 0.0;
    last_rate_ = 1.0;
    rate_bias_ = 0.0;
    has_correction_qpc_ = false;
    last_correction_qpc_ = 0;
    rate_clamped_ = false;
    last_status_ = AudioObservationStatus::RejectedNotRunning;
    return true;
}

bool GameplayClock::Seek(std::int64_t qpc_ticks, double song_microseconds) noexcept {
    if (!FinitePosition(song_microseconds) || qpc_ticks < 0) return false;
    if (!clock_.Seek(qpc_ticks, song_microseconds)) return false;
    anchor_source_ = AudioObservationSource::Unknown;
    has_observation_qpc_ = false;
    last_observation_qpc_ = 0;
    pending_negative_error_us_ = 0.0;
    last_error_us_ = 0.0;
    last_rate_ = 1.0;
    rate_bias_ = 0.0;
    has_correction_qpc_ = false;
    last_correction_qpc_ = 0;
    rate_clamped_ = false;
    last_status_ = AudioObservationStatus::RejectedNotRunning;
    return true;
}

bool GameplayClock::Pause(std::int64_t qpc_ticks) noexcept { return clock_.Pause(qpc_ticks); }
bool GameplayClock::Resume(std::int64_t qpc_ticks) noexcept { return clock_.Resume(qpc_ticks); }
bool GameplayClock::Stop() noexcept { return clock_.Stop(); }

std::optional<SongTime> GameplayClock::Now(std::int64_t qpc_ticks) const noexcept {
    if (!clock_.IsRunning()) return std::nullopt;
    return clock_.Snapshot().Map(qpc_ticks);
}

AudioObservationStatus GameplayClock::Observe(const AudioObservation& observation) noexcept {
    if (!ValidPolicy()) return Reject(AudioObservationStatus::RejectedPolicy);
    if (!clock_.IsRunning()) return Reject(AudioObservationStatus::RejectedNotRunning);
    if (observation.generation == 0 ||
        observation.generation != clock_.Generation()) {
        return Reject(AudioObservationStatus::RejectedGeneration);
    }
    if (!SourceEnabled(observation.source)) {
        return Reject(AudioObservationStatus::RejectedSource);
    }
    if (observation.qpc < 0 || observation.qpc_frequency <= 0 ||
        !FinitePosition(observation.song_us)) {
        return Reject(AudioObservationStatus::RejectedNonFinite);
    }
    if (observation.qpc_frequency != clock_.Frequency()) {
        // A source with a different QPC frequency cannot be compared with this
        // segment's anchors; reject before touching the integrator/diagnostics.
        return Reject(AudioObservationStatus::RejectedFrequency);
    }
    if (policy_.reject_quality_regression &&
        AudioObservationQuality(observation.source) <
            AudioObservationQuality(anchor_source_)) {
        return Reject(AudioObservationStatus::RejectedQualityRegression);
    }
    if (has_observation_qpc_ && observation.qpc < last_observation_qpc_) {
        return Reject(AudioObservationStatus::RejectedBackward);
    }

    const auto predicted = clock_.Snapshot().Map(observation.qpc);
    if (!predicted) return Reject(AudioObservationStatus::RejectedNotRunning);

    const double budget = BudgetFor(observation.source);
    const double error = observation.song_us - predicted->microseconds;
    if (!std::isfinite(error)) {
        return Reject(AudioObservationStatus::RejectedNonFinite);
    }
    if (std::abs(error) > budget) {
        return Reject(AudioObservationStatus::RejectedOutOfBudget);
    }

    if (policy_.use_rate_control) {
        // PI controller. The proportional term reacts to the current offset;
        // the integral term estimates the persistent rate mismatch so a long
        // song converges instead of holding a drift_rate * horizon residual.
        // Both terms are clamped and the anchor is joined at the CURRENT
        // predicted position, so the map is continuous and never goes back.
        const double horizon = policy_.control_horizon_us;
        const double horizon_s = horizon / 1'000'000.0;
        double dt_s = 0.0;
        if (has_correction_qpc_ && observation.qpc > last_correction_qpc_) {
            dt_s = static_cast<double>(observation.qpc - last_correction_qpc_) /
                   static_cast<double>(observation.qpc_frequency);
            if (dt_s > horizon_s) dt_s = horizon_s;
        }
        if (dt_s > 0.0) {
            rate_bias_ += (error / horizon) * (dt_s / horizon_s);
        }
        const double low = 1.0 - policy_.max_rate_delta;
        const double high = 1.0 + policy_.max_rate_delta;
        rate_bias_ = std::clamp(rate_bias_, -policy_.max_rate_delta,
                                policy_.max_rate_delta);
        const double raw_rate = 1.0 + rate_bias_ + error / horizon;
        const double rate = std::clamp(raw_rate, low, high);
        rate_clamped_ = rate != raw_rate;
        if (!clock_.ChangeRate(observation.qpc, rate)) {
            return Reject(AudioObservationStatus::RejectedOutOfBudget);
        }
        last_error_us_ = error;
        last_rate_ = clock_.Snapshot().Rate();
        last_correction_qpc_ = observation.qpc;
        has_correction_qpc_ = true;
        pending_negative_error_us_ = 0.0;
        anchor_source_ = observation.source;
        last_observation_qpc_ = observation.qpc;
        has_observation_qpc_ = true;
        last_status_ = AudioObservationStatus::AcceptedSlew;
        ++accepted_;
        return last_status_;
    }

    if (error < 0.0) {
        // Legacy step policy. Snapping the anchor backwards would violate
        // SongClock monotonicity and retime captured input; record the request
        // instead. The rate-control policy above is the supported correction.
        pending_negative_error_us_ = -error;
        return Reject(AudioObservationStatus::RejectedNegativeDrift);
    }
    if (!clock_.Observe(observation.qpc, observation.qpc_frequency,
                        observation.song_us, budget)) {
        return Reject(AudioObservationStatus::RejectedOutOfBudget);
    }

    pending_negative_error_us_ = 0.0;
    last_error_us_ = error;
    last_rate_ = clock_.Snapshot().Rate();
    rate_clamped_ = false;
    anchor_source_ = observation.source;
    last_observation_qpc_ = observation.qpc;
    has_observation_qpc_ = true;
    last_status_ = AudioObservationStatus::Accepted;
    ++accepted_;
    return last_status_;
}

} // namespace nosfix
