#include "nosfix/audio_mixer.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace nosfix::audio {
namespace {

constexpr std::uint32_t kMaxSourceRate = 384000;
constexpr std::uint32_t kMinSourceRate = 8000;
constexpr double kMaxRatio = 1024.0;
constexpr double kTwoPow32 = 4294967296.0;

} // namespace

bool MixerFormat::Valid() const noexcept {
    if (encoding == AudioEncoding::None) return false;
    if (channels == 0 || channels > kMixerMaxChannels) return false;
    if (sample_rate < kMinSourceRate || sample_rate > kMaxSourceRate) return false;
    return true;
}

std::uint32_t MixerFormat::BytesPerSample() const noexcept {
    switch (encoding) {
    case AudioEncoding::Int16: return 2;
    case AudioEncoding::Int24: return 3;
    case AudioEncoding::Int32: return 4;
    case AudioEncoding::Float32: return 4;
    case AudioEncoding::None: break;
    }
    return 0;
}

std::uint32_t MixerFormat::BytesPerFrame() const noexcept {
    return BytesPerSample() * static_cast<std::uint32_t>(channels);
}

bool MixerBufferDesc::Valid(const MixerFormat& format) const noexcept {
    if (!format.Valid()) return false;
    if (data == nullptr || frame_count == 0) return false;
    if (play_begin >= frame_count) return false;
    std::uint32_t play_frames = frame_count - play_begin;
    if (play_length != 0) {
        if (play_length > play_frames) return false;
        play_frames = play_length;
    }
    if (loop_count != 0) {
        if (loop_begin >= play_frames) return false;
        std::uint32_t loop_frames = play_frames - loop_begin;
        if (loop_length != 0) {
            if (loop_length > loop_frames) return false;
            loop_frames = loop_length;
        }
        if (loop_frames == 0) return false;
    }
    return true;
}

bool AudioMixer::Prepare(std::uint32_t device_rate,
                         std::uint32_t device_channels,
                         std::uint32_t max_block_frames) noexcept {
    if (device_rate < kMinSourceRate || device_rate > kMaxSourceRate) return false;
    if (device_channels == 0 || device_channels > kMixerMaxChannels) return false;
    if (max_block_frames < kMixerMinBlockFrames ||
        max_block_frames > kMixerMaxBlockFrames) {
        return false;
    }
    device_rate_ = device_rate;
    device_channels_ = device_channels;
    max_block_frames_ = max_block_frames;
    for (std::uint32_t bus = 0; bus < kMixerMaxBuses; ++bus) {
        bus_gain_[bus] = 1.0F;
    }
    for (auto& voice : voices_) {
        ResetRuntime(voice);
    }
    slot_mask_.store(0, std::memory_order_relaxed);
    prepared_ = true;
    return true;
}

void AudioMixer::ResetRuntime(VoiceRuntime& voice) noexcept {
    voice.format = MixerFormat{};
    voice.bus = 0;
    voice.epoch = 1;
    voice.destroyed = false;
    voice.volume = 1.0F;
    voice.pan = 0.0F;
    voice.ratio = 1.0F;
    voice.max_ratio = 2.0F;
    voice.callbacks = MixerVoiceCallbacks{};
    for (auto& buffer : voice.queue) buffer = MixerBufferDesc{};
    voice.queue_head = 0;
    voice.queue_count = 0;
    voice.position_q32 = 0;
    voice.step_q32 = 0;
    voice.played_q32 = 0;
    voice.loop_remaining = 0;
    voice.start_offset = 0;
    voice.play_end = 0;
    voice.loop_begin = 0;
    voice.loop_end = 0;
    voice.state = 0;
    voice.buffer_loaded = false;
    for (float& gain : voice.gain) gain = 1.0F;
    voice.published_state.store(0, std::memory_order_relaxed);
    voice.published_buffers.store(0, std::memory_order_relaxed);
    voice.published_samples.store(0, std::memory_order_relaxed);
    voice.published_context.store(nullptr, std::memory_order_relaxed);
    voice.published_epoch.store(1, std::memory_order_relaxed);
    voice.published_alive.store(false, std::memory_order_relaxed);
}

