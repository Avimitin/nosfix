#include "nosfix/xaudio2_bridge.hpp"

#include "test_support.hpp"

#include <cmath>
#include <cstring>
#include <vector>

// These tests behave like a compiled XAudio2 2.7 client: they never call a
// C++ method, they index the object's vtable at the byte offsets declared in
// nosfix/xaudio2_bridge.hpp. A layout change breaks a test, not a game.

using namespace nosfix::audio;

namespace {

template <typename Fn>
Fn SlotOf(void* object, std::uint32_t byte_offset) {
    auto* vtable = *reinterpret_cast<void* const**>(object);
    return reinterpret_cast<Fn>(const_cast<void*>(vtable[byte_offset / 8]));
}

using QueryInterfaceFn = std::int32_t (*)(void*, const void*, void**);
using AddRefFn = std::uint32_t (*)(void*);
using ReleaseFn = std::uint32_t (*)(void*);
using GetDeviceCountFn = std::int32_t (*)(void*, std::uint32_t*);
using GetDeviceDetailsFn = std::int32_t (*)(void*, std::uint32_t, Xa2DeviceDetails*);
using InitializeFn = std::int32_t (*)(void*, std::uint32_t, std::uint32_t);
using RegisterForCallbacksFn = std::int32_t (*)(void*, void*);
using UnregisterForCallbacksFn = void (*)(void*, void*);
using CreateSourceVoiceFn = std::int32_t (*)(void*, void**, const Xa2WaveFormat*,
                                             std::uint32_t, float, void*,
                                             const Xa2VoiceSends*,
                                             const Xa2EffectChain*);
using CreateSubmixVoiceFn = std::int32_t (*)(void*, void**, std::uint32_t,
                                             std::uint32_t, std::uint32_t,
                                             std::uint32_t, const Xa2VoiceSends*,
                                             const Xa2EffectChain*);
using CreateMasteringVoiceFn = std::int32_t (*)(void*, void**, std::uint32_t,
                                                std::uint32_t, std::uint32_t,
                                                std::uint32_t,
                                                const Xa2EffectChain*);
using StartEngineFn = std::int32_t (*)(void*);
using StopEngineFn = void (*)(void*);
using CommitChangesFn = std::int32_t (*)(void*, std::uint32_t);
using SetDebugConfigurationFn = void (*)(void*, const void*, void*);
using GenericFn = std::int32_t (*)(void*);

using GetVoiceDetailsFn = std::int32_t (*)(void*, Xa2VoiceDetails*);
using SetOutputVoicesFn = std::int32_t (*)(void*, const Xa2VoiceSends*);
using SetEffectChainFn = std::int32_t (*)(void*, const Xa2EffectChain*);
using EnableEffectFn = std::int32_t (*)(void*, std::uint32_t, std::uint32_t);
using GetEffectStateFn = void (*)(void*, std::uint32_t, std::int32_t*);
using SetEffectParametersFn = std::int32_t (*)(void*, std::uint32_t, const void*,
                                               std::uint32_t, std::uint32_t);
using SetFilterParametersFn = std::int32_t (*)(void*, const Xa2FilterParameters*,
                                               std::uint32_t);
using GetFilterParametersFn = void (*)(void*, Xa2FilterParameters*);
using SetVolumeFn = std::int32_t (*)(void*, float, std::uint32_t);
using GetVolumeFn = void (*)(void*, float*);
using SetChannelVolumesFn = std::int32_t (*)(void*, std::uint32_t, const float*,
                                             std::uint32_t);
using GetChannelVolumesFn = void (*)(void*, std::uint32_t, float*);
using SetOutputMatrixFn = std::int32_t (*)(void*, void*, std::uint32_t,
                                           std::uint32_t, const float*,
                                           std::uint32_t);
using GetOutputMatrixFn = void (*)(void*, void*, std::uint32_t, std::uint32_t,
                                   float*);
using DestroyVoiceFn = void (*)(void*);
using StartFn = std::int32_t (*)(void*, std::uint32_t, std::uint32_t);
using StopFn = std::int32_t (*)(void*, std::uint32_t, std::uint32_t);
using SubmitSourceBufferFn = std::int32_t (*)(void*, const Xa2Buffer*, const void*);
using FlushSourceBuffersFn = std::int32_t (*)(void*);
using DiscontinuityFn = std::int32_t (*)(void*);
using ExitLoopFn = std::int32_t (*)(void*, std::uint32_t);
using GetStateFn = void (*)(void*, Xa2VoiceState*, std::uint32_t);
using SetFrequencyRatioFn = std::int32_t (*)(void*, float, std::uint32_t);
using GetFrequencyRatioFn = void (*)(void*, float*);
using SetSourceSampleRateFn = std::int32_t (*)(void*, std::uint32_t);
using GetChannelMaskFn = std::int32_t (*)(void*, std::uint32_t*);

// ---------------------------------------------------------------------------
// Fake IXAudio2VoiceCallback / IXAudio2EngineCallback
// ---------------------------------------------------------------------------

struct CallbackLog {
    int pass_start = 0;
    int pass_end = 0;
    int stream_end = 0;
    int buffer_start = 0;
    int buffer_end = 0;
    int loop_end = 0;
    int last_bytes_required = 0;
    void* last_buffer_context = nullptr;
};

// Shape of a client IXAudio2VoiceCallback: a vtable pointer followed by the
// client's own data. The bridge only ever reads the vtable pointer.
struct ClientCallback {
    void* vtable;
    CallbackLog* log;
};

void OnPassStart(void* self, std::uint32_t bytes) {
    static_cast<ClientCallback*>(self)->log->pass_start += 1;
    static_cast<ClientCallback*>(self)->log->last_bytes_required =
        static_cast<int>(bytes);
}
void OnPassEnd(void* self) {
    static_cast<ClientCallback*>(self)->log->pass_end += 1;
}
void OnStreamEnd(void* self) {
    static_cast<ClientCallback*>(self)->log->stream_end += 1;
}
void OnBufferStart(void* self, void* context) {
    static_cast<ClientCallback*>(self)->log->buffer_start += 1;
    static_cast<ClientCallback*>(self)->log->last_buffer_context = context;
}
void OnBufferEnd(void* self, void* context) {
    static_cast<ClientCallback*>(self)->log->buffer_end += 1;
    static_cast<ClientCallback*>(self)->log->last_buffer_context = context;
}
void OnLoopEnd(void* self, void* context) {
    static_cast<ClientCallback*>(self)->log->loop_end += 1;
    static_cast<ClientCallback*>(self)->log->last_buffer_context = context;
}
void OnVoiceError(void* self, void* context, std::int32_t hr) {
    (void)self;
    (void)context;
    (void)hr;
}

void* kVoiceCallbackVtable[7] = {
    reinterpret_cast<void*>(&OnPassStart),   reinterpret_cast<void*>(&OnPassEnd),
    reinterpret_cast<void*>(&OnStreamEnd),   reinterpret_cast<void*>(&OnBufferStart),
    reinterpret_cast<void*>(&OnBufferEnd),   reinterpret_cast<void*>(&OnLoopEnd),
    reinterpret_cast<void*>(&OnVoiceError),
};

struct EngineCallbackLog {
    int pass_start = 0;
    int pass_end = 0;
    int critical = 0;
    std::int32_t last_error = 0;
};

struct ClientEngineCallback {
    void* vtable;
    EngineCallbackLog* log;
};

void OnEnginePassStart(void* self) {
    static_cast<ClientEngineCallback*>(self)->log->pass_start += 1;
}
void OnEnginePassEnd(void* self) {
    static_cast<ClientEngineCallback*>(self)->log->pass_end += 1;
}
void OnCriticalError(void* self, std::int32_t hr) {
    static_cast<ClientEngineCallback*>(self)->log->critical += 1;
    static_cast<ClientEngineCallback*>(self)->log->last_error = hr;
}

void* kEngineCallbackVtable[3] = {
    reinterpret_cast<void*>(&OnEnginePassStart),
    reinterpret_cast<void*>(&OnEnginePassEnd),
    reinterpret_cast<void*>(&OnCriticalError),
};

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

struct Fixture {
    Xa2Engine* engine = nullptr;
    void* master = nullptr;
    float left[2048] = {};
    float right[2048] = {};
    float* channels[2] = {left, right};

