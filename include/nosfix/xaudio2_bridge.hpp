#pragma once

#include "nosfix/audio_mixer.hpp"

#include <cstddef>
#include <cstdint>

// XAudio2 2.7-shaped engine surface, built for the libxact client.
//
// Status
// ------
// This is a *surface*, not a reimplementation of the XAudio2 DSP graph. It
// implements the slots libxact touches, keeps the rest of the 2.7 layout
// callable, and fails closed (returns a failure HRESULT and counts the call) on
// everything outside it. It never installs a hook, never opens a device and
// never creates a thread; the caller decides where it is created and who
// renders it.
//
// Surface tiers (coord/answers/audio-001.md, audio-002.md; 2026-10-30)
// -------------------------------------------------------------------
// libxact.dll is a 2.7-era client whose *engine* vtable is 2.7-shaped and whose
// *voice* vtable has no IUnknown prefix (19 base entries, DestroyVoice = idx 18).
//
// A. PROVEN AND EXERCISED by libxact (byte-verified call sites + argument
//    shapes). This is the surface a bridge must get right:
//      engine +0x20 GetDeviceDetails(0, details)
//      engine +0x48 CreateSubmixVoice(pp, 2, 44100, ..., 5, 0)
//      engine +0x50 CreateMasteringVoice(pp, 2|6, 44100, 0, 0)
//      voice  +0x10 SetEffectChain(chain{pEffect, initialState})
//      voice  +0x30 SetEffectParameters(0, block, 52, 0)
//      voice  +0x40 SetFilterParameters({Type,Frequency,OneOverQ}, 0)   [audio-002]
//      voice  +0x80 SetOutputMatrix(dest, 2, 2, matrix, 0)
//      voice  +0x90 DestroyVoice                                        [audio-001]
//    CreateFX is imported from XAPOFX1_5.dll and called with only rcx/rdx set;
//    the bridge never calls it, it records the descriptors it is handed.
//
// B. PROVEN LAYOUT, NOT CALLED by libxact: the remaining base slots
//    (GetVoiceDetails, SetOutputVoices, effect enable/state, GetEffectParameters,
//    Get/SetFilterParameters on master, volume and channel volumes,
//    GetOutputMatrix). They follow the same proven 19-entry table.
//
// C. SPECULATIVE, NO LIBXACT CALL SITE: every source-voice slot (+0x98..+0xE0)
//    and engine +0x40 CreateSourceVoice. libxact.dll contains no
//    CreateSourceVoice, no SubmitSourceBuffer and no WAVEFORMATEX; audio
//    submission happens inside avs2-core on the cue/xsb path (audio-002). The
//    bridge still implements a source voice because the mixer and the ASIO host
//    need one, but it is prototype-only surface: kLibxactUsesSourceVoice is
//    false, nothing here is claimed to be libxact-compatible, and every call is
//    reachable only through an explicitly bound cue voice in this stream's host.
//    The slot order is the SDK declaration order inferred from the proven 19-
//    entry base; only a live vtable dump can settle it.
//
// Threading
// ---------
// Engine and voice methods must be called from one owner thread (libxact's
// command worker). They only enqueue commands on the mixer. RenderBlock() is the
// only audio-thread entry and allocates/locks/logs nothing.
namespace nosfix::audio {

// Byte offsets of the slots this bridge exposes. Tests call through these
// offsets exactly like a compiled client does.
namespace xa27 {

// IXAudio2 (engine). 2.7 order: IUnknown, then device/initialize, then voices.
inline constexpr std::uint32_t kEngineQueryInterface = 0;
inline constexpr std::uint32_t kEngineAddRef = 8;
inline constexpr std::uint32_t kEngineRelease = 16;
inline constexpr std::uint32_t kEngineGetDeviceCount = 24;
inline constexpr std::uint32_t kEngineGetDeviceDetails = 32;
inline constexpr std::uint32_t kEngineInitialize = 40;
inline constexpr std::uint32_t kEngineRegisterForCallbacks = 48;
inline constexpr std::uint32_t kEngineUnregisterForCallbacks = 56;
inline constexpr std::uint32_t kEngineCreateSourceVoice = 64;
inline constexpr std::uint32_t kEngineCreateSubmixVoice = 72;
inline constexpr std::uint32_t kEngineCreateMasteringVoice = 80;
inline constexpr std::uint32_t kEngineStartEngine = 88;
inline constexpr std::uint32_t kEngineStopEngine = 96;
inline constexpr std::uint32_t kEngineCommitChanges = 104;
inline constexpr std::uint32_t kEngineGetPerformanceData = 112;
inline constexpr std::uint32_t kEngineSetDebugConfiguration = 120;
inline constexpr std::uint32_t kEngineFirstRejectionSlot = 128;

// IXAudio2Voice base, 2.8/2.9 order (no IUnknown prefix).
inline constexpr std::uint32_t kVoiceGetVoiceDetails = 0;
inline constexpr std::uint32_t kVoiceSetOutputVoices = 8;
inline constexpr std::uint32_t kVoiceSetEffectChain = 16;
inline constexpr std::uint32_t kVoiceEnableEffect = 24;
inline constexpr std::uint32_t kVoiceDisableEffect = 32;
inline constexpr std::uint32_t kVoiceGetEffectState = 40;
inline constexpr std::uint32_t kVoiceSetEffectParameters = 48;
inline constexpr std::uint32_t kVoiceGetEffectParameters = 56;
inline constexpr std::uint32_t kVoiceSetFilterParameters = 64;
inline constexpr std::uint32_t kVoiceGetFilterParameters = 72;
inline constexpr std::uint32_t kVoiceSetOutputFilterParameters = 80;
inline constexpr std::uint32_t kVoiceGetOutputFilterParameters = 88;
inline constexpr std::uint32_t kVoiceSetVolume = 96;
inline constexpr std::uint32_t kVoiceGetVolume = 104;
inline constexpr std::uint32_t kVoiceSetChannelVolumes = 112;
inline constexpr std::uint32_t kVoiceGetChannelVolumes = 120;
inline constexpr std::uint32_t kVoiceSetOutputMatrix = 128;
inline constexpr std::uint32_t kVoiceGetOutputMatrix = 136;
inline constexpr std::uint32_t kVoiceDestroyVoice = 144;

// IXAudio2SourceVoice (continues the base table). SPECULATIVE for libxact: no
// call site exists in libxact.dll (audio-002), so these are prototype-only.
// The order is the SDK declaration order inferred from the proven 19-entry base;
// see kLibxactUsesSourceVoice.
inline constexpr std::uint32_t kVoiceStart = 152;
inline constexpr std::uint32_t kVoiceStop = 160;
inline constexpr std::uint32_t kVoiceSubmitSourceBuffer = 168;
inline constexpr std::uint32_t kVoiceFlushSourceBuffers = 176;
inline constexpr std::uint32_t kVoiceDiscontinuity = 184;
inline constexpr std::uint32_t kVoiceExitLoop = 192;
inline constexpr std::uint32_t kVoiceGetState = 200;
inline constexpr std::uint32_t kVoiceSetFrequencyRatio = 208;
inline constexpr std::uint32_t kVoiceGetFrequencyRatio = 216;
inline constexpr std::uint32_t kVoiceSetSourceSampleRate = 224;
inline constexpr std::uint32_t kVoiceFirstRejectionSlot = 232;

// IXAudio2MasteringVoice::GetChannelMask sits right after the base table.
inline constexpr std::uint32_t kMasteringGetChannelMask = 152;

inline constexpr std::uint32_t kSlotBytes = 8;

} // namespace xa27

// False: libxact.dll never creates, starts or submits to a source voice. The
// source-voice surface in this header exists for this stream's prototype host
// and its tests, not for libxact compatibility. A value of true would require a
// byte-proven call site (none exists) and a proven slot order (only a live
// vtable dump can provide one).
inline constexpr bool kLibxactUsesSourceVoice = false;

// XAudio2 2.7 constants and POD layouts (transcribed interface facts).
inline constexpr std::uint32_t kXa2LoopInfinite = 255;
inline constexpr std::uint32_t kXa2EndOfStream = 0x0040;
inline constexpr std::uint32_t kXa2VoiceNoPitch = 0x0002;
inline constexpr std::uint32_t kXa2VoiceNoSamplesPlayed = 0x0100;
inline constexpr std::uint32_t kXa2CommitNow = 0;
inline constexpr float kXa2DefaultFrequencyRatio = 2.0F;

inline constexpr std::uint16_t kWaveFormatPcm = 0x0001;
inline constexpr std::uint16_t kWaveFormatIeeeFloat = 0x0003;
inline constexpr std::uint16_t kWaveFormatExtensible = 0xFFFE;

inline constexpr std::int32_t kXa2Ok = 0;
inline constexpr std::int32_t kXa2False = 1;
inline constexpr std::int32_t kXa2InvalidArgument = static_cast<std::int32_t>(0x80070057u);
inline constexpr std::int32_t kXa2OutOfMemory = static_cast<std::int32_t>(0x8007000Eu);
inline constexpr std::int32_t kXa2Pointer = static_cast<std::int32_t>(0x80004003u);
inline constexpr std::int32_t kXa2Fail = static_cast<std::int32_t>(0x80004005u);
inline constexpr std::int32_t kXa2NoInterface = static_cast<std::int32_t>(0x80004002u);
inline constexpr std::int32_t kXa2InvalidCall = static_cast<std::int32_t>(0x88960001u);
inline constexpr std::int32_t kXa2DeviceInvalidated =
    static_cast<std::int32_t>(0x88960004u);

// WAVEFORMATEX / WAVEFORMATEXTENSIBLE are declared with 1-byte packing in
// mmreg.h windows headers; sizeof(WAVEFORMATEX) is 18 and the extensible
// sub-format GUID sits at offset 24. The bridge must read those exact offsets.
#pragma pack(push, 1)
struct Xa2WaveFormat {
    std::uint16_t format_tag = 0;
    std::uint16_t channels = 0;
    std::uint32_t samples_per_sec = 0;
    std::uint32_t avg_bytes_per_sec = 0;
    std::uint16_t block_align = 0;
    std::uint16_t bits_per_sample = 0;
    std::uint16_t cb_size = 0;
};

struct Xa2WaveFormatExtensible {
    Xa2WaveFormat format{};
    std::uint16_t valid_bits_per_sample = 0;
    std::uint32_t channel_mask = 0;
    std::uint8_t sub_format[16] = {};
};

// XAUDIO2_DEVICE_DETAILS (2.7). libxact allocates a 1030-byte local and reads
// the channel count as a uint16 at byte 1030 (analyze/evidence/
// verified_xact_0x1800012e0.c, v118/v119), i.e. DeviceID then DisplayName, each
// WCHAR[256], then Role, then WAVEFORMATEXTENSIBLE. Spice declares the same
// order. Both names are filled with the configured name so a client that reads
// either field sees a valid string.
struct Xa2DeviceDetails {
    std::uint16_t device_id[256] = {};
    std::uint16_t display_name[256] = {};
    std::uint32_t role = 0;
    Xa2WaveFormatExtensible output_format{};
};
#pragma pack(pop)

struct Xa2Buffer {
    std::uint32_t flags = 0;
    std::uint32_t audio_bytes = 0;
    const std::uint8_t* audio_data = nullptr;
    std::uint32_t play_begin = 0;
    std::uint32_t play_length = 0;
    std::uint32_t loop_begin = 0;
    std::uint32_t loop_length = 0;
    std::uint32_t loop_count = 0;
    void* context = nullptr;
};

struct Xa2VoiceState {
    void* current_buffer_context = nullptr;
    std::uint32_t buffers_queued = 0;
    std::uint64_t samples_played = 0;
};

struct Xa2VoiceDetails {
    std::uint32_t creation_flags = 0;
    std::uint32_t active_flags = 0;
    std::uint32_t input_channels = 0;
    std::uint32_t input_sample_rate = 0;
};

struct Xa2EffectDescriptor {
    void* effect = nullptr;
    std::int32_t initial_state = 0;
    std::uint32_t output_channels = 0;
};

struct Xa2EffectChain {
    std::uint32_t effect_count = 0;
    const Xa2EffectDescriptor* descriptors = nullptr;
};

struct Xa2SendDescriptor {
    std::uint32_t flags = 0;
    void* output_voice = nullptr;
};

struct Xa2VoiceSends {
    std::uint32_t send_count = 0;
    const Xa2SendDescriptor* sends = nullptr;
};

struct Xa2FilterParameters {
    std::uint32_t type = 0;
    float frequency = 0.0F;
    float one_over_q = 0.0F;
};

static_assert(sizeof(Xa2WaveFormat) == 18, "WAVEFORMATEX is 18 bytes");
static_assert(sizeof(Xa2WaveFormatExtensible) == 40, "WAVEFORMATEXTENSIBLE is 40 bytes");
static_assert(sizeof(Xa2DeviceDetails) == 1068, "XAUDIO2_DEVICE_DETAILS (2.7)");
static_assert(offsetof(Xa2DeviceDetails, role) == 1024, "device details role");
static_assert(offsetof(Xa2DeviceDetails, output_format) == 1028, "device details format");
static_assert(offsetof(Xa2DeviceDetails, output_format) + 2 == 1030,
              "channel count offset used by libxact");
static_assert(sizeof(Xa2Buffer) == 48, "XAUDIO2_BUFFER is 48 bytes on x64");
static_assert(sizeof(Xa2VoiceState) == 24, "XAUDIO2_VOICE_STATE is 24 bytes");
static_assert(sizeof(Xa2VoiceDetails) == 16, "XAUDIO2_VOICE_DETAILS is 16 bytes");
static_assert(offsetof(Xa2WaveFormatExtensible, sub_format) == 24,
              "WAVEFORMATEXTENSIBLE sub-format offset");
static_assert(sizeof(Xa2EffectDescriptor) == 16, "XAUDIO2_EFFECT_DESCRIPTOR");
static_assert(sizeof(Xa2EffectChain) == 16, "XAUDIO2_EFFECT_CHAIN");
static_assert(sizeof(Xa2SendDescriptor) == 16, "XAUDIO2_SEND_DESCRIPTOR");
static_assert(sizeof(Xa2VoiceSends) == 16, "XAUDIO2_VOICE_SENDS");

struct Xa2BridgeConfig {
    // Device side of the mixer. The mastering voice must request exactly these
    // channels and this rate; nothing is resampled or remixed.
    std::uint32_t device_rate = 44100;
    std::uint32_t device_channels = 2;
    std::uint32_t max_block_frames = 1024;

