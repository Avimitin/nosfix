#include "nosfix/xaudio2_bridge.hpp"

#include <atomic>
#include <cstring>
#include <new>

namespace nosfix::audio {
namespace {

constexpr std::uint32_t kMaxVoiceSlots = kMixerMaxVoices;
constexpr std::uint32_t kMaxEffects = 4;
constexpr std::uint32_t kMaxSends = 4;
constexpr std::uint32_t kMaxPendingOps = 32;
constexpr std::uint32_t kMaxEngineCallbacks = 4;
constexpr std::uint32_t kEffectParameterBytes = 32;

// XAudio2 2.7 engine vtable: slots 0..120 are real, 128.. are a rejection tail.
constexpr std::uint32_t kEngineVtableEntries = 24;
// Voice vtable: 0..224 real, 232..288 rejection tail.
constexpr std::uint32_t kVoiceVtableEntries = 37;

constexpr std::uint8_t kPcmSubFormat[16] = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
                                            0x10, 0x00, 0x80, 0x00, 0x00, 0xAA,
                                            0x00, 0x38, 0x9B, 0x71};
constexpr std::uint8_t kFloatSubFormat[16] = {0x03, 0x00, 0x00, 0x00, 0x00, 0x00,
                                              0x10, 0x00, 0x80, 0x00, 0x00, 0xAA,
                                              0x00, 0x38, 0x9B, 0x71};
constexpr std::uint8_t kIidIUnknown[16] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                           0x00, 0x00, 0xC0, 0x00, 0x00, 0x00,
                                           0x00, 0x00, 0x00, 0x46};
constexpr std::uint8_t kIidIXAudio2_27[16] = {0x58, 0x1F, 0xCF, 0x8B, 0xE7, 0x9F,
                                              0x83, 0x45, 0x8A, 0xC6, 0xE2, 0xAD,
                                              0xC4, 0x65, 0xC8, 0xBB};

enum class OpKind : std::uint8_t {
    Volume,
    Frequency,
    Matrix,
    Start,
    Stop,
    ExitLoop,
};

struct PendingOp {
    OpKind kind = OpKind::Volume;
    Xa2Voice* voice = nullptr;
    Xa2Voice* destination = nullptr;
    std::uint32_t operation_set = 0;
    float f0 = 0.0F;
    float f1 = 0.0F;
    float matrix[4] = {0.0F, 0.0F, 0.0F, 0.0F};
};

[[nodiscard]] bool GuidEquals(const void* guid, const std::uint8_t (&bytes)[16]) noexcept {
    return guid != nullptr && std::memcmp(guid, bytes, 16) == 0;
}


} // namespace

// ---------------------------------------------------------------------------
// Objects
// ---------------------------------------------------------------------------

struct Xa2Voice {
    const void* const* vtable = nullptr;
    Xa2Engine* engine = nullptr;
    Xa2VoiceKind kind = Xa2VoiceKind::Free;
    std::uint32_t index = 0;

    std::uint32_t input_channels = 0;
    std::uint32_t input_rate = 0;
    std::uint32_t bytes_per_frame = 0;
    std::uint32_t creation_flags = 0;
    std::uint32_t processing_stage = 0;

    std::int32_t mixer_voice = 0;
    std::uint32_t bus = 0;

    float volume = 1.0F;
    float pan = 0.0F;
    float ratio = 1.0F;
    float max_ratio = 2.0F;
    float matrix[4] = {1.0F, 0.0F, 0.0F, 1.0F};
    std::uint32_t matrix_source_channels = 2;
    std::uint32_t matrix_destination_channels = 2;

    void* callback = nullptr;

    Xa2EffectDescriptor effects[kMaxEffects]{};
    std::uint32_t effect_count = 0;
    bool effect_enabled[kMaxEffects]{};
    std::uint8_t effect_parameters[kMaxEffects][kEffectParameterBytes]{};
    std::uint32_t effect_parameter_bytes[kMaxEffects]{};

    Xa2SendDescriptor sends[kMaxSends]{};
    std::uint32_t send_count = 0;

    Xa2FilterParameters filter{};
    Xa2FilterParameters output_filter{};

    bool alive = false;
};

struct Xa2Engine {
    const void* const* vtable = nullptr;
    AudioMixer mixer;
    Xa2BridgeConfig config{};
    std::uint32_t ref_count = 1;
    bool destroy_on_release = false;
    bool engine_started = false;
    bool initialized = false;

    Xa2Voice voices[kMaxVoiceSlots]{};
    std::int32_t mastering_index = -1;
    std::uint32_t submix_count = 0;

    void* engine_callbacks[kMaxEngineCallbacks]{};
    std::uint32_t engine_callback_count = 0;

    PendingOp pending[kMaxPendingOps]{};
    std::uint32_t pending_count = 0;

    std::atomic<std::uint64_t> engine_calls{0};
    std::atomic<std::uint64_t> voice_calls{0};
    std::atomic<std::uint64_t> unsupported_slots{0};
    std::atomic<std::uint64_t> rejected_calls{0};
    std::atomic<std::uint64_t> effects_recorded{0};
    std::atomic<std::uint64_t> effect_parameters_recorded{0};
    std::atomic<std::uint64_t> filters_recorded{0};
    std::atomic<std::uint64_t> matrices_applied{0};
    std::atomic<std::uint64_t> matrices_rejected{0};
    std::atomic<std::uint64_t> operation_sets_committed{0};
    std::atomic<std::uint64_t> operation_sets_dropped{0};
    std::atomic<std::uint64_t> buffers_submitted{0};
    std::atomic<std::uint64_t> buffers_rejected{0};
    std::atomic<std::uint64_t> callback_invocations{0};
    std::atomic<std::uint64_t> render_blocks{0};
    std::atomic<std::uint64_t> sends_unapplied{0};
    std::atomic<std::uint64_t> channel_volumes_rejected{0};
    std::atomic<std::uint32_t> live_sources{0};
    std::atomic<std::uint32_t> live_submixes{0};
};