    explicit Fixture(Xa2BridgeConfig config = {}) {
        engine = Xa2BridgeCreate(config);
    }
    ~Fixture() { Xa2BridgeDestroy(engine); }

    [[nodiscard]] bool CreateMaster() {
        if (engine == nullptr) return false;
        return SlotOf<CreateMasteringVoiceFn>(engine, xa27::kEngineCreateMasteringVoice)(
                   engine, &master, 2, 44100, 0, 0, nullptr) == kXa2Ok;
    }

    [[nodiscard]] void* CreateSource(const Xa2WaveFormat& format,
                                     void* callback = nullptr,
                                     const Xa2VoiceSends* sends = nullptr,
                                     const Xa2EffectChain* chain = nullptr,
                                     float max_ratio = 2.0F) {
        void* voice = nullptr;
        const std::int32_t status =
            SlotOf<CreateSourceVoiceFn>(engine, xa27::kEngineCreateSourceVoice)(
                engine, &voice, &format, 0, max_ratio, callback, sends, chain);
        (void)status;
        return voice;
    }

    [[nodiscard]] std::int32_t Render(std::uint32_t frames,
                                      std::int64_t block_start = 0) {
        std::memset(left, 0, sizeof(left));
        std::memset(right, 0, sizeof(right));
        const bool ok = Xa2BridgeRenderBlock(engine, block_start, frames, channels, 2);
        return ok ? kXa2Ok : kXa2InvalidCall;
    }
};

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

Xa2WaveFormat Pcm16Format(std::uint32_t rate, std::uint16_t channels) {
    Xa2WaveFormat format;
    format.format_tag = kWaveFormatPcm;
    format.channels = channels;
    format.samples_per_sec = rate;
    format.bits_per_sample = 16;
    format.block_align = static_cast<std::uint16_t>(channels * 2U);
    format.avg_bytes_per_sec = rate * channels * 2U;
    return format;
}

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

void TestSlotTable() {
    CHECK(xa27::kEngineGetDeviceDetails == 32);
    CHECK(xa27::kEngineCreateMasteringVoice == 80);
    CHECK(xa27::kVoiceSetEffectChain == 16);
    CHECK(xa27::kVoiceSetOutputMatrix == 128);
    CHECK(xa27::kVoiceStart == 152);
    CHECK(xa27::kVoiceSetSourceSampleRate == 224);

    Fixture fixture;
    CHECK(fixture.engine != nullptr);
    for (std::uint32_t slot = 0; slot < 16; ++slot) {
        CHECK(SlotOf<void*>(fixture.engine, slot * 8) != nullptr);
    }
    CHECK(fixture.CreateMaster());
    CHECK(Xa2VoiceFromInterface(fixture.master) != nullptr);
    CHECK(Xa2VoiceGetKind(Xa2VoiceFromInterface(fixture.master)) ==
          Xa2VoiceKind::Mastering);

    // Every declared slot of every voice kind resolves to a callable stub.
    void* source = fixture.CreateSource(FloatFormat(44100, 1));
    CHECK(source != nullptr);
    for (std::uint32_t slot = 0; slot < 37; ++slot) {
        CHECK(SlotOf<void*>(source, slot * 8) != nullptr);
        CHECK(SlotOf<void*>(fixture.master, slot * 8) != nullptr);
    }

    // The rejection tail is a real call, counted, not a jump into nothing.
    const Xa2BridgeStats before = Xa2BridgeGetStats(fixture.engine);
    CHECK(SlotOf<GenericFn>(source, 232)(source) == kXa2InvalidCall);
    CHECK(SlotOf<GenericFn>(fixture.engine, 128)(fixture.engine) == kXa2InvalidCall);
    const Xa2BridgeStats after = Xa2BridgeGetStats(fixture.engine);
    // One voice-tail call and one engine-tail call were counted.
    CHECK(after.unsupported_slots == before.unsupported_slots + 2);
    CHECK(after.rejected_calls == before.rejected_calls + 1);
}

void TestDeviceDetailsAndInitialize() {
    Xa2BridgeConfig config;
    config.device_name = "ASIO bridge";
    Fixture fixture(config);
    CHECK(fixture.engine != nullptr);

    std::uint32_t count = 0;
    CHECK(SlotOf<GetDeviceCountFn>(fixture.engine, xa27::kEngineGetDeviceCount)(
              fixture.engine, &count) == kXa2Ok);
    CHECK(count == 1);

    Xa2DeviceDetails details;
    CHECK(SlotOf<GetDeviceDetailsFn>(fixture.engine, xa27::kEngineGetDeviceDetails)(
              fixture.engine, 0, &details) == kXa2Ok);
    // libxact reads the channel count from byte 1030 of this structure.
    std::uint16_t channels_at_1030 = 0;
    std::memcpy(&channels_at_1030,
                reinterpret_cast<const std::uint8_t*>(&details) + 1030, 2);
    CHECK(channels_at_1030 == 2);
    CHECK(details.output_format.format.samples_per_sec == 44100);
    CHECK(details.output_format.format.format_tag == kWaveFormatExtensible);
    CHECK(details.display_name[0] == 'A');
    CHECK(details.device_id[0] == 'A');

    CHECK(SlotOf<GetDeviceDetailsFn>(fixture.engine, xa27::kEngineGetDeviceDetails)(
              fixture.engine, 1, &details) == kXa2InvalidCall);

    CHECK(SlotOf<InitializeFn>(fixture.engine, xa27::kEngineInitialize)(
              fixture.engine, 0, 0) == kXa2Ok);
    CHECK(SlotOf<InitializeFn>(fixture.engine, xa27::kEngineInitialize)(
              fixture.engine, 0, 0) == kXa2InvalidCall);

    CHECK(SlotOf<StartEngineFn>(fixture.engine, xa27::kEngineStartEngine)(
              fixture.engine) == kXa2Ok);
    SlotOf<StopEngineFn>(fixture.engine, xa27::kEngineStopEngine)(fixture.engine);
    CHECK(SlotOf<CommitChangesFn>(fixture.engine, xa27::kEngineCommitChanges)(
              fixture.engine, 0) == kXa2InvalidCall);
    SlotOf<SetDebugConfigurationFn>(fixture.engine,
                                    xa27::kEngineSetDebugConfiguration)(
        fixture.engine, nullptr, nullptr);

    void* object = nullptr;
    const std::uint8_t iid_xaudio2[16] = {0x58, 0x1F, 0xCF, 0x8B, 0xE7, 0x9F,
                                          0x83, 0x45, 0x8A, 0xC6, 0xE2, 0xAD,
                                          0xC4, 0x65, 0xC8, 0xBB};
    CHECK(SlotOf<QueryInterfaceFn>(fixture.engine, xa27::kEngineQueryInterface)(
              fixture.engine, iid_xaudio2, &object) == kXa2Ok);
    CHECK(object == fixture.engine);
    CHECK(SlotOf<ReleaseFn>(fixture.engine, xa27::kEngineRelease)(fixture.engine) >= 1);
    CHECK(SlotOf<AddRefFn>(fixture.engine, xa27::kEngineAddRef)(fixture.engine) >= 1);
    CHECK(SlotOf<AddRefFn>(fixture.engine, xa27::kEngineAddRef)(fixture.engine) >= 1);
}

void TestMasteringVoiceShape() {
    Fixture fixture;
    CHECK(fixture.engine != nullptr);

    void* voice = nullptr;
    // Wrong channel count and wrong rate are refused: no remixing or resampling.
    CHECK(SlotOf<CreateMasteringVoiceFn>(fixture.engine,
                                         xa27::kEngineCreateMasteringVoice)(
              fixture.engine, &voice, 6, 44100, 0, 0, nullptr) == kXa2InvalidCall);
    CHECK(SlotOf<CreateMasteringVoiceFn>(fixture.engine,
                                         xa27::kEngineCreateMasteringVoice)(
              fixture.engine, &voice, 2, 48000, 0, 0, nullptr) == kXa2InvalidCall);
    CHECK(SlotOf<CreateMasteringVoiceFn>(fixture.engine,
                                         xa27::kEngineCreateMasteringVoice)(
              fixture.engine, &voice, 2, 44100, 1, 0, nullptr) == kXa2InvalidCall);
    CHECK(SlotOf<CreateMasteringVoiceFn>(fixture.engine,
                                         xa27::kEngineCreateMasteringVoice)(
              fixture.engine, &voice, 2, 44100, 0, 1, nullptr) == kXa2InvalidCall);
    CHECK(fixture.CreateMaster());
    CHECK(SlotOf<CreateMasteringVoiceFn>(fixture.engine,
                                         xa27::kEngineCreateMasteringVoice)(
              fixture.engine, &voice, 2, 44100, 0, 0, nullptr) == kXa2InvalidCall);

    std::uint32_t mask = 0;
    CHECK(SlotOf<GetChannelMaskFn>(fixture.master, xa27::kMasteringGetChannelMask)(
              fixture.master, &mask) == kXa2Ok);
    CHECK(mask == 0x3U);

    Xa2VoiceDetails details;
    CHECK(SlotOf<GetVoiceDetailsFn>(fixture.master, xa27::kVoiceGetVoiceDetails)(
              fixture.master, &details) == kXa2Ok);
    CHECK(details.input_channels == 2);
    CHECK(details.input_sample_rate == 44100);

    // Source-only slots on the mastering voice fail closed and are counted. Slot
    // 152 is GetChannelMask on a mastering voice, so probe 160 (Start of a source
    // voice) which is unsupported here.
    CHECK(SlotOf<StartFn>(fixture.master, 160)(fixture.master, 0, 0) ==
          kXa2InvalidCall);
}

// Prototype-only: libxact has no CreateSourceVoice (audio-002), so this gate is
// this bridge's own boundary, not a libxact-derived constraint.
void TestSourceVoiceFormatGate() {
    Fixture fixture;
    CHECK(fixture.engine != nullptr);
    CHECK(fixture.CreateMaster());

    // ADPCM / XMA / WMA are not decoded by this bridge.
    Xa2WaveFormat adpcm = Pcm16Format(44100, 1);
    adpcm.format_tag = 0x0002;
    CHECK(fixture.CreateSource(adpcm) == nullptr);

    Xa2WaveFormat eight_bit = Pcm16Format(44100, 1);
    eight_bit.bits_per_sample = 8;
    eight_bit.block_align = 1;
    CHECK(fixture.CreateSource(eight_bit) == nullptr);

    Xa2WaveFormat bad_align = Pcm16Format(44100, 2);
    bad_align.block_align = 2;
    CHECK(fixture.CreateSource(bad_align) == nullptr);

    Xa2WaveFormat bad_rate = FloatFormat(1000, 1);
    CHECK(fixture.CreateSource(bad_rate) == nullptr);

    CHECK(fixture.CreateSource(Pcm16Format(44100, 1)) != nullptr);
    CHECK(fixture.CreateSource(Pcm16Format(22050, 2)) != nullptr);
    CHECK(fixture.CreateSource(FloatFormat(48000, 1)) != nullptr);

    // WAVE_FORMAT_EXTENSIBLE with a float sub-format and with a PCM sub-format.
    Xa2WaveFormatExtensible extensible{};
    extensible.format.format_tag = kWaveFormatExtensible;
    extensible.format.channels = 2;
    extensible.format.samples_per_sec = 44100;
    extensible.format.bits_per_sample = 32;
    extensible.format.block_align = 8;
    extensible.format.avg_bytes_per_sec = 44100 * 8;
    extensible.format.cb_size = 22;
    extensible.valid_bits_per_sample = 32;
    const std::uint8_t float_sub[16] = {0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10,
                                        0x00, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38,
                                        0x9B, 0x71};
    std::memcpy(extensible.sub_format, float_sub, 16);
    CHECK(fixture.CreateSource(extensible.format) != nullptr);

    const std::uint8_t unknown_sub[16] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66,
                                          0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC,
                                          0xDD, 0xEE, 0xFF, 0x00};
    std::memcpy(extensible.sub_format, unknown_sub, 16);
    CHECK(fixture.CreateSource(extensible.format) == nullptr);
}