bool AudioMixer::DecodeHandle(std::int32_t handle, std::uint32_t& index) const noexcept {
    if (handle <= 0) return false;
    const std::uint32_t raw = static_cast<std::uint32_t>(handle) - 1U;
    const std::uint32_t slot = raw % kMixerMaxVoices;
    const std::uint32_t epoch = raw / kMixerMaxVoices;
    if (slot >= kMixerMaxVoices) return false;
    if (epoch != voices_[slot].epoch) return false;
    if (!voices_[slot].published_alive.load(std::memory_order_acquire)) return false;
    index = slot;
    return true;
}

std::int32_t AudioMixer::CreateVoice(const MixerVoiceDesc& desc) noexcept {
    if (!prepared_ || !desc.format.Valid()) {
        rejected_.fetch_add(1, std::memory_order_relaxed);
        return 0;
    }
    if (desc.bus >= kMixerMaxBuses) {
        rejected_.fetch_add(1, std::memory_order_relaxed);
        return 0;
    }
    if (!(desc.max_frequency_ratio >= 1.0F / 64.0F) ||
        desc.max_frequency_ratio > static_cast<float>(kMaxRatio)) {
        rejected_.fetch_add(1, std::memory_order_relaxed);
        return 0;
    }

    std::int32_t slot = -1;
    std::uint64_t mask = slot_mask_.load(std::memory_order_relaxed);
    while (slot < 0) {
        std::uint32_t candidate = kMixerMaxVoices;
        for (std::uint32_t index = 0; index < kMixerMaxVoices; ++index) {
            if ((mask & (1ULL << index)) == 0) {
                candidate = index;
                break;
            }
        }
        if (candidate == kMixerMaxVoices) break;
        const std::uint64_t desired = mask | (1ULL << candidate);
        if (slot_mask_.compare_exchange_weak(mask, desired,
                                             std::memory_order_acq_rel,
                                             std::memory_order_relaxed)) {
            slot = static_cast<std::int32_t>(candidate);
        }
    }
    if (slot < 0) {
        rejected_.fetch_add(1, std::memory_order_relaxed);
        return 0;
    }

    const std::uint32_t epoch =
        voices_created_.fetch_add(1, std::memory_order_relaxed) + 1U;
    VoiceRuntime& voice = voices_[static_cast<std::uint32_t>(slot)];
    ResetRuntime(voice);
    voice.format = desc.format;
    voice.bus = desc.bus;
    voice.callbacks = desc.callbacks;
    voice.max_ratio = desc.max_frequency_ratio;
    voice.volume = 1.0F;
    voice.pan = 0.0F;
    voice.ratio = 1.0F;
    voice.epoch = epoch;
    voice.destroyed = false;
    voice.state = 0;
    voice.buffer_loaded = false;
    voice.step_q32 =
        (static_cast<std::uint64_t>(desc.format.sample_rate) << 32U) /
        static_cast<std::uint64_t>(device_rate_);
    if (voice.step_q32 == 0) voice.step_q32 = 1;
    for (float& gain : voice.gain) gain = 1.0F;
    voice.published_state.store(0, std::memory_order_relaxed);
    voice.published_buffers.store(0, std::memory_order_relaxed);
    voice.published_samples.store(0, std::memory_order_relaxed);
    voice.published_context.store(nullptr, std::memory_order_relaxed);
    voice.published_epoch.store(epoch, std::memory_order_relaxed);
    voice.published_alive.store(true, std::memory_order_release);

    return static_cast<std::int32_t>(epoch * kMixerMaxVoices) + slot + 1;
}

void AudioMixer::DestroyVoice(std::int32_t voice) noexcept {
    std::uint32_t index = 0;
    if (!prepared_ || !DecodeHandle(voice, index)) {
        rejected_.fetch_add(1, std::memory_order_relaxed);
        return;
    }
    Command command;
    command.kind = CommandKind::Destroy;
    command.voice = static_cast<std::uint8_t>(index);
    if (!PushCommand(command)) return;
    VoiceRuntime& target = voices_[index];
    target.destroyed = true;
    target.published_alive.store(false, std::memory_order_release);
    ++target.epoch;
}

bool AudioMixer::PushCommand(const Command& command) noexcept {
    const std::uint32_t head = command_head_.load(std::memory_order_relaxed);
    const std::uint32_t tail = command_tail_.load(std::memory_order_acquire);
    if (tail - head >= kMixerCommandCapacity) {
        command_overflow_.fetch_add(1, std::memory_order_relaxed);
        return false;
    }
    commands_[tail & (kMixerCommandCapacity - 1U)] = command;
    command_tail_.store(tail + 1U, std::memory_order_release);
    return true;
}

