#pragma once

#include <atomic>
#include <cstdint>

// Bounded, allocation-free audio mixer for the XAudio2 2.7 bridge.
//
// Threading model
// ---------------
// * A single owner thread (the libxact command worker in the game, a test thread
//   otherwise) calls CreateVoice/DestroyVoice/SubmitBuffer/StartVoice/... . Those
//   calls never touch playback state directly: each one writes a POD command into
//   a bounded single-producer/single-consumer ring.
// * The audio callback thread calls Render(). It drains the command ring, then
//   mixes. It allocates nothing, takes no locks, logs nothing, waits for nothing
//   and calls only the registered POD voice callbacks. A full ring is refused and
//   counted; it is never silently dropped.
// * GetState()/Stats() are lock-free atomic snapshots safe from any thread.
//
// Timeline
// --------
// Render(block_start_sample, frames, ...) is expressed in the device render
// timeline: block_start_sample is the sample index of the first frame of the
// block (ASIO samplePosition). A StartVoice offset is applied inside that block,
// so a keysound dispatched with a sample-accurate offset starts on exactly that
// frame. Voices created on the same mixer share this one timeline with BGM.
//
// Scope (fails closed outside it)
// -------------------------------
// * Source formats: PCM 16/24/32-bit little endian and IEEE float32, 1..8
//   channels. Anything else is rejected by the bridge before a voice exists; the
//   mixer itself rejects an invalid MixerFormat.
// * A voice mixes into one bus; buses mix into the device output at a uniform
//   gain. Non-uniform (pan/filter) matrices are rejected by the bridge, not
//   approximated here.
// * Rate conversion is linear interpolation driven by an exact 32.32 fixed-point
//   step; it is a pitch/source-rate ratio, not a high quality resampler.
namespace nosfix::audio {

enum class AudioEncoding : std::uint8_t {
    None,
    Int16,
    Int24,
    Int32,
    Float32,
};

struct MixerFormat {
    std::uint32_t sample_rate = 0;
    std::uint16_t channels = 0;
    AudioEncoding encoding = AudioEncoding::None;

    [[nodiscard]] bool Valid() const noexcept;
    [[nodiscard]] std::uint32_t BytesPerFrame() const noexcept;
    [[nodiscard]] std::uint32_t BytesPerSample() const noexcept;
};

inline constexpr std::uint32_t kMixerMaxVoices = 64;
inline constexpr std::uint32_t kMixerMaxBuses = 16;
inline constexpr std::uint32_t kMixerMaxChannels = 8;
inline constexpr std::uint32_t kMixerMaxBlockFrames = 2048;
inline constexpr std::uint32_t kMixerMinBlockFrames = 1;
inline constexpr std::uint32_t kMixerMaxBuffersPerVoice = 4;
inline constexpr std::uint32_t kMixerCommandCapacity = 256; // power of two
inline constexpr std::uint32_t kMixerLoopInfinite = 255;    // XAUDIO2_LOOP_INFINITE
inline constexpr std::uint32_t kMixerEndOfStream = 0x0040;  // XAUDIO2_END_OF_STREAM

struct MixerBufferDesc {
    const std::uint8_t* data = nullptr;
    std::uint32_t frame_count = 0; // frames in `data`
    std::uint32_t play_begin = 0;
    std::uint32_t play_length = 0; // 0 = to the end
    std::uint32_t loop_begin = 0;  // relative to play_begin
    std::uint32_t loop_length = 0; // 0 = to the end of the play region
    std::uint32_t loop_count = 0;  // 0 = no loop, kMixerLoopInfinite = forever
    std::uint32_t flags = 0;
    void* context = nullptr;