void TestRenderAndEndOfStream() {
    Fixture fixture;
    CHECK(fixture.CreateMaster());

    CallbackLog log;
    ClientCallback client{kVoiceCallbackVtable, &log};

    void* voice = fixture.CreateSource(FloatFormat(44100, 1), &client);
    CHECK(voice != nullptr);

    const float samples[4] = {0.25F, 0.5F, 0.75F, 1.0F};
    Xa2Buffer buffer;
    buffer.flags = kXa2EndOfStream;
    buffer.audio_bytes = sizeof(samples);
    buffer.audio_data = reinterpret_cast<const std::uint8_t*>(samples);
    buffer.context = const_cast<float*>(samples);
    CHECK(SlotOf<SubmitSourceBufferFn>(voice, xa27::kVoiceSubmitSourceBuffer)(
              voice, &buffer, nullptr) == kXa2Ok);

    // GetState reports the state the audio thread has applied: a submitted buffer
    // becomes visible after the block that drains the command ring. libxact uses
    // GetState for progress/lapse, where one buffer of lag is harmless.
    Xa2VoiceState state;
    SlotOf<GetStateFn>(voice, xa27::kVoiceGetState)(voice, &state, 0);
    CHECK(state.buffers_queued == 0);
    CHECK(state.samples_played == 0);
    CHECK(log.buffer_start == 0); // not started yet

    CHECK(SlotOf<StartFn>(voice, xa27::kVoiceStart)(voice, 0, 0) == kXa2Ok);
    CHECK(fixture.Render(4) == kXa2Ok);
    CHECK(std::fabs(fixture.left[0] - 0.25F) < 1e-6F);
    CHECK(std::fabs(fixture.left[1] - 0.5F) < 1e-6F);
    CHECK(std::fabs(fixture.left[2] - 0.75F) < 1e-6F);
    CHECK(std::fabs(fixture.left[3] - 1.0F) < 1e-6F);
    CHECK(std::fabs(fixture.right[0] - 0.25F) < 1e-6F); // mono -> both channels

    CHECK(log.buffer_start == 1);
    CHECK(log.buffer_end == 1);
    CHECK(log.stream_end == 1);
    CHECK(log.pass_start == 1);
    CHECK(log.pass_end == 1);
    CHECK(log.last_buffer_context == samples);
    CHECK(log.last_bytes_required == 4 * 4); // frames * bytes per frame

    SlotOf<GetStateFn>(voice, xa27::kVoiceGetState)(voice, &state, 0);
    CHECK(state.buffers_queued == 0);
    CHECK(state.samples_played == 4);

    // A second block is silent: the voice ended.
    CHECK(fixture.Render(4) == kXa2Ok);
    CHECK(std::fabs(fixture.left[0]) < 1e-9F);
    CHECK(log.stream_end == 1);
}