bool AudioMixer::SubmitBuffer(std::int32_t voice,
                              const MixerBufferDesc& buffer) noexcept {
    std::uint32_t index = 0;
    if (!prepared_ || !DecodeHandle(voice, index) ||
        !buffer.Valid(voices_[index].format)) {
        rejected_.fetch_add(1, std::memory_order_relaxed);
        return false;
    }
    Command command;
    command.kind = CommandKind::Submit;
    command.voice = static_cast<std::uint8_t>(index);
    command.a = buffer.play_begin;
    command.b = buffer.play_length;
    command.c = buffer.loop_begin;
    command.d = buffer.loop_length;
    command.e = buffer.loop_count;
    command.f = buffer.flags;
    command.data = buffer.data;
    command.context = buffer.context;
    command.f0 = static_cast<float>(buffer.frame_count);
    return PushCommand(command);
}

bool AudioMixer::StartVoice(std::int32_t voice, std::uint32_t offset_in_block) noexcept {
    std::uint32_t index = 0;
    if (!prepared_ || !DecodeHandle(voice, index)) {
        rejected_.fetch_add(1, std::memory_order_relaxed);
        return false;
    }
    if (offset_in_block >= max_block_frames_) {
        rejected_.fetch_add(1, std::memory_order_relaxed);
        return false;
    }
    Command command;
    command.kind = CommandKind::Start;
    command.voice = static_cast<std::uint8_t>(index);
    command.a = offset_in_block;
    return PushCommand(command);
}

bool AudioMixer::StopVoice(std::int32_t voice) noexcept {
    std::uint32_t index = 0;
    if (!prepared_ || !DecodeHandle(voice, index)) {
        rejected_.fetch_add(1, std::memory_order_relaxed);
        return false;
    }
    Command command;
    command.kind = CommandKind::Stop;
    command.voice = static_cast<std::uint8_t>(index);
    return PushCommand(command);
}

bool AudioMixer::FlushVoice(std::int32_t voice) noexcept {
    std::uint32_t index = 0;
    if (!prepared_ || !DecodeHandle(voice, index)) {
        rejected_.fetch_add(1, std::memory_order_relaxed);
        return false;
    }
    Command command;
    command.kind = CommandKind::Flush;
    command.voice = static_cast<std::uint8_t>(index);
    return PushCommand(command);
}

bool AudioMixer::SetVoiceVolumePan(std::int32_t voice, float volume,
                                   float pan) noexcept {
    std::uint32_t index = 0;
    if (!prepared_ || !DecodeHandle(voice, index)) {
        rejected_.fetch_add(1, std::memory_order_relaxed);
        return false;
    }
    if (!std::isfinite(volume) || volume < 0.0F || volume > 64.0F ||
        !std::isfinite(pan) || pan < -1.0F || pan > 1.0F) {
        rejected_.fetch_add(1, std::memory_order_relaxed);
        return false;
    }
    Command command;
    command.kind = CommandKind::VolumePan;
    command.voice = static_cast<std::uint8_t>(index);
    command.f0 = volume;
    command.f1 = pan;
    return PushCommand(command);
}

bool AudioMixer::SetVoiceFrequencyRatio(std::int32_t voice, float ratio) noexcept {
    std::uint32_t index = 0;
    if (!prepared_ || !DecodeHandle(voice, index)) {
        rejected_.fetch_add(1, std::memory_order_relaxed);
        return false;
    }
    const float limit = voices_[index].max_ratio;
    if (!std::isfinite(ratio) || ratio < 1.0F / 64.0F || ratio > limit) {
        rejected_.fetch_add(1, std::memory_order_relaxed);
        return false;
    }
    Command command;
    command.kind = CommandKind::Frequency;
    command.voice = static_cast<std::uint8_t>(index);
    command.f0 = ratio;
    return PushCommand(command);
}

bool AudioMixer::ExitVoiceLoop(std::int32_t voice) noexcept {
    std::uint32_t index = 0;
    if (!prepared_ || !DecodeHandle(voice, index)) {
        rejected_.fetch_add(1, std::memory_order_relaxed);
        return false;
    }
    Command command;
    command.kind = CommandKind::ExitLoop;
    command.voice = static_cast<std::uint8_t>(index);
    return PushCommand(command);
}

