#pragma once

#include "nosfix/asio_output.hpp"
#include "nosfix/keysound_dispatch.hpp"
#include "nosfix/xaudio2_bridge.hpp"

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>

// Binds the XAudio2 2.7 bridge, its mixer and the keysound queue to one ASIO
// output device and one render-sample timeline.
//
// Ownership
// ---------
// * Control thread: Open/Start/Stop/Service/Restart/Close, BindCueVoice,
//   PushKeysound, Stats. Same rule as AsioOutput: one thread for the device
//   lifecycle.
// * Audio thread: exactly one function, the AsioRenderCallback thunk. It
//   dispatches due keysounds (sample-accurate offsets), then renders the mixer
//   for the same block on the same device timeline. It allocates nothing and
//   calls no game code.
//
// Failure model
// -------------
// * No default device, channel pair, rate or buffer size: every value comes from
//   AsioHostConfig and is validated by AsioOutput.
// * A block without a valid driver sample position is silence and is counted. No
//   position is invented.
// * A device generation change (stop/start, reset, driver-observed jump) stops
//   every mixer voice, because their cursors belong to the old device run.
// * A keysound whose cue id has no bound voice is refused by the sink and
//   counted by the queue; it is never played on an arbitrary voice.
namespace nosfix::audio {

inline constexpr std::uint32_t kHostMaxCueBindings = 64;

struct AsioHostConfig {
    // Device binding. engine_sample_rate and the output channel count must match
    // the bridge config; Open fails otherwise instead of resampling or remixing.
    AsioOpenOptions output{};

    // Mixer/engine side. device_rate must equal output.engine_sample_rate and
    // device_channels must equal output.output_channels.size().
    Xa2BridgeConfig bridge{};

    std::uint32_t keysound_capacity = 1024; // power of two, 64..65536

    // First sequence number handed out by NextSequence(). Must be > 0.
    std::uint64_t initial_sequence = 1;

    // Mixer scratch size. Must be >= the ASIO buffer size.
    std::uint32_t mixer_max_block_frames = 1024;
};

struct AsioHostStats {
    std::uint64_t blocks = 0;
    std::uint64_t silent_blocks = 0;   // no valid sample position
    std::uint64_t render_failures = 0; // mixer refused the block
    std::uint64_t keysounds_dispatched = 0;
    std::uint64_t keysounds_late = 0;
    std::uint64_t keysounds_sink_rejected = 0;
    std::uint64_t keysounds_stale = 0;
    std::uint64_t generation_changes = 0;
    std::uint64_t buffer_size_changes = 0;
    std::uint64_t unmapped_dispatches = 0;
    std::uint64_t voice_starts = 0;
    std::uint32_t last_block_frames = 0;
    std::uint64_t last_generation = 0;
};

class AsioAudioHost final {
public:
    AsioAudioHost();
    ~AsioAudioHost();

    AsioAudioHost(const AsioAudioHost&) = delete;
    AsioAudioHost& operator=(const AsioAudioHost&) = delete;

    // Creates the COM driver for options.driver_clsid. Fails closed on any
    // mismatch between the ASIO options and the bridge config, an invalid
    // keysound capacity, a driver error, or a mixer that cannot be prepared.
    [[nodiscard]] bool Open(const AsioHostConfig& config, std::string& error);
    // Same, with an injected driver (fake driver tests).
    [[nodiscard]] bool OpenWithDriver(std::unique_ptr<AsioDriver> driver,
                                      const AsioHostConfig& config,
                                      std::string& error);

    [[nodiscard]] bool Start(std::string& error);
    void Stop() noexcept;
    void Close() noexcept;
    [[nodiscard]] bool Service(std::string& error);
    [[nodiscard]] bool Restart(std::uint64_t new_generation,
                               std::int32_t new_buffer_frames,
                               std::string& error);