namespace {

extern const void* const kEngineVtable[kEngineVtableEntries];
extern const void* const kSourceVtable[kVoiceVtableEntries];
extern const void* const kSubmVtable[kVoiceVtableEntries];
extern const void* const kMasterVtable[kVoiceVtableEntries];

[[nodiscard]] const void* const* VtableFor(Xa2VoiceKind kind) noexcept {
    switch (kind) {
    case Xa2VoiceKind::Source: return kSourceVtable;
    case Xa2VoiceKind::Submix: return kSubmVtable;
    case Xa2VoiceKind::Mastering: return kMasterVtable;
    case Xa2VoiceKind::Free: break;
    }
    return nullptr;
}

// Slot past the implemented surface: return a failure HRESULT and count it. The
// receiver ignores the arguments, which is why a shorter signature is safe on
// x64; this exists so an out-of-layout call is counted instead of jumping into
// unrelated memory.
std::int32_t UnsupportedSlot(void* self) noexcept {
    (void)self;
    return kXa2InvalidCall;
}

std::int32_t EngineUnsupportedSlot(void* self) noexcept {
    auto* engine = static_cast<Xa2Engine*>(self);
    engine->unsupported_slots.fetch_add(1, std::memory_order_relaxed);
    engine->rejected_calls.fetch_add(1, std::memory_order_relaxed);
    return kXa2InvalidCall;
}

[[nodiscard]] Xa2Engine* EngineOf(void* self) noexcept {
    return static_cast<Xa2Engine*>(self);
}

[[nodiscard]] Xa2Voice* VoiceOf(void* self) noexcept {
    return static_cast<Xa2Voice*>(self);
}

void CountUnsupported(Xa2Engine* engine) noexcept {
    if (engine != nullptr) {
        engine->unsupported_slots.fetch_add(1, std::memory_order_relaxed);
    }
}

void CallVoiceCallback(Xa2Voice* voice, std::uint32_t slot,
                       std::uint32_t word, void* pointer) noexcept {
    if (voice == nullptr || voice->callback == nullptr) return;
    Xa2Engine* engine = voice->engine;
    void** vtable = *reinterpret_cast<void***>(voice->callback);
    if (vtable == nullptr || vtable[slot] == nullptr) return;
    engine->callback_invocations.fetch_add(1, std::memory_order_relaxed);
    switch (slot) {
    case 0:
        reinterpret_cast<void (*)(void*, std::uint32_t)>(vtable[0])(voice->callback, word);
        break;
    case 1:
        reinterpret_cast<void (*)(void*)>(vtable[1])(voice->callback);
        break;
    case 2:
        reinterpret_cast<void (*)(void*)>(vtable[2])(voice->callback);
        break;
    case 3:
    case 4:
    case 5:
        reinterpret_cast<void (*)(void*, void*)>(vtable[slot])(voice->callback, pointer);
        break;
    case 6:
        reinterpret_cast<void (*)(void*, void*, std::int32_t)>(vtable[6])(
            voice->callback, pointer, static_cast<std::int32_t>(word));
        break;
    default:
        break;
    }
}

// --- mixer -> IXAudio2VoiceCallback thunks --------------------------------
void MixerPassStart(void* context, std::uint32_t frames) noexcept {
    auto* voice = static_cast<Xa2Voice*>(context);
    CallVoiceCallback(voice, 0, frames * voice->bytes_per_frame, nullptr);
}

void MixerPassEnd(void* context) noexcept {
    CallVoiceCallback(static_cast<Xa2Voice*>(context), 1, 0, nullptr);
}

void MixerStreamEnd(void* context) noexcept {
    CallVoiceCallback(static_cast<Xa2Voice*>(context), 2, 0, nullptr);
}

void MixerBufferStart(void* context, void* buffer_context) noexcept {
    CallVoiceCallback(static_cast<Xa2Voice*>(context), 3, 0, buffer_context);
}

void MixerBufferEnd(void* context, void* buffer_context) noexcept {
    CallVoiceCallback(static_cast<Xa2Voice*>(context), 4, 0, buffer_context);
}

void MixerLoopEnd(void* context, void* buffer_context) noexcept {
    CallVoiceCallback(static_cast<Xa2Voice*>(context), 5, 0, buffer_context);
}

void MixerVoiceError(void* context, std::uint32_t hresult) noexcept {
    CallVoiceCallback(static_cast<Xa2Voice*>(context), 6, hresult, nullptr);
}

[[nodiscard]] MixerVoiceCallbacks MixerCallbacksFor(Xa2Voice* voice) noexcept {
    MixerVoiceCallbacks callbacks;
    callbacks.context = voice;
    callbacks.voice_processing_pass_start = &MixerPassStart;
    callbacks.voice_processing_pass_end = &MixerPassEnd;
    callbacks.stream_end = &MixerStreamEnd;
    callbacks.buffer_start = &MixerBufferStart;
    callbacks.buffer_end = &MixerBufferEnd;
    callbacks.loop_end = &MixerLoopEnd;
    callbacks.voice_error = &MixerVoiceError;
    return callbacks;
}

// --- voice bookkeeping ----------------------------------------------------
[[nodiscard]] Xa2Voice* FindVoice(Xa2Engine* engine, std::int32_t index) noexcept {
    if (engine == nullptr || index < 0 ||
        static_cast<std::uint32_t>(index) >= kMaxVoiceSlots) {
        return nullptr;
    }
    Xa2Voice& voice = engine->voices[index];
    return voice.alive ? &voice : nullptr;
}

[[nodiscard]] Xa2Voice* VoiceFromInterface(void* interface_pointer) noexcept {
    if (interface_pointer == nullptr) return nullptr;
    auto* voice = static_cast<Xa2Voice*>(interface_pointer);
    const void* const* vtable = voice->vtable;
    if (vtable != kSourceVtable && vtable != kSubmVtable && vtable != kMasterVtable) {
        return nullptr;
    }
    return voice->alive ? voice : nullptr;
}


[[nodiscard]] std::int32_t ResolveBus(Xa2Engine* engine, const Xa2VoiceSends* sends,
                                      std::uint32_t& bus_out) noexcept {
    bus_out = 0;
    if (sends == nullptr || sends->send_count == 0) return kXa2Ok;
    if (sends->sends == nullptr || sends->send_count > kMaxSends) {
        return kXa2InvalidCall;
    }
    Xa2Voice* destination = VoiceFromInterface(sends->sends[0].output_voice);
    if (destination == nullptr || destination->engine != engine) return kXa2InvalidCall;
    if (destination->kind == Xa2VoiceKind::Mastering) {
        bus_out = 0;
        return kXa2Ok;
    }
    if (destination->kind == Xa2VoiceKind::Submix) {
        bus_out = destination->bus;
        return kXa2Ok;
    }
    return kXa2InvalidCall;
}

[[nodiscard]] Xa2Voice* AllocateVoice(Xa2Engine* engine, Xa2VoiceKind kind) noexcept {
    for (std::uint32_t index = 0; index < kMaxVoiceSlots; ++index) {
        Xa2Voice& voice = engine->voices[index];
        if (voice.alive) continue;
        voice = Xa2Voice{};
        voice.vtable = VtableFor(kind);
        voice.engine = engine;
        voice.kind = kind;
        voice.index = index;
        voice.alive = true;
        voice.volume = 1.0F;
        voice.pan = 0.0F;
        voice.ratio = 1.0F;
        voice.matrix[0] = 1.0F;
        voice.matrix[3] = 1.0F;
        return &voice;
    }
    return nullptr;
}

void ReleaseVoice(Xa2Engine* engine, Xa2Voice* voice) noexcept {
    if (engine == nullptr || voice == nullptr) return;
    if (voice->kind == Xa2VoiceKind::Source && voice->mixer_voice > 0) {
        engine->mixer.DestroyVoice(voice->mixer_voice);
        engine->live_sources.fetch_sub(1, std::memory_order_relaxed);
    } else if (voice->kind == Xa2VoiceKind::Submix) {
        engine->mixer.SetBusGain(voice->bus, 1.0F);
        engine->live_submixes.fetch_sub(1, std::memory_order_relaxed);
    } else if (voice->kind == Xa2VoiceKind::Mastering) {
        engine->mastering_index = -1;
    }
    voice->alive = false;
    voice->kind = Xa2VoiceKind::Free;
    voice->vtable = nullptr;
    voice->engine = nullptr;
}

[[nodiscard]] bool PushPending(Xa2Engine* engine, const PendingOp& op) noexcept {
    if (engine->pending_count >= kMaxPendingOps) {
        engine->operation_sets_dropped.fetch_add(1, std::memory_order_relaxed);
        engine->rejected_calls.fetch_add(1, std::memory_order_relaxed);
        return false;
    }
    engine->pending[engine->pending_count++] = op;
    return true;
}

void ApplyVolume(Xa2Voice* voice, float volume, float pan) noexcept {
    Xa2Engine* engine = voice->engine;
    voice->volume = volume;
    voice->pan = pan;
    switch (voice->kind) {
    case Xa2VoiceKind::Source:
        if (voice->mixer_voice > 0) {
            engine->mixer.SetVoiceVolumePan(voice->mixer_voice, volume, pan);
        }
        break;
    case Xa2VoiceKind::Submix:
    case Xa2VoiceKind::Mastering:
        // A bus has no pan law in this mixer; the volume is the bus gain.
        engine->mixer.SetBusGain(voice->bus, volume);
        break;
    case Xa2VoiceKind::Free:
        break;
    }
}

[[nodiscard]] bool ApplyMatrix(Xa2Voice* voice, const float (&matrix)[4]) noexcept {
    Xa2Engine* engine = voice->engine;
    const float first = matrix[0];
    const bool uniform = matrix[1] == first && matrix[2] == first && matrix[3] == first;
    const float maximum =
        matrix[0] > matrix[3] ? matrix[0] : matrix[3];

    if (uniform) {
        voice->matrix[0] = matrix[0];
        voice->matrix[1] = matrix[1];
        voice->matrix[2] = matrix[2];
        voice->matrix[3] = matrix[3];
        ApplyVolume(voice, first, 0.0F);
        engine->matrices_applied.fetch_add(1, std::memory_order_relaxed);
        return true;
    }
    // A diagonal 2x2 matrix is a per-channel gain; express it as the maximum
    // gain and a pan that attenuates the other channel.
    if (matrix[1] == 0.0F && matrix[2] == 0.0F && matrix[0] >= 0.0F &&
        matrix[3] >= 0.0F && maximum > 0.0F) {
        voice->matrix[0] = matrix[0];
        voice->matrix[1] = matrix[1];
        voice->matrix[2] = matrix[2];
        voice->matrix[3] = matrix[3];
        const float pan = (matrix[3] - matrix[0]) / maximum;
        ApplyVolume(voice, maximum, pan);
        engine->matrices_applied.fetch_add(1, std::memory_order_relaxed);
        return true;
    }
    // Not representable: do not record it, so GetOutputMatrix keeps the last
    // matrix that was actually applied.
    engine->matrices_rejected.fetch_add(1, std::memory_order_relaxed);
    engine->rejected_calls.fetch_add(1, std::memory_order_relaxed);
    return false;
}

void ApplyPending(Xa2Engine* engine, PendingOp& op) noexcept {
    (void)engine;
    Xa2Voice* voice = op.voice;
    if (voice == nullptr || !voice->alive) return;
    switch (op.kind) {
    case OpKind::Volume:
        ApplyVolume(voice, op.f0, op.f1);
        break;
    case OpKind::Frequency:
        voice->ratio = op.f0;
        if (voice->mixer_voice > 0) {
            voice->engine->mixer.SetVoiceFrequencyRatio(voice->mixer_voice, op.f0);
        }
        break;
    case OpKind::Matrix:
        (void)ApplyMatrix(voice, op.matrix);
        break;
    case OpKind::Start:
        if (voice->kind == Xa2VoiceKind::Source && voice->mixer_voice > 0) {
            voice->engine->mixer.StartVoice(voice->mixer_voice, 0);
        }
        break;
    case OpKind::Stop:
        if (voice->kind == Xa2VoiceKind::Source && voice->mixer_voice > 0) {
            voice->engine->mixer.StopVoice(voice->mixer_voice);
        }
        break;
    case OpKind::ExitLoop:
        if (voice->kind == Xa2VoiceKind::Source && voice->mixer_voice > 0) {
            voice->engine->mixer.ExitVoiceLoop(voice->mixer_voice);
        }
        break;
    }
}

// --- engine slots ---------------------------------------------------------
std::int32_t EngineQueryInterface(void* self, const void* riid, void** object) noexcept {
    auto* engine = EngineOf(self);
    engine->engine_calls.fetch_add(1, std::memory_order_relaxed);
    if (object == nullptr) return kXa2Pointer;
    if (GuidEquals(riid, kIidIUnknown) || GuidEquals(riid, kIidIXAudio2_27)) {
        *object = self;
        ++engine->ref_count;
        return kXa2Ok;
    }
    *object = nullptr;
    return kXa2NoInterface;
}

std::uint32_t EngineAddRef(void* self) noexcept {
    auto* engine = EngineOf(self);
    return ++engine->ref_count;
}

std::uint32_t EngineRelease(void* self) noexcept {
    auto* engine = EngineOf(self);
    engine->engine_calls.fetch_add(1, std::memory_order_relaxed);
    if (engine->ref_count == 0) return 0;
    const std::uint32_t remaining = --engine->ref_count;
    if (remaining == 0 && engine->destroy_on_release) {
        Xa2BridgeDestroy(engine);
        return 0;
    }
    return remaining;
}

std::int32_t EngineGetDeviceCount(void* self, std::uint32_t* count) noexcept {
    auto* engine = EngineOf(self);
    engine->engine_calls.fetch_add(1, std::memory_order_relaxed);
    if (count == nullptr) return kXa2Pointer;
    *count = 1;
    return kXa2Ok;
}

std::int32_t EngineGetDeviceDetails(void* self, std::uint32_t index,
                                    Xa2DeviceDetails* details) noexcept {
    auto* engine = EngineOf(self);
    engine->engine_calls.fetch_add(1, std::memory_order_relaxed);
    if (index != 0 || details == nullptr) {
        engine->rejected_calls.fetch_add(1, std::memory_order_relaxed);
        return kXa2InvalidCall;
    }
    Xa2DeviceDetails filled;
    const char* name = engine->config.device_name != nullptr ? engine->config.device_name
                                                             : "";
    std::uint32_t length = 0;
    while (name[length] != '\0' && length < 255) ++length;
    for (std::uint32_t i = 0; i < length; ++i) {
        const std::uint16_t character = static_cast<std::uint16_t>(
            static_cast<unsigned char>(name[i]));
        filled.device_id[i] = character;
        filled.display_name[i] = character;
    }
    filled.role = 0; // GlobalDefaultDevice
    filled.output_format.format.format_tag = kWaveFormatExtensible;
    filled.output_format.format.channels =
        static_cast<std::uint16_t>(engine->config.device_channels);
    filled.output_format.format.samples_per_sec = engine->config.device_rate;
    filled.output_format.format.block_align =
        static_cast<std::uint16_t>(engine->config.device_channels * 4U);
    filled.output_format.format.avg_bytes_per_sec =
        engine->config.device_rate * engine->config.device_channels * 4U;
    filled.output_format.format.bits_per_sample = 32;
    filled.output_format.format.cb_size = 22;
    filled.output_format.valid_bits_per_sample = 32;
    filled.output_format.channel_mask =
        engine->config.device_channels == 2 ? 0x3U
                                            : (engine->config.device_channels == 6 ? 0x3FU
                                                                                   : 0U);
    std::memcpy(filled.output_format.sub_format, kFloatSubFormat,
                sizeof(kFloatSubFormat));
    *details = filled;
    return kXa2Ok;
}

std::int32_t EngineInitialize(void* self, std::uint32_t flags,
                              std::uint32_t processor) noexcept {
    auto* engine = EngineOf(self);
    engine->engine_calls.fetch_add(1, std::memory_order_relaxed);
    if (engine->initialized) return kXa2InvalidCall;
    (void)flags;
    (void)processor;
    engine->initialized = true;
    return kXa2Ok;
}

std::int32_t EngineRegisterForCallbacks(void* self, void* callback) noexcept {
    auto* engine = EngineOf(self);
    engine->engine_calls.fetch_add(1, std::memory_order_relaxed);
    if (callback == nullptr) return kXa2Pointer;
    for (std::uint32_t i = 0; i < engine->engine_callback_count; ++i) {
        if (engine->engine_callbacks[i] == callback) return kXa2InvalidCall;
    }
    if (engine->engine_callback_count >= kMaxEngineCallbacks) {
        engine->rejected_calls.fetch_add(1, std::memory_order_relaxed);
        return kXa2InvalidCall;
    }
    engine->engine_callbacks[engine->engine_callback_count++] = callback;
    return kXa2Ok;
}

void EngineUnregisterForCallbacks(void* self, void* callback) noexcept {
    auto* engine = EngineOf(self);
    engine->engine_calls.fetch_add(1, std::memory_order_relaxed);
    for (std::uint32_t i = 0; i < engine->engine_callback_count; ++i) {
        if (engine->engine_callbacks[i] != callback) continue;
        for (std::uint32_t j = i + 1; j < engine->engine_callback_count; ++j) {
            engine->engine_callbacks[j - 1] = engine->engine_callbacks[j];
        }
        engine->engine_callbacks[--engine->engine_callback_count] = nullptr;
        return;
    }
}

[[nodiscard]] std::int32_t ValidateSourceFormat(const Xa2WaveFormat* format,
                                                MixerFormat& out) noexcept {
    if (format == nullptr) return kXa2Pointer;
    if (format->channels == 0 || format->channels > kMixerMaxChannels) {
        return kXa2InvalidCall;
    }
    std::uint16_t tag = format->format_tag;
    std::uint16_t bits = format->bits_per_sample;
    if (tag == kWaveFormatExtensible) {
        if (format->cb_size != 22) return kXa2InvalidCall;
        const auto* extensible = reinterpret_cast<const Xa2WaveFormatExtensible*>(format);
        if (std::memcmp(extensible->sub_format, kPcmSubFormat, 16) == 0) {
            tag = kWaveFormatPcm;
            if (extensible->valid_bits_per_sample != 0) {
                bits = extensible->valid_bits_per_sample;
            }
        } else if (std::memcmp(extensible->sub_format, kFloatSubFormat, 16) == 0) {
            tag = kWaveFormatIeeeFloat;
        } else {
            return kXa2InvalidCall;
        }
    }
    AudioEncoding encoding = AudioEncoding::None;
    if (tag == kWaveFormatPcm) {
        switch (bits) {
        case 16: encoding = AudioEncoding::Int16; break;
        case 24: encoding = AudioEncoding::Int24; break;
        case 32: encoding = AudioEncoding::Int32; break;
        default: return kXa2InvalidCall;
        }
    } else if (tag == kWaveFormatIeeeFloat) {
        if (bits != 32) return kXa2InvalidCall;
        encoding = AudioEncoding::Float32;
    } else {
        // ADPCM, XMA, WMA and every other compressed tag are not decoded here.
        return kXa2InvalidCall;
    }
    if (format->block_align !=
        static_cast<std::uint16_t>(format->channels * (bits / 8U))) {
        return kXa2InvalidCall;
    }
    if (format->samples_per_sec < 8000 || format->samples_per_sec > 384000) {
        return kXa2InvalidCall;
    }
    out.sample_rate = format->samples_per_sec;
    out.channels = format->channels;
    out.encoding = encoding;
    return kXa2Ok;
}

void CopyEffectChain(Xa2Engine* engine, Xa2Voice* voice,
                     const Xa2EffectChain* chain) noexcept {
    voice->effect_count = 0;
    if (chain == nullptr) return;
    if (chain->effect_count > kMaxEffects || (chain->effect_count != 0 &&
                                              chain->descriptors == nullptr)) {
        engine->rejected_calls.fetch_add(1, std::memory_order_relaxed);
        return;
    }
    voice->effect_count = chain->effect_count;
    for (std::uint32_t i = 0; i < chain->effect_count; ++i) {
        voice->effects[i] = chain->descriptors[i];
        voice->effect_enabled[i] = chain->descriptors[i].initial_state != 0;
        voice->effect_parameter_bytes[i] = 0;
    }
    engine->effects_recorded.fetch_add(chain->effect_count, std::memory_order_relaxed);
}

void CopySends(Xa2Engine* engine, Xa2Voice* voice, const Xa2VoiceSends* sends) noexcept {
    voice->send_count = 0;
    if (sends == nullptr) return;
    if (sends->send_count > kMaxSends || (sends->send_count != 0 && sends->sends == nullptr)) {
        engine->rejected_calls.fetch_add(1, std::memory_order_relaxed);
        return;
    }
    voice->send_count = sends->send_count;
    for (std::uint32_t i = 0; i < sends->send_count; ++i) {
        voice->sends[i].flags = sends->sends[i].flags;
        voice->sends[i].output_voice = sends->sends[i].output_voice;
    }
}

// PROTOTYPE-ONLY surface (audio-002): libxact.dll has no CreateSourceVoice call
// site and no WAVEFORMATEX, so nothing here is libxact-compatible. The slot is
// implemented because the mixer and the ASIO host need a source voice.
std::int32_t EngineCreateSourceVoice(void* self, Xa2Voice** source_voice,
                                     const Xa2WaveFormat* format, std::uint32_t flags,
                                     float max_frequency_ratio, void* callback,
                                     const Xa2VoiceSends* sends,
                                     const Xa2EffectChain* effect_chain) noexcept {
    auto* engine = EngineOf(self);
    engine->engine_calls.fetch_add(1, std::memory_order_relaxed);
    if (source_voice == nullptr) return kXa2Pointer;
    *source_voice = nullptr;
    if ((flags & ~static_cast<std::uint32_t>(kXa2VoiceNoPitch | 0x1U)) != 0) {
        engine->rejected_calls.fetch_add(1, std::memory_order_relaxed);
        return kXa2InvalidCall;
    }
    MixerFormat mixer_format;
    std::int32_t status = ValidateSourceFormat(format, mixer_format);
    if (status < 0) {
        engine->rejected_calls.fetch_add(1, std::memory_order_relaxed);
        return status;
    }
    // There is no libxact call site to pin this argument (audio-002 withdrew the
    // earlier claim that 0x180003143 was CreateSourceVoice; it is
    // SetFilterParameters). A caller that leaves MaxFrequencyRatio in whatever
    // register state is common in this code base, so an implausible value is
    // treated as the documented default (2.0) rather than failing the voice.
    if (!(max_frequency_ratio >= 1.0F / 64.0F) || max_frequency_ratio > 1024.0F) {
        max_frequency_ratio = kXa2DefaultFrequencyRatio;
    }
    std::uint32_t bus = 0;
    status = ResolveBus(engine, sends, bus);
    if (status < 0) {
        engine->rejected_calls.fetch_add(1, std::memory_order_relaxed);
        return status;
    }

    Xa2Voice* voice = AllocateVoice(engine, Xa2VoiceKind::Source);
    if (voice == nullptr) {
        engine->rejected_calls.fetch_add(1, std::memory_order_relaxed);
        return kXa2OutOfMemory;
    }
    voice->input_channels = mixer_format.channels;
    voice->input_rate = mixer_format.sample_rate;
    voice->bytes_per_frame = mixer_format.BytesPerFrame();
    voice->creation_flags = flags;
    voice->callback = callback;
    voice->max_ratio = max_frequency_ratio;
    CopyEffectChain(engine, voice, effect_chain);
    CopySends(engine, voice, sends);

    MixerVoiceDesc desc;
    desc.format = mixer_format;
    desc.max_frequency_ratio = max_frequency_ratio;
    desc.bus = bus;
    desc.callbacks = MixerCallbacksFor(voice);
    const std::int32_t mixer_voice = engine->mixer.CreateVoice(desc);
    if (mixer_voice <= 0) {
        voice->alive = false;
        voice->kind = Xa2VoiceKind::Free;
        voice->vtable = nullptr;
        voice->engine = nullptr;
        engine->rejected_calls.fetch_add(1, std::memory_order_relaxed);
        return kXa2OutOfMemory;
    }
    voice->mixer_voice = mixer_voice;
    engine->live_sources.fetch_add(1, std::memory_order_relaxed);
    *source_voice = voice;
    return kXa2Ok;
}

std::int32_t EngineCreateSubmixVoice(void* self, Xa2Voice** submix_voice,
                                     std::uint32_t input_channels,
                                     std::uint32_t input_sample_rate,
                                     std::uint32_t flags,
                                     std::uint32_t processing_stage,
                                     const Xa2VoiceSends* sends,
                                     const Xa2EffectChain* effect_chain) noexcept {
    auto* engine = EngineOf(self);
    engine->engine_calls.fetch_add(1, std::memory_order_relaxed);
    if (submix_voice == nullptr) return kXa2Pointer;
    *submix_voice = nullptr;
    // audio-001 leaves the Flags/ProcessingStage order unproven (the call site
    // shows one stack word of 5 and one of 0). Both are recorded and neither
    // changes what this mixer renders, so any combination is accepted; a stage
    // beyond the documented 0..7 range is refused.
    if (input_channels == 0 || input_channels > kMixerMaxChannels ||
        input_sample_rate != engine->config.device_rate || processing_stage > 7 ||
        flags > 7) {
        engine->rejected_calls.fetch_add(1, std::memory_order_relaxed);
        return kXa2InvalidCall;
    }
    if (engine->submix_count + 1 >= kMixerMaxBuses) {
        engine->rejected_calls.fetch_add(1, std::memory_order_relaxed);
        return kXa2OutOfMemory;
    }
    Xa2Voice* voice = AllocateVoice(engine, Xa2VoiceKind::Submix);
    if (voice == nullptr) {
        engine->rejected_calls.fetch_add(1, std::memory_order_relaxed);
        return kXa2OutOfMemory;
    }
    voice->input_channels = input_channels;
    voice->input_rate = input_sample_rate;
    voice->bytes_per_frame = input_channels * 4U;
    voice->creation_flags = flags;
    voice->processing_stage = processing_stage;
    voice->bus = 1U + engine->submix_count;
    ++engine->submix_count;
    CopyEffectChain(engine, voice, effect_chain);
    CopySends(engine, voice, sends);
    engine->mixer.SetBusGain(voice->bus, 1.0F);
    engine->live_submixes.fetch_add(1, std::memory_order_relaxed);
    *submix_voice = voice;
    return kXa2Ok;
}

std::int32_t EngineCreateMasteringVoice(void* self, Xa2Voice** mastering_voice,
                                        std::uint32_t input_channels,
                                        std::uint32_t input_sample_rate,
                                        std::uint32_t flags, std::uint32_t device_index,
                                        const Xa2EffectChain* effect_chain) noexcept {
    auto* engine = EngineOf(self);
    engine->engine_calls.fetch_add(1, std::memory_order_relaxed);
    if (mastering_voice == nullptr) return kXa2Pointer;
    *mastering_voice = nullptr;
    if (engine->mastering_index >= 0) {
        engine->rejected_calls.fetch_add(1, std::memory_order_relaxed);
        return kXa2InvalidCall;
    }
    if (flags != 0 || device_index != 0) {
        engine->rejected_calls.fetch_add(1, std::memory_order_relaxed);
        return kXa2InvalidCall;
    }
    // No resampling and no remixing: the requested device shape must be the one
    // the mixer was prepared with, otherwise the bridge fails and the host must
    // reconfigure.
    if (input_channels != engine->config.device_channels ||
        (input_sample_rate != 0 && input_sample_rate != engine->config.device_rate)) {
        engine->rejected_calls.fetch_add(1, std::memory_order_relaxed);
        return kXa2InvalidCall;
    }
    Xa2Voice* voice = AllocateVoice(engine, Xa2VoiceKind::Mastering);
    if (voice == nullptr) {
        engine->rejected_calls.fetch_add(1, std::memory_order_relaxed);
        return kXa2OutOfMemory;
    }
    voice->input_channels =
        input_channels != 0 ? input_channels : engine->config.device_channels;
    voice->input_rate =
        input_sample_rate != 0 ? input_sample_rate : engine->config.device_rate;
    voice->bytes_per_frame = voice->input_channels * 4U;
    voice->bus = 0;
    CopyEffectChain(engine, voice, effect_chain);
    engine->mastering_index = static_cast<std::int32_t>(voice->index);
    *mastering_voice = voice;
    return kXa2Ok;
}

std::int32_t EngineStartEngine(void* self) noexcept {
    auto* engine = EngineOf(self);
    engine->engine_calls.fetch_add(1, std::memory_order_relaxed);
    engine->engine_started = true;
    return kXa2Ok;
}

void EngineStopEngine(void* self) noexcept {
    auto* engine = EngineOf(self);
    engine->engine_calls.fetch_add(1, std::memory_order_relaxed);
    engine->engine_started = false;
    for (auto& voice : engine->voices) {
        if (voice.alive && voice.kind == Xa2VoiceKind::Source && voice.mixer_voice > 0) {
            engine->mixer.StopVoice(voice.mixer_voice);
        }
    }
}

std::int32_t EngineCommitChanges(void* self, std::uint32_t operation_set) noexcept {
    auto* engine = EngineOf(self);
    engine->engine_calls.fetch_add(1, std::memory_order_relaxed);
    if (operation_set == 0) {
        engine->rejected_calls.fetch_add(1, std::memory_order_relaxed);
        return kXa2InvalidCall;
    }
    const bool all = operation_set == 0xFFFFFFFFU;
    std::uint32_t kept = 0;
    for (std::uint32_t i = 0; i < engine->pending_count; ++i) {
        PendingOp& op = engine->pending[i];
        if (all || op.operation_set == operation_set) {
            ApplyPending(engine, op);
            engine->operation_sets_committed.fetch_add(1, std::memory_order_relaxed);
        } else {
            engine->pending[kept++] = op;
        }
    }
    for (std::uint32_t i = kept; i < engine->pending_count; ++i) {
        engine->pending[i] = PendingOp{};
    }
    engine->pending_count = kept;
    return kXa2Ok;
}

void EngineGetPerformanceData(void* self, void* performance_data) noexcept {
    auto* engine = EngineOf(self);
    engine->engine_calls.fetch_add(1, std::memory_order_relaxed);
    // XAUDIO2_PERFORMANCE_DATA has no 2.7-stable size in this tree; writing a
    // guessed size could corrupt the caller's stack. The call is counted only.
    (void)performance_data;
}

void EngineSetDebugConfiguration(void* self, const void* configuration,
                                 void* reserved) noexcept {
    auto* engine = EngineOf(self);
    engine->engine_calls.fetch_add(1, std::memory_order_relaxed);
    (void)configuration;
    (void)reserved;
}

// --- voice slots ----------------------------------------------------------
std::int32_t VoiceUnsupported(void* self) noexcept {
    Xa2Voice* voice = VoiceOf(self);
    CountUnsupported(voice != nullptr ? voice->engine : nullptr);
    return kXa2InvalidCall;
}

std::int32_t VoiceSourceOnly(void* self) noexcept {
    Xa2Voice* voice = VoiceOf(self);
    CountUnsupported(voice != nullptr ? voice->engine : nullptr);
    return kXa2InvalidCall;
}

std::int32_t VoiceGetVoiceDetails(void* self, Xa2VoiceDetails* details) noexcept {
    Xa2Voice* voice = VoiceOf(self);
    voice->engine->voice_calls.fetch_add(1, std::memory_order_relaxed);
    if (details == nullptr) return kXa2Pointer;
    details->creation_flags = voice->creation_flags;
    details->active_flags = 0;
    details->input_channels = voice->input_channels;
    details->input_sample_rate = voice->input_rate;
    return kXa2Ok;
}

std::int32_t VoiceSetOutputVoices(void* self, const Xa2VoiceSends* sends) noexcept {
    Xa2Voice* voice = VoiceOf(self);
    Xa2Engine* engine = voice->engine;
    engine->voice_calls.fetch_add(1, std::memory_order_relaxed);
    if (sends != nullptr && sends->send_count > kMaxSends) {
        engine->rejected_calls.fetch_add(1, std::memory_order_relaxed);
        return kXa2InvalidCall;
    }
    // Routing was fixed at creation; a later change is recorded but not applied.
    if (voice->send_count != 0) {
        engine->sends_unapplied.fetch_add(1, std::memory_order_relaxed);
    }
    CopySends(engine, voice, sends);
    return kXa2Ok;
}

std::int32_t VoiceSetEffectChain(void* self, const Xa2EffectChain* chain) noexcept {
    Xa2Voice* voice = VoiceOf(self);
    voice->engine->voice_calls.fetch_add(1, std::memory_order_relaxed);
    CopyEffectChain(voice->engine, voice, chain);
    return kXa2Ok;
}

std::int32_t VoiceEnableEffect(void* self, std::uint32_t index,
                               std::uint32_t operation_set) noexcept {
    Xa2Voice* voice = VoiceOf(self);
    voice->engine->voice_calls.fetch_add(1, std::memory_order_relaxed);
    (void)operation_set;
    if (index >= voice->effect_count) {
        voice->engine->rejected_calls.fetch_add(1, std::memory_order_relaxed);
        return kXa2InvalidCall;
    }
    voice->effect_enabled[index] = true;
    return kXa2Ok;
}

std::int32_t VoiceDisableEffect(void* self, std::uint32_t index,
                                std::uint32_t operation_set) noexcept {
    Xa2Voice* voice = VoiceOf(self);
    voice->engine->voice_calls.fetch_add(1, std::memory_order_relaxed);
    (void)operation_set;
    if (index >= voice->effect_count) {
        voice->engine->rejected_calls.fetch_add(1, std::memory_order_relaxed);
        return kXa2InvalidCall;
    }
    voice->effect_enabled[index] = false;
    return kXa2Ok;
}

void VoiceGetEffectState(void* self, std::uint32_t index, std::int32_t* enabled) noexcept {
    Xa2Voice* voice = VoiceOf(self);
    voice->engine->voice_calls.fetch_add(1, std::memory_order_relaxed);
    if (enabled == nullptr) return;
    *enabled = index < voice->effect_count && voice->effect_enabled[index] ? 1 : 0;
}

std::int32_t VoiceSetEffectParameters(void* self, std::uint32_t index,
                                      const void* parameters, std::uint32_t frames,
                                      std::uint32_t operation_set) noexcept {
    Xa2Voice* voice = VoiceOf(self);
    Xa2Engine* engine = voice->engine;
    engine->voice_calls.fetch_add(1, std::memory_order_relaxed);
    (void)frames;
    (void)operation_set;
    if (index >= voice->effect_count) {
        engine->rejected_calls.fetch_add(1, std::memory_order_relaxed);
        return kXa2InvalidCall;
    }
    // The parameter block size is not known to this bridge (the client owns the
    // XAPO structure definition), so it is never copied: only the fact that
    // parameters were set is recorded. No effect DSP runs, so GetEffectParameters
    // returns zeros. Reading 32 bytes from an unknown block would over-read the
    // client's stack.
    voice->effect_parameter_bytes[index] = parameters != nullptr ? 1U : 0U;
    if (parameters != nullptr) {
        std::memset(voice->effect_parameters[index], 0,
                    sizeof(voice->effect_parameters[index]));
    }
    engine->effect_parameters_recorded.fetch_add(1, std::memory_order_relaxed);
    return kXa2Ok;
}

std::int32_t VoiceGetEffectParameters(void* self, std::uint32_t index,
                                      void* parameters, std::uint32_t bytes) noexcept {
    Xa2Voice* voice = VoiceOf(self);
    voice->engine->voice_calls.fetch_add(1, std::memory_order_relaxed);
    if (index >= voice->effect_count) {
        voice->engine->rejected_calls.fetch_add(1, std::memory_order_relaxed);
        return kXa2InvalidCall;
    }
    if (parameters != nullptr && bytes != 0) {
        std::memset(parameters, 0, bytes);
    }
    return kXa2Ok;
}

// PROVEN libxact path (audio-002): the three parallel 12-byte records
// {int Type; float Frequency; float OneOverQ} at CLibXact +0x58/+0x64/+0x70 are
// handed to the voices at +0x15970/+0x15978/+0x15980 through slot +0x40 with
// OperationSet = 0. The parameters are recorded; no filter DSP runs in this
// mixer, so a filtered cue sounds unfiltered here (counted in filters_recorded).
std::int32_t VoiceSetFilterParameters(void* self,
                                      const Xa2FilterParameters* parameters,
                                      std::uint32_t operation_set) noexcept {
    Xa2Voice* voice = VoiceOf(self);
    Xa2Engine* engine = voice->engine;
    engine->voice_calls.fetch_add(1, std::memory_order_relaxed);
    (void)operation_set;
    if (parameters == nullptr) return kXa2Pointer;
    voice->filter = *parameters;
    engine->filters_recorded.fetch_add(1, std::memory_order_relaxed);
    return kXa2Ok;
}

void VoiceGetFilterParameters(void* self, Xa2FilterParameters* parameters) noexcept {
    Xa2Voice* voice = VoiceOf(self);
    voice->engine->voice_calls.fetch_add(1, std::memory_order_relaxed);
    if (parameters != nullptr) *parameters = voice->filter;
}

std::int32_t VoiceSetOutputFilterParameters(void* self, void* destination,
                                            const Xa2FilterParameters* parameters,
                                            std::uint32_t operation_set) noexcept {
    Xa2Voice* voice = VoiceOf(self);
    voice->engine->voice_calls.fetch_add(1, std::memory_order_relaxed);
    (void)destination;
    (void)operation_set;
    if (parameters == nullptr) return kXa2Pointer;
    voice->output_filter = *parameters;
    voice->engine->filters_recorded.fetch_add(1, std::memory_order_relaxed);
    return kXa2Ok;
}

void VoiceGetOutputFilterParameters(void* self, void* destination,
                                    Xa2FilterParameters* parameters) noexcept {
    Xa2Voice* voice = VoiceOf(self);
    voice->engine->voice_calls.fetch_add(1, std::memory_order_relaxed);
    (void)destination;
    if (parameters != nullptr) *parameters = voice->output_filter;
}

std::int32_t VoiceSetVolume(void* self, float volume, std::uint32_t operation_set) noexcept {
    Xa2Voice* voice = VoiceOf(self);
    Xa2Engine* engine = voice->engine;
    engine->voice_calls.fetch_add(1, std::memory_order_relaxed);
    if (!(volume >= 0.0F) || volume > 64.0F) {
        engine->rejected_calls.fetch_add(1, std::memory_order_relaxed);
        return kXa2InvalidArgument;
    }
    if (operation_set != kXa2CommitNow) {
        PendingOp op;
        op.kind = OpKind::Volume;
        op.voice = voice;
        op.operation_set = operation_set;
        op.f0 = volume;
        op.f1 = voice->pan;
        return PushPending(engine, op) ? kXa2Ok : kXa2InvalidCall;
    }
    ApplyVolume(voice, volume, voice->pan);
    return kXa2Ok;
}

void VoiceGetVolume(void* self, float* volume) noexcept {
    Xa2Voice* voice = VoiceOf(self);
    voice->engine->voice_calls.fetch_add(1, std::memory_order_relaxed);
    if (volume != nullptr) *volume = voice->volume;
}

std::int32_t VoiceSetChannelVolumes(void* self, std::uint32_t channels,
                                    const float* volumes,
                                    std::uint32_t operation_set) noexcept {
    Xa2Voice* voice = VoiceOf(self);
    Xa2Engine* engine = voice->engine;
    engine->voice_calls.fetch_add(1, std::memory_order_relaxed);
    if (channels == 0 || volumes == nullptr || channels != voice->input_channels) {
        engine->rejected_calls.fetch_add(1, std::memory_order_relaxed);
        return kXa2InvalidCall;
    }
    // Only a uniform channel volume is representable without a full matrix.
    for (std::uint32_t i = 1; i < channels; ++i) {
        if (volumes[i] != volumes[0]) {
            engine->channel_volumes_rejected.fetch_add(1, std::memory_order_relaxed);
            engine->rejected_calls.fetch_add(1, std::memory_order_relaxed);
            return kXa2InvalidCall;
        }
    }
    return VoiceSetVolume(self, volumes[0], operation_set);
}

void VoiceGetChannelVolumes(void* self, std::uint32_t channels,
                            float* volumes) noexcept {
    Xa2Voice* voice = VoiceOf(self);
    voice->engine->voice_calls.fetch_add(1, std::memory_order_relaxed);
    if (volumes == nullptr) return;
    for (std::uint32_t i = 0; i < channels; ++i) {
        volumes[i] = voice->volume;
    }
}

std::int32_t VoiceSetOutputMatrix(void* self, void* destination,
                                  std::uint32_t source_channels,
                                  std::uint32_t destination_channels,
                                  const float* matrix,
                                  std::uint32_t operation_set) noexcept {
    Xa2Voice* voice = VoiceOf(self);
    Xa2Engine* engine = voice->engine;
    engine->voice_calls.fetch_add(1, std::memory_order_relaxed);
    if (matrix == nullptr || source_channels == 0 || destination_channels == 0 ||
        source_channels > 8 || destination_channels > 8) {
        engine->rejected_calls.fetch_add(1, std::memory_order_relaxed);
        return kXa2InvalidCall;
    }
    // The matrix belongs to `this` (the source voice) and describes how it feeds
    // `destination`. The destination is only validated.
    if (destination != nullptr) {
        Xa2Voice* target = VoiceFromInterface(destination);
        if (target == nullptr || target->engine != engine) {
            engine->rejected_calls.fetch_add(1, std::memory_order_relaxed);
            return kXa2InvalidCall;
        }
    }
    if (source_channels != 2 || destination_channels != 2) {
        engine->matrices_rejected.fetch_add(1, std::memory_order_relaxed);
        engine->rejected_calls.fetch_add(1, std::memory_order_relaxed);
        return kXa2InvalidCall;
    }
    float values[4];
    std::memcpy(values, matrix, sizeof(values));
    for (float value : values) {
        if (!(value >= 0.0F) || value > 64.0F) {
            engine->matrices_rejected.fetch_add(1, std::memory_order_relaxed);
            engine->rejected_calls.fetch_add(1, std::memory_order_relaxed);
            return kXa2InvalidArgument;
        }
    }
    if (operation_set != kXa2CommitNow) {
        PendingOp op;
        op.kind = OpKind::Matrix;
        op.voice = voice;
        op.destination = nullptr;
        op.operation_set = operation_set;
        std::memcpy(op.matrix, values, sizeof(values));
        return PushPending(engine, op) ? kXa2Ok : kXa2InvalidCall;
    }
    return ApplyMatrix(voice, values) ? kXa2Ok : kXa2InvalidCall;
}

void VoiceGetOutputMatrix(void* self, void* destination, std::uint32_t source_channels,
                          std::uint32_t destination_channels, float* matrix) noexcept {
    Xa2Voice* voice = VoiceOf(self);
    voice->engine->voice_calls.fetch_add(1, std::memory_order_relaxed);
    (void)destination;
    if (matrix == nullptr) return;
    for (std::uint32_t channel = 0; channel < source_channels; ++channel) {
        for (std::uint32_t output = 0; output < destination_channels; ++output) {
            const std::uint32_t index = channel * destination_channels + output;
            matrix[index] = index < 4 ? voice->matrix[index] : 0.0F;
        }
    }
}

void VoiceDestroy(void* self) noexcept {
    Xa2Voice* voice = VoiceOf(self);
    Xa2Engine* engine = voice->engine;
    engine->voice_calls.fetch_add(1, std::memory_order_relaxed);
    ReleaseVoice(engine, voice);
}

std::int32_t VoiceStart(void* self, std::uint32_t flags,
                        std::uint32_t operation_set) noexcept {
    Xa2Voice* voice = VoiceOf(self);
    Xa2Engine* engine = voice->engine;
    engine->voice_calls.fetch_add(1, std::memory_order_relaxed);
    if (voice->kind != Xa2VoiceKind::Source || flags != 0) {
        engine->rejected_calls.fetch_add(1, std::memory_order_relaxed);
        return kXa2InvalidCall;
    }
    if (operation_set != kXa2CommitNow) {
        PendingOp op;
        op.kind = OpKind::Start;
        op.voice = voice;
        op.operation_set = operation_set;
        return PushPending(engine, op) ? kXa2Ok : kXa2InvalidCall;
    }
    engine->mixer.StartVoice(voice->mixer_voice, 0);
    return kXa2Ok;
}

std::int32_t VoiceStop(void* self, std::uint32_t flags,
                       std::uint32_t operation_set) noexcept {
    Xa2Voice* voice = VoiceOf(self);
    Xa2Engine* engine = voice->engine;
    engine->voice_calls.fetch_add(1, std::memory_order_relaxed);
    if (voice->kind != Xa2VoiceKind::Source || flags != 0) {
        engine->rejected_calls.fetch_add(1, std::memory_order_relaxed);
        return kXa2InvalidCall;
    }
    if (operation_set != kXa2CommitNow) {
        PendingOp op;
        op.kind = OpKind::Stop;
        op.voice = voice;
        op.operation_set = operation_set;
        return PushPending(engine, op) ? kXa2Ok : kXa2InvalidCall;
    }
    engine->mixer.StopVoice(voice->mixer_voice);
    return kXa2Ok;
}

// PROTOTYPE-ONLY surface (audio-002): libxact never submits a buffer and the
// module contains no XAUDIO2_END_OF_STREAM. Kept for this stream's host/tests.
std::int32_t VoiceSubmitSourceBuffer(void* self, const Xa2Buffer* buffer,
                                     const void* buffer_wma) noexcept {
    Xa2Voice* voice = VoiceOf(self);
    Xa2Engine* engine = voice->engine;
    engine->voice_calls.fetch_add(1, std::memory_order_relaxed);
    if (voice->kind != Xa2VoiceKind::Source) {
        engine->rejected_calls.fetch_add(1, std::memory_order_relaxed);
        return kXa2InvalidCall;
    }
    if (buffer == nullptr || buffer->audio_data == nullptr || buffer_wma != nullptr) {
        engine->rejected_calls.fetch_add(1, std::memory_order_relaxed);
        return kXa2Pointer;
    }
    if ((buffer->flags & ~static_cast<std::uint32_t>(kXa2EndOfStream)) != 0 ||
        voice->bytes_per_frame == 0) {
        engine->rejected_calls.fetch_add(1, std::memory_order_relaxed);
        return kXa2InvalidCall;
    }
    const std::uint32_t frames = buffer->audio_bytes / voice->bytes_per_frame;
    MixerBufferDesc desc;
    desc.data = buffer->audio_data;
    desc.frame_count = frames;
    desc.play_begin = buffer->play_begin;
    desc.play_length = buffer->play_length;
    desc.loop_begin = buffer->loop_begin;
    desc.loop_length = buffer->loop_length;
    desc.loop_count = buffer->loop_count;
    desc.flags = buffer->flags;
    desc.context = buffer->context;
    if (!engine->mixer.SubmitBuffer(voice->mixer_voice, desc)) {
        engine->buffers_rejected.fetch_add(1, std::memory_order_relaxed);
        engine->rejected_calls.fetch_add(1, std::memory_order_relaxed);
        return kXa2InvalidCall;
    }
    engine->buffers_submitted.fetch_add(1, std::memory_order_relaxed);
    return kXa2Ok;
}

std::int32_t VoiceFlushSourceBuffers(void* self) noexcept {
    Xa2Voice* voice = VoiceOf(self);
    Xa2Engine* engine = voice->engine;
    engine->voice_calls.fetch_add(1, std::memory_order_relaxed);
    if (voice->kind != Xa2VoiceKind::Source) {
        engine->rejected_calls.fetch_add(1, std::memory_order_relaxed);
        return kXa2InvalidCall;
    }
    engine->mixer.FlushVoice(voice->mixer_voice);
    return kXa2Ok;
}

std::int32_t VoiceDiscontinuity(void* self) noexcept {
    Xa2Voice* voice = VoiceOf(self);
    voice->engine->voice_calls.fetch_add(1, std::memory_order_relaxed);
    if (voice->kind != Xa2VoiceKind::Source) {
        voice->engine->rejected_calls.fetch_add(1, std::memory_order_relaxed);
        return kXa2InvalidCall;
    }
    return kXa2Ok;
}

std::int32_t VoiceExitLoop(void* self, std::uint32_t operation_set) noexcept {
    Xa2Voice* voice = VoiceOf(self);
    Xa2Engine* engine = voice->engine;
    engine->voice_calls.fetch_add(1, std::memory_order_relaxed);
    if (voice->kind != Xa2VoiceKind::Source) {
        engine->rejected_calls.fetch_add(1, std::memory_order_relaxed);
        return kXa2InvalidCall;
    }
    if (operation_set != kXa2CommitNow) {
        PendingOp op;
        op.kind = OpKind::ExitLoop;
        op.voice = voice;
        op.operation_set = operation_set;
        return PushPending(engine, op) ? kXa2Ok : kXa2InvalidCall;
    }
    engine->mixer.ExitVoiceLoop(voice->mixer_voice);
    return kXa2Ok;
}

void VoiceGetState(void* self, Xa2VoiceState* state, std::uint32_t flags) noexcept {
    Xa2Voice* voice = VoiceOf(self);
    Xa2Engine* engine = voice->engine;
    engine->voice_calls.fetch_add(1, std::memory_order_relaxed);
    if (state == nullptr) return;
    std::memset(state, 0, sizeof(*state));
    if (voice->kind != Xa2VoiceKind::Source) return;
    const MixerVoiceState mixer_state = engine->mixer.GetState(voice->mixer_voice);
    state->current_buffer_context = mixer_state.current_buffer_context;
    state->buffers_queued = mixer_state.buffers_queued;
    state->samples_played =
        (flags & kXa2VoiceNoSamplesPlayed) != 0 ? 0 : mixer_state.samples_played;
}

std::int32_t VoiceSetFrequencyRatio(void* self, float ratio,
                                    std::uint32_t operation_set) noexcept {
    Xa2Voice* voice = VoiceOf(self);
    Xa2Engine* engine = voice->engine;
    engine->voice_calls.fetch_add(1, std::memory_order_relaxed);
    if (voice->kind != Xa2VoiceKind::Source) {
        engine->rejected_calls.fetch_add(1, std::memory_order_relaxed);
        return kXa2InvalidCall;
    }
    if (!(ratio >= 1.0F / 64.0F) || ratio > voice->max_ratio) {
        engine->rejected_calls.fetch_add(1, std::memory_order_relaxed);
        return kXa2InvalidArgument;
    }
    if (operation_set != kXa2CommitNow) {
        PendingOp op;
        op.kind = OpKind::Frequency;
        op.voice = voice;
        op.operation_set = operation_set;
        op.f0 = ratio;
        return PushPending(engine, op) ? kXa2Ok : kXa2InvalidCall;
    }
    voice->ratio = ratio;
    engine->mixer.SetVoiceFrequencyRatio(voice->mixer_voice, ratio);
    return kXa2Ok;
}

void VoiceGetFrequencyRatio(void* self, float* ratio) noexcept {
    Xa2Voice* voice = VoiceOf(self);
    voice->engine->voice_calls.fetch_add(1, std::memory_order_relaxed);
    if (ratio != nullptr) *ratio = voice->ratio;
}

std::int32_t VoiceSetSourceSampleRate(void* self, std::uint32_t rate) noexcept {
    Xa2Voice* voice = VoiceOf(self);
    Xa2Engine* engine = voice->engine;
    engine->voice_calls.fetch_add(1, std::memory_order_relaxed);
    (void)rate;
    // A live source-rate change would need a mixer-side rebuild of the resample
    // step and the read cursor. Refused rather than mis-played.
    engine->rejected_calls.fetch_add(1, std::memory_order_relaxed);
    return kXa2InvalidCall;
}

std::int32_t MasteringGetChannelMask(void* self, std::uint32_t* mask) noexcept {
    Xa2Voice* voice = VoiceOf(self);
    voice->engine->voice_calls.fetch_add(1, std::memory_order_relaxed);
    if (mask == nullptr) return kXa2Pointer;
    *mask = voice->input_channels == 2 ? 0x3U
                                       : (voice->input_channels == 6 ? 0x3FU : 0U);
    return kXa2Ok;
}

// --- vtables --------------------------------------------------------------
#define NOSFIX_SLOT(function) reinterpret_cast<const void*>(&function)

const void* const kEngineVtable[kEngineVtableEntries] = {
    /*   0 */ NOSFIX_SLOT(EngineQueryInterface),
    /*   8 */ NOSFIX_SLOT(EngineAddRef),
    /*  16 */ NOSFIX_SLOT(EngineRelease),
    /*  24 */ NOSFIX_SLOT(EngineGetDeviceCount),
    /*  32 */ NOSFIX_SLOT(EngineGetDeviceDetails),
    /*  40 */ NOSFIX_SLOT(EngineInitialize),
    /*  48 */ NOSFIX_SLOT(EngineRegisterForCallbacks),
    /*  56 */ NOSFIX_SLOT(EngineUnregisterForCallbacks),
    /*  64 */ NOSFIX_SLOT(EngineCreateSourceVoice),
    /*  72 */ NOSFIX_SLOT(EngineCreateSubmixVoice),
    /*  80 */ NOSFIX_SLOT(EngineCreateMasteringVoice),
    /*  88 */ NOSFIX_SLOT(EngineStartEngine),
    /*  96 */ NOSFIX_SLOT(EngineStopEngine),
    /* 104 */ NOSFIX_SLOT(EngineCommitChanges),
    /* 112 */ NOSFIX_SLOT(EngineGetPerformanceData),
    /* 120 */ NOSFIX_SLOT(EngineSetDebugConfiguration),
    /* 128 */ NOSFIX_SLOT(EngineUnsupportedSlot),
    /* 136 */ NOSFIX_SLOT(EngineUnsupportedSlot),
    /* 144 */ NOSFIX_SLOT(EngineUnsupportedSlot),
    /* 152 */ NOSFIX_SLOT(EngineUnsupportedSlot),
    /* 160 */ NOSFIX_SLOT(EngineUnsupportedSlot),
    /* 168 */ NOSFIX_SLOT(EngineUnsupportedSlot),
    /* 176 */ NOSFIX_SLOT(EngineUnsupportedSlot),
    /* 184 */ NOSFIX_SLOT(EngineUnsupportedSlot),
};

const void* const kSourceVtable[kVoiceVtableEntries] = {
    /*   0 */ NOSFIX_SLOT(VoiceGetVoiceDetails),
    /*   8 */ NOSFIX_SLOT(VoiceSetOutputVoices),
    /*  16 */ NOSFIX_SLOT(VoiceSetEffectChain),
    /*  24 */ NOSFIX_SLOT(VoiceEnableEffect),
    /*  32 */ NOSFIX_SLOT(VoiceDisableEffect),
    /*  40 */ NOSFIX_SLOT(VoiceGetEffectState),
    /*  48 */ NOSFIX_SLOT(VoiceSetEffectParameters),
    /*  56 */ NOSFIX_SLOT(VoiceGetEffectParameters),
    /*  64 */ NOSFIX_SLOT(VoiceSetFilterParameters),
    /*  72 */ NOSFIX_SLOT(VoiceGetFilterParameters),
    /*  80 */ NOSFIX_SLOT(VoiceSetOutputFilterParameters),
    /*  88 */ NOSFIX_SLOT(VoiceGetOutputFilterParameters),
    /*  96 */ NOSFIX_SLOT(VoiceSetVolume),
    /* 104 */ NOSFIX_SLOT(VoiceGetVolume),
    /* 112 */ NOSFIX_SLOT(VoiceSetChannelVolumes),
    /* 120 */ NOSFIX_SLOT(VoiceGetChannelVolumes),
    /* 128 */ NOSFIX_SLOT(VoiceSetOutputMatrix),
    /* 136 */ NOSFIX_SLOT(VoiceGetOutputMatrix),
    /* 144 */ NOSFIX_SLOT(VoiceDestroy),
    /* 152 */ NOSFIX_SLOT(VoiceStart),
    /* 160 */ NOSFIX_SLOT(VoiceStop),
    /* 168 */ NOSFIX_SLOT(VoiceSubmitSourceBuffer),
    /* 176 */ NOSFIX_SLOT(VoiceFlushSourceBuffers),
    /* 184 */ NOSFIX_SLOT(VoiceDiscontinuity),
    /* 192 */ NOSFIX_SLOT(VoiceExitLoop),
    /* 200 */ NOSFIX_SLOT(VoiceGetState),
    /* 208 */ NOSFIX_SLOT(VoiceSetFrequencyRatio),
    /* 216 */ NOSFIX_SLOT(VoiceGetFrequencyRatio),
    /* 224 */ NOSFIX_SLOT(VoiceSetSourceSampleRate),
    /* 232 */ NOSFIX_SLOT(VoiceUnsupported),
    /* 240 */ NOSFIX_SLOT(VoiceUnsupported),
    /* 248 */ NOSFIX_SLOT(VoiceUnsupported),
    /* 256 */ NOSFIX_SLOT(VoiceUnsupported),
    /* 264 */ NOSFIX_SLOT(VoiceUnsupported),
    /* 272 */ NOSFIX_SLOT(VoiceUnsupported),
    /* 280 */ NOSFIX_SLOT(VoiceUnsupported),
    /* 288 */ NOSFIX_SLOT(VoiceUnsupported),
};

const void* const kSubmVtable[kVoiceVtableEntries] = {
    /*   0 */ NOSFIX_SLOT(VoiceGetVoiceDetails),
    /*   8 */ NOSFIX_SLOT(VoiceSetOutputVoices),
    /*  16 */ NOSFIX_SLOT(VoiceSetEffectChain),
    /*  24 */ NOSFIX_SLOT(VoiceEnableEffect),
    /*  32 */ NOSFIX_SLOT(VoiceDisableEffect),
    /*  40 */ NOSFIX_SLOT(VoiceGetEffectState),
    /*  48 */ NOSFIX_SLOT(VoiceSetEffectParameters),
    /*  56 */ NOSFIX_SLOT(VoiceGetEffectParameters),
    /*  64 */ NOSFIX_SLOT(VoiceSetFilterParameters),
    /*  72 */ NOSFIX_SLOT(VoiceGetFilterParameters),
    /*  80 */ NOSFIX_SLOT(VoiceSetOutputFilterParameters),
    /*  88 */ NOSFIX_SLOT(VoiceGetOutputFilterParameters),
    /*  96 */ NOSFIX_SLOT(VoiceSetVolume),
    /* 104 */ NOSFIX_SLOT(VoiceGetVolume),
    /* 112 */ NOSFIX_SLOT(VoiceSetChannelVolumes),
    /* 120 */ NOSFIX_SLOT(VoiceGetChannelVolumes),
    /* 128 */ NOSFIX_SLOT(VoiceSetOutputMatrix),
    /* 136 */ NOSFIX_SLOT(VoiceGetOutputMatrix),
    /* 144 */ NOSFIX_SLOT(VoiceDestroy),
    /* 152 */ NOSFIX_SLOT(VoiceSourceOnly),
    /* 160 */ NOSFIX_SLOT(VoiceSourceOnly),
    /* 168 */ NOSFIX_SLOT(VoiceSourceOnly),
    /* 176 */ NOSFIX_SLOT(VoiceSourceOnly),
    /* 184 */ NOSFIX_SLOT(VoiceSourceOnly),
    /* 192 */ NOSFIX_SLOT(VoiceSourceOnly),
    /* 200 */ NOSFIX_SLOT(VoiceSourceOnly),
    /* 208 */ NOSFIX_SLOT(VoiceSourceOnly),
    /* 216 */ NOSFIX_SLOT(VoiceSourceOnly),
    /* 224 */ NOSFIX_SLOT(VoiceSourceOnly),
    /* 232 */ NOSFIX_SLOT(VoiceUnsupported),
    /* 240 */ NOSFIX_SLOT(VoiceUnsupported),
    /* 248 */ NOSFIX_SLOT(VoiceUnsupported),
    /* 256 */ NOSFIX_SLOT(VoiceUnsupported),
    /* 264 */ NOSFIX_SLOT(VoiceUnsupported),
    /* 272 */ NOSFIX_SLOT(VoiceUnsupported),
    /* 280 */ NOSFIX_SLOT(VoiceUnsupported),
    /* 288 */ NOSFIX_SLOT(VoiceUnsupported),
};

const void* const kMasterVtable[kVoiceVtableEntries] = {
    /*   0 */ NOSFIX_SLOT(VoiceGetVoiceDetails),
    /*   8 */ NOSFIX_SLOT(VoiceSetOutputVoices),
    /*  16 */ NOSFIX_SLOT(VoiceSetEffectChain),
    /*  24 */ NOSFIX_SLOT(VoiceEnableEffect),
    /*  32 */ NOSFIX_SLOT(VoiceDisableEffect),
    /*  40 */ NOSFIX_SLOT(VoiceGetEffectState),
    /*  48 */ NOSFIX_SLOT(VoiceSetEffectParameters),
    /*  56 */ NOSFIX_SLOT(VoiceGetEffectParameters),
    /*  64 */ NOSFIX_SLOT(VoiceSetFilterParameters),
    /*  72 */ NOSFIX_SLOT(VoiceGetFilterParameters),
    /*  80 */ NOSFIX_SLOT(VoiceSetOutputFilterParameters),
    /*  88 */ NOSFIX_SLOT(VoiceGetOutputFilterParameters),
    /*  96 */ NOSFIX_SLOT(VoiceSetVolume),
    /* 104 */ NOSFIX_SLOT(VoiceGetVolume),
    /* 112 */ NOSFIX_SLOT(VoiceSetChannelVolumes),
    /* 120 */ NOSFIX_SLOT(VoiceGetChannelVolumes),
    /* 128 */ NOSFIX_SLOT(VoiceSetOutputMatrix),
    /* 136 */ NOSFIX_SLOT(VoiceGetOutputMatrix),
    /* 144 */ NOSFIX_SLOT(VoiceDestroy),
    /* 152 */ NOSFIX_SLOT(MasteringGetChannelMask),
    /* 160 */ NOSFIX_SLOT(VoiceUnsupported),
    /* 168 */ NOSFIX_SLOT(VoiceUnsupported),
    /* 176 */ NOSFIX_SLOT(VoiceUnsupported),
    /* 184 */ NOSFIX_SLOT(VoiceUnsupported),
    /* 192 */ NOSFIX_SLOT(VoiceUnsupported),
    /* 200 */ NOSFIX_SLOT(VoiceUnsupported),
    /* 208 */ NOSFIX_SLOT(VoiceUnsupported),
    /* 216 */ NOSFIX_SLOT(VoiceUnsupported),
    /* 224 */ NOSFIX_SLOT(VoiceUnsupported),
    /* 232 */ NOSFIX_SLOT(VoiceUnsupported),
    /* 240 */ NOSFIX_SLOT(VoiceUnsupported),
    /* 248 */ NOSFIX_SLOT(VoiceUnsupported),
    /* 256 */ NOSFIX_SLOT(VoiceUnsupported),
    /* 264 */ NOSFIX_SLOT(VoiceUnsupported),
    /* 272 */ NOSFIX_SLOT(VoiceUnsupported),
    /* 280 */ NOSFIX_SLOT(VoiceUnsupported),
    /* 288 */ NOSFIX_SLOT(VoiceUnsupported),
};

#undef NOSFIX_SLOT

} // namespace