void TestStartBeforeSubmit() {
    Fixture fixture;
    CHECK(fixture.CreateMaster());
    CallbackLog log;
    ClientCallback client{kVoiceCallbackVtable, &log};
    void* voice = fixture.CreateSource(FloatFormat(44100, 1), &client);
    CHECK(voice != nullptr);

    CHECK(SlotOf<StartFn>(voice, xa27::kVoiceStart)(voice, 0, 0) == kXa2Ok);
    CHECK(log.stream_end == 0); // Started with an empty queue: no fake EOS
    CHECK(log.buffer_start == 0);

    const float samples[2] = {0.5F, 0.5F};
    Xa2Buffer buffer;
    buffer.audio_bytes = sizeof(samples);
    buffer.audio_data = reinterpret_cast<const std::uint8_t*>(samples);
    CHECK(SlotOf<SubmitSourceBufferFn>(voice, xa27::kVoiceSubmitSourceBuffer)(
              voice, &buffer, nullptr) == kXa2Ok);
    CHECK(log.buffer_start == 0); // applied when the block is rendered
    CHECK(fixture.Render(2) == kXa2Ok);
    CHECK(log.buffer_start == 1);
    CHECK(std::fabs(fixture.left[0] - 0.5F) < 1e-6F);
}

void TestLoops() {
    Fixture fixture;
    CHECK(fixture.CreateMaster());
    CallbackLog log;
    ClientCallback client{kVoiceCallbackVtable, &log};
    void* voice = fixture.CreateSource(FloatFormat(44100, 1), &client);
    CHECK(voice != nullptr);

    const float samples[2] = {0.5F, 1.0F};
    Xa2Buffer buffer;
    buffer.audio_bytes = sizeof(samples);
    buffer.audio_data = reinterpret_cast<const std::uint8_t*>(samples);
    buffer.loop_count = 3;
    buffer.loop_begin = 0;
    buffer.loop_length = 0;
    CHECK(SlotOf<SubmitSourceBufferFn>(voice, xa27::kVoiceSubmitSourceBuffer)(
              voice, &buffer, nullptr) == kXa2Ok);
    CHECK(SlotOf<StartFn>(voice, xa27::kVoiceStart)(voice, 0, 0) == kXa2Ok);

    CHECK(fixture.Render(8) == kXa2Ok);
    // Two frames played four times: 3 loop repeats then the buffer ends.
    const float expected[8] = {0.5F, 1.0F, 0.5F, 1.0F, 0.5F, 1.0F, 0.5F, 1.0F};
    for (int i = 0; i < 8; ++i) {
        CHECK(std::fabs(fixture.left[i] - expected[i]) < 1e-6F);
    }
    CHECK(log.loop_end == 3);
    CHECK(log.buffer_end == 1);
    CHECK(log.stream_end == 0); // no END_OF_STREAM flag on this buffer

    // ExitLoop: the voice stops looping at the next wrap.
    CHECK(SlotOf<ExitLoopFn>(voice, xa27::kVoiceExitLoop)(voice, 0) == kXa2Ok);
    CHECK(fixture.Render(4) == kXa2Ok);
}

void TestOperationSets() {
    Fixture fixture;
    CHECK(fixture.CreateMaster());
    void* voice = fixture.CreateSource(FloatFormat(44100, 1));
    CHECK(voice != nullptr);
    const float samples[4] = {1.0F, 1.0F, 1.0F, 1.0F};
    Xa2Buffer buffer;
    buffer.audio_bytes = sizeof(samples);
    buffer.audio_data = reinterpret_cast<const std::uint8_t*>(samples);
    buffer.loop_count = kXa2LoopInfinite;
    CHECK(SlotOf<SubmitSourceBufferFn>(voice, xa27::kVoiceSubmitSourceBuffer)(
              voice, &buffer, nullptr) == kXa2Ok);
    CHECK(SlotOf<StartFn>(voice, xa27::kVoiceStart)(voice, 0, 0) == kXa2Ok);

    CHECK(fixture.Render(2) == kXa2Ok);
    CHECK(std::fabs(fixture.left[0] - 1.0F) < 1e-6F);

    // Volume 0.25 deferred to operation set 7: not audible before the commit.
    CHECK(SlotOf<SetVolumeFn>(voice, xa27::kVoiceSetVolume)(voice, 0.25F, 7) ==
          kXa2Ok);
    CHECK(fixture.Render(2) == kXa2Ok);
    CHECK(std::fabs(fixture.left[0] - 1.0F) < 1e-6F);

    CHECK(SlotOf<CommitChangesFn>(fixture.engine, xa27::kEngineCommitChanges)(
              fixture.engine, 7) == kXa2Ok);
    CHECK(fixture.Render(2) == kXa2Ok);
    CHECK(std::fabs(fixture.left[0] - 0.25F) < 1e-6F);
    CHECK(Xa2BridgeGetStats(fixture.engine).operation_sets_committed >= 1);

    // A deferred start is not applied either.
    void* second = fixture.CreateSource(FloatFormat(44100, 1));
    CHECK(second != nullptr);
    CHECK(SlotOf<SubmitSourceBufferFn>(second, xa27::kVoiceSubmitSourceBuffer)(
              second, &buffer, nullptr) == kXa2Ok);
    CHECK(SlotOf<StartFn>(second, xa27::kVoiceStart)(second, 0, 9) == kXa2Ok);
    CHECK(fixture.Render(2) == kXa2Ok);
    CHECK(std::fabs(fixture.left[0] - 0.25F) < 1e-6F); // only the first voice
    CHECK(SlotOf<CommitChangesFn>(fixture.engine, xa27::kEngineCommitChanges)(
              fixture.engine, 0xFFFFFFFFU) == kXa2Ok);
    CHECK(fixture.Render(2) == kXa2Ok);
    CHECK(fixture.left[0] > 0.25F); // second voice added
}

