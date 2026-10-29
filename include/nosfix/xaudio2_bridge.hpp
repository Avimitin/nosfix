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

// ===========================================================================
// libxact CLibXact cue table and command handoff (round 2)
//
// Evidence: analyze/impl-20261030/ABI_COMPLETION.md E17 (proven from bytes),
// analyze/evidence/verified_xact_0x1800092e0.c, nosxact_0x180008a10.c.
//
// Proven layout of the CLibXact object:
//   this+8         cue[0]: 1024 records x 568 bytes (581632 bytes of table)
//   this+12        cue[0].cmd; a slot is free when its cmd dword is 0, and
//                  add_cue computes it as (DWORD*)this + 3 + 142 * index
//   this+581640    first byte after the table. A completion status dword is
//                  written by FinishCommand somewhere around here, but the pack
//                  renders that store as `this+568`, which cannot be right (it
//                  would land inside cue[0], 8 bytes into the record at +0x230).
//                  The offset is recorded and NOT used: kStatusWordProven=false.
//   this+581644    event  (SetEvent, avs2_core_25)
//   this+581648    waiter (avs2_core_26(handle, 1, 16, buf, 60000 ms))
//   this+581652    lock   (avs2_core_16/17)
//
// Proven command words: 0 = free slot, 2 = the shutdown case of
// CLibXact::execute (0x18000A869: tear the engine down, CoUninitialize, return).
// No other value is proven, so this model transports a command word verbatim,
// applies it through a caller-supplied sink and counts it; it never assigns
// semantics to a number the evidence does not pin down.
//
// What this replaces
// ------------------
// CLibXact::execute polls cue[0].cmd and calls Sleep(2) at 0x18000942F when it
// is zero, so a command that lands just after a poll waits up to 2 ms. The
// handoff below keeps the same cue records and the same command word, but the
// owner appends the cue index to a bounded ring and the audio block start drains
// it once. A command posted at any point during block N is therefore applied at
// the start of block N+1: a bounded one-block latency instead of a 0..2 ms
// sleep, and no polling loop. A full ring or a full table is refused and
// counted; nothing is dropped silently.
//
// The event/waiter/lock slots are *not* touched by this model. Completion is
// published through a callback the caller can bind to those slots once their
// widths are proven; see XactCompletionSink.
namespace xact {

// --- proven layout constants ------------------------------------------------
inline constexpr std::uint32_t kCueCount = 1024;
inline constexpr std::uint32_t kCueRecordBytes = 568;
inline constexpr std::uint32_t kCueTableOffset = 8;
inline constexpr std::uint32_t kCueCommandOffset = 4; // record+4; object+12 for cue[0]
inline constexpr std::uint32_t kCueTableBytes = kCueCount * kCueRecordBytes; // 581632
inline constexpr std::uint32_t kCueTableEnd = kCueTableOffset + kCueTableBytes; // 581640
inline constexpr std::uint32_t kStatusWordOffset = kCueTableEnd; // 581640, unproven use
inline constexpr std::uint32_t kEventWordOffset = 581644;
inline constexpr std::uint32_t kWaiterWordOffset = 581648;
inline constexpr std::uint32_t kLockWordOffset = 581652;
inline constexpr std::uint32_t kObjectMinBytes = kLockWordOffset + 4; // 581656
inline constexpr std::uint32_t kCommandWaiterItemBytes = 16; // avs2_core_26(.., 1, 16, ..)

// False: FinishCommand's status store is not pinned to an offset that is
// compatible with the cue table extent (see the comment above). No code in this
// tree writes or reads a status word.
inline constexpr bool kStatusWordProven = false;

// Proven command words only. 1 and everything above 2 are unknown.
inline constexpr std::int32_t kCommandFree = 0;
inline constexpr std::int32_t kCommandShutdown = 2;

inline constexpr std::uint32_t kInvalidCue = 0xFFFFFFFFu;

// Owner -> audio handoff: a bounded ring of cue indices, drained once per block.
inline constexpr std::uint32_t kCommandRingCapacity = 256; // power of two
inline constexpr std::uint32_t kHandoffLatencyBlocks = 1;

// One cue record, exactly 568 bytes, field for field as printed by
// FinishCommand's own diagnostic format string
// "cue[%d] : cmd->%d p1->%d p2->%d p3->%f p4->%f p5->%f p6->%f p7->%d
//  s1->%s s2->%s" and by add_cue's stores.
#pragma pack(push, 1)
struct XactCueRecord {
    std::uint8_t flag = 0;         // +0x000  byte flag/type
    std::uint8_t reserved0[3] = {}; // +0x001
    std::int32_t cmd = 0;          // +0x004  command word, 0 = free
    std::int32_t p1 = 0;           // +0x008
    std::int32_t p2 = 0;           // +0x00C
    float p3 = 0.0F;               // +0x010
    float p4 = 0.0F;               // +0x014
    float p5 = 0.0F;               // +0x018
    float p6 = 0.0F;               // +0x01C
    std::uint64_t p7 = 0;          // +0x020
    char s1[260] = {};             // +0x028  sound / cue name
    char s2[260] = {};             // +0x12C  second name
    std::uint8_t tail[8] = {};     // +0x230  padding to 568
};
#pragma pack(pop)

static_assert(sizeof(XactCueRecord) == kCueRecordBytes, "cue record is 568 bytes");
static_assert(offsetof(XactCueRecord, cmd) == kCueCommandOffset, "cue cmd offset");
static_assert(offsetof(XactCueRecord, p1) == 0x008, "cue p1 offset");
static_assert(offsetof(XactCueRecord, p7) == 0x020, "cue p7 offset");
static_assert(offsetof(XactCueRecord, s1) == 0x028, "cue s1 offset");
static_assert(offsetof(XactCueRecord, s2) == 0x12C, "cue s2 offset");
static_assert(offsetof(XactCueRecord, tail) == 0x230, "cue padding offset");
static_assert(kCueTableOffset + kCueCommandOffset == 12, "this+12 is cue[0].cmd");
static_assert(kCueTableEnd <= kEventWordOffset, "cue table ends before the event slot");

// A view over a CLibXact-shaped object owned by the caller. Attach() validates
// size and alignment and fails closed: a refused table stays detached, every
// accessor then returns kInvalidCue / nullptr / 0 and every write is ignored.
//
// The command word is the publication point (libxact itself treats a nonzero
// cmd dword as "this slot has work"), so Command()/SetCommand() use atomic
// acquire/release on the caller's memory. Record fields are written before the
// word is published.
class XactCueTable final {
public:
    XactCueTable() = default;