// ---------------------------------------------------------------------------
// Public helpers
// ---------------------------------------------------------------------------

Xa2Engine* Xa2BridgeCreate(const Xa2BridgeConfig& config) noexcept {
    if (config.device_rate < 8000 || config.device_rate > 384000) return nullptr;
    if (config.device_channels == 0 || config.device_channels > kMixerMaxChannels) {
        return nullptr;
    }
    if (config.max_block_frames == 0 || config.max_block_frames > kMixerMaxBlockFrames) {
        return nullptr;
    }
    auto* engine = new (std::nothrow) Xa2Engine();
    if (engine == nullptr) return nullptr;
    engine->vtable = kEngineVtable;
    engine->config = config;
    engine->destroy_on_release = config.destroy_on_release;
    engine->ref_count = 1;
    if (!engine->mixer.Prepare(config.device_rate, config.device_channels,
                               config.max_block_frames)) {
        delete engine;
        return nullptr;
    }
    for (auto& voice : engine->voices) {
        voice = Xa2Voice{};
    }
    return engine;
}

void Xa2BridgeDestroy(Xa2Engine* engine) noexcept {
    if (engine == nullptr) return;
    delete engine;
}

AudioMixer* Xa2BridgeMixer(Xa2Engine* engine) noexcept {
    return engine != nullptr ? &engine->mixer : nullptr;
}

