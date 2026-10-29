#include "test_support.hpp"

#include "nosfix/audio_timeline.hpp"

#include <cmath>
#include <cstdint>
#include <limits>

namespace {

using nosfix::audio::AudioTimeline;
using nosfix::audio::AudioTimelineConfig;
using nosfix::audio::DeviceClockObservation;
using nosfix::audio::SourceRateCursor;
using nosfix::audio::TimelineStatus;

constexpr std::int64_t kFrequency = 10'000'000;
constexpr std::uint32_t kRate = 44100;
constexpr std::int64_t kEpoch = 1'000'000'000; // 100 s of QPC uptime

DeviceClockObservation Observation(std::int64_t qpc, std::int64_t sample,
                                   std::uint32_t buffer = 128,
                                   std::uint32_t latency = 256,
                                   bool discontinuity = false) {
    DeviceClockObservation observation;
    observation.driver_sample_position = sample;
    observation.qpc_ticks = qpc;
    observation.qpc_frequency = kFrequency;
    observation.buffer_frames = buffer;
    observation.output_latency_frames = latency;
    observation.discontinuity = discontinuity;
    return observation;
}

} // namespace

int main() {
    AudioTimeline timeline;
    AudioTimelineConfig config;
    config.qpc_frequency = kFrequency;
    config.sample_rate = kRate;
    config.nominal_buffer_frames = 128;
    config.output_latency_frames = 256;
    CHECK(timeline.Configure(config));
    CHECK(!timeline.IsValid());
    CHECK(!timeline.MapQpcToRenderSample(kEpoch));

    CHECK(timeline.BeginGeneration(1, kEpoch, 0));
    CHECK(timeline.IsValid() && timeline.IsRunning());
    CHECK(timeline.Generation() == 1);
    CHECK(!timeline.BeginGeneration(1, kEpoch + 1, 0));
    CHECK(!timeline.BeginGeneration(0, kEpoch + 1, 0));

    // 2500 ticks = 0.25 ms = 11.025 samples at 44.1 kHz. Integer QPC
    // subtraction before scaling keeps sub-millisecond timestamps precise.
    const auto fractional = timeline.MapQpcToRenderSample(kEpoch + 2'500);
    CHECK(fractional && *fractional == 11);
    const auto audible = timeline.MapQpcToAudible(kEpoch + 2'500);
    CHECK(audible);
    CHECK(audible->nominal == -245);  // 11 - 256 reported output latency
    CHECK(audible->earliest == -373); // one buffer of uncertainty
    CHECK(audible->latest == -117);
    const auto back = timeline.MapRenderSampleToQpc(11);
    CHECK(back && std::llabs(*back - (kEpoch + 2'494)) <= 1);

    // 128 frames at 44100 Hz is 29024.94 ticks; the observation lands at 128.
    const std::int64_t qpc128 = kEpoch + 29'025;
    CHECK(timeline.Observe(Observation(qpc128, 128)) == TimelineStatus::Ok);
    CHECK(timeline.Counters().observations == 1);
    const std::int64_t qpc256 = kEpoch + 58'050;
    CHECK(timeline.Observe(Observation(qpc256, 256)) == TimelineStatus::Ok);
    CHECK(timeline.Generation() == 1);

    // Same QPC is rejected instead of re-anchoring.
    CHECK(timeline.Observe(Observation(qpc256, 256)) ==
          TimelineStatus::StaleObservation);
    CHECK(timeline.Counters().rejected_stale == 1);
    CHECK(timeline.Observe(Observation(qpc256 + 10, 300, 0, 256)) ==
          TimelineStatus::InvalidArgument);
    CHECK(timeline.Observe(Observation(qpc256 + 10, 300, 128, 256)) ==
          TimelineStatus::Ok);

    // A 44-sample error is absorbed by a bounded correction, not a jump.
    const auto corrected = timeline.MapQpcToRenderSample(qpc256 + 10);
    CHECK(corrected && *corrected >= 256 && *corrected <= 260);
    CHECK(timeline.Counters().corrections > 0);
    CHECK(timeline.DiscontinuityThresholdFrames() >= 128);

    // A large jump is a new device generation, not drift.
    const std::int64_t qpcJump = qpc256 + 100'000;
    CHECK(timeline.Observe(Observation(qpcJump, 10'000)) ==
          TimelineStatus::Discontinuity);
    CHECK(timeline.Generation() == 2);
    CHECK(timeline.Counters().discontinuities == 1);
    const auto after_jump = timeline.MapQpcToRenderSample(qpcJump);
    CHECK(after_jump && *after_jump == 10'000);

    // Pause freezes and hides mapping. Resume preserves the frozen sample and
    // the device generation, and does not count paused wall time.
    const std::int64_t pause_qpc = qpcJump + 10'000;
    CHECK(timeline.Pause(pause_qpc));
    CHECK(!timeline.IsRunning());
    CHECK(!timeline.MapQpcToRenderSample(pause_qpc + 1));
    const auto frozen = timeline.FrozenRenderSample();
    CHECK(frozen.has_value());
    CHECK(timeline.Observe(Observation(pause_qpc + 1, 11'000)) ==
          TimelineStatus::Paused);
    CHECK(!timeline.Resume(pause_qpc - 1));
    const std::int64_t resume_qpc = pause_qpc + 50'000'000;
    CHECK(timeline.Resume(resume_qpc));
    CHECK(timeline.Generation() == 2);
    const auto resumed = timeline.MapQpcToRenderSample(resume_qpc + 2'500);
    CHECK(resumed && *resumed == *frozen + 11);
    CHECK(timeline.Counters().pause_resume == 2);

    // Seek is an explicit discontinuity with a caller-chosen generation.
    CHECK(!timeline.Seek(2, resume_qpc + 100, 0));
    CHECK(timeline.Seek(3, resume_qpc + 100, 50'000));
    CHECK(timeline.Generation() == 3);
    CHECK(timeline.MapQpcToRenderSample(resume_qpc + 100).has_value());
    CHECK(!timeline.Seek(1, resume_qpc + 200, 0));

    // Reported latency changes move the audible estimate, not render mapping.
    const auto before_latency = timeline.MapQpcToAudible(resume_qpc + 100);
    CHECK(before_latency);
    CHECK(timeline.SetOutputLatency(512));
    const auto after_latency = timeline.MapQpcToAudible(resume_qpc + 100);
    CHECK(after_latency);
    CHECK(after_latency->nominal == before_latency->nominal - 256);
    CHECK(timeline.OutputLatencyFrames() == 512);
    CHECK(!timeline.SetOutputLatency(static_cast<std::uint32_t>(kRate * 100)));

    // Buffer size changes are reconfigurations even when position looks close.
    const std::int64_t buffer_qpc = resume_qpc + 200;
    CHECK(timeline.Observe(Observation(buffer_qpc, 50'000, 256, 512)) ==
          TimelineStatus::Discontinuity);
    CHECK(timeline.BufferFrames() == 256);

    // Reconfiguration preserves the generation floor: an old or zero
    // generation must never become valid again.
    const std::uint64_t floor = timeline.Generation();
    CHECK(timeline.Configure(config));
    CHECK(!timeline.BeginGeneration(0, buffer_qpc, 0));
    CHECK(!timeline.BeginGeneration(floor, buffer_qpc, 0));
    CHECK(timeline.BeginGeneration(floor + 1, buffer_qpc, 0));
    CHECK(timeline.Generation() == floor + 1);

    AudioTimeline zero_generation;
    CHECK(zero_generation.Configure(config));
    CHECK(!zero_generation.BeginGeneration(0, kEpoch, 0));
    CHECK(zero_generation.BeginGeneration(1, kEpoch, 0));

    // Huge absolute QPC values: subtract the int64 anchors first.
    AudioTimeline large;
    CHECK(large.Configure(config));
    const std::int64_t huge = 9'000'000'000'000'000LL;
    CHECK(large.BeginGeneration(10, huge, 0));
    CHECK(large.MapQpcToRenderSample(huge + 2'500) == 11);
    const auto large_back = large.MapRenderSampleToQpc(11);
    CHECK(large_back && std::llabs(*large_back - (huge + 2'494)) <= 1);

    // Invalid configuration is rejected transactionally.
    AudioTimeline invalid;
    auto bad = config;
    bad.qpc_frequency = 0;
    CHECK(!invalid.Configure(bad));
    bad = config;
    bad.sample_rate = 0;
    CHECK(!invalid.Configure(bad));
    bad = config;
    bad.nominal_buffer_frames = 0;
    CHECK(!invalid.Configure(bad));
    bad = config;
    bad.drift_smoothing = 0.0;
    CHECK(!invalid.Configure(bad));
    bad = config;
    bad.discontinuity_threshold_frames = -1;
    CHECK(!invalid.Configure(bad));

    // Exact rational source/device sample conversion.
    std::int64_t scaled = 0;
    CHECK(nosfix::audio::TryScaleSourceSamples(44'100, 44'100, 48'000, scaled));
    CHECK(scaled == 48'000);
    CHECK(nosfix::audio::TryScaleSourceSamples(100, 44'100, 48'000, scaled));
    CHECK(scaled == 109); // 108.843..., rounded to nearest
    CHECK(nosfix::audio::TryScaleSourceSamples(-44'100, 44'100, 48'000, scaled));
    CHECK(scaled == -48'000);
    CHECK(nosfix::audio::TryScaleSourceSamples(1'587'600'000, 44'100, 48'000,
                                               scaled));
    CHECK(scaled == 1'728'000'000);
    CHECK(!nosfix::audio::TryScaleSourceSamples(
        std::numeric_limits<std::int64_t>::max(), 44'100, 48'000, scaled));
    CHECK(!nosfix::audio::TryScaleSourceSamples(
        std::numeric_limits<std::int64_t>::min(), 44'100, 48'000, scaled));
    // Force the 128-bit quotient itself out of range: high word >= divisor.
    CHECK(!nosfix::audio::TryScaleSourceSamples(
        std::numeric_limits<std::int64_t>::max(), 1, 4'000'000'000U, scaled));
    CHECK(!nosfix::audio::TryScaleSourceSamples(1, 0, 48'000, scaled));
    CHECK(!nosfix::audio::TryScaleSourceSamples(1, 44'100, 0, scaled));

    SourceRateCursor cursor;
    CHECK(cursor.Configure(44'100, 48'000, 1'000));
    const auto at_zero = cursor.SourceAtDeviceFrame(0);
    CHECK(at_zero && *at_zero == 1'000);
    const auto at_one = cursor.SourceAtDeviceFrame(1);
    CHECK(at_one && *at_one == 1'001);
    const auto at_day = cursor.SourceAtDeviceFrame(1'728'000'000);
    CHECK(at_day && *at_day == 1'587'601'000);
    SourceRateCursor invalid_cursor;
    CHECK(!invalid_cursor.Configure(0, 48'000, 0));
    CHECK(!invalid_cursor.SourceAtDeviceFrame(0));

    return test::Finish("audio_timeline");
}
