#include "nosfix/asio_audio_host.hpp"

#include "fake_asio_driver.hpp"
#include "test_support.hpp"

#include <cmath>
#include <cstring>
#include <memory>
#include <string>

// Backend tests: the fake ASIO driver delivers real callback blocks, the host
// renders the bridge mixer on the device sample timeline, and keysounds are
// dispatched with a sample-accurate offset. No real device is opened.

using namespace nosfix::audio;

namespace {

template <typename Fn>
Fn SlotOf(void* object, std::uint32_t byte_offset) {
    auto* vtable = *reinterpret_cast<void* const**>(object);
    return reinterpret_cast<Fn>(const_cast<void*>(vtable[byte_offset / 8]));
}

using CreateSourceVoiceFn = std::int32_t (*)(void*, void**, const Xa2WaveFormat*,
                                             std::uint32_t, float, void*,
                                             const Xa2VoiceSends*,
                                             const Xa2EffectChain*);
using CreateMasteringVoiceFn = std::int32_t (*)(void*, void**, std::uint32_t,
                                                std::uint32_t, std::uint32_t,
                                                std::uint32_t,
                                                const Xa2EffectChain*);
using SubmitSourceBufferFn = std::int32_t (*)(void*, const Xa2Buffer*, const void*);
using StartFn = std::int32_t (*)(void*, std::uint32_t, std::uint32_t);

AsioHostConfig MakeConfig(std::int32_t buffer_frames = 128) {
    AsioHostConfig config;
    config.output.driver_clsid = "{5AF0A297-63D3-43D5-BCAD-838C1494D3B7}";
    config.output.engine_sample_rate = 44100.0;
    config.output.buffer_frames = buffer_frames;
    config.output.output_channels = {0, 1};
    config.output.initial_generation = 1;
    config.bridge.device_rate = 44100;
    config.bridge.device_channels = 2;
    config.bridge.device_name = "fake";
    config.mixer_max_block_frames = 1024;
    config.keysound_capacity = 256;
    return config;
}

std::unique_ptr<FakeAsioDriver> MakeDriver(std::int32_t outputs = 8,
                                           std::int32_t buffer_latency = 256) {
    FakeAsioDriver::Config driver_config;
    driver_config.outputs = outputs;
    driver_config.inputs = 0;
    driver_config.sample_rate = 44100.0;
    driver_config.range = {64, 1024, 256, -1};
    driver_config.format = AsioSampleFormat::Float32Lsb;
    driver_config.output_latency = buffer_latency;
    driver_config.name = "Fake ASIO";
    return std::make_unique<FakeAsioDriver>(driver_config);
}

Xa2WaveFormat FloatFormat(std::uint32_t rate, std::uint16_t channels) {
    Xa2WaveFormat format;
    format.format_tag = kWaveFormatIeeeFloat;
    format.channels = channels;
    format.samples_per_sec = rate;
    format.bits_per_sample = 32;
    format.block_align = static_cast<std::uint16_t>(channels * 4U);
    format.avg_bytes_per_sec = rate * channels * 4U;
    return format;
}

// Deterministic mapper for offset tests: qpc N maps to sample N.
bool LinearMapper(void* context, std::int64_t qpc, std::int64_t& sample) noexcept {
    (void)context;
    sample = qpc;
    return true;
}

void CreateMastering(Xa2Engine* engine, void** master) {
    CHECK(SlotOf<CreateMasteringVoiceFn>(engine, xa27::kEngineCreateMasteringVoice)(
              engine, master, 2, 44100, 0, 0, nullptr) == kXa2Ok);
}

void* CreateSource(Xa2Engine* engine, const Xa2WaveFormat& format) {
    void* voice = nullptr;
    SlotOf<CreateSourceVoiceFn>(engine, xa27::kEngineCreateSourceVoice)(
        engine, &voice, &format, 0, 2.0F, nullptr, nullptr, nullptr);
    return voice;
}

void SubmitLooping(Xa2Engine* engine, void* voice, const float* samples,
                   std::uint32_t count) {
    (void)engine; // the voice already carries its engine pointer
    Xa2Buffer buffer;
    buffer.audio_bytes = count * 4U;
    buffer.audio_data = reinterpret_cast<const std::uint8_t*>(samples);
    buffer.loop_count = kXa2LoopInfinite;
    CHECK(SlotOf<SubmitSourceBufferFn>(voice, xa27::kVoiceSubmitSourceBuffer)(
              voice, &buffer, nullptr) == kXa2Ok);
    CHECK(SlotOf<StartFn>(voice, xa27::kVoiceStart)(voice, 0, 0) == kXa2Ok);
}

// ---------------------------------------------------------------------------

void TestOpenRejectsMismatchedConfiguration() {
    std::string error;

    {
        AsioHostConfig config = MakeConfig();
        config.bridge.device_rate = 48000;
        AsioAudioHost host;
        CHECK(!host.OpenWithDriver(MakeDriver(), config, error));
        CHECK(!error.empty());
        CHECK(host.Engine() == nullptr);
    }
    {
        AsioHostConfig config = MakeConfig();
        config.bridge.device_channels = 6;
        AsioAudioHost host;
        CHECK(!host.OpenWithDriver(MakeDriver(), config, error));
    }
    {
        AsioHostConfig config = MakeConfig();
        config.output.output_channels.clear();
        AsioAudioHost host;
        CHECK(!host.OpenWithDriver(MakeDriver(), config, error));
    }
    {
        AsioHostConfig config = MakeConfig();
        config.output.driver_clsid.clear();
        AsioAudioHost host;
        CHECK(!host.OpenWithDriver(MakeDriver(), config, error));
    }
    {
        AsioHostConfig config = MakeConfig();
        config.output.buffer_frames = 0;
        AsioAudioHost host;
        CHECK(!host.OpenWithDriver(MakeDriver(), config, error));
    }
    {
        AsioHostConfig config = MakeConfig();
        config.keysound_capacity = 100; // not a power of two
        AsioAudioHost host;
        CHECK(!host.OpenWithDriver(MakeDriver(), config, error));
    }
    {
        AsioHostConfig config = MakeConfig();
        config.mixer_max_block_frames = 64; // smaller than the ASIO buffer
        AsioAudioHost host;
        CHECK(!host.OpenWithDriver(MakeDriver(), config, error));
    }
    {
        AsioHostConfig config = MakeConfig();
        config.initial_sequence = 0;
        AsioAudioHost host;
        CHECK(!host.OpenWithDriver(MakeDriver(), config, error));
    }
    {
        // The driver reports a different rate than requested.
        FakeAsioDriver::Config driver_config;
        driver_config.sample_rate = 48000.0;
        auto other = std::make_unique<FakeAsioDriver>(driver_config);
        AsioHostConfig config = MakeConfig();
        AsioAudioHost host;
        CHECK(!host.OpenWithDriver(std::move(other), config, error));
        CHECK(!error.empty());
    }
    {
        // The control case: the same config with a conforming driver opens.
        AsioHostConfig config = MakeConfig();
        AsioAudioHost host;
        CHECK(host.OpenWithDriver(MakeDriver(), config, error));
    }
}

void TestOpenAcceptsExplicitDevice() {
    std::string error;
    AsioAudioHost host;
    auto driver = MakeDriver();
    FakeAsioDriver* raw = driver.get();
    AsioHostConfig config = MakeConfig();
    CHECK(host.OpenWithDriver(std::move(driver), config, error));
    CHECK(host.Engine() != nullptr);
    CHECK(host.Keysounds() != nullptr);
    CHECK(host.Keysounds()->Capacity() == 256);
    CHECK(raw->create_count == 1);
    CHECK(raw->last_create_frames == 128);
    CHECK(raw->last_create_channels == 2);

    const AsioOutputInfo info = host.Output().Info();
    CHECK(info.buffer_frames == 128);
    CHECK(std::fabs(info.sample_rate - 44100.0) < 1e-9);
    CHECK(info.output_latency_reported);
    CHECK(info.output_latency_frames == 256);

    CHECK(host.Start(error));
    CHECK(host.Output().IsRunning());
    CHECK(host.Generation() >= 1);
    host.Stop();
    host.Close();
    CHECK(host.Engine() == nullptr);
}

void TestOpenFailsWithoutLatencyOrWithBadStart() {
    std::string error;
    {
        FakeAsioDriver::Config driver_config;
        driver_config.fail_latency = true;
        auto failing = std::make_unique<FakeAsioDriver>(driver_config);
        AsioHostConfig config = MakeConfig();
        AsioAudioHost host;
        CHECK(!host.OpenWithDriver(std::move(failing), config, error));
        CHECK(!error.empty());
    }
    {
        FakeAsioDriver::Config driver_config;
        driver_config.fail_start = true;
        auto failing = std::make_unique<FakeAsioDriver>(driver_config);
        AsioHostConfig config = MakeConfig();
        AsioAudioHost host;
        CHECK(host.OpenWithDriver(std::move(failing), config, error));
        CHECK(!host.Start(error));
        CHECK(!host.Output().IsRunning());
        CHECK(!host.Stats().blocks);
    }
    {
        // A CLSID that is not registered anywhere on this machine: CreateSystem
        // AsioDriver fails before any driver method is called, so no device is
        // opened and no default device is substituted.
        AsioHostConfig config = MakeConfig();
        config.output.driver_clsid = "{DEADBEEF-0000-0000-0000-000000000000}";
        AsioAudioHost host;
        CHECK(!host.Open(config, error));
        CHECK(!error.empty());
        CHECK(host.Engine() == nullptr);
    }
}

void TestRenderReachesTheDriverBuffers() {
    std::string error;
    AsioAudioHost host;
    auto driver = MakeDriver();
    FakeAsioDriver* raw = driver.get();
    CHECK(host.OpenWithDriver(std::move(driver), MakeConfig(), error));
    CHECK(host.Start(error));

    void* master = nullptr;
    CreateMastering(host.Engine(), &master);
    void* voice = CreateSource(host.Engine(), FloatFormat(44100, 1));
    CHECK(voice != nullptr);
    const float samples[4] = {0.25F, -0.25F, 0.5F, -0.5F};
    SubmitLooping(host.Engine(), voice, samples, 4);

    const std::int32_t index = raw->next_index();
    raw->Fire(true, 0);
    float expected[4] = {0.25F, -0.25F, 0.5F, -0.5F};
    for (int frame = 0; frame < 128; ++frame) {
        const float value = raw->Value<float>(0, index, frame);
        CHECK(std::fabs(value - expected[frame % 4]) < 1e-6F);
    }
    const AsioHostStats stats = host.Stats();
    CHECK(stats.blocks == 1);
    CHECK(stats.silent_blocks == 0);
    CHECK(stats.voice_starts == 0); // no keysound was dispatched
    CHECK(stats.last_block_frames == 128);
    CHECK(stats.last_generation >= 1);

    // Second block continues on the device timeline.
    const std::int32_t index2 = raw->next_index();
    raw->Fire(true, 128);
    CHECK(std::fabs(raw->Value<float>(0, index2, 0) - expected[0]) < 1e-6F);
    CHECK(host.Stats().blocks == 2);
}

void TestKeysoundOffsetWithDeterministicMapper() {
    std::string error;
    AsioAudioHost host;
    auto driver = MakeDriver();
    FakeAsioDriver* raw = driver.get();
    CHECK(host.OpenWithDriver(std::move(driver), MakeConfig(), error));
    CHECK(host.Start(error));
    host.SetKeysoundMapper(&LinearMapper, nullptr);

    void* master = nullptr;
    CreateMastering(host.Engine(), &master);
    void* voice = CreateSource(host.Engine(), FloatFormat(44100, 1));
    CHECK(voice != nullptr);
    const float samples[1] = {0.75F};
    SubmitLooping(host.Engine(), voice, samples, 1);
    CHECK(host.BindCueVoice(11, Xa2VoiceFromInterface(voice)));

    // Sample 37 of the block starting at 1000.
    KeysoundRequest request;
    request.generation = host.Generation();
    request.capture_qpc = 1037;
    request.due_qpc = 1037;
    request.cue_id = 11;
    CHECK(host.PushKeysound(request).accepted());
    CHECK(host.NextSequence() == 2);

    const std::int32_t index = raw->next_index();
    raw->Fire(true, 1000);
    const int offset = 37;
    for (int frame = 0; frame < offset; ++frame) {
        CHECK(std::fabs(raw->Value<float>(0, index, frame)) < 1e-9F);
    }
    CHECK(std::fabs(raw->Value<float>(0, index, offset) - 0.75F) < 1e-6F);
    CHECK(std::fabs(raw->Value<float>(0, index, offset + 1) - 0.75F) < 1e-6F);
    const AsioHostStats stats = host.Stats();
    CHECK(stats.keysounds_dispatched == 1);
    CHECK(stats.keysounds_late == 0);
    CHECK(stats.voice_starts == 1);
    CHECK(host.Keysounds()->SnapshotStats().dispatched == 1);
}

void TestLateKeysoundStartsAtBlockStart() {
    std::string error;
    AsioAudioHost host;
    auto driver = MakeDriver();
    FakeAsioDriver* raw = driver.get();
    CHECK(host.OpenWithDriver(std::move(driver), MakeConfig(), error));
    CHECK(host.Start(error));
    host.SetKeysoundMapper(&LinearMapper, nullptr);
    void* master = nullptr;
    CreateMastering(host.Engine(), &master);
    void* voice = CreateSource(host.Engine(), FloatFormat(44100, 1));
    const float samples[1] = {0.5F};
    SubmitLooping(host.Engine(), voice, samples, 1);
    CHECK(host.BindCueVoice(3, Xa2VoiceFromInterface(voice)));

    KeysoundRequest request;
    request.generation = host.Generation();
    request.capture_qpc = 100; // far before the block at 2000
    request.due_qpc = 100;
    request.cue_id = 3;
    CHECK(host.PushKeysound(request).accepted());

    const std::int32_t index = raw->next_index();
    raw->Fire(true, 2000);
    CHECK(std::fabs(raw->Value<float>(0, index, 0) - 0.5F) < 1e-6F);
    const AsioHostStats stats = host.Stats();
    CHECK(stats.keysounds_late == 1);
    CHECK(stats.keysounds_dispatched == 1);
}

void TestUnboundCueIsRefused() {
    std::string error;
    AsioAudioHost host;
    auto driver = MakeDriver();
    FakeAsioDriver* raw = driver.get();
    CHECK(host.OpenWithDriver(std::move(driver), MakeConfig(), error));
    CHECK(host.Start(error));
    host.SetKeysoundMapper(&LinearMapper, nullptr);
    void* master = nullptr;
    CreateMastering(host.Engine(), &master);

    KeysoundRequest request;
    request.generation = host.Generation();
    request.capture_qpc = 10;
    request.cue_id = 99; // nothing bound
    CHECK(host.PushKeysound(request).accepted());
    const std::int32_t index = raw->next_index();
    raw->Fire(true, 0);
    for (int frame = 0; frame < 16; ++frame) {
        CHECK(std::fabs(raw->Value<float>(0, index, frame)) < 1e-9F);
    }
    CHECK(host.Stats().keysounds_sink_rejected == 1);
    CHECK(host.Stats().unmapped_dispatches == 1);
    CHECK(host.Keysounds()->SnapshotStats().sink_rejected == 1);
}

void TestStaleGenerationIsDropped() {
    std::string error;
    AsioAudioHost host;
    auto driver = MakeDriver();
    FakeAsioDriver* raw = driver.get();
    CHECK(host.OpenWithDriver(std::move(driver), MakeConfig(), error));
    CHECK(host.Start(error));
    host.SetKeysoundMapper(&LinearMapper, nullptr);
    void* master = nullptr;
    CreateMastering(host.Engine(), &master);
    void* voice = CreateSource(host.Engine(), FloatFormat(44100, 1));
    const float samples[1] = {1.0F};
    // The voice is bound but never started: only the sink could start it, so a
    // silent block proves the stale event never reached the sink.
    Xa2Buffer buffer;
    buffer.audio_bytes = sizeof(samples);
    buffer.audio_data = reinterpret_cast<const std::uint8_t*>(samples);
    buffer.loop_count = kXa2LoopInfinite;
    CHECK(SlotOf<SubmitSourceBufferFn>(voice, xa27::kVoiceSubmitSourceBuffer)(
              voice, &buffer, nullptr) == kXa2Ok);
    CHECK(host.BindCueVoice(5, Xa2VoiceFromInterface(voice)));

    KeysoundRequest request;
    request.generation = host.Generation() + 100; // future generation
    request.capture_qpc = 10;
    request.cue_id = 5;
    CHECK(host.PushKeysound(request).accepted());
    const std::int32_t index = raw->next_index();
    raw->Fire(true, 0);
    CHECK(host.Stats().keysounds_stale == 1);
    CHECK(std::fabs(raw->Value<float>(0, index, 0)) < 1e-9F);
}

void TestGenerationChangeStopsVoices() {
    std::string error;
    AsioAudioHost host;
    auto driver = MakeDriver();
    FakeAsioDriver* raw = driver.get();
    CHECK(host.OpenWithDriver(std::move(driver), MakeConfig(), error));
    CHECK(host.Start(error));
    void* master = nullptr;
    CreateMastering(host.Engine(), &master);
    void* voice = CreateSource(host.Engine(), FloatFormat(44100, 1));
    const float samples[2] = {1.0F, 1.0F};
    SubmitLooping(host.Engine(), voice, samples, 2);

    const std::int32_t first = raw->next_index();
    raw->Fire(true, 0);
    CHECK(std::fabs(raw->Value<float>(0, first, 0) - 1.0F) < 1e-6F);

    const std::uint64_t before = host.Generation();
    CHECK(host.Restart(before + 1, 128, error)); // Restart starts the driver
    CHECK(host.Generation() > before);
    CHECK(host.Output().IsRunning());

    const std::int32_t second = raw->next_index();
    raw->Fire(true, 0);
    for (int frame = 0; frame < 32; ++frame) {
        CHECK(std::fabs(raw->Value<float>(0, second, frame)) < 1e-9F);
    }
    CHECK(host.Stats().generation_changes >= 1);
}

void TestBufferSizeChange() {
    std::string error;
    AsioAudioHost host;
    auto driver = MakeDriver();
    FakeAsioDriver* raw = driver.get();
    CHECK(host.OpenWithDriver(std::move(driver), MakeConfig(), error));
    CHECK(host.Start(error));
    void* master = nullptr;
    CreateMastering(host.Engine(), &master);
    raw->Fire(true, 0);
    CHECK(host.Stats().last_block_frames == 128);

    CHECK(host.Restart(host.Generation() + 1, 256, error));
    CHECK(raw->last_create_frames == 256);
    const std::int32_t index = raw->next_index();
    raw->Fire(true, 0);
    CHECK(host.Stats().last_block_frames == 256);
    CHECK(host.Stats().buffer_size_changes >= 1);
    (void)index;
}

void TestRestartBeyondMixerBlockIsRefused() {
    std::string error;
    AsioAudioHost host;
    CHECK(host.OpenWithDriver(MakeDriver(), MakeConfig(), error));
    CHECK(host.Start(error));
    CHECK(!host.Restart(host.Generation() + 1, 2048, error));
    CHECK(!error.empty());
}

void TestPauseAndSilentBlocks() {
    std::string error;
    AsioAudioHost host;
    auto driver = MakeDriver();
    FakeAsioDriver* raw = driver.get();
    CHECK(host.OpenWithDriver(std::move(driver), MakeConfig(), error));
    CHECK(host.Start(error));
    void* master = nullptr;
    CreateMastering(host.Engine(), &master);
    void* voice = CreateSource(host.Engine(), FloatFormat(44100, 1));
    const float samples[1] = {1.0F};
    SubmitLooping(host.Engine(), voice, samples, 1);

    host.Output().SetPaused(true);
    const std::int32_t index = raw->next_index();
    raw->Fire(true, 0);
    CHECK(std::fabs(raw->Value<float>(0, index, 0)) < 1e-9F);
    CHECK(host.Stats().blocks == 0); // the callback is not invoked while paused

    host.Output().SetPaused(false);
    raw->Fire(true, 128);
    CHECK(std::fabs(raw->Value<float>(0, raw->next_index() ^ 1, 0) - 1.0F) < 1e-6F);

    // A block without a valid driver position is silence and is counted.
    AsioRenderContext invalid;
    invalid.generation = host.Generation();
    invalid.block_start_sample = 0;
    invalid.buffer_frames = 128;
    invalid.sample_position_valid = false;
    float left[128] = {};
    float right[128] = {};
    float* channels[2] = {left, right};
    CHECK(!host.RenderForTest(invalid, channels));
    CHECK(host.Stats().silent_blocks == 1);

    // A block larger than the mixer scratch is refused.
    AsioRenderContext oversized = invalid;
    oversized.sample_position_valid = true;
    oversized.buffer_frames = 4096;
    CHECK(!host.RenderForTest(oversized, channels));
    CHECK(host.Stats().render_failures == 1);
}

void TestTimelineMapperIsUsedByDefault() {
    std::string error;
    AsioAudioHost host;
    auto driver = MakeDriver();
    FakeAsioDriver* raw = driver.get();
    CHECK(host.OpenWithDriver(std::move(driver), MakeConfig(), error));
    CHECK(host.Start(error));
    void* master = nullptr;
    CreateMastering(host.Engine(), &master);
    void* voice = CreateSource(host.Engine(), FloatFormat(44100, 1));
    const float samples[1] = {1.0F};
    SubmitLooping(host.Engine(), voice, samples, 1);
    CHECK(host.BindCueVoice(21, Xa2VoiceFromInterface(voice)));

    raw->Fire(true, 0); // anchors the timeline
    const auto target_qpc = host.Output().Timeline().MapRenderSampleToQpc(128 + 37);
    CHECK(target_qpc.has_value());
    KeysoundRequest request;
    request.generation = host.Generation();
    request.capture_qpc = *target_qpc;
    request.due_qpc = *target_qpc;
    request.cue_id = 21;
    CHECK(host.PushKeysound(request).accepted());

    const std::int32_t index = raw->next_index();
    raw->Fire(true, 128);
    int first_audible = -1;
    for (int frame = 0; frame < 128; ++frame) {
        if (std::fabs(raw->Value<float>(0, index, frame)) > 0.5F) {
            first_audible = frame;
            break;
        }
    }
    CHECK(host.Stats().keysounds_dispatched == 1);
    CHECK(host.Stats().keysounds_stale == 0);
    CHECK(first_audible >= 0);
    // The bounded drift correction may move the anchor by a couple of frames.
    CHECK(first_audible >= 35);
    CHECK(first_audible <= 39);
}

void TestPushFillsSequenceAndValidates() {
    std::string error;
    AsioAudioHost host;
    CHECK(host.OpenWithDriver(MakeDriver(), MakeConfig(), error));
    KeysoundRequest request;
    request.generation = 1;
    request.capture_qpc = 1;
    request.cue_id = 1;
    const KeysoundPushResult result = host.PushKeysound(request);
    CHECK(result.accepted());
    const auto stats = host.Keysounds()->SnapshotStats();
    CHECK(stats.pushed == 1);
    CHECK(stats.pending == 1);

    KeysoundRequest invalid;
    invalid.generation = 0; // invalid
    invalid.capture_qpc = 1;
    CHECK(host.PushKeysound(invalid).status == KeysoundPushStatus::InvalidArgument);
    KeysoundQueue* queue = host.Keysounds();
    CHECK(queue != nullptr);
}

void TestBindings() {
    std::string error;
    AsioAudioHost host;
    CHECK(host.OpenWithDriver(MakeDriver(), MakeConfig(), error));
    void* master = nullptr;
    CreateMastering(host.Engine(), &master);
    void* voice = CreateSource(host.Engine(), FloatFormat(44100, 1));
    CHECK(host.BindCueVoice(7, Xa2VoiceFromInterface(voice)));
    CHECK(host.BoundVoice(7) == Xa2VoiceFromInterface(voice));
    CHECK(host.BoundVoice(0) == nullptr);
    CHECK(host.BindCueVoice(7, nullptr));
    CHECK(host.BoundVoice(7) == nullptr);
    CHECK(host.BindCueVoice(0, Xa2VoiceFromInterface(voice)));
    CHECK(host.BoundVoice(0) == nullptr);
}

} // namespace

int main() {
    TestOpenRejectsMismatchedConfiguration();
    TestOpenAcceptsExplicitDevice();
    TestOpenFailsWithoutLatencyOrWithBadStart();
    TestRenderReachesTheDriverBuffers();
    TestKeysoundOffsetWithDeterministicMapper();
    TestLateKeysoundStartsAtBlockStart();
    TestUnboundCueIsRefused();
    TestStaleGenerationIsDropped();
    TestGenerationChangeStopsVoices();
    TestBufferSizeChange();
    TestRestartBeyondMixerBlockIsRefused();
    TestPauseAndSilentBlocks();
    TestTimelineMapperIsUsedByDefault();
    TestPushFillsSequenceAndValidates();
    TestBindings();
    return test::Finish("asio_backend");
}