const Xa2BridgeConfig* Xa2BridgeConfigOf(const Xa2Engine* engine) noexcept {
    return engine != nullptr ? &engine->config : nullptr;
}

bool Xa2BridgeRenderBlock(Xa2Engine* engine, std::int64_t block_start_sample,
                          std::uint32_t frames, float* const* output,
                          std::uint32_t output_channels) noexcept {
    if (engine == nullptr) return false;
    for (std::uint32_t i = 0; i < engine->engine_callback_count; ++i) {
        void* callback = engine->engine_callbacks[i];
        if (callback == nullptr) continue;
        void** vtable = *reinterpret_cast<void***>(callback);
        if (vtable != nullptr && vtable[0] != nullptr) {
            reinterpret_cast<void (*)(void*)>(vtable[0])(callback);
        }
    }
    const bool rendered =
        engine->mixer.Render(block_start_sample, frames, output, output_channels);
    for (std::uint32_t i = 0; i < engine->engine_callback_count; ++i) {
        void* callback = engine->engine_callbacks[i];
        if (callback == nullptr) continue;
        void** vtable = *reinterpret_cast<void***>(callback);
        if (vtable != nullptr && vtable[1] != nullptr) {
            reinterpret_cast<void (*)(void*)>(vtable[1])(callback);
        }
    }
    engine->render_blocks.fetch_add(1, std::memory_order_relaxed);
    return rendered;
}