void TestMultipleVoicesAndMatrices() {
    Fixture fixture;
    CHECK(fixture.CreateMaster());
    void* mono = fixture.CreateSource(FloatFormat(44100, 1));
    void* stereo = fixture.CreateSource(FloatFormat(44100, 2));
    CHECK(mono != nullptr);
    CHECK(stereo != nullptr);

    const float mono_samples[2] = {0.25F, 0.25F};
    const float stereo_samples[2 * 2] = {0.5F, 0.25F, 0.5F, 0.25F};
    Xa2Buffer mono_buffer;
    mono_buffer.audio_bytes = sizeof(mono_samples);
    mono_buffer.audio_data = reinterpret_cast<const std::uint8_t*>(mono_samples);
    mono_buffer.loop_count = kXa2LoopInfinite;
    Xa2Buffer stereo_buffer;
    stereo_buffer.audio_bytes = sizeof(stereo_samples);
    stereo_buffer.audio_data = reinterpret_cast<const std::uint8_t*>(stereo_samples);
    stereo_buffer.loop_count = kXa2LoopInfinite;

    CHECK(SlotOf<SubmitSourceBufferFn>(mono, xa27::kVoiceSubmitSourceBuffer)(
              mono, &mono_buffer, nullptr) == kXa2Ok);
    CHECK(SlotOf<SubmitSourceBufferFn>(stereo, xa27::kVoiceSubmitSourceBuffer)(
              stereo, &stereo_buffer, nullptr) == kXa2Ok);
    CHECK(SlotOf<StartFn>(mono, xa27::kVoiceStart)(mono, 0, 0) == kXa2Ok);
    CHECK(SlotOf<StartFn>(stereo, xa27::kVoiceStart)(stereo, 0, 0) == kXa2Ok);
    CHECK(fixture.Render(2) == kXa2Ok);
    CHECK(std::fabs(fixture.left[0] - 0.75F) < 1e-6F);
    CHECK(std::fabs(fixture.right[0] - 0.5F) < 1e-6F);

    // Uniform 2x2 matrix = scalar gain on the mono voice.
    const float half[4] = {0.5F, 0.5F, 0.5F, 0.5F};
    CHECK(SlotOf<SetOutputMatrixFn>(mono, xa27::kVoiceSetOutputMatrix)(
              mono, fixture.master, 2, 2, half, 0) == kXa2Ok);
    CHECK(fixture.Render(2) == kXa2Ok);
    CHECK(std::fabs(fixture.left[0] - (0.5F + 0.125F)) < 1e-6F);

    // Diagonal matrix = balance: left keeps 1.0, right is attenuated to 0.5.
    const float balance[4] = {1.0F, 0.0F, 0.0F, 0.5F};
    CHECK(SlotOf<SetOutputMatrixFn>(mono, xa27::kVoiceSetOutputMatrix)(
              mono, fixture.master, 2, 2, balance, 0) == kXa2Ok);
    CHECK(fixture.Render(2) == kXa2Ok);
    // Left keeps the mono 0.25 + stereo 0.5; right keeps stereo 0.25 plus the
    // attenuated mono 0.25 * 0.5 = 0.125.
    CHECK(std::fabs(fixture.left[0] - 0.75F) < 1e-6F);
    CHECK(std::fabs(fixture.right[0] - 0.375F) < 1e-6F);

    // A cross-feed matrix cannot be represented by this mixer: refused, counted.
    const float cross[4] = {1.0F, 1.0F, 0.0F, 0.0F};
    CHECK(SlotOf<SetOutputMatrixFn>(mono, xa27::kVoiceSetOutputMatrix)(
              mono, fixture.master, 2, 2, cross, 0) == kXa2InvalidCall);
    CHECK(Xa2BridgeGetStats(fixture.engine).matrices_rejected >= 1);

    // 5.1-sized matrices are refused rather than approximated.
    float wide[36] = {};
    CHECK(SlotOf<SetOutputMatrixFn>(mono, xa27::kVoiceSetOutputMatrix)(
              mono, fixture.master, 2, 6, wide, 0) == kXa2InvalidCall);

    float readback[4] = {};
    SlotOf<GetOutputMatrixFn>(mono, xa27::kVoiceGetOutputMatrix)(
        mono, fixture.master, 2, 2, readback);
    CHECK(std::fabs(readback[0] - 1.0F) < 1e-6F);
    CHECK(std::fabs(readback[3] - 0.5F) < 1e-6F);
}

void TestSubmixRoutingAndBusGain() {
    Fixture fixture;
    CHECK(fixture.CreateMaster());
    void* submix = nullptr;
    CHECK(SlotOf<CreateSubmixVoiceFn>(fixture.engine, xa27::kEngineCreateSubmixVoice)(
              fixture.engine, &submix, 2, 44100, 0, 3, nullptr, nullptr) == kXa2Ok);
    CHECK(submix != nullptr);
    CHECK(Xa2VoiceGetKind(Xa2VoiceFromInterface(submix)) == Xa2VoiceKind::Submix);

    Xa2SendDescriptor send;
    send.output_voice = submix;
    Xa2VoiceSends sends;
    sends.send_count = 1;
    sends.sends = &send;
    void* voice = fixture.CreateSource(FloatFormat(44100, 1), nullptr, &sends);
    CHECK(voice != nullptr);

    const float samples[2] = {1.0F, 1.0F};
    Xa2Buffer buffer;
    buffer.audio_bytes = sizeof(samples);
    buffer.audio_data = reinterpret_cast<const std::uint8_t*>(samples);
    buffer.loop_count = kXa2LoopInfinite;
    CHECK(SlotOf<SubmitSourceBufferFn>(voice, xa27::kVoiceSubmitSourceBuffer)(
              voice, &buffer, nullptr) == kXa2Ok);
    CHECK(SlotOf<StartFn>(voice, xa27::kVoiceStart)(voice, 0, 0) == kXa2Ok);
    CHECK(fixture.Render(2) == kXa2Ok);
    CHECK(std::fabs(fixture.left[0] - 1.0F) < 1e-6F);

    // Submix volume is the category gain.
    CHECK(SlotOf<SetVolumeFn>(submix, xa27::kVoiceSetVolume)(submix, 0.5F, 0) ==
          kXa2Ok);
    CHECK(fixture.Render(2) == kXa2Ok);
    CHECK(std::fabs(fixture.left[0] - 0.5F) < 1e-6F);

    // More submix voices than mixer buses fail closed.
    std::uint32_t created = 0;
    for (std::uint32_t i = 0; i < 20; ++i) {
        void* extra = nullptr;
        const std::int32_t status =
            SlotOf<CreateSubmixVoiceFn>(fixture.engine, xa27::kEngineCreateSubmixVoice)(
                fixture.engine, &extra, 2, 44100, 0, 1, nullptr, nullptr);
        if (status == kXa2Ok) ++created;
    }
    CHECK(created + 1 <= kMixerMaxBuses - 1);
}