    // Control thread. Binds a cue id to a source voice created through the
    // bridge. Binding cue_id 0 or a null voice clears the entry.
    [[nodiscard]] bool BindCueVoice(std::uint32_t cue_id, Xa2Voice* voice) noexcept;
    [[nodiscard]] Xa2Voice* BoundVoice(std::uint32_t cue_id) const noexcept;

    // Installs the QPC -> render-sample mapper used by DispatchDue. Setup only:
    // call while no callback is running (before Start or after Stop). The
    // integration normally leaves the AsioOutput timeline mapper in place; this
    // exists for hosts that own a different clock and for deterministic tests.
    void SetKeysoundMapper(QpcToSampleMapper mapper, void* context) noexcept;

    // Producer side of the keysound queue. The caller sets generation,
    // capture_qpc and cue_id; sequence is filled from NextSequence() when zero.
    [[nodiscard]] KeysoundPushResult PushKeysound(KeysoundRequest request) noexcept;
    [[nodiscard]] std::uint64_t NextSequence() noexcept {
        return sequence_.fetch_add(1, std::memory_order_relaxed);
    }

    [[nodiscard]] Xa2Engine* Engine() noexcept { return engine_; }
    [[nodiscard]] AsioOutput& Output() noexcept { return output_; }
    [[nodiscard]] const AsioOutput& Output() const noexcept { return output_; }
    [[nodiscard]] KeysoundQueue* Keysounds() noexcept { return keysounds_.get(); }
    [[nodiscard]] std::uint64_t Generation() const noexcept { return output_.Generation(); }

    [[nodiscard]] AsioHostStats Stats() const noexcept;

    // The render body, exposed for a fake driver that calls it directly.
    [[nodiscard]] bool RenderForTest(const AsioRenderContext& context,
                                     float* const* channels) noexcept {
        return Render(context, channels);
    }

private:
    static bool RenderThunk(void* context, const AsioRenderContext& ctx,
                            float* const* channels) noexcept;
    static bool MapQpcThunk(void* context, std::int64_t qpc_ticks,
                            std::int64_t& sample_out) noexcept;
    static bool StartVoiceThunk(void* context,
                                const KeysoundDispatch& dispatch) noexcept;

    [[nodiscard]] bool OpenInternal(std::unique_ptr<AsioDriver> driver,
                                    const AsioHostConfig& config,
                                    std::string& error,
                                    bool use_system_driver);
    [[nodiscard]] bool Render(const AsioRenderContext& context,
                              float* const* channels) noexcept;

    AsioOutput output_{};
    AsioHostConfig config_{};
    std::unique_ptr<KeysoundQueue> keysounds_;
    Xa2Engine* engine_ = nullptr;
    bool open_ = false;

    struct CueBinding {
        std::uint32_t cue_id = 0;
        Xa2Voice* voice = nullptr;
    };
    CueBinding bindings_[kHostMaxCueBindings]{};

    std::atomic<std::uint64_t> sequence_{1};

    // Audio-thread state.
    std::uint64_t last_generation_ = 0;
    std::uint32_t last_block_frames_ = 0;

    std::atomic<std::uint64_t> blocks_{0};
    std::atomic<std::uint64_t> silent_blocks_{0};
    std::atomic<std::uint64_t> render_failures_{0};
    std::atomic<std::uint64_t> dispatched_{0};
    std::atomic<std::uint64_t> late_{0};
    std::atomic<std::uint64_t> sink_rejected_{0};
    std::atomic<std::uint64_t> stale_{0};
    std::atomic<std::uint64_t> generation_changes_{0};
    std::atomic<std::uint64_t> buffer_size_changes_{0};
    std::atomic<std::uint64_t> unmapped_{0};
    std::atomic<std::uint64_t> voice_starts_{0};
    std::atomic<std::uint32_t> last_block_frames_published_{0};
    std::atomic<std::uint64_t> last_generation_published_{0};
};

} // namespace nosfix::audio