bool AudioMixer::SetBusGain(std::uint32_t bus, float gain) noexcept {
    if (!prepared_ || bus >= kMixerMaxBuses || !std::isfinite(gain) ||
        gain < 0.0F || gain > 64.0F) {
        rejected_.fetch_add(1, std::memory_order_relaxed);
        return false;
    }
    Command command;
    command.kind = CommandKind::BusGain;
    command.bus = static_cast<std::uint16_t>(bus);
    command.f0 = gain;
    return PushCommand(command);
}

bool AudioMixer::VoiceAlive(std::int32_t voice) const noexcept {
    std::uint32_t index = 0;
    return prepared_ && DecodeHandle(voice, index);
}

void AudioMixer::StopAllVoices() noexcept {
    if (!prepared_) return;
    const std::uint64_t mask = slot_mask_.load(std::memory_order_acquire);
    for (std::uint32_t index = 0; index < kMixerMaxVoices; ++index) {
        if ((mask & (1ULL << index)) == 0) continue;
        Command command;
        command.kind = CommandKind::Stop;
        command.voice = static_cast<std::uint8_t>(index);
        if (!PushCommand(command)) return;
    }
}

MixerVoiceState AudioMixer::GetState(std::int32_t voice) const noexcept {
    MixerVoiceState state;
    std::uint32_t index = 0;
    if (!prepared_ || !DecodeHandle(voice, index)) return state;
    const VoiceRuntime& target = voices_[index];
    const std::uint32_t published = target.published_state.load(std::memory_order_relaxed);
    state.active = published == 1;
    state.ended = published == 2;
    state.buffers_queued = target.published_buffers.load(std::memory_order_relaxed);
    state.samples_played = target.published_samples.load(std::memory_order_relaxed);
    state.current_buffer_context =
        target.published_context.load(std::memory_order_relaxed);
    return state;
}

MixerStats AudioMixer::Stats() const noexcept {
    MixerStats stats;
    stats.blocks = blocks_.load(std::memory_order_relaxed);
    stats.frames_rendered = frames_rendered_.load(std::memory_order_relaxed);
    stats.underruns = underruns_.load(std::memory_order_relaxed);
    stats.starved_voices = starved_voices_.load(std::memory_order_relaxed);
    stats.clipped_frames = clipped_frames_.load(std::memory_order_relaxed);
    stats.command_overflow = command_overflow_.load(std::memory_order_relaxed);
    stats.buffer_overflow = buffer_overflow_.load(std::memory_order_relaxed);
    stats.rejected = rejected_.load(std::memory_order_relaxed);
    stats.max_active_voices = max_active_voices_.load(std::memory_order_relaxed);
    stats.voices_created = voices_created_.load(std::memory_order_relaxed);
    return stats;
}

void AudioMixer::Publish(std::uint32_t index) noexcept {
    VoiceRuntime& voice = voices_[index];
    voice.published_state.store(voice.state, std::memory_order_relaxed);
    voice.published_buffers.store(voice.queue_count, std::memory_order_relaxed);
    voice.published_samples.store(voice.played_q32 >> 32U, std::memory_order_relaxed);
    voice.published_context.store(
        voice.queue_count != 0 ? voice.queue[voice.queue_head].context : nullptr,
        std::memory_order_relaxed);
}

void AudioMixer::LoadCurrentBuffer(std::uint32_t index) noexcept {
    VoiceRuntime& voice = voices_[index];
    if (voice.queue_count == 0) {
        voice.buffer_loaded = false;
        return;
    }
    const MixerBufferDesc& buffer = voice.queue[voice.queue_head];
    const std::uint32_t play_frames = buffer.play_length != 0
                                          ? buffer.play_length
                                          : buffer.frame_count - buffer.play_begin;
    voice.play_end = buffer.play_begin + play_frames;
    voice.loop_begin = buffer.play_begin + buffer.loop_begin;
    if (buffer.loop_count != 0) {
        const std::uint32_t loop_frames = buffer.loop_length != 0
                                              ? buffer.loop_length
                                              : play_frames - buffer.loop_begin;
        voice.loop_end = voice.loop_begin + loop_frames;
        voice.loop_remaining = buffer.loop_count;
    } else {
        voice.loop_end = voice.play_end;
        voice.loop_remaining = 0;
    }
    voice.position_q32 = static_cast<std::uint64_t>(buffer.play_begin) << 32U;
    voice.buffer_loaded = true;
    if (voice.callbacks.buffer_start != nullptr) {
        voice.callbacks.buffer_start(voice.callbacks.context, buffer.context);
    }
}

