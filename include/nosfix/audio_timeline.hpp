#pragma once

#include <cstdint>
#include <optional>

// Device sample timeline for an ASIO output.
//
// Terminology is deliberate:
//   render sample  = the sample index of a buffer handed to ASIO by the
//                    bufferSwitch callback. At callback time the buffer starts
//                    at `driver_sample_position` (ASIO samplePosition).
//   audible sample = an estimate of the sample leaving the DAC:
//                    render_sample - driver_reported_output_latency.
// ASIOGetLatencies() includes the driver's double buffering. The estimate is
// therefore a window, not a measurement: it is not the DAC position and it
// does not prove what a listener hears. No code here claims otherwise.
//
// Driver timeInfo.systemTime is nanoseconds derived by the driver (on Windows
// the SDK says from timeGetTime), not QPC. Observations therefore use a host
// QPC captured at callback entry for correlation; the driver timestamp is
// carried separately as diagnostic data by asio_output.
namespace nosfix::audio {

enum class TimelineStatus {
    Ok,
    NotStarted,
    InvalidArgument,
    StaleObservation,
    Discontinuity,
    Paused,
};

struct AudioTimelineConfig {
    std::int64_t qpc_frequency = 0;
    std::uint32_t sample_rate = 0;
    std::uint32_t nominal_buffer_frames = 0;
    std::uint32_t output_latency_frames = 0;

    // An observation further than this from the prediction is treated as a
    // discontinuity (seek, device reset, driver restart), not as drift.
    // 0 selects max(4 * nominal buffer, sample_rate / 200) (5 ms at 44.1 kHz).
    std::int64_t discontinuity_threshold_frames = 0;

    // Maximum position correction applied per observation while tracking
    // drift. 0 selects max(1, nominal_buffer / 64).
    std::int64_t max_correction_frames_per_observation = 0;

    // Smoothing factor for the reported drift estimate, in (0, 1].
    double drift_smoothing = 0.125;
};

struct DeviceClockObservation {
    // ASIO timeInfo.samplePosition: the sample position of the buffer just
    // delivered to the callback (its first sample).
    std::int64_t driver_sample_position = 0;

    // Host QueryPerformanceCounter value captured at callback entry.
    std::int64_t qpc_ticks = 0;
    std::int64_t qpc_frequency = 0;

    std::uint32_t buffer_frames = 0;

    // ASIOGetLatencies() output latency in samples. Includes the driver's
    // double buffer. Values are allowed to change only on a buffer/rate change.
    std::uint32_t output_latency_frames = 0;

    // The caller knows a reset/restart/seek happened. The timeline advances
    // its device generation and reanchors instead of treating it as drift.
    bool discontinuity = false;
};

struct AudibleWindow {
    std::int64_t earliest = 0; // nominal - one buffer
    std::int64_t nominal = 0;  // render position - reported output latency
    std::int64_t latest = 0;   // nominal + one buffer
};

struct TimelineCounters {
    std::uint64_t observations = 0;
    std::uint64_t discontinuities = 0;
    std::uint64_t corrections = 0;
    std::uint64_t rejected_stale = 0;
    std::uint64_t latency_changes = 0;
    std::uint64_t pause_resume = 0;
};

// Owned and mutated by the audio callback thread. Map calls are cheap and
// allocation free. Cross-thread readers need a snapshot published by the
// integration; this class is not atomic.
class AudioTimeline final {
public:
    AudioTimeline() = default;

    // Reconfigures the rate/buffer/latency and invalidates the mapping. The
    // generation floor is preserved; call BeginGeneration with a larger value.
    [[nodiscard]] bool Configure(const AudioTimelineConfig& config) noexcept;

    // Starts a mapping. The generation must be nonzero and strictly greater
    // than the current floor (zero on a fresh object). Configure() preserves the
    // floor so a reconfigured timeline cannot reactivate an old generation.
    [[nodiscard]] bool BeginGeneration(std::uint64_t generation,
                                       std::int64_t qpc_ticks,
                                       std::int64_t driver_sample_position) noexcept;

    [[nodiscard]] TimelineStatus Observe(
        const DeviceClockObservation& observation) noexcept;

    // Render-domain mapping. nullopt if the timeline is not started, paused or
    // the arithmetic would leave int64.
    [[nodiscard]] std::optional<std::int64_t> MapQpcToRenderSample(
        std::int64_t qpc_ticks) const noexcept;

    [[nodiscard]] std::optional<AudibleWindow> MapQpcToAudible(
        std::int64_t qpc_ticks) const noexcept;