void TestFrequencyRatioAndRateConversion() {
    Fixture fixture;
    CHECK(fixture.CreateMaster());
    void* voice = fixture.CreateSource(FloatFormat(22050, 1));
    CHECK(voice != nullptr);
    const float samples[8] = {1.0F, 1.0F, 1.0F, 1.0F, 1.0F, 1.0F, 1.0F, 1.0F};
    Xa2Buffer buffer;
    buffer.audio_bytes = sizeof(samples);
    buffer.audio_data = reinterpret_cast<const std::uint8_t*>(samples);
    buffer.flags = kXa2EndOfStream;
    CHECK(SlotOf<SubmitSourceBufferFn>(voice, xa27::kVoiceSubmitSourceBuffer)(
              voice, &buffer, nullptr) == kXa2Ok);
    CHECK(SlotOf<StartFn>(voice, xa27::kVoiceStart)(voice, 0, 0) == kXa2Ok);
    CHECK(fixture.Render(16) == kXa2Ok);
    // 22050 -> 44100 consumes half a source frame per output frame: eight
    // source frames cover exactly sixteen output frames.
    for (int i = 0; i < 16; ++i) {
        CHECK(std::fabs(fixture.left[i] - 1.0F) < 1e-6F);
    }
    Xa2VoiceState state;
    SlotOf<GetStateFn>(voice, xa27::kVoiceGetState)(voice, &state, 0);
    CHECK(state.samples_played == 8);

    // Ratio 2.0 doubles the source rate back to 44100.
    void* ratio_voice = fixture.CreateSource(FloatFormat(22050, 1), nullptr, nullptr,
                                             nullptr, 4.0F);
    CHECK(ratio_voice != nullptr);
    CHECK(SlotOf<SetFrequencyRatioFn>(ratio_voice, xa27::kVoiceSetFrequencyRatio)(
              ratio_voice, 2.0F, 0) == kXa2Ok);
    float ratio = 0.0F;
    SlotOf<GetFrequencyRatioFn>(ratio_voice, xa27::kVoiceGetFrequencyRatio)(
        ratio_voice, &ratio);
    CHECK(std::fabs(ratio - 2.0F) < 1e-6F);
    // Beyond MaxFrequencyRatio: refused.
    CHECK(SlotOf<SetFrequencyRatioFn>(ratio_voice, xa27::kVoiceSetFrequencyRatio)(
              ratio_voice, 8.0F, 0) == kXa2InvalidArgument);
    CHECK(SlotOf<SetSourceSampleRateFn>(ratio_voice, xa27::kVoiceSetSourceSampleRate)(
              ratio_voice, 48000) == kXa2InvalidCall);
}

void TestEffectsAreRecordedNotApplied() {
    Fixture fixture;
    std::int32_t dummy_effect = 0;
    Xa2EffectDescriptor descriptor;
    descriptor.effect = &dummy_effect;
    descriptor.initial_state = 1;
    descriptor.output_channels = 2;
    Xa2EffectChain chain;
    chain.effect_count = 1;
    chain.descriptors = &descriptor;
    void* master = nullptr;
    CHECK(SlotOf<CreateMasteringVoiceFn>(fixture.engine,
                                         xa27::kEngineCreateMasteringVoice)(
              fixture.engine, &master, 2, 44100, 0, 0, &chain) == kXa2Ok);
    CHECK(master != nullptr);
    CHECK(Xa2BridgeGetStats(fixture.engine).effects_recorded == 1);

    std::int32_t enabled = 0;
    SlotOf<GetEffectStateFn>(master, xa27::kVoiceGetEffectState)(master, 0, &enabled);
    CHECK(enabled == 1);
    CHECK(SlotOf<EnableEffectFn>(master, xa27::kVoiceEnableEffect)(master, 1, 0) ==
          kXa2InvalidCall);
    CHECK(SlotOf<SetEffectParametersFn>(master, xa27::kVoiceSetEffectParameters)(
              master, 0, &dummy_effect, 8, 0) == kXa2Ok);
    // The XAPO object is never invoked: effects_applied stays zero.
    CHECK(Xa2BridgeGetStats(fixture.engine).effects_applied == 0);

    Xa2FilterParameters filter;
    filter.type = 0;
    filter.frequency = 0.5F;
    filter.one_over_q = 1.0F;
    CHECK(SlotOf<SetFilterParametersFn>(master, xa27::kVoiceSetFilterParameters)(
              master, &filter, 0) == kXa2Ok);
    Xa2FilterParameters readback;
    SlotOf<GetFilterParametersFn>(master, xa27::kVoiceGetFilterParameters)(
        master, &readback);
    CHECK(std::fabs(readback.frequency - 0.5F) < 1e-6F);
    CHECK(Xa2BridgeGetStats(fixture.engine).filters_applied == 0);
}

void TestChannelVolumesAndMisc() {
    Fixture fixture;
    CHECK(fixture.CreateMaster());
    void* voice = fixture.CreateSource(FloatFormat(44100, 2));
    CHECK(voice != nullptr);

    const float uniform[2] = {0.5F, 0.5F};
    CHECK(SlotOf<SetChannelVolumesFn>(voice, xa27::kVoiceSetChannelVolumes)(
              voice, 2, uniform, 0) == kXa2Ok);
    float volumes[2] = {};
    SlotOf<GetChannelVolumesFn>(voice, xa27::kVoiceGetChannelVolumes)(voice, 2,
                                                                     volumes);
    CHECK(std::fabs(volumes[0] - 0.5F) < 1e-6F);

    const float per_channel[2] = {0.5F, 0.25F};
    CHECK(SlotOf<SetChannelVolumesFn>(voice, xa27::kVoiceSetChannelVolumes)(
              voice, 2, per_channel, 0) == kXa2InvalidCall);
    const float wrong_count[3] = {0.5F, 0.5F, 0.5F};
    CHECK(SlotOf<SetChannelVolumesFn>(voice, xa27::kVoiceSetChannelVolumes)(
              voice, 3, wrong_count, 0) == kXa2InvalidCall);

    float gain = 0.0F;
    SlotOf<GetVolumeFn>(voice, xa27::kVoiceGetVolume)(voice, &gain);
    CHECK(std::fabs(gain - 0.5F) < 1e-6F);
    CHECK(SlotOf<SetVolumeFn>(voice, xa27::kVoiceSetVolume)(voice, -1.0F, 0) ==
          kXa2InvalidArgument);

    CHECK(SlotOf<FlushSourceBuffersFn>(voice, xa27::kVoiceFlushSourceBuffers)(voice) ==
          kXa2Ok);
    CHECK(SlotOf<DiscontinuityFn>(voice, xa27::kVoiceDiscontinuity)(voice) == kXa2Ok);
    CHECK(SlotOf<StopFn>(voice, xa27::kVoiceStop)(voice, 1, 0) == kXa2InvalidCall);

    // Submitting a compressed format at the buffer level is not possible; a null
    // data pointer and a WMA companion are rejected.
    Xa2Buffer buffer;
    buffer.audio_bytes = 8;
    CHECK(SlotOf<SubmitSourceBufferFn>(voice, xa27::kVoiceSubmitSourceBuffer)(
              voice, &buffer, nullptr) == kXa2Pointer);
    std::int32_t wma = 0;
    CHECK(SlotOf<SubmitSourceBufferFn>(voice, xa27::kVoiceSubmitSourceBuffer)(
              voice, &buffer, &wma) == kXa2Pointer);

    SlotOf<DestroyVoiceFn>(voice, xa27::kVoiceDestroyVoice)(voice);
    SlotOf<DestroyVoiceFn>(fixture.master, xa27::kVoiceDestroyVoice)(fixture.master);
}