    XactCueTable(const XactCueTable&) = delete;
    XactCueTable& operator=(const XactCueTable&) = delete;

    [[nodiscard]] bool Attach(void* object_base, std::size_t object_bytes) noexcept;
    void Detach() noexcept;
    [[nodiscard]] bool Attached() const noexcept { return base_ != nullptr; }

    [[nodiscard]] std::uint8_t* Base() const noexcept { return base_; }
    [[nodiscard]] std::size_t BaseBytes() const noexcept { return base_bytes_; }

    // cue[index], or nullptr when detached / out of range.
    [[nodiscard]] XactCueRecord* Record(std::uint32_t index) noexcept;
    [[nodiscard]] const XactCueRecord* Record(std::uint32_t index) const noexcept;

    // The command word of cue[index] (record+4). 0 when out of range.
    [[nodiscard]] std::int32_t Command(std::uint32_t index) const noexcept;
    void SetCommand(std::uint32_t index, std::int32_t command) noexcept;

    // Lowest free slot at or after `start` (scanning to the end of the table),
    // or kInvalidCue. add_cue scans from cue[0]; the scan is bounded by
    // kCueCount and never allocates.
    [[nodiscard]] std::uint32_t FindFree(std::uint32_t start = 0) const noexcept;

private:
    [[nodiscard]] std::int32_t* CommandSlot(std::uint32_t index) noexcept;
    [[nodiscard]] const std::int32_t* CommandSlot(std::uint32_t index) const noexcept;