void AudioMixer::AdvanceBuffer(std::uint32_t index) noexcept {
    VoiceRuntime& voice = voices_[index];
    const MixerBufferDesc finished = voice.queue[voice.queue_head];
    if (voice.callbacks.buffer_end != nullptr) {
        voice.callbacks.buffer_end(voice.callbacks.context, finished.context);
    }
    voice.queue_head = (voice.queue_head + 1U) % kMixerMaxBuffersPerVoice;
    if (voice.queue_count != 0) --voice.queue_count;
    voice.buffer_loaded = false;

    if (voice.queue_count == 0) {
        voice.state = (finished.flags & kMixerEndOfStream) != 0 ? 2U : 0U;
        if (voice.state == 2U && voice.callbacks.stream_end != nullptr) {
            voice.callbacks.stream_end(voice.callbacks.context);
        }
        Publish(index);
        return;
    }
    LoadCurrentBuffer(index);
    Publish(index);
}

void AudioMixer::StartVoiceInternal(std::uint32_t index, std::uint32_t offset) noexcept {
    VoiceRuntime& voice = voices_[index];
    voice.start_offset = offset;
    voice.state = 1U;
    if (voice.queue_count != 0 && !voice.buffer_loaded) {
        LoadCurrentBuffer(index);
    }
    Publish(index);
}

void AudioMixer::ApplyCommand(const Command& command) noexcept {
    const std::uint32_t index = command.voice;
    if (index >= kMixerMaxVoices) return;
    VoiceRuntime& voice = voices_[index];
    switch (command.kind) {
    case CommandKind::Start:
        StartVoiceInternal(index, command.a);
        break;
    case CommandKind::Stop:
        voice.state = 0;
        voice.start_offset = 0;
        Publish(index);
        break;
    case CommandKind::Flush:
        voice.queue_head = 0;
        voice.queue_count = 0;
        voice.state = 0;
        voice.buffer_loaded = false;
        voice.position_q32 = 0;
        voice.played_q32 = 0;
        voice.start_offset = 0;
        Publish(index);
        break;
    case CommandKind::Submit: {
        if (voice.queue_count >= kMixerMaxBuffersPerVoice) {
            buffer_overflow_.fetch_add(1, std::memory_order_relaxed);
            break;
        }
        MixerBufferDesc buffer;
        buffer.data = command.data;
        buffer.frame_count = static_cast<std::uint32_t>(command.f0);
        buffer.play_begin = command.a;
        buffer.play_length = command.b;
        buffer.loop_begin = command.c;
        buffer.loop_length = command.d;
        buffer.loop_count = command.e;
        buffer.flags = command.f;
        buffer.context = command.context;
        const std::uint32_t slot =
            (voice.queue_head + voice.queue_count) % kMixerMaxBuffersPerVoice;
        voice.queue[slot] = buffer;
        ++voice.queue_count;
        if (voice.state == 1U && !voice.buffer_loaded) {
            LoadCurrentBuffer(index);
        }
        Publish(index);
        break;
    }
    case CommandKind::VolumePan: {
        voice.volume = command.f0;
        voice.pan = command.f1;
        const float left =
            command.f0 * (command.f1 > 0.0F ? 1.0F - command.f1 : 1.0F);
        const float right =
            command.f0 * (command.f1 < 0.0F ? 1.0F + command.f1 : 1.0F);
        for (std::uint32_t channel = 0; channel < kMixerMaxChannels; ++channel) {
            voice.gain[channel] = voice.volume;
        }
        voice.gain[0] = left;
        voice.gain[1] = right;
        Publish(index);
        break;
    }
    case CommandKind::Frequency: {
        voice.ratio = command.f0;
        const double frames = static_cast<double>(command.f0) *
                              static_cast<double>(voice.format.sample_rate) *
                              kTwoPow32 / static_cast<double>(device_rate_);
        if (!(frames >= 1.0) || frames > static_cast<double>(kMaxRatio) * kTwoPow32) {
            voice.step_q32 = 1ULL << 32U;
        } else {
            voice.step_q32 = static_cast<std::uint64_t>(frames + 0.5);
            if (voice.step_q32 == 0) voice.step_q32 = 1;
        }
        Publish(index);
        break;
    }
    case CommandKind::ExitLoop:
        voice.loop_remaining = 0;
        break;
    case CommandKind::BusGain:
        if (command.bus < kMixerMaxBuses) bus_gain_[command.bus] = command.f0;
        break;
    case CommandKind::Destroy:
        voice.state = 0;
        voice.queue_head = 0;
        voice.queue_count = 0;
        voice.buffer_loaded = false;
        voice.start_offset = 0;
        voice.position_q32 = 0;
        voice.published_state.store(0, std::memory_order_relaxed);
        voice.published_buffers.store(0, std::memory_order_relaxed);
        voice.published_context.store(nullptr, std::memory_order_relaxed);
        slot_mask_.fetch_and(~(1ULL << index), std::memory_order_acq_rel);
        break;
    }
}