    [[nodiscard]] bool Valid(const MixerFormat& format) const noexcept;
};

struct MixerVoiceCallbacks {
    void* context = nullptr;
    void (*voice_processing_pass_start)(void* context,
                                        std::uint32_t frames) noexcept = nullptr;
    void (*voice_processing_pass_end)(void* context) noexcept = nullptr;
    void (*stream_end)(void* context) noexcept = nullptr;
    void (*buffer_start)(void* context, void* buffer_context) noexcept = nullptr;
    void (*buffer_end)(void* context, void* buffer_context) noexcept = nullptr;
    void (*loop_end)(void* context, void* buffer_context) noexcept = nullptr;
    void (*voice_error)(void* context, std::uint32_t hresult) noexcept = nullptr;
};

struct MixerVoiceDesc {
    MixerFormat format{};
    float max_frequency_ratio = 2.0F;
    std::uint32_t bus = 0; // 0 = master; 1..kMixerMaxBuses-1 = submix
    MixerVoiceCallbacks callbacks{};
};

struct MixerVoiceState {
    bool active = false;
    bool ended = false;
    std::uint32_t buffers_queued = 0;
    std::uint64_t samples_played = 0; // source frames consumed
    void* current_buffer_context = nullptr;
};

struct MixerStats {
    std::uint64_t blocks = 0;
    std::uint64_t frames_rendered = 0;
    std::uint64_t underruns = 0;
    std::uint64_t starved_voices = 0;
    std::uint64_t clipped_frames = 0;
    std::uint64_t command_overflow = 0;
    std::uint64_t buffer_overflow = 0;
    std::uint64_t rejected = 0;
    std::uint32_t active_voices = 0;
    std::uint32_t max_active_voices = 0;
    std::uint32_t voices_created = 0;
};

class AudioMixer final {
public:
    AudioMixer() = default;
    ~AudioMixer() = default;

    AudioMixer(const AudioMixer&) = delete;
    AudioMixer& operator=(const AudioMixer&) = delete;

    // Configures the device side. Called once before any voice exists. Bounded
    // and allocation free: every buffer is a member array. Rejects a rate outside
    // 8000..384000, channels outside 1..kMixerMaxChannels and a block larger than
    // kMixerMaxBlockFrames.
    [[nodiscard]] bool Prepare(std::uint32_t device_rate,
                               std::uint32_t device_channels,
                               std::uint32_t max_block_frames) noexcept;

    [[nodiscard]] bool Prepared() const noexcept { return prepared_; }
    [[nodiscard]] std::uint32_t DeviceRate() const noexcept { return device_rate_; }
    [[nodiscard]] std::uint32_t DeviceChannels() const noexcept { return device_channels_; }
    [[nodiscard]] std::uint32_t MaxBlockFrames() const noexcept { return max_block_frames_; }

    // --- owner-thread API -------------------------------------------------
    // A rejected command returns false and is counted in Stats(); callers may
    // ignore the result because the failure is never silent.
    // Returns a handle > 0, or 0 on rejection (not prepared, invalid format,
    // pool exhausted, command ring full).
    [[nodiscard]] std::int32_t CreateVoice(const MixerVoiceDesc& desc) noexcept;
    void DestroyVoice(std::int32_t voice) noexcept;

    bool SubmitBuffer(std::int32_t voice,
                                    const MixerBufferDesc& buffer) noexcept;
    // Starts the voice at offset_in_block inside the next rendered block. An
    // offset beyond the block start is clamped to the block start (late) by the
    // renderer, which counts it as a starved start.
    bool StartVoice(std::int32_t voice,
                                  std::uint32_t offset_in_block = 0) noexcept;
    bool StopVoice(std::int32_t voice) noexcept;
    bool FlushVoice(std::int32_t voice) noexcept;
    bool SetVoiceVolumePan(std::int32_t voice, float volume,
                                         float pan) noexcept;
    bool SetVoiceFrequencyRatio(std::int32_t voice, float ratio) noexcept;
    // XAudio2 ExitLoop: play the current loop region to its end, then continue.
    bool ExitVoiceLoop(std::int32_t voice) noexcept;
    bool SetBusGain(std::uint32_t bus, float gain) noexcept;
    // Stops every allocated voice (device reset / generation change). Each stop
    // is a bounded command; if the ring is full the rest are counted as overflow
    // and remain running.
    void StopAllVoices() noexcept;

    // --- any thread -------------------------------------------------------
    [[nodiscard]] MixerVoiceState GetState(std::int32_t voice) const noexcept;
    [[nodiscard]] MixerStats Stats() const noexcept;
    [[nodiscard]] bool VoiceAlive(std::int32_t voice) const noexcept;

    // --- audio callback thread -------------------------------------------
    // Renders one block of the device timeline into planar float channels.
    // Clears `output` first. Returns false when the block cannot be rendered
    // (not prepared, frames == 0 or > max_block_frames, null channel pointer);
    // the caller treats that as an underrun. When a voice has no data it is
    // counted as starved and contributes silence.
    [[nodiscard]] bool Render(std::int64_t block_start_sample,
                              std::uint32_t frames,
                              float* const* output,
                              std::uint32_t output_channels) noexcept;

private:
    enum class CommandKind : std::uint8_t {
        Start,
        Stop,
        Flush,
        Submit,
        VolumePan,
        Frequency,
        ExitLoop,
        Destroy,
        BusGain,
    };

