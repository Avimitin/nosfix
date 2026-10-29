#include "test_support.hpp"

#include "fake_asio_driver.hpp"
#include "nosfix/audio_timeline.hpp"
#include "nosfix/asio_output.hpp"

#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace {

using namespace nosfix::audio;

struct RenderProbe {
    std::uint32_t calls = 0;
    std::int64_t last_start = 0;
    std::uint64_t last_generation = 0;
    std::uint32_t last_frames = 0;
    bool last_sample_position_valid = false;
    bool last_time_info_mode = false;
    bool fill = true;
    float left = 0.25F;
    float right = -0.5F;
};

bool Render(void* context, const AsioRenderContext& render_context,
            float* const* channels) noexcept {
    auto* probe = static_cast<RenderProbe*>(context);
    ++probe->calls;
    probe->last_start = render_context.block_start_sample;
    probe->last_generation = render_context.generation;
    probe->last_frames = render_context.buffer_frames;
    probe->last_sample_position_valid = render_context.sample_position_valid;
    probe->last_time_info_mode = render_context.time_info_mode;
    if (!probe->fill) return false;
    for (std::uint32_t frame = 0; frame < render_context.buffer_frames; ++frame) {
        channels[0][frame] = probe->left;
        channels[1][frame] = probe->right;
    }
    return true;
}

AsioOpenOptions BaseOptions() {
    AsioOpenOptions options;
    options.driver_clsid = "{00000000-0000-0000-0000-000000000001}";
    options.expected_driver_name = "Fake ASIO";
    options.engine_sample_rate = 44100.0;
    options.buffer_frames = 128;
    options.output_channels = {0, 1};
    options.initial_generation = 7;
    return options;
}

std::unique_ptr<FakeAsioDriver> MakeFake(FakeAsioDriver::Config config = {}) {
    return std::make_unique<FakeAsioDriver>(std::move(config));
}

void SleepOneMillisecond() {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
}

} // namespace