void Xa2BridgeNotifyCriticalError(Xa2Engine* engine, std::int32_t hresult) noexcept {
    if (engine == nullptr) return;
    for (std::uint32_t i = 0; i < engine->engine_callback_count; ++i) {
        void* callback = engine->engine_callbacks[i];
        if (callback == nullptr) continue;
        void** vtable = *reinterpret_cast<void***>(callback);
        if (vtable != nullptr && vtable[2] != nullptr) {
            reinterpret_cast<void (*)(void*, std::int32_t)>(vtable[2])(callback, hresult);
        }
    }
}

Xa2BridgeStats Xa2BridgeGetStats(const Xa2Engine* engine) noexcept {
    Xa2BridgeStats stats;
    if (engine == nullptr) return stats;
    stats.engine_calls = engine->engine_calls.load(std::memory_order_relaxed);
    stats.voice_calls = engine->voice_calls.load(std::memory_order_relaxed);
    stats.unsupported_slots = engine->unsupported_slots.load(std::memory_order_relaxed);
    stats.rejected_calls = engine->rejected_calls.load(std::memory_order_relaxed);
    stats.effects_recorded = engine->effects_recorded.load(std::memory_order_relaxed);
    stats.effects_applied = 0;
    stats.filters_recorded = engine->filters_recorded.load(std::memory_order_relaxed);
    stats.filters_applied = 0;
    stats.matrices_applied = engine->matrices_applied.load(std::memory_order_relaxed);
    stats.matrices_rejected = engine->matrices_rejected.load(std::memory_order_relaxed);
    stats.operation_sets_committed =
        engine->operation_sets_committed.load(std::memory_order_relaxed);
    stats.operation_sets_dropped =
        engine->operation_sets_dropped.load(std::memory_order_relaxed);
    stats.buffers_submitted =
        engine->buffers_submitted.load(std::memory_order_relaxed);
    stats.buffers_rejected = engine->buffers_rejected.load(std::memory_order_relaxed);
    stats.callback_invocations =
        engine->callback_invocations.load(std::memory_order_relaxed);
    stats.render_blocks = engine->render_blocks.load(std::memory_order_relaxed);
    stats.live_source_voices = engine->live_sources.load(std::memory_order_relaxed);
    stats.live_submix_voices = engine->live_submixes.load(std::memory_order_relaxed);
    return stats;
}