    // Name reported by GetDeviceDetails. Empty is allowed (all-zero strings).
    const char* device_name = "";

    // If true, the final Release() frees the engine. Default false: a client
    // that forgets to release keeps a valid object instead of a dangling one.
    bool destroy_on_release = false;

    // Reported by GetPerformanceData as the processing quantum.
    std::uint32_t quantum_frames = 441;
};

enum class Xa2VoiceKind : std::uint8_t {
    Free = 0,
    Mastering = 1,
    Submix = 2,
    Source = 3,
};

struct Xa2BridgeStats {
    std::uint64_t engine_calls = 0;
    std::uint64_t voice_calls = 0;
    std::uint64_t unsupported_slots = 0;
    std::uint64_t rejected_calls = 0;
    std::uint64_t effects_recorded = 0;
    std::uint64_t effects_applied = 0; // always 0: no DSP is implemented
    std::uint64_t filters_recorded = 0;
    std::uint64_t filters_applied = 0; // always 0
    std::uint64_t matrices_applied = 0;
    std::uint64_t matrices_rejected = 0;
    std::uint64_t operation_sets_committed = 0;
    std::uint64_t operation_sets_dropped = 0;
    std::uint64_t buffers_submitted = 0;
    std::uint64_t buffers_rejected = 0;
    std::uint64_t callback_invocations = 0;
    std::uint64_t render_blocks = 0;
    std::uint32_t live_source_voices = 0;
    std::uint32_t live_submix_voices = 0;
};

// Opaque COM-shaped objects. The first pointer-sized field is the vtable.
struct Xa2Engine;
struct Xa2Voice;

// Creates an engine whose vtable is the surface above. Returns nullptr when the
// configuration is invalid or the object cannot be allocated. The engine owns a
// mixer; call Xa2BridgeMixer() to inspect it (tests) or to render it (host).
[[nodiscard]] Xa2Engine* Xa2BridgeCreate(const Xa2BridgeConfig& config) noexcept;
void Xa2BridgeDestroy(Xa2Engine* engine) noexcept;

// Engine-side helpers that are not part of the COM surface.
[[nodiscard]] AudioMixer* Xa2BridgeMixer(Xa2Engine* engine) noexcept;
[[nodiscard]] const Xa2BridgeConfig* Xa2BridgeConfigOf(const Xa2Engine* engine) noexcept;
// Renders one block of the shared sample timeline. Calls the registered
// IXAudio2EngineCallback OnProcessingPassStart/End around the mix. RT-safe.
[[nodiscard]] bool Xa2BridgeRenderBlock(Xa2Engine* engine,
                                        std::int64_t block_start_sample,
                                        std::uint32_t frames,
                                        float* const* output,
                                        std::uint32_t output_channels) noexcept;
// Calls OnCriticalError on every registered engine callback. Control thread.
void Xa2BridgeNotifyCriticalError(Xa2Engine* engine, std::int32_t hresult) noexcept;
[[nodiscard]] Xa2BridgeStats Xa2BridgeGetStats(const Xa2Engine* engine) noexcept;

// Looks up a voice by its COM pointer; nullptr if it is not one of ours.
[[nodiscard]] Xa2Voice* Xa2VoiceFromInterface(void* voice_interface) noexcept;
[[nodiscard]] Xa2VoiceKind Xa2VoiceGetKind(const Xa2Voice* voice) noexcept;
// Input channels / rate of a voice, for tests and diagnostics.
[[nodiscard]] bool Xa2VoiceGetFormat(const Xa2Voice* voice,
                                     std::uint32_t& channels,
                                     std::uint32_t& sample_rate) noexcept;
// Starts a source voice at a sample offset inside the next rendered block. Used
// by the keysound sink; false if the voice is not a live source voice.
[[nodiscard]] bool Xa2VoiceStartAt(Xa2Voice* voice,
                                   std::uint32_t offset_in_block) noexcept;
[[nodiscard]] bool Xa2VoiceSetVolumePan(Xa2Voice* voice, float volume,
                                        float pan) noexcept;

} // namespace nosfix::audio