// ---------------------------------------------------------------------------
// A. The surface libxact actually calls (byte-proven in audio-001/audio-002).
// Every call below mirrors a call site in libxact.dll; if one of these changes
// shape, the bridge is no longer a drop-in for this client.
// ---------------------------------------------------------------------------
void TestLibxactProvenSurface() {
    Xa2BridgeConfig config;
    config.device_name = "proven";
    Fixture fixture(config);
    CHECK(fixture.engine != nullptr);
    const Xa2BridgeStats before = Xa2BridgeGetStats(fixture.engine);

    // engine +0x20: GetDeviceDetails(0, details); libxact reads nChannels at
    // byte 1030 of the record.
    Xa2DeviceDetails details;
    CHECK(SlotOf<GetDeviceDetailsFn>(fixture.engine, xa27::kEngineGetDeviceDetails)(
              fixture.engine, 0, &details) == kXa2Ok);
    std::uint16_t channels_at_1030 = 0;
    std::memcpy(&channels_at_1030,
                reinterpret_cast<const std::uint8_t*>(&details) + 1030, 2);
    CHECK(channels_at_1030 == 2);

    // XAPOFX effect object recorded by reference; libxact passes the reverb
    // descriptor into the mastering voice's chain.
    std::int32_t reverb_object = 0;
    Xa2EffectDescriptor descriptor;
    descriptor.effect = &reverb_object;
    descriptor.initial_state = 0;
    descriptor.output_channels = 2;
    Xa2EffectChain chain;
    chain.effect_count = 1;
    chain.descriptors = &descriptor;

    // engine +0x50: CreateMasteringVoice(pp, 2, 44100, 0, 0, chain)
    void* master = nullptr;
    CHECK(SlotOf<CreateMasteringVoiceFn>(fixture.engine,
                                         xa27::kEngineCreateMasteringVoice)(
              fixture.engine, &master, 2, 44100, 0, 0, &chain) == kXa2Ok);
    CHECK(master != nullptr);

    // engine +0x48: CreateSubmixVoice(pp, 2, 44100, <5>, <0>, 0, 0). audio-001
    // leaves the Flags/ProcessingStage order unproven (one stack word is 5, the
    // other 0), so both orders must be accepted.
    void* submix_a = nullptr;
    void* submix_b = nullptr;
    CHECK(SlotOf<CreateSubmixVoiceFn>(fixture.engine, xa27::kEngineCreateSubmixVoice)(
              fixture.engine, &submix_a, 2, 44100, 5, 0, nullptr, nullptr) == kXa2Ok);
    CHECK(SlotOf<CreateSubmixVoiceFn>(fixture.engine, xa27::kEngineCreateSubmixVoice)(
              fixture.engine, &submix_b, 2, 44100, 0, 5, nullptr, nullptr) == kXa2Ok);

    // voice +0x10: SetEffectChain with a one-descriptor chain.
    CHECK(SlotOf<SetEffectChainFn>(submix_a, xa27::kVoiceSetEffectChain)(
              submix_a, &chain) == kXa2Ok);
    // voice +0x30: SetEffectParameters(0, block, 52, 0) - the 52-byte XAPOFX
    // parameter block.
    std::uint8_t reverb_parameters[52] = {};
    CHECK(SlotOf<SetEffectParametersFn>(master, xa27::kVoiceSetEffectParameters)(
              master, 0, reverb_parameters, 52, 0) == kXa2Ok);

    // voice +0x40: SetFilterParameters({Type, Frequency, OneOverQ}, 0) on the
    // mastering voice and both submixes (the three 12-byte records).
    void* const filtered[3] = {master, submix_a, submix_b};
    for (std::uint32_t i = 0; i < 3; ++i) {
        Xa2FilterParameters filter;
        filter.type = i; // libxact derives Type 0..3 from comparisons
        filter.frequency = 0.5F;
        filter.one_over_q = 1.0F;
        CHECK(SlotOf<SetFilterParametersFn>(filtered[i],
                                            xa27::kVoiceSetFilterParameters)(
                  filtered[i], &filter, 0) == kXa2Ok);
    }
    // Filters are recorded, never applied: this mixer has no filter DSP, so a
    // filtered cue is audible unfiltered. That is a known gap on a proven path.
    CHECK(Xa2BridgeGetStats(fixture.engine).filters_recorded == 3);
    CHECK(Xa2BridgeGetStats(fixture.engine).filters_applied == 0);

    // voice +0x80: SetOutputMatrix(submix -> master, 2, 2, identity, 0).
    const float identity[4] = {1.0F, 0.0F, 0.0F, 1.0F};
    CHECK(SlotOf<SetOutputMatrixFn>(submix_a, xa27::kVoiceSetOutputMatrix)(
              submix_a, master, 2, 2, identity, 0) == kXa2Ok);
    CHECK(Xa2BridgeGetStats(fixture.engine).matrices_applied >= 1);

    // voice +0x90: DestroyVoice is index 18, not 12 or 36. A wrong value would
    // call SetOutputMatrix or a rejection stub instead.
    CHECK(SlotOf<DestroyVoiceFn>(submix_b, xa27::kVoiceDestroyVoice) != nullptr);
    SlotOf<DestroyVoiceFn>(submix_b, xa27::kVoiceDestroyVoice)(submix_b);
    CHECK(Xa2VoiceFromInterface(submix_b) == nullptr);
    CHECK(Xa2BridgeGetStats(fixture.engine).live_submix_voices == 1);
    SlotOf<DestroyVoiceFn>(submix_a, xa27::kVoiceDestroyVoice)(submix_a);
    CHECK(Xa2BridgeGetStats(fixture.engine).live_submix_voices == 0);

    const Xa2BridgeStats after = Xa2BridgeGetStats(fixture.engine);
    CHECK(after.unsupported_slots == before.unsupported_slots);
    CHECK(after.effects_recorded == before.effects_recorded + 2);
}