Xa2Voice* Xa2VoiceFromInterface(void* voice_interface) noexcept {
    return VoiceFromInterface(voice_interface);
}

Xa2VoiceKind Xa2VoiceGetKind(const Xa2Voice* voice) noexcept {
    return voice != nullptr ? voice->kind : Xa2VoiceKind::Free;
}

bool Xa2VoiceGetFormat(const Xa2Voice* voice, std::uint32_t& channels,
                       std::uint32_t& sample_rate) noexcept {
    if (voice == nullptr) return false;
    channels = voice->input_channels;
    sample_rate = voice->input_rate;
    return true;
}

bool Xa2VoiceStartAt(Xa2Voice* voice, std::uint32_t offset_in_block) noexcept {
    if (voice == nullptr || !voice->alive || voice->kind != Xa2VoiceKind::Source ||
        voice->engine == nullptr || voice->mixer_voice <= 0) {
        return false;
    }
    return voice->engine->mixer.StartVoice(voice->mixer_voice, offset_in_block);
}

bool Xa2VoiceSetVolumePan(Xa2Voice* voice, float volume, float pan) noexcept {
    if (voice == nullptr || !voice->alive || voice->kind != Xa2VoiceKind::Source ||
        voice->engine == nullptr || voice->mixer_voice <= 0) {
        return false;
    }
    if (!voice->engine->mixer.SetVoiceVolumePan(voice->mixer_voice, volume, pan)) {
        return false;
    }
    voice->volume = volume;
    voice->pan = pan;
    return true;
}