    std::uint8_t* base_ = nullptr;
    std::size_t base_bytes_ = 0;
};

// One command as the audio block sees it: the record fields copied out of the
// table, plus the handoff metadata. Copies of s1/s2 are bounded member arrays;
// nothing here points into the game object after the drain returned.
struct XactCommandEvent {
    std::uint64_t sequence = 0;
    std::uint64_t generation = 0;
    std::uint32_t slot = 0;
    std::uint32_t latency_blocks = kHandoffLatencyBlocks;
    std::int32_t cmd = kCommandFree;
    std::int32_t p1 = 0;
    std::int32_t p2 = 0;
    float p3 = 0.0F;
    float p4 = 0.0F;
    float p5 = 0.0F;
    float p6 = 0.0F;
    std::uint64_t p7 = 0;
    char s1[260] = {};
    char s2[260] = {};
};

// Applied at the start of an audio block. Must obey audio-thread rules: no
// allocation, no lock, no logging, no waiting. Return false if the command could
// not be applied; the command is still retired and the failure is counted.
using XactCommandSink = bool (*)(void* context, const XactCommandEvent& event) noexcept;

// Published once per retired command with status 0 (applied) or 1 (refused).
// This is the model of FinishCommand's "set status, signal, then let the waiter
// go" step. The real event/waiter slots (+581644/+581648) are not touched here
// because their widths are unproven; the caller binds this callback to them.
using XactCompletionSink = void (*)(void* context,
                                    const XactCommandEvent& event,
                                    std::int32_t status) noexcept;

struct XactCommandStats {
    std::uint64_t posted = 0;
    std::uint64_t applied = 0;
    std::uint64_t apply_failed = 0;
    std::uint64_t ring_overflow = 0;      // ring full: not posted, counted
    std::uint64_t table_full = 0;         // all 1024 cue slots busy
    std::uint64_t unknown_command = 0;    // applied, but the value is unproven
    std::uint64_t shutdown_commands = 0;  // command word 2 (0x18000A869)
    std::uint64_t refused_after_shutdown = 0;
    std::uint64_t abandoned_shutdown = 0; // queued behind a shutdown command
    std::uint64_t abandoned_generation = 0;
    std::uint64_t invalid = 0;
    std::uint64_t generation_changes = 0;
    std::uint64_t drains = 0;
    std::uint32_t pending = 0;
    std::uint32_t high_watermark = 0;
};

// Owner -> audio handoff for the cue table above.
//
// Producer: one thread (the game's command producer). Consumer: the audio
// callback, through DrainAtBlockStart(). Single producer / single consumer; the
// ring holds cue indices only, so no record payload is copied twice.
class XactCommandRing final {
public:
    enum class PostStatus : std::uint8_t {
        Accepted = 0,
        TableFull,  // no free cue slot
        RingFull,   // ring at capacity
        Shutdown,   // a shutdown command (2) was already consumed
        Invalid,    // detached table, or the free command word as a command
    };

    XactCommandRing() = default;

    XactCommandRing(const XactCommandRing&) = delete;
    XactCommandRing& operator=(const XactCommandRing&) = delete;

    // Binds the ring to an attached table. Called once, before any producer or
    // consumer runs. Detaches on a null or detached table.
    [[nodiscard]] bool Prepare(XactCueTable* table) noexcept;
    [[nodiscard]] bool Prepared() const noexcept { return table_ != nullptr; }

    // Writes `record` into the lowest free cue slot and appends the slot to the
    // ring. The command word is published last. A refused post leaves the table
    // unchanged and is counted.
    [[nodiscard]] PostStatus Post(const XactCueRecord& record) noexcept;

    // Drains every queued command once, at the start of an audio block. A
    // command posted after this call is applied by the next block (one block of
    // latency). Returns the number of commands the sink saw.
    std::uint32_t DrainAtBlockStart(XactCommandSink sink,
                                    void* sink_context,
                                    XactCompletionSink completion,
                                    void* completion_context) noexcept;

    // A new game/device generation invalidates every queued and pending cue:
    // non-zero command words are cleared, the ring is emptied, and both are
    // counted. A repeated generation is a no-op.
    void SetGeneration(std::uint64_t generation) noexcept;
    [[nodiscard]] std::uint64_t Generation() const noexcept {
        return generation_.load(std::memory_order_relaxed);
    }
    [[nodiscard]] bool ShutdownReceived() const noexcept {
        return shutdown_.load(std::memory_order_relaxed);
    }
    [[nodiscard]] std::uint32_t Pending() const noexcept;
    [[nodiscard]] XactCommandStats Stats() const noexcept;

private:
    XactCueTable* table_ = nullptr;

    std::uint32_t slots_[kCommandRingCapacity]{};
    std::atomic<std::uint32_t> head_{0};
    std::atomic<std::uint32_t> tail_{0};

    std::atomic<std::uint64_t> generation_{0};
    std::atomic<std::uint64_t> sequence_{0};
    std::atomic<bool> shutdown_{false};

    std::atomic<std::uint64_t> posted_{0};
    std::atomic<std::uint64_t> applied_{0};
    std::atomic<std::uint64_t> apply_failed_{0};
    std::atomic<std::uint64_t> ring_overflow_{0};
    std::atomic<std::uint64_t> table_full_{0};
    std::atomic<std::uint64_t> unknown_command_{0};
    std::atomic<std::uint64_t> shutdown_commands_{0};
    std::atomic<std::uint64_t> refused_after_shutdown_{0};
    std::atomic<std::uint64_t> abandoned_shutdown_{0};
    std::atomic<std::uint64_t> abandoned_generation_{0};
    std::atomic<std::uint64_t> invalid_{0};
    std::atomic<std::uint64_t> generation_changes_{0};
    std::atomic<std::uint64_t> drains_{0};
    std::atomic<std::uint32_t> high_watermark_{0};
};

} // namespace xact

} // namespace nosfix::audio