    [[nodiscard]] std::optional<std::int64_t> MapRenderSampleToQpc(
        std::int64_t render_sample) const noexcept;

    // Device pause: the driver stopped rendering. Mapping is unavailable until
    // Resume. The generation is unchanged because it is the same device run.
    [[nodiscard]] bool Pause(std::int64_t qpc_ticks) noexcept;
    [[nodiscard]] bool Resume(std::int64_t qpc_ticks) noexcept;

    // Explicit discontinuity with a caller-supplied device generation (seek,
    // device reset, driver restart). Must be nonzero and greater than the
    // current generation floor.
    [[nodiscard]] bool Seek(std::uint64_t generation,
                            std::int64_t qpc_ticks,
                            std::int64_t driver_sample_position) noexcept;

    [[nodiscard]] bool SetOutputLatency(std::uint32_t frames) noexcept;

    [[nodiscard]] bool IsValid() const noexcept { return valid_; }
    [[nodiscard]] bool IsRunning() const noexcept { return valid_ && running_; }
    [[nodiscard]] std::uint64_t Generation() const noexcept { return generation_; }
    [[nodiscard]] std::int64_t QpcFrequency() const noexcept { return qpc_frequency_; }
    [[nodiscard]] std::uint32_t SampleRate() const noexcept { return sample_rate_; }
    [[nodiscard]] std::uint32_t BufferFrames() const noexcept { return buffer_frames_; }
    [[nodiscard]] std::uint32_t OutputLatencyFrames() const noexcept { return output_latency_frames_; }
    [[nodiscard]] std::int64_t DiscontinuityThresholdFrames() const noexcept {
        return discontinuity_threshold_frames_;
    }
    [[nodiscard]] double DriftPpm() const noexcept { return drift_ppm_; }
    [[nodiscard]] TimelineCounters Counters() const noexcept { return counters_; }

    // Frozen render sample while paused. nullopt when running or invalid.
    [[nodiscard]] std::optional<std::int64_t> FrozenRenderSample() const noexcept;

private:
    [[nodiscard]] std::optional<std::int64_t> SampleAt(std::int64_t qpc_ticks) const noexcept;
    void Reanchor(std::uint64_t generation,
                  std::int64_t qpc_ticks,
                  std::int64_t driver_sample_position) noexcept;
    void UpdateDerivedLimits() noexcept;

    std::int64_t qpc_frequency_ = 0;
    std::uint32_t sample_rate_ = 0;
    std::uint32_t buffer_frames_ = 0;
    std::uint32_t output_latency_frames_ = 0;
    std::int64_t discontinuity_threshold_frames_ = 0;
    std::int64_t max_correction_frames_ = 0;
    std::int64_t threshold_setting_ = 0;
    std::int64_t correction_setting_ = 0;
    double drift_smoothing_ = 0.125;

    std::int64_t anchor_qpc_ = 0;
    std::int64_t anchor_sample_ = 0;
    std::uint64_t generation_ = 0;
    bool valid_ = false;
    bool running_ = false;

    bool have_observation_ = false;
    std::int64_t last_observation_qpc_ = 0;
    std::int64_t last_observation_sample_ = 0;

    double drift_ppm_ = 0.0;
    TimelineCounters counters_{};
};

// Converts a source-domain sample count (SamplesPlayed) to a device-rate count.
// This is a unit conversion, not a claim that the source counter equals the
// DAC position. Rounds to nearest; returns false on invalid rates / overflow.
[[nodiscard]] bool TryScaleSourceSamples(std::int64_t source_samples,
                                         std::uint32_t source_rate,
                                         std::uint32_t destination_rate,
                                         std::int64_t& out) noexcept;

// Exact rational cursor for one voice's source counters while the device runs
// at another rate. `origin_source` is the source sample corresponding to
// device frame 0. No accumulated rounding: the source position at any device
// frame is computed from the integer frame count each time.
class SourceRateCursor final {
public:
    [[nodiscard]] bool Configure(std::uint32_t source_rate,
                                 std::uint32_t device_rate,
                                 std::int64_t origin_source) noexcept;
    [[nodiscard]] bool Valid() const noexcept { return source_rate_ != 0; }
    [[nodiscard]] std::optional<std::int64_t> SourceAtDeviceFrame(
        std::int64_t device_frame) const noexcept;
private:
    std::uint32_t source_rate_ = 0;
    std::uint32_t device_rate_ = 0;
    std::int64_t origin_source_ = 0;
};

} // namespace nosfix::audio