// ---------------------------------------------------------------------------
// libxact cue table and command handoff (round 2)
// ---------------------------------------------------------------------------

namespace xact {
namespace {

void RaiseMaximum(std::atomic<std::uint32_t>& target, std::uint32_t value) noexcept {
    std::uint32_t current = target.load(std::memory_order_relaxed);
    while (value > current &&
           !target.compare_exchange_weak(current, value, std::memory_order_relaxed,
                                         std::memory_order_relaxed)) {
    }
}

} // namespace

bool XactCueTable::Attach(void* object_base, std::size_t object_bytes) noexcept {
    Detach();
    if (object_base == nullptr) return false;
    if (object_bytes < kObjectMinBytes) return false;
    const auto address = reinterpret_cast<std::uintptr_t>(object_base);
    if (address % alignof(std::uint64_t) != 0) return false;
    base_ = static_cast<std::uint8_t*>(object_base);
    base_bytes_ = object_bytes;
    return true;
}

void XactCueTable::Detach() noexcept {
    base_ = nullptr;
    base_bytes_ = 0;
}

XactCueRecord* XactCueTable::Record(std::uint32_t index) noexcept {
    if (base_ == nullptr || index >= kCueCount) return nullptr;
    return reinterpret_cast<XactCueRecord*>(base_ + kCueTableOffset +
                                            static_cast<std::size_t>(index) * kCueRecordBytes);
}

const XactCueRecord* XactCueTable::Record(std::uint32_t index) const noexcept {
    if (base_ == nullptr || index >= kCueCount) return nullptr;
    return reinterpret_cast<const XactCueRecord*>(
        base_ + kCueTableOffset + static_cast<std::size_t>(index) * kCueRecordBytes);
}

std::int32_t* XactCueTable::CommandSlot(std::uint32_t index) noexcept {
    if (base_ == nullptr || index >= kCueCount) return nullptr;
    return reinterpret_cast<std::int32_t*>(base_ + kCueTableOffset +
                                           static_cast<std::size_t>(index) * kCueRecordBytes +
                                           kCueCommandOffset);
}

const std::int32_t* XactCueTable::CommandSlot(std::uint32_t index) const noexcept {
    if (base_ == nullptr || index >= kCueCount) return nullptr;
    return reinterpret_cast<const std::int32_t*>(
        base_ + kCueTableOffset + static_cast<std::size_t>(index) * kCueRecordBytes +
        kCueCommandOffset);
}

std::int32_t XactCueTable::Command(std::uint32_t index) const noexcept {
    const std::int32_t* slot = CommandSlot(index);
    if (slot == nullptr) return kCommandFree;
    return std::atomic_ref<const std::int32_t>(*slot).load(std::memory_order_acquire);
}

void XactCueTable::SetCommand(std::uint32_t index, std::int32_t command) noexcept {
    std::int32_t* slot = CommandSlot(index);
    if (slot == nullptr) return;
    std::atomic_ref<std::int32_t>(*slot).store(command, std::memory_order_release);
}

std::uint32_t XactCueTable::FindFree(std::uint32_t start) const noexcept {
    if (base_ == nullptr) return kInvalidCue;
    for (std::uint32_t index = start; index < kCueCount; ++index) {
        if (Command(index) == kCommandFree) return index;
    }
    return kInvalidCue;
}

bool XactCommandRing::Prepare(XactCueTable* table) noexcept {
    if (table == nullptr || !table->Attached()) {
        table_ = nullptr;
        return false;
    }
    table_ = table;
    head_.store(0, std::memory_order_relaxed);
    tail_.store(0, std::memory_order_relaxed);
    shutdown_.store(false, std::memory_order_relaxed);
    return true;
}

XactCommandRing::PostStatus XactCommandRing::Post(const XactCueRecord& record) noexcept {
    if (table_ == nullptr || !table_->Attached()) {
        invalid_.fetch_add(1, std::memory_order_relaxed);
        return PostStatus::Invalid;
    }
    if (record.cmd == kCommandFree) {
        // A zero command word is the free marker; posting it would publish a slot
        // with no work and the consumer would retire a slot it never owned.
        invalid_.fetch_add(1, std::memory_order_relaxed);
        return PostStatus::Invalid;
    }
    if (shutdown_.load(std::memory_order_relaxed)) {
        refused_after_shutdown_.fetch_add(1, std::memory_order_relaxed);
        return PostStatus::Shutdown;
    }

    // Ring capacity is checked before the table is touched: a full ring must not
    // leave a half-written record behind.
    const std::uint32_t head = head_.load(std::memory_order_acquire);
    const std::uint32_t tail = tail_.load(std::memory_order_relaxed);
    const std::uint32_t used = tail - head;
    if (used >= kCommandRingCapacity) {
        ring_overflow_.fetch_add(1, std::memory_order_relaxed);
        return PostStatus::RingFull;
    }

    // add_cue takes the lowest free slot, so this is the same choice.
    const std::uint32_t slot = table_->FindFree();
    if (slot == kInvalidCue) {
        table_full_.fetch_add(1, std::memory_order_relaxed);
        return PostStatus::TableFull;
    }

    XactCueRecord* target = table_->Record(slot);
    if (target == nullptr) {
        invalid_.fetch_add(1, std::memory_order_relaxed);
        return PostStatus::Invalid;
    }
    // Payload first with the command word still free, index next, command word
    // last: the command word is the publication point.
    *target = record;
    table_->SetCommand(slot, kCommandFree);
    slots_[tail & (kCommandRingCapacity - 1U)] = slot;
    tail_.store(tail + 1U, std::memory_order_release);
    table_->SetCommand(slot, record.cmd);

    const std::uint32_t pending = used + 1U;
    RaiseMaximum(high_watermark_, pending);
    posted_.fetch_add(1, std::memory_order_relaxed);
    return PostStatus::Accepted;
}

std::uint32_t XactCommandRing::DrainAtBlockStart(XactCommandSink sink,
                                                 void* sink_context,
                                                 XactCompletionSink completion,
                                                 void* completion_context) noexcept {
    if (table_ == nullptr || !table_->Attached() || sink == nullptr) return 0;
    drains_.fetch_add(1, std::memory_order_relaxed);

    const std::uint32_t head = head_.load(std::memory_order_relaxed);
    const std::uint32_t tail = tail_.load(std::memory_order_acquire);
    std::uint32_t applied_here = 0;
    std::uint32_t index = head;
    while (index != tail) {
        const std::uint32_t slot = slots_[index & (kCommandRingCapacity - 1U)];
        ++index;
        const XactCueRecord* source = table_->Record(slot);
        if (source == nullptr) {
            invalid_.fetch_add(1, std::memory_order_relaxed);
            continue;
        }
        const std::int32_t command = table_->Command(slot);

        XactCommandEvent event;
        event.sequence = sequence_.fetch_add(1, std::memory_order_relaxed) + 1U;
        event.generation = generation_.load(std::memory_order_relaxed);
        event.slot = slot;
        event.cmd = command;
        event.p1 = source->p1;
        event.p2 = source->p2;
        event.p3 = source->p3;
        event.p4 = source->p4;
        event.p5 = source->p5;
        event.p6 = source->p6;
        event.p7 = source->p7;
        std::memcpy(event.s1, source->s1, sizeof(event.s1));
        std::memcpy(event.s2, source->s2, sizeof(event.s2));

        if (command == kCommandShutdown) {
            // 0x18000A869: the worker tears the engine down and returns. The
            // model must not apply it to the sink and must refuse what follows.
            shutdown_.store(true, std::memory_order_relaxed);
            shutdown_commands_.fetch_add(1, std::memory_order_relaxed);
            const std::uint32_t behind = tail - index;
            if (behind != 0) {
                abandoned_shutdown_.fetch_add(behind, std::memory_order_relaxed);
            }
            table_->SetCommand(slot, kCommandFree);
            index = tail;
            break;
        }
        if (command == kCommandFree) {
            // Cleared under the consumer (generation change): nothing to do.
            invalid_.fetch_add(1, std::memory_order_relaxed);
            continue;
        }
        // Only 0 (free) and 2 (shutdown) are proven command values; anything
        // else is transported to the sink unchanged and counted as unproven.
        unknown_command_.fetch_add(1, std::memory_order_relaxed);

        const bool ok = sink(sink_context, event);
        if (ok) {
            applied_.fetch_add(1, std::memory_order_relaxed);
            ++applied_here;
        } else {
            apply_failed_.fetch_add(1, std::memory_order_relaxed);
        }
        if (completion != nullptr) completion(completion_context, event, ok ? 0 : 1);
        table_->SetCommand(slot, kCommandFree);
    }

    head_.store(tail, std::memory_order_release);
    return applied_here;
}

void XactCommandRing::SetGeneration(std::uint64_t generation) noexcept {
    if (generation_.load(std::memory_order_relaxed) == generation) return;

    // A generation change is a device/game reset: every pending cue belongs to
    // the old run. Non-zero command words are cleared so a stale record cannot be
    // picked up by the next generation, and the ring is emptied.
    std::uint64_t abandoned = 0;
    if (table_ != nullptr && table_->Attached()) {
        for (std::uint32_t index = 0; index < kCueCount; ++index) {
            if (table_->Command(index) != kCommandFree) {
                table_->SetCommand(index, kCommandFree);
                ++abandoned;
            }
        }
    }
    abandoned_generation_.fetch_add(abandoned, std::memory_order_relaxed);
    head_.store(0, std::memory_order_relaxed);
    tail_.store(0, std::memory_order_relaxed);
    sequence_.store(0, std::memory_order_relaxed);
    shutdown_.store(false, std::memory_order_relaxed);
    generation_.store(generation, std::memory_order_relaxed);
    generation_changes_.fetch_add(1, std::memory_order_relaxed);
}

std::uint32_t XactCommandRing::Pending() const noexcept {
    const std::uint32_t head = head_.load(std::memory_order_acquire);
    const std::uint32_t tail = tail_.load(std::memory_order_acquire);
    const std::uint32_t used = tail - head;
    return used > kCommandRingCapacity ? kCommandRingCapacity : used;
}

XactCommandStats XactCommandRing::Stats() const noexcept {
    XactCommandStats stats;
    stats.posted = posted_.load(std::memory_order_relaxed);
    stats.applied = applied_.load(std::memory_order_relaxed);
    stats.apply_failed = apply_failed_.load(std::memory_order_relaxed);
    stats.ring_overflow = ring_overflow_.load(std::memory_order_relaxed);
    stats.table_full = table_full_.load(std::memory_order_relaxed);
    stats.unknown_command = unknown_command_.load(std::memory_order_relaxed);
    stats.shutdown_commands = shutdown_commands_.load(std::memory_order_relaxed);
    stats.refused_after_shutdown = refused_after_shutdown_.load(std::memory_order_relaxed);
    stats.abandoned_shutdown = abandoned_shutdown_.load(std::memory_order_relaxed);
    stats.abandoned_generation = abandoned_generation_.load(std::memory_order_relaxed);
    stats.invalid = invalid_.load(std::memory_order_relaxed);
    stats.generation_changes = generation_changes_.load(std::memory_order_relaxed);
    stats.drains = drains_.load(std::memory_order_relaxed);
    stats.pending = Pending();
    stats.high_watermark = high_watermark_.load(std::memory_order_relaxed);
    return stats;
}

} // namespace xact

} // namespace nosfix::audio