    struct Command {
        CommandKind kind = CommandKind::Start;
        std::uint8_t voice = 0;
        std::uint16_t bus = 0;
        std::uint32_t a = 0;
        std::uint32_t b = 0;
        std::uint32_t c = 0;
        std::uint32_t d = 0;
        std::uint32_t e = 0;
        std::uint32_t f = 0;
        float f0 = 0.0F;
        float f1 = 0.0F;
        const std::uint8_t* data = nullptr;
        void* context = nullptr;
    };

    struct VoiceRuntime {
        // Audio-thread state.
        MixerBufferDesc queue[kMixerMaxBuffersPerVoice]{};
        std::uint32_t queue_head = 0;
        std::uint32_t queue_count = 0;
        std::uint64_t position_q32 = 0; // source frame << 32 inside the current buffer
        std::uint64_t step_q32 = 0;     // source frames advanced per output frame
        std::uint64_t played_q32 = 0;   // cumulative source frames << 32
        std::uint32_t loop_remaining = 0;
        std::uint32_t start_offset = 0;
        std::uint32_t play_end = 0;
        std::uint32_t loop_begin = 0;
        std::uint32_t loop_end = 0;
        std::uint8_t state = 0; // 0 idle, 1 playing, 2 ended
        bool buffer_loaded = false;
        float gain[kMixerMaxChannels]{}; // per source channel

        // Owner-thread state.
        MixerFormat format{};
        std::uint32_t bus = 0;
        std::uint32_t epoch = 1;
        bool destroyed = false;
        float volume = 1.0F;
        float pan = 0.0F;
        float ratio = 1.0F;
        float max_ratio = 2.0F;
        MixerVoiceCallbacks callbacks{};

        // Published atomics.
        std::atomic<std::uint32_t> published_state{0};
        std::atomic<std::uint32_t> published_buffers{0};
        std::atomic<std::uint64_t> published_samples{0};
        std::atomic<void*> published_context{nullptr};
        std::atomic<std::uint64_t> published_epoch{1};
        std::atomic<bool> published_alive{false};
    };

    static void ResetRuntime(VoiceRuntime& voice) noexcept;
    [[nodiscard]] bool PushCommand(const Command& command) noexcept;
    void ApplyCommand(const Command& command) noexcept;
    void StartVoiceInternal(std::uint32_t index, std::uint32_t offset) noexcept;
    void LoadCurrentBuffer(std::uint32_t index) noexcept;
    void AdvanceBuffer(std::uint32_t index) noexcept;
    void Publish(std::uint32_t index) noexcept;
    [[nodiscard]] bool DecodeHandle(std::int32_t handle, std::uint32_t& index) const noexcept;

    bool prepared_ = false;
    std::uint32_t device_rate_ = 0;
    std::uint32_t device_channels_ = 0;
    std::uint32_t max_block_frames_ = 0;
    float bus_gain_[kMixerMaxBuses]{};

    VoiceRuntime voices_[kMixerMaxVoices]{};

    // Commands: single producer (owner), single consumer (render).
    Command commands_[kMixerCommandCapacity]{};
    std::atomic<std::uint32_t> command_head_{0};
    std::atomic<std::uint32_t> command_tail_{0};

    // Bit set = slot allocated. The render thread clears a bit when it applies a
    // Destroy command, so a slot is only reused after the audio thread stopped
    // reading it. CreateVoice sets a bit before writing any voice field.
    std::atomic<std::uint64_t> slot_mask_{0};

    std::atomic<std::uint64_t> blocks_{0};
    std::atomic<std::uint64_t> frames_rendered_{0};
    std::atomic<std::uint64_t> underruns_{0};
    std::atomic<std::uint64_t> starved_voices_{0};
    std::atomic<std::uint64_t> clipped_frames_{0};
    std::atomic<std::uint64_t> command_overflow_{0};
    std::atomic<std::uint64_t> buffer_overflow_{0};
    std::atomic<std::uint64_t> rejected_{0};
    std::atomic<std::uint32_t> max_active_voices_{0};
    std::atomic<std::uint32_t> voices_created_{0};

    // Scratch accumulators, sized by Prepare(). Audio thread only.
    float bus_scratch_[kMixerMaxBuses][kMixerMaxChannels][kMixerMaxBlockFrames]{};
};

} // namespace nosfix::audio