bool AudioMixer::Render(std::int64_t block_start_sample, std::uint32_t frames,
                        float* const* output,
                        std::uint32_t output_channels) noexcept {
    (void)block_start_sample;
    if (!prepared_ || frames == 0 || frames > max_block_frames_ ||
        output == nullptr || output_channels == 0 ||
        output_channels > kMixerMaxChannels) {
        underruns_.fetch_add(1, std::memory_order_relaxed);
        return false;
    }
    for (std::uint32_t channel = 0; channel < output_channels; ++channel) {
        if (output[channel] == nullptr) {
            underruns_.fetch_add(1, std::memory_order_relaxed);
            return false;
        }
    }

    for (std::uint32_t bus = 0; bus < kMixerMaxBuses; ++bus) {
        for (std::uint32_t channel = 0; channel < kMixerMaxChannels; ++channel) {
            std::fill(bus_scratch_[bus][channel],
                      bus_scratch_[bus][channel] + frames, 0.0F);
        }
    }

    std::uint32_t head = command_head_.load(std::memory_order_relaxed);
    const std::uint32_t tail = command_tail_.load(std::memory_order_acquire);
    while (head != tail) {
        ApplyCommand(commands_[head & (kMixerCommandCapacity - 1U)]);
        ++head;
    }
    command_head_.store(head, std::memory_order_release);

    std::uint32_t active = 0;
    const bool two_channels = output_channels >= 2;
    for (std::uint32_t index = 0; index < kMixerMaxVoices; ++index) {
        VoiceRuntime& voice = voices_[index];
        if (voice.published_state.load(std::memory_order_acquire) != 1U) continue;
        if (voice.state != 1U || voice.queue_count == 0 || !voice.buffer_loaded) {
            continue;
        }

        ++active;
        if (voice.callbacks.voice_processing_pass_start != nullptr) {
            voice.callbacks.voice_processing_pass_start(voice.callbacks.context,
                                                        frames);
        }

        std::uint32_t begin = voice.start_offset;
        voice.start_offset = 0;
        if (begin >= frames) {
            begin = 0;
            starved_voices_.fetch_add(1, std::memory_order_relaxed);
        }

        const std::uint32_t bus = voice.bus < kMixerMaxBuses ? voice.bus : 0U;
        const std::uint32_t source_channels =
            static_cast<std::uint32_t>(voice.format.channels);
        const std::uint32_t frame_bytes = voice.format.BytesPerFrame();

        for (std::uint32_t frame = begin; frame < frames; ++frame) {
            if (voice.state != 1U || !voice.buffer_loaded) break;
            const MixerBufferDesc& buffer = voice.queue[voice.queue_head];
            const std::uint32_t position =
                static_cast<std::uint32_t>(voice.position_q32 >> 32U);
            const std::uint32_t fraction =
                static_cast<std::uint32_t>(voice.position_q32 & 0xFFFFFFFFULL);
            const std::uint32_t next =
                position + 1U < voice.play_end ? position + 1U : position;
            const std::uint8_t* current =
                buffer.data + static_cast<std::size_t>(position) * frame_bytes;
            const std::uint8_t* following =
                buffer.data + static_cast<std::size_t>(next) * frame_bytes;
            const float fraction_scale =
                static_cast<float>(fraction) * (1.0F / 4294967296.0F);

            for (std::uint32_t channel = 0; channel < source_channels; ++channel) {
                float sample = 0.0F;
                switch (voice.format.encoding) {
                case AudioEncoding::Int16: {
                    std::int16_t a = 0;
                    std::int16_t b = 0;
                    std::memcpy(&a, current + channel * 2U, sizeof(a));
                    std::memcpy(&b, following + channel * 2U, sizeof(b));
                    sample = (static_cast<float>(a) +
                              (static_cast<float>(b) - static_cast<float>(a)) *
                                  fraction_scale) /
                             32768.0F;
                    break;
                }
                case AudioEncoding::Int24: {
                    const std::uint8_t* pa = current + channel * 3U;
                    const std::uint8_t* pb = following + channel * 3U;
                    const std::int32_t a =
                        (static_cast<std::int32_t>(pa[0]) << 8) |
                        (static_cast<std::int32_t>(pa[1]) << 16) |
                        (static_cast<std::int32_t>(pa[2]) << 24);
                    const std::int32_t b =
                        (static_cast<std::int32_t>(pb[0]) << 8) |
                        (static_cast<std::int32_t>(pb[1]) << 16) |
                        (static_cast<std::int32_t>(pb[2]) << 24);
                    sample = (static_cast<float>(a >> 8) +
                              (static_cast<float>(b >> 8) -
                               static_cast<float>(a >> 8)) * fraction_scale) /
                             8388608.0F;
                    break;
                }
                case AudioEncoding::Int32: {
                    std::int32_t a = 0;
                    std::int32_t b = 0;
                    std::memcpy(&a, current + channel * 4U, sizeof(a));
                    std::memcpy(&b, following + channel * 4U, sizeof(b));
                    sample = (static_cast<float>(a) +
                              (static_cast<float>(b) - static_cast<float>(a)) *
                                  fraction_scale) /
                             2147483648.0F;
                    break;
                }
                case AudioEncoding::Float32: {
                    float a = 0.0F;
                    float b = 0.0F;
                    std::memcpy(&a, current + channel * 4U, sizeof(a));
                    std::memcpy(&b, following + channel * 4U, sizeof(b));
                    sample = a + (b - a) * fraction_scale;
                    if (!std::isfinite(sample)) sample = 0.0F;
                    break;
                }
                case AudioEncoding::None:
                    break;
                }

                if (source_channels == 1 && two_channels) {
                    bus_scratch_[bus][0][frame] += sample * voice.gain[0];
                    bus_scratch_[bus][1][frame] += sample * voice.gain[1];
                } else {
                    const std::uint32_t destination =
                        channel < output_channels ? channel : channel % output_channels;
                    bus_scratch_[bus][destination][frame] +=
                        sample * voice.gain[channel];
                }
            }

            voice.position_q32 += voice.step_q32;
            voice.played_q32 += voice.step_q32;
            if (voice.position_q32 >=
                (static_cast<std::uint64_t>(voice.play_end) << 32U)) {
                if (voice.loop_remaining != 0) {
                    if (voice.loop_remaining != kMixerLoopInfinite) {
                        --voice.loop_remaining;
                    }
                    voice.position_q32 =
                        static_cast<std::uint64_t>(voice.loop_begin) << 32U;
                    if (voice.callbacks.loop_end != nullptr) {
                        voice.callbacks.loop_end(voice.callbacks.context,
                                                 buffer.context);
                    }
                } else {
                    AdvanceBuffer(index);
                }
            }
        }

        if (voice.callbacks.voice_processing_pass_end != nullptr) {
            voice.callbacks.voice_processing_pass_end(voice.callbacks.context);
        }
        Publish(index);
    }

    std::uint64_t clipped = 0;
    for (std::uint32_t channel = 0; channel < output_channels; ++channel) {
        float* destination = output[channel];
        for (std::uint32_t frame = 0; frame < frames; ++frame) {
            float sum = 0.0F;
            for (std::uint32_t bus = 0; bus < kMixerMaxBuses; ++bus) {
                sum += bus_scratch_[bus][channel][frame] * bus_gain_[bus];
            }
            if (!std::isfinite(sum)) {
                sum = 0.0F;
                ++clipped;
            } else if (sum > 1.0F) {
                sum = 1.0F;
                ++clipped;
            } else if (sum < -1.0F) {
                sum = -1.0F;
                ++clipped;
            }
            destination[frame] = sum;
        }
    }
    if (clipped != 0) clipped_frames_.fetch_add(clipped, std::memory_order_relaxed);

    blocks_.fetch_add(1, std::memory_order_relaxed);
    frames_rendered_.fetch_add(frames, std::memory_order_relaxed);
    if (active != 0) {
        const std::uint32_t previous = max_active_voices_.load(std::memory_order_relaxed);
        if (active > previous) {
            max_active_voices_.store(active, std::memory_order_relaxed);
        }
    }
    return true;
}

} // namespace nosfix::audio
