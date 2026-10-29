#include "nosfix/audio_timeline.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

#if defined(_MSC_VER)
#include <intrin.h>
#endif

namespace nosfix::audio {
namespace {

constexpr std::int64_t kMinQpc = std::numeric_limits<std::int64_t>::min();
constexpr std::int64_t kMaxQpc = std::numeric_limits<std::int64_t>::max();
constexpr std::int64_t kMinSample = std::numeric_limits<std::int64_t>::min();
constexpr std::int64_t kMaxSample = std::numeric_limits<std::int64_t>::max();

bool CheckedSubtract(std::int64_t a, std::int64_t b, std::int64_t& out) noexcept {
    if (b > 0 && a < kMinQpc + b) return false;
    if (b < 0 && a > kMaxQpc + b) return false;
    out = a - b;
    return true;
}

bool CheckedAdd(std::int64_t a, std::int64_t b, std::int64_t& out) noexcept {
    if (b > 0 && a > kMaxQpc - b) return false;
    if (b < 0 && a < kMinQpc - b) return false;
    out = a + b;
    return true;
}

std::int64_t SaturatingAdd(std::int64_t a, std::int64_t b) noexcept {
    std::int64_t out = 0;
    if (!CheckedAdd(a, b, out)) return b > 0 ? kMaxSample : kMinSample;
    return out;
}

std::optional<std::int64_t> RoundToSample(double value) noexcept {
    if (!std::isfinite(value)) return std::nullopt;
    // Keep a margin below int64 limits so llround itself cannot overflow.
    constexpr double kLimit = 9.0e18;
    if (value <= -kLimit || value >= kLimit) return std::nullopt;
    const double rounded = std::round(value);
    if (rounded <= -kLimit || rounded >= kLimit) return std::nullopt;
    return static_cast<std::int64_t>(rounded);
}

bool ScaleMagnitude(std::uint64_t magnitude,
                    std::uint32_t multiply,
                    std::uint32_t divide,
                    std::uint64_t& quotient_out) noexcept {
    if (divide == 0) return false;
    if (magnitude == 0) {
        quotient_out = 0;
        return true;
    }
#if defined(_MSC_VER) && defined(_M_X64)
    unsigned __int64 high = 0;
    const unsigned __int64 low = _umul128(magnitude,
                                          static_cast<unsigned __int64>(multiply),
                                          &high);
    const unsigned __int64 divisor = static_cast<unsigned __int64>(divide);
    if (high >= divisor) return false; // quotient would exceed 64 bits
    unsigned __int64 remainder = 0;
    unsigned __int64 quotient = _udiv128(high, low, divisor, &remainder);
    // Round to nearest. The increment cannot overflow for realistic rates, but
    // check anyway.
    const unsigned __int64 twice = remainder * 2U;
    if (twice >= divide || twice < remainder) {
        if (quotient == std::numeric_limits<unsigned __int64>::max()) return false;
        ++quotient;
    }
    quotient_out = quotient;
    return true;
#else
    const long double product =
        static_cast<long double>(magnitude) * static_cast<long double>(multiply);
    const long double quotient = product / static_cast<long double>(divide);
    if (!(quotient >= 0.0L) || !std::isfinite(quotient)) return false;
    const long double rounded = std::floor(quotient + 0.5L);
    constexpr long double kTwoTo64 = 18446744073709551616.0L;
    if (rounded >= kTwoTo64) return false;
    quotient_out = static_cast<std::uint64_t>(rounded);
    return true;
#endif
}

} // namespace

bool AudioTimeline::Configure(const AudioTimelineConfig& config) noexcept {
    if (config.qpc_frequency <= 0) return false;
    if (config.sample_rate < 8000 || config.sample_rate > 384000) return false;
    if (config.nominal_buffer_frames == 0 || config.nominal_buffer_frames > 65536) {
        return false;
    }
    if (config.output_latency_frames > config.sample_rate * 10U) return false;
    if (!(config.drift_smoothing > 0.0) || config.drift_smoothing > 1.0) {
        return false;
    }
    if (config.discontinuity_threshold_frames < 0) return false;
    if (config.max_correction_frames_per_observation < 0) return false;

    qpc_frequency_ = config.qpc_frequency;
    sample_rate_ = config.sample_rate;
    buffer_frames_ = config.nominal_buffer_frames;
    output_latency_frames_ = config.output_latency_frames;
    threshold_setting_ = config.discontinuity_threshold_frames;
    correction_setting_ = config.max_correction_frames_per_observation;
    drift_smoothing_ = config.drift_smoothing;
    UpdateDerivedLimits();

    anchor_qpc_ = 0;
    anchor_sample_ = 0;
    // generation_ is a monotonic floor: reconfiguration must not let an old
    // generation become valid again.
    valid_ = false;
    running_ = false;
    have_observation_ = false;
    last_observation_qpc_ = 0;
    last_observation_sample_ = 0;
    drift_ppm_ = 0.0;
    counters_ = {};
    return true;
}

void AudioTimeline::UpdateDerivedLimits() noexcept {
    if (threshold_setting_ > 0) {
        discontinuity_threshold_frames_ = threshold_setting_;
    } else {
        const std::int64_t rate_based =
            static_cast<std::int64_t>(sample_rate_) / 200; // 5 ms
        const std::int64_t buffer_based =
            static_cast<std::int64_t>(buffer_frames_) * 4;
        discontinuity_threshold_frames_ = std::max<std::int64_t>(buffer_based,
                                                                 rate_based);
        if (discontinuity_threshold_frames_ < 1) discontinuity_threshold_frames_ = 1;
    }
    if (correction_setting_ > 0) {
        max_correction_frames_ = correction_setting_;
    } else {
        max_correction_frames_ = std::max<std::int64_t>(
            1, static_cast<std::int64_t>(buffer_frames_) / 64);
    }
}

bool AudioTimeline::BeginGeneration(std::uint64_t generation,
                                    std::int64_t qpc_ticks,
                                    std::int64_t driver_sample_position) noexcept {
    if (qpc_frequency_ == 0) return false;
    if (generation == 0 || generation <= generation_) return false;
    Reanchor(generation, qpc_ticks, driver_sample_position);
    return true;
}

void AudioTimeline::Reanchor(std::uint64_t generation,
                             std::int64_t qpc_ticks,
                             std::int64_t driver_sample_position) noexcept {
    anchor_qpc_ = qpc_ticks;
    anchor_sample_ = driver_sample_position;
    generation_ = generation;
    valid_ = true;
    running_ = true;
    have_observation_ = false;
    last_observation_qpc_ = 0;
    last_observation_sample_ = 0;
    drift_ppm_ = 0.0;
}

std::optional<std::int64_t> AudioTimeline::SampleAt(std::int64_t qpc_ticks) const noexcept {
    if (!valid_ || !running_ || qpc_frequency_ <= 0) return std::nullopt;
    std::int64_t delta_qpc = 0;
    if (!CheckedSubtract(qpc_ticks, anchor_qpc_, delta_qpc)) return std::nullopt;
    const double samples =
        static_cast<double>(anchor_sample_) +
        (static_cast<double>(delta_qpc) * static_cast<double>(sample_rate_)) /
            static_cast<double>(qpc_frequency_);
    return RoundToSample(samples);
}

TimelineStatus AudioTimeline::Observe(
    const DeviceClockObservation& observation) noexcept {
    if (!valid_) return TimelineStatus::NotStarted;
    if (!running_) return TimelineStatus::Paused;
    if (observation.qpc_frequency != qpc_frequency_) return TimelineStatus::InvalidArgument;
    if (observation.buffer_frames == 0) return TimelineStatus::InvalidArgument;
    if (observation.driver_sample_position < 0) return TimelineStatus::InvalidArgument;
    if (observation.output_latency_frames >
        static_cast<std::uint32_t>(sample_rate_) * 10U) {
        return TimelineStatus::InvalidArgument;
    }
    if (have_observation_ && observation.qpc_ticks <= last_observation_qpc_) {
        ++counters_.rejected_stale;
        return TimelineStatus::StaleObservation;
    }

    bool buffer_changed = false;
    if (observation.buffer_frames != buffer_frames_) {
        buffer_frames_ = observation.buffer_frames;
        buffer_changed = true;
        UpdateDerivedLimits();
    }
    if (observation.output_latency_frames != output_latency_frames_) {
        output_latency_frames_ = observation.output_latency_frames;
        ++counters_.latency_changes;
    }

    const auto predicted = SampleAt(observation.qpc_ticks);
    if (!predicted) return TimelineStatus::InvalidArgument;

    const double error = static_cast<double>(observation.driver_sample_position) -
                         static_cast<double>(*predicted);
    const bool discontinuous =
        observation.discontinuity || buffer_changed ||
        std::fabs(error) > static_cast<double>(discontinuity_threshold_frames_);

    ++counters_.observations;

    if (discontinuous) {
        ++counters_.discontinuities;
        Reanchor(generation_ + 1, observation.qpc_ticks,
                 observation.driver_sample_position);
        have_observation_ = true;
        last_observation_qpc_ = observation.qpc_ticks;
        last_observation_sample_ = observation.driver_sample_position;
        return TimelineStatus::Discontinuity;
    }

    if (have_observation_) {
        std::int64_t elapsed_qpc = 0;
        if (CheckedSubtract(observation.qpc_ticks, last_observation_qpc_, elapsed_qpc) &&
            elapsed_qpc > 0) {
            const double expected_samples =
                static_cast<double>(sample_rate_) * static_cast<double>(elapsed_qpc) /
                static_cast<double>(qpc_frequency_);
            if (expected_samples > 0.0) {
                const double actual_samples =
                    static_cast<double>(observation.driver_sample_position) -
                    static_cast<double>(last_observation_sample_);
                double instant_ppm =
                    (actual_samples / expected_samples - 1.0) * 1'000'000.0;
                instant_ppm = std::clamp(instant_ppm, -100'000.0, 100'000.0);
                drift_ppm_ += drift_smoothing_ * (instant_ppm - drift_ppm_);
            }
        }
    }

    // Bounded position feedback: absorb small observation error without ever
    // jumping the mapping. A real seek/reset exceeds the threshold above.
    std::int64_t correction = 0;
    if (error >= static_cast<double>(max_correction_frames_)) {
        correction = max_correction_frames_;
    } else if (error <= -static_cast<double>(max_correction_frames_)) {
        correction = -max_correction_frames_;
    } else {
        correction = static_cast<std::int64_t>(std::llround(error));
    }
    if (correction != 0) {
        std::int64_t adjusted = 0;
        if (CheckedAdd(anchor_sample_, correction, adjusted)) {
            anchor_sample_ = adjusted;
            ++counters_.corrections;
        }
    }

    have_observation_ = true;
    last_observation_qpc_ = observation.qpc_ticks;
    last_observation_sample_ = observation.driver_sample_position;
    return TimelineStatus::Ok;
}

std::optional<std::int64_t> AudioTimeline::MapQpcToRenderSample(
    std::int64_t qpc_ticks) const noexcept {
    return SampleAt(qpc_ticks);
}

std::optional<AudibleWindow> AudioTimeline::MapQpcToAudible(
    std::int64_t qpc_ticks) const noexcept {
    const auto rendered = SampleAt(qpc_ticks);
    if (!rendered) return std::nullopt;
    AudibleWindow window;
    window.nominal = SaturatingAdd(*rendered, -static_cast<std::int64_t>(output_latency_frames_));
    window.earliest = SaturatingAdd(window.nominal, -static_cast<std::int64_t>(buffer_frames_));
    window.latest = SaturatingAdd(window.nominal, static_cast<std::int64_t>(buffer_frames_));
    return window;
}

std::optional<std::int64_t> AudioTimeline::MapRenderSampleToQpc(
    std::int64_t render_sample) const noexcept {
    if (!valid_ || qpc_frequency_ <= 0) return std::nullopt;
    std::int64_t delta_samples = 0;
    if (!CheckedSubtract(render_sample, anchor_sample_, delta_samples)) return std::nullopt;
    const double qpc =
        static_cast<double>(anchor_qpc_) +
        (static_cast<double>(delta_samples) * static_cast<double>(qpc_frequency_)) /
            static_cast<double>(sample_rate_);
    return RoundToSample(qpc);
}

bool AudioTimeline::Pause(std::int64_t qpc_ticks) noexcept {
    if (!valid_ || !running_) return false;
    const auto frozen = SampleAt(qpc_ticks);
    if (!frozen) return false;
    anchor_qpc_ = qpc_ticks;
    anchor_sample_ = *frozen;
    running_ = false;
    have_observation_ = false;
    ++counters_.pause_resume;
    return true;
}

bool AudioTimeline::Resume(std::int64_t qpc_ticks) noexcept {
    if (!valid_ || running_) return false;
    if (qpc_ticks < anchor_qpc_) return false;
    anchor_qpc_ = qpc_ticks;
    running_ = true;
    have_observation_ = false;
    ++counters_.pause_resume;
    return true;
}

bool AudioTimeline::Seek(std::uint64_t generation,
                         std::int64_t qpc_ticks,
                         std::int64_t driver_sample_position) noexcept {
    if (qpc_frequency_ == 0) return false;
    if (generation == 0 || generation <= generation_) return false;
    Reanchor(generation, qpc_ticks, driver_sample_position);
    ++counters_.discontinuities;
    return true;
}

bool AudioTimeline::SetOutputLatency(std::uint32_t frames) noexcept {
    if (sample_rate_ == 0) return false;
    if (frames > sample_rate_ * 10U) return false;
    if (frames == output_latency_frames_) return true;
    output_latency_frames_ = frames;
    ++counters_.latency_changes;
    return true;
}

std::optional<std::int64_t> AudioTimeline::FrozenRenderSample() const noexcept {
    if (!valid_ || running_) return std::nullopt;
    return anchor_sample_;
}

bool TryScaleSourceSamples(std::int64_t source_samples,
                           std::uint32_t source_rate,
                           std::uint32_t destination_rate,
                           std::int64_t& out) noexcept {
    if (source_rate == 0 || destination_rate == 0) return false;
    if (source_samples == 0 || source_rate == destination_rate) {
        out = source_samples;
        return true;
    }
    const bool negative = source_samples < 0;
    const std::uint64_t magnitude =
        negative ? (0ULL - static_cast<std::uint64_t>(source_samples))
                 : static_cast<std::uint64_t>(source_samples);
    std::uint64_t quotient = 0;
    if (!ScaleMagnitude(magnitude, destination_rate, source_rate, quotient)) {
        return false;
    }
    const std::uint64_t positive_limit =
        static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max());
    if (negative) {
        if (quotient > positive_limit + 1ULL) return false;
        if (quotient == positive_limit + 1ULL) {
            out = std::numeric_limits<std::int64_t>::min();
            return true;
        }
        out = -static_cast<std::int64_t>(quotient);
        return true;
    }
    if (quotient > positive_limit) return false;
    out = static_cast<std::int64_t>(quotient);
    return true;
}

bool SourceRateCursor::Configure(std::uint32_t source_rate,
                                 std::uint32_t device_rate,
                                 std::int64_t origin_source) noexcept {
    if (source_rate == 0 || device_rate == 0) return false;
    source_rate_ = source_rate;
    device_rate_ = device_rate;
    origin_source_ = origin_source;
    return true;
}

std::optional<std::int64_t> SourceRateCursor::SourceAtDeviceFrame(
    std::int64_t device_frame) const noexcept {
    if (!Valid()) return std::nullopt;
    std::int64_t scaled = 0;
    if (!TryScaleSourceSamples(device_frame, device_rate_, source_rate_, scaled)) {
        return std::nullopt;
    }
    std::int64_t result = 0;
    if (!CheckedAdd(origin_source_, scaled, result)) return std::nullopt;
    return result;
}

} // namespace nosfix::audio