int main() {
    // Native packed ASIO ABI layout and hi/lo conversion are validated by
    // tests/asio_native_abi_tests.cpp, not against the host-side logical types.

    // Direct conversion checks: clipping, NaN, rounding, 24-bit packing.
    {
        const float source[4] = {0.25F, -0.5F, 1.5F, -1.5F};
        std::int16_t dest16[4] = {};
        std::uint64_t clipped = 0;
        ConvertPlanarFloatToAsio(AsioSampleFormat::Int16Lsb, source, dest16, 4,
                                 clipped);
        CHECK(dest16[0] == 8192);
        CHECK(dest16[1] == -16384);
        CHECK(dest16[2] == 32767);
        CHECK(dest16[3] == -32767);
        CHECK(clipped == 2);

        std::uint8_t dest24[9] = {};
        const float source24[3] = {-1.0F, 0.5F, 0.0F};
        clipped = 0;
        ConvertPlanarFloatToAsio(AsioSampleFormat::Int24Lsb, source24, dest24, 3,
                                 clipped);
        CHECK(dest24[0] == 0x01 && dest24[1] == 0x00 && dest24[2] == 0x80);
        CHECK(dest24[3] == 0x00 && dest24[4] == 0x00 && dest24[5] == 0x40);
        CHECK(dest24[6] == 0x00 && dest24[7] == 0x00 && dest24[8] == 0x00);
        CHECK(clipped == 0);

        std::int32_t dest32[2] = {};
        const float source32[2] = {0.25F, -0.25F};
        ConvertPlanarFloatToAsio(AsioSampleFormat::Int32Lsb, source32, dest32, 2,
                                 clipped);
        CHECK(dest32[0] == 536870912);
        CHECK(dest32[1] == -536870912);

        float destf[3] = {};
        const float sourcef[3] = {2.0F, -2.0F,
                                  std::numeric_limits<float>::quiet_NaN()};
        clipped = 0;
        ConvertPlanarFloatToAsio(AsioSampleFormat::Float32Lsb, sourcef, destf, 3,
                                 clipped);
        CHECK(destf[0] == 1.0F);
        CHECK(destf[1] == -1.0F);
        CHECK(destf[2] == 0.0F);
        CHECK(clipped == 2);

        double destd[1] = {};
        const float sourced[1] = {0.75F};
        ConvertPlanarFloatToAsio(AsioSampleFormat::Float64Lsb, sourced, destd, 1,
                                 clipped);
        CHECK(destd[0] == 0.75);
    }

    // Open rejects incomplete or ambiguous bindings instead of guessing.
    {
        AsioOutput output;
        AsioOpenOptions options = BaseOptions();
        options.output_channels.clear();
        std::string error;
        CHECK(!output.Open(MakeFake(), options, error));
        CHECK(!output.IsOpen());
        CHECK(!error.empty());
    }
    {
        AsioOutput output;
        AsioOpenOptions options = BaseOptions();
        options.output_channels = {0, 8};
        std::string error;
        CHECK(!output.Open(MakeFake(), options, error));
        CHECK(!error.empty());
    }
    {
        AsioOutput output;
        AsioOpenOptions options = BaseOptions();
        options.output_channels = {0, 0};
        std::string error;
        CHECK(!output.Open(MakeFake(), options, error));
    }
    {
        AsioOutput output;
        AsioOpenOptions options = BaseOptions();
        options.buffer_frames = 48; // below the fake minimum
        std::string error;
        CHECK(!output.Open(MakeFake(), options, error));
        CHECK(!error.empty());
    }
    {
        AsioOutput output;
        AsioOpenOptions options = BaseOptions();
        options.buffer_frames = 100; // within range but not a power of two
        std::string error;
        CHECK(!output.Open(MakeFake(), options, error));
    }
    {
        AsioOutput output;
        AsioOpenOptions options = BaseOptions();
        options.expected_driver_name = "Not The Fake";
        std::string error;
        CHECK(!output.Open(MakeFake(), options, error));
        CHECK(!error.empty());
    }
    {
        AsioOutput output;
        AsioOpenOptions options = BaseOptions();
        options.engine_sample_rate = 48000.0;
        std::string error;
        CHECK(!output.Open(MakeFake(), options, error));
    }
    {
        FakeAsioDriver::Config config;
        config.channel_formats = {AsioSampleFormat::Int16Lsb,
                                  AsioSampleFormat::Float32Lsb};
        AsioOutput output;
        std::string error;
        CHECK(!output.Open(MakeFake(config), BaseOptions(), error));
        CHECK(!error.empty());
    }
    {
        FakeAsioDriver::Config config;
        config.format = AsioSampleFormat::Unsupported;
        AsioOutput output;
        std::string error;
        CHECK(!output.Open(MakeFake(config), BaseOptions(), error));
    }
    {
        FakeAsioDriver::Config config;
        config.fail_latency = true;
        AsioOutput output;
        std::string error;
        CHECK(!output.Open(MakeFake(config), BaseOptions(), error));
        CHECK(!error.empty());
    }
    {
        FakeAsioDriver::Config config;
        config.fail_create = true;
        AsioOutput output;
        std::string error;
        CHECK(!output.Open(MakeFake(config), BaseOptions(), error));
    }

    // Successful open: explicit channels, reported latency, no implicit start.
    auto fake_owner = MakeFake();
    FakeAsioDriver* fake = fake_owner.get();
    AsioOutput output;
    std::string error;
    CHECK(output.Open(std::move(fake_owner), BaseOptions(), error));
    CHECK(output.IsOpen());
    CHECK(!output.IsRunning());
    CHECK(fake->create_count == 1);
    CHECK(fake->start_count == 0);
    CHECK(output.Info().output_latency_frames == 256);
    CHECK(output.Info().output_latency_reported);
    CHECK(output.Info().format == AsioSampleFormat::Float32Lsb);
    CHECK(output.Info().driver_name == "Fake ASIO");
    CHECK(output.Info().channels.size() == 2);
    CHECK(output.Info().channels[0].channel == 0);
    CHECK(output.Info().channels[1].channel == 1);
    CHECK(output.Generation() == 7);

    RenderProbe probe;
    output.SetRenderCallback(&Render, &probe);
    CHECK(output.Start(error));
    CHECK(output.IsRunning());
    CHECK(fake->start_count == 1);

    fake->Fire();
    CHECK(probe.calls == 1);
    CHECK(probe.last_start == 0);
    CHECK(probe.last_generation == 7);
    CHECK(probe.last_frames == 128);
    CHECK(probe.last_sample_position_valid);
    CHECK(fake->Value<float>(0, 0, 0) == 0.25F);
    CHECK(fake->Value<float>(1, 0, 0) == -0.5F);

    fake->Fire();
    CHECK(probe.calls == 2);
    CHECK(probe.last_start == 128);
    CHECK(probe.last_generation == output.Generation());
    CHECK(fake->Value<float>(0, 1, 0) == 0.25F);
    CHECK(fake->Value<float>(1, 1, 0) == -0.5F);
    CHECK(output.Timeline().IsValid());
    CHECK(fake->output_ready_count == 2);
    CHECK(output.TakeEvents().callback_count == 2);
    CHECK(output.TakeEvents().underrun_count == 0);

    // Pause outputs silence without counting an underrun and without calling
    // the render callback.
    const std::uint32_t calls_before_pause = probe.calls;
    const std::uint64_t underruns_before_pause = output.TakeEvents().underrun_count;
    output.SetPaused(true);
    const std::int32_t paused_index = fake->next_index();
    fake->Fire();
    CHECK(probe.calls == calls_before_pause);
    CHECK(fake->Value<float>(0, paused_index, 0) == 0.0F);
    CHECK(output.TakeEvents().underrun_count == underruns_before_pause);
    output.SetPaused(false);

    // A renderer that cannot fill the block yields silence and is visible as an
    // underrun outside the callback.
    probe.fill = false;
    const std::uint64_t underruns_before_failure = output.TakeEvents().underrun_count;
    const std::int32_t failed_index = fake->next_index();
    fake->Fire();
    CHECK(output.TakeEvents().underrun_count == underruns_before_failure + 1);
    CHECK(fake->Value<float>(0, failed_index, 0) == 0.0F);
    probe.fill = true;

    // Legacy bufferSwitch (no timeInfo) still gets a block-aligned position via
    // the driver's callback-safe sample-position query.
    fake->FireLegacy();
    CHECK(probe.last_sample_position_valid);
    CHECK(output.TakeEvents().last_sample_position == probe.last_start);

    // TimeInfo support negotiation is visible to the renderer.
    CHECK(fake->MessageToHost(7) == 1);
    fake->Fire();
    CHECK(probe.last_time_info_mode);

    // A driver reset request is handled outside the callback by Service().
    const std::uint64_t generation_before_reset = output.Generation();
    const std::int32_t starts_before_reset = fake->start_count;
    CHECK(fake->MessageToHost(3) == 1);
    CHECK(output.ResetRequested());
    CHECK(output.Service(error));
    CHECK(output.Generation() == generation_before_reset + 1);
    CHECK(fake->start_count == starts_before_reset + 1);
    CHECK(!output.ResetRequested());
    probe.calls = 0;
    fake->Fire();
    CHECK(probe.calls == 1);
    CHECK(probe.last_generation == generation_before_reset + 1);
    CHECK(probe.last_start == 0);

    // Buffer-size changes must recreate buffers outside the callback.
    const std::int32_t starts_before_size = fake->start_count;
    CHECK(fake->MessageToHost(4, 512) == 1);
    CHECK(output.Service(error));
    CHECK(fake->last_create_frames == 512);
    CHECK(fake->start_count == starts_before_size + 1);
    CHECK(output.Info().buffer_frames == 512);
    probe.calls = 0;
    fake->Fire();
    CHECK(probe.last_frames == 512);
    CHECK(probe.calls == 1);

    // Latency changes are refetched and applied to the audible estimate.
    fake->SetOutputLatency(384);
    CHECK(fake->MessageToHost(6) == 1);
    CHECK(output.Service(error));
    fake->Fire();
    CHECK(output.Timeline().OutputLatencyFrames() == 384);
    CHECK(output.Info().output_latency_frames == 384);

    // Restart advances generation, recreates buffers, resets sample position.
    output.SetPaused(false);
    const std::uint64_t generation_before_restart = output.Generation();
    const std::uint64_t restart_generation = generation_before_restart + 1;
    CHECK(output.Restart(restart_generation, 256, error));
    CHECK(output.Generation() == restart_generation);
    CHECK(fake->last_create_frames == 256);
    CHECK(fake->start_count >= starts_before_size + 2);
    probe.calls = 0;
    fake->Fire();
    CHECK(probe.calls == 1);
    CHECK(probe.last_generation == restart_generation);
    CHECK(probe.last_start == 0);
    CHECK(probe.last_frames == 256);
    CHECK(!output.Restart(restart_generation, 256, error));
    CHECK(!error.empty());
    CHECK(output.Restart(restart_generation + 1, 64, error));
    CHECK(output.Generation() == restart_generation + 1);
    CHECK(fake->last_create_frames == 64);

    // A sample-rate change cannot be applied to a fixed-rate engine silently.
    fake->FireSampleRate(48000.0);
    CHECK(!output.Service(error));
    CHECK(!error.empty());

    // Callback allocation freedom, including conversion and timeline update.
    SleepOneMillisecond();
    fake->Fire();
    test::BeginAllocationProbe();
    fake->Fire();
    const long long callback_allocations = test::EndAllocationProbe();
    CHECK(callback_allocations == 0);

    // ASIOStart resets samplePosition to zero; a stop/start must therefore
    // advance the generation even without restarting the driver buffers.
    const std::uint64_t generation_before_stop = output.Generation();
    output.Stop();
    CHECK(!output.IsRunning());
    CHECK(output.Start(error));
    CHECK(output.Generation() == generation_before_stop + 1);
    probe.calls = 0;
    SleepOneMillisecond();
    fake->Fire();
    CHECK(probe.calls == 1);
    CHECK(probe.last_generation == generation_before_stop + 1);
    CHECK(probe.last_start == 0);

    output.SetPaused(true);
    output.Stop();
    CHECK(!output.IsRunning());
    output.Close();
    CHECK(!output.IsOpen());
    CHECK(fake->shutdown_count == 1);

    // Int16 output conversion through the whole callback path.
    {
        FakeAsioDriver::Config config;
        config.format = AsioSampleFormat::Int16Lsb;
        auto owner = MakeFake(config);
        FakeAsioDriver* int16 = owner.get();
        AsioOutput int16_output;
        std::string int16_error;
        CHECK(int16_output.Open(std::move(owner), BaseOptions(), int16_error));
        RenderProbe int16_probe;
        int16_output.SetRenderCallback(&Render, &int16_probe);
        CHECK(int16_output.Start(int16_error));
        int16->Fire();
        CHECK(int16->Value<std::int16_t>(0, 0, 0) == 8192);
        CHECK(int16->Value<std::int16_t>(1, 0, 0) == -16384);
        int16_output.Close();
    }

    // Explicit channel order is honored: mix channel 0 goes to ASIO output 1.
    {
        auto owner = MakeFake();
        FakeAsioDriver* reversed = owner.get();
        AsioOutput reversed_output;
        AsioOpenOptions options = BaseOptions();
        options.output_channels = {1, 0};
        std::string reversed_error;
        CHECK(reversed_output.Open(std::move(owner), options, reversed_error));
        CHECK(reversed_output.Info().channels[0].channel == 1);
        CHECK(reversed_output.Info().channels[1].channel == 0);
        RenderProbe reversed_probe;
        reversed_output.SetRenderCallback(&Render, &reversed_probe);
        CHECK(reversed_output.Start(reversed_error));
        reversed->Fire();
        CHECK(reversed->Value<float>(0, 0, 0) == 0.25F);
        CHECK(reversed->Value<float>(1, 0, 0) == -0.5F);
        reversed_output.Close();
    }

    // A 48 kHz engine/device pair is negotiated explicitly; no resampling is
    // silently inserted between the mix and the device.
    {
        FakeAsioDriver::Config config;
        config.sample_rate = 48000.0;
        auto owner = MakeFake(config);
        FakeAsioDriver* rate48 = owner.get();
        AsioOutput output48;
        AsioOpenOptions options = BaseOptions();
        options.engine_sample_rate = 48000.0;
        std::string rate_error;
        CHECK(output48.Open(std::move(owner), options, rate_error));
        CHECK(output48.Info().sample_rate == 48000.0);
        CHECK(output48.Timeline().SampleRate() == 48000U);
        RenderProbe rate_probe;
        output48.SetRenderCallback(&Render, &rate_probe);
        CHECK(output48.Start(rate_error));
        rate48->Fire();
        CHECK(rate_probe.calls == 1);
        CHECK(rate_probe.last_start == 0);
        output48.Close();
    }

    // Start/CreateBuffers failure after a previous good open must not leave the
    // object claiming to run, and a reopen must advance the lifetime floor.
    {
        auto owner = MakeFake();
        FakeAsioDriver* failing = owner.get();
        AsioOutput failing_output;
        std::string failing_error;
        CHECK(failing_output.Open(std::move(owner), BaseOptions(), failing_error));
        failing->SetFailStart(true);
        CHECK(!failing_output.Start(failing_error));
        CHECK(!failing_output.IsRunning());
        CHECK(failing_output.Generation() == 7);
        failing->SetFailStart(false);
        CHECK(failing_output.Start(failing_error));
        CHECK(failing_output.IsRunning());
        CHECK(failing_output.Generation() == 7);
        failing->SetFailCreate(true);
        CHECK(!failing_output.Restart(8, 128, failing_error));
        CHECK(!failing_output.IsRunning());
        CHECK(failing_output.Generation() == 7);
        failing_output.Close();

        auto retry_owner = MakeFake();
        CHECK(failing_output.Open(std::move(retry_owner), BaseOptions(), failing_error));
        CHECK(failing_output.Generation() == 8);
        CHECK(failing_output.Start(failing_error));
        CHECK(failing_output.IsRunning());
        failing_output.Close();
    }

    // A driver may invoke bufferSwitch synchronously inside ASIOStart, and may
    // do so from another thread before Start returns. Both first blocks must be
    // rendered with the prepared generation, not dropped by !running.
    {
        FakeAsioDriver::Config config;
        config.callback_inside_start = true;
        auto owner = MakeFake(config);
        FakeAsioDriver* inline_driver = owner.get();
        AsioOutput inline_output;
        std::string inline_error;
        CHECK(inline_output.Open(std::move(owner), BaseOptions(), inline_error));
        RenderProbe inline_probe;
        inline_output.SetRenderCallback(&Render, &inline_probe);
        CHECK(inline_output.Start(inline_error));
        CHECK(inline_probe.calls == 1);
        CHECK(inline_probe.last_start == 0);
        CHECK(inline_probe.last_generation == 7);
        CHECK(inline_driver->Value<float>(0, 0, 0) == 0.25F);
        const std::uint64_t inline_generation = inline_output.Generation();
        CHECK(inline_output.Restart(inline_generation + 1, 128, inline_error));
        CHECK(inline_probe.calls == 2);
        CHECK(inline_probe.last_start == 0);
        CHECK(inline_probe.last_generation == inline_generation + 1);
        inline_output.Close();
    }
    {
        FakeAsioDriver::Config config;
        config.callback_thread_inside_start = true;
        auto owner = MakeFake(config);
        FakeAsioDriver* thread_driver = owner.get();
        AsioOutput thread_output;
        std::string thread_error;
        CHECK(thread_output.Open(std::move(owner), BaseOptions(), thread_error));
        RenderProbe thread_probe;
        thread_output.SetRenderCallback(&Render, &thread_probe);
        CHECK(thread_output.Start(thread_error));
        CHECK(thread_probe.calls == 1);
        CHECK(thread_probe.last_start == 0);
        CHECK(thread_probe.last_generation == 7);
        CHECK(thread_driver->Value<float>(0, 0, 0) == 0.25F);
        thread_output.Close();
    }

    // A driver-observed sample-position jump advances the published generation;
    // keysound events from the old device timeline must be rejected.
    {
        auto owner = MakeFake();
        FakeAsioDriver* jumped = owner.get();
        AsioOutput jump_output;
        std::string jump_error;
        CHECK(jump_output.Open(std::move(owner), BaseOptions(), jump_error));
        RenderProbe jump_probe;
        jump_output.SetRenderCallback(&Render, &jump_probe);
        CHECK(jump_output.Start(jump_error));
        jumped->Fire(true, 0);
        CHECK(jump_output.Generation() == 7);
        jumped->Fire(true, 10'000);
        CHECK(jump_output.Timeline().Generation() == 8);
        CHECK(jump_output.Generation() == 8);
        CHECK(jump_probe.last_generation == 8);
        jump_output.Close();
    }

    // A driver position that cannot be represented plus one buffer is rejected,
    // not wrapped; the block is rendered as silence-position-unknown.
    {
        auto owner = MakeFake();
        FakeAsioDriver* invalid_driver = owner.get();
        AsioOutput invalid_output;
        std::string invalid_error;
        CHECK(invalid_output.Open(std::move(owner), BaseOptions(), invalid_error));
        RenderProbe invalid_probe;
        invalid_output.SetRenderCallback(&Render, &invalid_probe);
        CHECK(invalid_output.Start(invalid_error));
        invalid_driver->Fire(true, 0);
        CHECK(invalid_probe.last_sample_position_valid);
        invalid_driver->Fire(true, std::numeric_limits<std::int64_t>::max());
        CHECK(!invalid_probe.last_sample_position_valid);
        CHECK(invalid_probe.last_start == 128);
        invalid_output.Close();
    }

    // Close/Open preserves a lifetime generation floor: a reopen with the same
    // configured generation must advance instead of reusing it, and the
    // timeline floor must not reject the new value.
    {
        AsioOutput reopened;
        std::string reopen_error;
        auto first_owner = MakeFake();
        FakeAsioDriver* first_driver = first_owner.get();
        CHECK(reopened.Open(std::move(first_owner), BaseOptions(), reopen_error));
        CHECK(reopened.Generation() == 7);
        RenderProbe first_probe;
        reopened.SetRenderCallback(&Render, &first_probe);
        CHECK(reopened.Start(reopen_error));
        first_driver->Fire(true, 0);
        CHECK(first_probe.last_generation == 7);
        reopened.Stop();
        reopened.Close();
        CHECK(!reopened.IsOpen());
        CHECK(reopened.Generation() == 7);

        auto second_owner = MakeFake();
        FakeAsioDriver* second_driver = second_owner.get();
        CHECK(reopened.Open(std::move(second_owner), BaseOptions(), reopen_error));
        CHECK(reopened.Generation() == 8);
        RenderProbe second_probe;
        reopened.SetRenderCallback(&Render, &second_probe);
        CHECK(reopened.Start(reopen_error));
        second_driver->Fire(true, 0);
        CHECK(second_probe.last_generation == 8);
        CHECK(reopened.Timeline().IsValid());
        reopened.Close();
    }

    return test::Finish("asio_output");
}
