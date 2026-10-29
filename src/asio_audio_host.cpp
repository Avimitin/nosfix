#include "nosfix/asio_audio_host.hpp"

#include <cmath>
#include <new>
#include <utility>

namespace nosfix::audio {

AsioAudioHost::AsioAudioHost() = default;

AsioAudioHost::~AsioAudioHost() {
    Close();
}

bool AsioAudioHost::Open(const AsioHostConfig& config, std::string& error) {
    return OpenInternal(nullptr, config, error, true);
}

bool AsioAudioHost::OpenWithDriver(std::unique_ptr<AsioDriver> driver,
                                   const AsioHostConfig& config,
                                   std::string& error) {
    if (driver == nullptr) {
        error = "no driver supplied";
        return false;
    }
    return OpenInternal(std::move(driver), config, error, false);
}

bool AsioAudioHost::OpenInternal(std::unique_ptr<AsioDriver> driver,
                                 const AsioHostConfig& config, std::string& error,
                                 bool use_system_driver) {
    if (open_) {
        error = "already open";
        return false;
    }
    if (config.output.driver_clsid.empty()) {
        error = "an exact driver CLSID is required";
        return false;
    }
    if (config.output.output_channels.empty()) {
        error = "explicit output channels are required";
        return false;
    }
    if (config.output.buffer_frames <= 0) {
        error = "an explicit buffer size is required";
        return false;
    }
    if (!(config.output.engine_sample_rate > 0.0)) {
        error = "an explicit engine sample rate is required";
        return false;
    }
    if (config.bridge.device_rate !=
        static_cast<std::uint32_t>(config.output.engine_sample_rate)) {
        error = "bridge rate does not match the ASIO engine rate";
        return false;
    }
    if (config.bridge.device_channels != config.output.output_channels.size()) {
        error = "bridge channels do not match the explicit output channel count";
        return false;
    }
    if (config.mixer_max_block_frames < static_cast<std::uint32_t>(config.output.buffer_frames)) {
        error = "mixer block size is smaller than the ASIO buffer";
        return false;
    }
    if (config.keysound_capacity < KeysoundQueue::kMinCapacity ||
        config.keysound_capacity > KeysoundQueue::kMaxCapacity ||
        (config.keysound_capacity & (config.keysound_capacity - 1U)) != 0) {
        error = "keysound capacity must be a power of two in 64..65536";
        return false;
    }
    if (config.initial_sequence == 0) {
        error = "the initial keysound sequence must be nonzero";
        return false;
    }

    Xa2BridgeConfig bridge = config.bridge;
    bridge.max_block_frames = config.mixer_max_block_frames;
    engine_ = Xa2BridgeCreate(bridge);
    if (engine_ == nullptr) {
        error = "the mixer could not be prepared for this rate/channels/block";
        return false;
    }
    keysounds_.reset(new (std::nothrow) KeysoundQueue(config.keysound_capacity));
    if (keysounds_ == nullptr || !keysounds_->Valid()) {
        keysounds_.reset();
        Xa2BridgeDestroy(engine_);
        engine_ = nullptr;
        error = "the keysound queue could not be allocated";
        return false;
    }
    keysounds_->SetMapper(&MapQpcThunk, this);
    config_ = config;
    sequence_.store(config.initial_sequence, std::memory_order_relaxed);

    const bool opened = use_system_driver
                            ? output_.OpenSystem(config.output, error)
                            : output_.Open(std::move(driver), config.output, error);
    if (!opened) {
        keysounds_.reset();
        Xa2BridgeDestroy(engine_);
        engine_ = nullptr;
        config_ = AsioHostConfig{};
        return false;
    }

    // The driver answered with its real buffer size, rate and channel list. The
    // host never silently accepts a different one.
    const AsioOutputInfo info = output_.Info();
    const char* mismatch = nullptr;
    if (info.buffer_frames != config.output.buffer_frames) {
        mismatch = "driver buffer size differs from the request";
    } else if (std::fabs(info.sample_rate - config.output.engine_sample_rate) > 1e-6) {
        mismatch = "driver sample rate differs from the request";
    } else if (info.channels.size() != config.output.output_channels.size()) {
        mismatch = "driver channel count differs from the request";
    } else if (!info.output_latency_reported) {
        mismatch = "driver did not report output latency";
    }
    if (mismatch != nullptr) {
        error = mismatch;
        output_.Close();
        keysounds_.reset();
        Xa2BridgeDestroy(engine_);
        engine_ = nullptr;
        config_ = AsioHostConfig{};
        return false;
    }

    output_.SetRenderCallback(&RenderThunk, this);
    last_generation_ = 0;
    last_block_frames_ = 0;
    open_ = true;
    return true;
}

bool AsioAudioHost::Start(std::string& error) {
    if (!open_) {
        error = "not open";
        return false;
    }
    if (!output_.Start(error)) return false;
    last_generation_ = output_.Generation();
    last_generation_published_.store(last_generation_, std::memory_order_relaxed);
    return true;
}

void AsioAudioHost::Stop() noexcept {
    if (!open_) return;
    output_.Stop();
}

void AsioAudioHost::Close() noexcept {
    if (keysounds_) keysounds_->SetMapper(nullptr, nullptr);
    output_.Close();
    if (engine_ != nullptr) {
        Xa2BridgeDestroy(engine_);
        engine_ = nullptr;
    }
    keysounds_.reset();
    open_ = false;
    last_generation_ = 0;
    last_block_frames_ = 0;
}

bool AsioAudioHost::Service(std::string& error) {
    if (!open_) {
        error = "not open";
        return false;
    }
    return output_.Service(error);
}

bool AsioAudioHost::Restart(std::uint64_t new_generation,
                            std::int32_t new_buffer_frames, std::string& error) {
    if (!open_) {
        error = "not open";
        return false;
    }
    if (new_buffer_frames > static_cast<std::int32_t>(config_.mixer_max_block_frames)) {
        error = "requested buffer exceeds the mixer block size";
        return false;
    }
    return output_.Restart(new_generation, new_buffer_frames, error);
}

bool AsioAudioHost::BindCueVoice(std::uint32_t cue_id, Xa2Voice* voice) noexcept {
    if (cue_id == 0) {
        for (auto& binding : bindings_) binding = CueBinding{};
        return true;
    }
    if (voice == nullptr) {
        for (auto& binding : bindings_) {
            if (binding.cue_id == cue_id) binding = CueBinding{};
        }
        return true;
    }
    for (auto& binding : bindings_) {
        if (binding.cue_id == cue_id) {
            binding.voice = voice;
            return true;
        }
    }
    for (auto& binding : bindings_) {
        if (binding.cue_id == 0) {
            binding.cue_id = cue_id;
            binding.voice = voice;
            return true;
        }
    }
    return false;
}

void AsioAudioHost::SetKeysoundMapper(QpcToSampleMapper mapper,
                                      void* context) noexcept {
    if (keysounds_) keysounds_->SetMapper(mapper, context);
}

Xa2Voice* AsioAudioHost::BoundVoice(std::uint32_t cue_id) const noexcept {
    if (cue_id == 0) return nullptr;
    for (const auto& binding : bindings_) {
        if (binding.cue_id == cue_id) return binding.voice;
    }
    return nullptr;
}

KeysoundPushResult AsioAudioHost::PushKeysound(KeysoundRequest request) noexcept {
    if (keysounds_ == nullptr) return {KeysoundPushStatus::InvalidArgument};
    if (request.sequence == 0) {
        request.sequence = NextSequence();
    }
    return keysounds_->Push(request);
}

bool AsioAudioHost::RenderThunk(void* context, const AsioRenderContext& ctx,
                                float* const* channels) noexcept {
    return static_cast<AsioAudioHost*>(context)->Render(ctx, channels);
}

bool AsioAudioHost::MapQpcThunk(void* context, std::int64_t qpc_ticks,
                                std::int64_t& sample_out) noexcept {
    auto* host = static_cast<AsioAudioHost*>(context);
    const auto mapped = host->output_.Timeline().MapQpcToRenderSample(qpc_ticks);
    if (!mapped) return false;
    sample_out = *mapped;
    return true;
}

bool AsioAudioHost::StartVoiceThunk(void* context,
                                    const KeysoundDispatch& dispatch) noexcept {
    auto* host = static_cast<AsioAudioHost*>(context);
    Xa2Voice* voice = host->BoundVoice(dispatch.cue_id);
    if (voice == nullptr) {
        host->unmapped_.fetch_add(1, std::memory_order_relaxed);
        return false;
    }
    const float gain = std::pow(10.0F, dispatch.volume_db / 20.0F);
    if (!Xa2VoiceSetVolumePan(voice, gain, dispatch.pan)) return false;
    if (!Xa2VoiceStartAt(voice, dispatch.offset_in_block)) return false;
    host->voice_starts_.fetch_add(1, std::memory_order_relaxed);
    return true;
}

bool AsioAudioHost::Render(const AsioRenderContext& context,
                           float* const* channels) noexcept {
    blocks_.fetch_add(1, std::memory_order_relaxed);
    if (engine_ == nullptr || keysounds_ == nullptr) {
        render_failures_.fetch_add(1, std::memory_order_relaxed);
        return false;
    }
    if (!context.sample_position_valid) {
        silent_blocks_.fetch_add(1, std::memory_order_relaxed);
        return false;
    }
    if (context.buffer_frames == 0 ||
        context.buffer_frames > config_.mixer_max_block_frames) {
        render_failures_.fetch_add(1, std::memory_order_relaxed);
        return false;
    }

    if (context.generation != last_generation_) {
        if (last_generation_ != 0) {
            generation_changes_.fetch_add(1, std::memory_order_relaxed);
        }
        last_generation_ = context.generation;
        last_generation_published_.store(last_generation_, std::memory_order_relaxed);
        AudioMixer* mixer = Xa2BridgeMixer(engine_);
        if (mixer != nullptr) mixer->StopAllVoices();
    }
    if (context.buffer_frames != last_block_frames_) {
        buffer_size_changes_.fetch_add(1, std::memory_order_relaxed);
        last_block_frames_ = context.buffer_frames;
        last_block_frames_published_.store(last_block_frames_,
                                           std::memory_order_relaxed);
    }

    const KeysoundDispatchOutcome outcome = keysounds_->DispatchDue(
        context.block_start_sample, context.buffer_frames, context.generation,
        &StartVoiceThunk, this);
    if (outcome.dispatched != 0) {
        dispatched_.fetch_add(outcome.dispatched, std::memory_order_relaxed);
    }
    if (outcome.late != 0) late_.fetch_add(outcome.late, std::memory_order_relaxed);
    if (outcome.sink_rejected != 0) {
        sink_rejected_.fetch_add(outcome.sink_rejected, std::memory_order_relaxed);
    }
    if (outcome.dropped_stale != 0) {
        stale_.fetch_add(outcome.dropped_stale, std::memory_order_relaxed);
    }

    const bool rendered = Xa2BridgeRenderBlock(
        engine_, context.block_start_sample, context.buffer_frames, channels,
        config_.bridge.device_channels);
    if (!rendered) {
        render_failures_.fetch_add(1, std::memory_order_relaxed);
        return false;
    }
    return true;
}

AsioHostStats AsioAudioHost::Stats() const noexcept {
    AsioHostStats stats;
    stats.blocks = blocks_.load(std::memory_order_relaxed);
    stats.silent_blocks = silent_blocks_.load(std::memory_order_relaxed);
    stats.render_failures = render_failures_.load(std::memory_order_relaxed);
    stats.keysounds_dispatched = dispatched_.load(std::memory_order_relaxed);
    stats.keysounds_late = late_.load(std::memory_order_relaxed);
    stats.keysounds_sink_rejected = sink_rejected_.load(std::memory_order_relaxed);
    stats.keysounds_stale = stale_.load(std::memory_order_relaxed);
    stats.generation_changes = generation_changes_.load(std::memory_order_relaxed);
    stats.buffer_size_changes = buffer_size_changes_.load(std::memory_order_relaxed);
    stats.unmapped_dispatches = unmapped_.load(std::memory_order_relaxed);
    stats.voice_starts = voice_starts_.load(std::memory_order_relaxed);
    stats.last_block_frames = last_block_frames_published_.load(std::memory_order_relaxed);
    stats.last_generation = last_generation_published_.load(std::memory_order_relaxed);
    return stats;
}

} // namespace nosfix::audio