// ---------------------------------------------------------------------------
// C. Speculative surface: no libxact call site. This is prototype-only and is
// asserted to be marked as such, so nobody mistakes it for libxact support.
// ---------------------------------------------------------------------------
void TestSpeculativeSourceSurfaceIsMarked() {
    CHECK(!kLibxactUsesSourceVoice);

    Fixture fixture;
    CHECK(fixture.CreateMaster());
    const Xa2BridgeStats before = Xa2BridgeGetStats(fixture.engine);
    void* voice = fixture.CreateSource(FloatFormat(44100, 1));
    CHECK(voice != nullptr);
    const Xa2BridgeStats created = Xa2BridgeGetStats(fixture.engine);

    // Creating a source voice is not something libxact does; the surface exists
    // for this stream's host. It works, but it proves nothing about the client.
    CHECK(created.live_source_voices == 1);
    CHECK(created.unsupported_slots == before.unsupported_slots);

    // SDK-declaration order from the proven 19-entry base: +0xA8 submit,
    // +0x98 start, +0xC8 state. Only a live vtable dump can confirm the order.
    const float samples[2] = {0.5F, 0.5F};
    Xa2Buffer buffer;
    buffer.audio_bytes = sizeof(samples);
    buffer.audio_data = reinterpret_cast<const std::uint8_t*>(samples);
    buffer.loop_count = kXa2LoopInfinite;
    CHECK(SlotOf<SubmitSourceBufferFn>(voice, xa27::kVoiceSubmitSourceBuffer)(
              voice, &buffer, nullptr) == kXa2Ok);
    CHECK(SlotOf<StartFn>(voice, xa27::kVoiceStart)(voice, 0, 0) == kXa2Ok);
    CHECK(fixture.Render(2) == kXa2Ok);
    CHECK(std::fabs(fixture.left[0] - 0.5F) < 1e-6F);
    Xa2VoiceState state;
    SlotOf<GetStateFn>(voice, xa27::kVoiceGetState)(voice, &state, 0);
    CHECK(state.samples_played == 2);

    // Every slot past the source table stays fail-closed and counted.
    CHECK(SlotOf<GenericFn>(voice, xa27::kVoiceFirstRejectionSlot)(voice) ==
          kXa2InvalidCall);
    CHECK(Xa2BridgeGetStats(fixture.engine).unsupported_slots ==
          before.unsupported_slots + 1);
}

void TestEngineCallbacks() {
    Fixture fixture;
    CHECK(fixture.CreateMaster());
    EngineCallbackLog log;
    ClientEngineCallback client{kEngineCallbackVtable, &log};
    CHECK(SlotOf<RegisterForCallbacksFn>(fixture.engine,
                                         xa27::kEngineRegisterForCallbacks)(
              fixture.engine, &client) == kXa2Ok);
    CHECK(SlotOf<RegisterForCallbacksFn>(fixture.engine,
                                         xa27::kEngineRegisterForCallbacks)(
              fixture.engine, &client) == kXa2InvalidCall);

    CHECK(fixture.Render(4) == kXa2Ok);
    CHECK(log.pass_start == 1);
    CHECK(log.pass_end == 1);
    Xa2BridgeNotifyCriticalError(fixture.engine, kXa2DeviceInvalidated);
    CHECK(log.critical == 1);
    CHECK(log.last_error == kXa2DeviceInvalidated);

    SlotOf<UnregisterForCallbacksFn>(fixture.engine,
                                     xa27::kEngineUnregisterForCallbacks)(
        fixture.engine, &client);
    CHECK(fixture.Render(4) == kXa2Ok);
    CHECK(log.pass_start == 1);
}

void TestRenderIsAllocationFree() {
    Fixture fixture;
    CHECK(fixture.CreateMaster());
    void* voice = fixture.CreateSource(FloatFormat(44100, 1));
    CHECK(voice != nullptr);
    const float samples[16] = {0.1F, 0.2F, 0.3F, 0.4F, 0.5F, 0.6F, 0.7F, 0.8F,
                               0.9F, 0.8F, 0.7F, 0.6F, 0.5F, 0.4F, 0.3F, 0.2F};
    Xa2Buffer buffer;
    buffer.audio_bytes = sizeof(samples);
    buffer.audio_data = reinterpret_cast<const std::uint8_t*>(samples);
    buffer.loop_count = kXa2LoopInfinite;
    CHECK(SlotOf<SubmitSourceBufferFn>(voice, xa27::kVoiceSubmitSourceBuffer)(
              voice, &buffer, nullptr) == kXa2Ok);
    CHECK(SlotOf<StartFn>(voice, xa27::kVoiceStart)(voice, 0, 0) == kXa2Ok);
    CHECK(fixture.Render(16) == kXa2Ok);

    test::BeginAllocationProbe();
    for (int block = 0; block < 8; ++block) {
        const bool ok = Xa2BridgeRenderBlock(fixture.engine, block * 128, 128,
                                             fixture.channels, 2);
        CHECK(ok);
    }
    const long long allocations = test::EndAllocationProbe();
    CHECK(allocations == 0);
}

void TestInvalidConfiguration() {
    Xa2BridgeConfig config;
    config.device_channels = 0;
    CHECK(Xa2BridgeCreate(config) == nullptr);
    config.device_channels = 2;
    config.device_rate = 1000;
    CHECK(Xa2BridgeCreate(config) == nullptr);
    config.device_rate = 44100;
    config.max_block_frames = 0;
    CHECK(Xa2BridgeCreate(config) == nullptr);
    config.max_block_frames = kMixerMaxBlockFrames + 1;
    CHECK(Xa2BridgeCreate(config) == nullptr);
    config.max_block_frames = 128;

    Xa2Engine* engine = Xa2BridgeCreate(config);
    CHECK(engine != nullptr);
    CHECK(Xa2BridgeMixer(engine) != nullptr);
    CHECK(Xa2BridgeConfigOf(engine) != nullptr);
    // A block larger than the prepared maximum is refused rather than truncated.
    float left[256] = {};
    float right[256] = {};
    float* channels[2] = {left, right};
    CHECK(!Xa2BridgeRenderBlock(engine, 0, 256, channels, 2));
    CHECK(Xa2BridgeRenderBlock(engine, 0, 128, channels, 2));
    CHECK(!Xa2BridgeRenderBlock(engine, 0, 128, channels, 3)); // wrong channel count
    Xa2BridgeDestroy(engine);
}

} // namespace

int main() {
    TestSlotTable();
    TestDeviceDetailsAndInitialize();
    TestMasteringVoiceShape();
    TestLibxactProvenSurface();
    TestSpeculativeSourceSurfaceIsMarked();
    TestSourceVoiceFormatGate();
    TestRenderAndEndOfStream();
    TestStartBeforeSubmit();
    TestLoops();
    TestOperationSets();
    TestMultipleVoicesAndMatrices();
    TestSubmixRoutingAndBusGain();
    TestFrequencyRatioAndRateConversion();
    TestEffectsAreRecordedNotApplied();
    TestChannelVolumesAndMisc();
    TestEngineCallbacks();
    TestRenderIsAllocationFree();
    TestInvalidConfiguration();
    return test::Finish("xaudio2_bridge");
}
