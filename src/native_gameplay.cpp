#include "nosfix/native_gameplay.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace nosfix {
namespace {

bool FiniteTime(double value) noexcept {
    return std::isfinite(value) && std::abs(value) <= kNativeMaximumTimeUs;
}

bool ValidEdge(InputEdge edge) noexcept {
    return edge == InputEdge::Press || edge == InputEdge::Release;
}

} // namespace

const char* GameplayStatusName(GameplayStatus status) noexcept {
    switch (status) {
    case GameplayStatus::Ok: return "ok";
    case GameplayStatus::NotConfigured: return "not-configured";
    case GameplayStatus::NotStarted: return "not-started";
    case GameplayStatus::InvalidConfig: return "invalid-config";
    case GameplayStatus::GenerationZero: return "generation-zero";
    case GameplayStatus::GenerationNotNew: return "generation-not-new";
    case GameplayStatus::StaleGeneration: return "stale-generation";
    case GameplayStatus::NonFiniteTime: return "non-finite-time";
    case GameplayStatus::BackwardTime: return "backward-time";
    case GameplayStatus::WatermarkAhead: return "watermark-ahead";
    case GameplayStatus::WatermarkRegressed: return "watermark-regressed";
    case GameplayStatus::InputQueueFull: return "input-queue-full";
    case GameplayStatus::InputNotChronological: return "input-not-chronological";
    case GameplayStatus::DuplicateInput: return "duplicate-input";
    case GameplayStatus::InvalidInput: return "invalid-input";
    case GameplayStatus::InsufficientCapacity: return "insufficient-capacity";
    }
    return "invalid";
}

GameplayStatus NativeGameplay::Configure(std::span<const NativeNoteSpec> notes,
                                         std::span<const ChartEvent> chart_events,
                                         std::span<const ScheduledSound> sounds,
                                         const NativeNoteConfig& note_config,
                                         std::uint32_t input_capacity,
                                         ConfigureFailpoint failpoint) {
    if (input_capacity == 0 || input_capacity > kMaximumInputQueueCapacity) {
        return GameplayStatus::InvalidConfig;
    }

    // From here the old instance may be mutated by a later step (including an
    // allocation failure), so make it explicitly unusable first. A failure never
    // leaves a partially configured/running object; lifetime generation floors
    // remain in the note/chart cursors and last_begin_generation_.
    configured_ = false;
    started_ = false;
    queue_head_ = 0;
    queue_count_ = 0;
    recent_head_ = 0;
    recent_count_ = 0;
    has_last_capture_ = false;
    watermark_us_ = 0.0;
    last_now_us_ = 0.0;
    generation_ = 0;

    std::vector<ChartEvent> chart_storage;
    chart_storage.reserve(chart_events.size());
    std::vector<std::uint64_t> ids;
    ids.reserve(chart_events.size());
    for (const ChartEvent& event : chart_events) {
        if (!std::isfinite(event.song_us)) return GameplayStatus::InvalidConfig;
        ChartEvent copy = event;
        copy.song_us = event.song_us - note_config.eligibility_total_offset_us;
        if (!FiniteTime(copy.song_us)) return GameplayStatus::InvalidConfig;
        chart_storage.push_back(copy);
        ids.push_back(event.id);
    }
    std::sort(ids.begin(), ids.end());
    for (std::size_t i = 1; i < ids.size(); ++i) {
        if (ids[i] == ids[i - 1]) return GameplayStatus::InvalidConfig;
    }

    std::vector<ScheduledSound> sound_storage;
    sound_storage.reserve(sounds.size());
    for (const ScheduledSound& sound : sounds) {
        if ((!sound.has_start && !sound.has_release) ||
            !std::isfinite(sound.velocity)) {
            return GameplayStatus::InvalidConfig;
        }
        if (sound.has_start && !FiniteTime(sound.start_us)) {
            return GameplayStatus::InvalidConfig;
        }
        if (sound.has_release && !FiniteTime(sound.release_us)) {
            return GameplayStatus::InvalidConfig;
        }
        if (sound.has_start && sound.has_release &&
            sound.release_us < sound.start_us) {
            return GameplayStatus::InvalidConfig;
        }
        sound_storage.push_back(sound);
    }
    std::vector<SoundCrossing> crossings;
    for (std::size_t i = 0; i < sound_storage.size(); ++i) {
        const ScheduledSound& sound = sound_storage[i];
        if (sound.has_start) {
            crossings.push_back(SoundCrossing{sound.start_us,
                                              static_cast<std::uint32_t>(i),
                                              SoundAction::Start, sound.sound_id,
                                              sound.velocity});
        }
        if (sound.has_release) {
            crossings.push_back(SoundCrossing{sound.release_us,
                                              static_cast<std::uint32_t>(i),
                                              SoundAction::Cut,
                                              sound.release_target_id, 0.0f});
        }
    }
    std::sort(crossings.begin(), crossings.end(),
              [](const SoundCrossing& a, const SoundCrossing& b) {
                  if (a.at_us != b.at_us) return a.at_us < b.at_us;
                  if (a.action != b.action) {
                      return static_cast<std::uint8_t>(a.action) <
                             static_cast<std::uint8_t>(b.action);
                  }
                  return a.schedule_index < b.schedule_index;
              });

    if (failpoint == ConfigureFailpoint::BeforeNotes) {
        return GameplayStatus::InvalidConfig;
    }
    const NoteConfigStatus note_status = notes_.Configure(notes, note_config);
    if (note_status != NoteConfigStatus::Accepted) return GameplayStatus::InvalidConfig;
    if (failpoint == ConfigureFailpoint::AfterNotes) {
        return GameplayStatus::InvalidConfig;
    }
    if (chart_.Configure(chart_storage) != ChartTableStatus::accepted) {
        return GameplayStatus::InvalidConfig;
    }
    if (failpoint == ConfigureFailpoint::AfterChart) {
        return GameplayStatus::InvalidConfig;
    }

    sounds_ = std::move(sound_storage);
    sound_crossings_ = std::move(crossings);
    sound_cursor_ = 0;
    if (failpoint == ConfigureFailpoint::AfterSounds) {
        return GameplayStatus::InvalidConfig;
    }
    queue_.assign(input_capacity, TimedInputEvent{});
    if (failpoint == ConfigureFailpoint::AfterQueue) {
        return GameplayStatus::InvalidConfig;
    }
    queue_capacity_ = input_capacity;
    queue_head_ = 0;
    queue_count_ = 0;
    sound_cursor_ = 0;
    recent_head_ = 0;
    recent_count_ = 0;
    has_last_capture_ = false;
    last_capture_us_ = 0.0;
    watermark_us_ = 0.0;
    last_now_us_ = 0.0;
    generation_ = 0;
    started_ = false;
    configured_ = true;
    return GameplayStatus::Ok;
}

GameplayStatus NativeGameplay::Begin(std::uint64_t generation, double start_us) {
    if (!configured_) return GameplayStatus::NotConfigured;
    if (generation == 0) return GameplayStatus::GenerationZero;
    if (generation <= last_begin_generation_ ||
        generation <= notes_.LastBeginGeneration() ||
        generation <= chart_.LastResetGeneration()) {
        return GameplayStatus::GenerationNotNew;
    }
    if (!FiniteTime(start_us)) return GameplayStatus::NonFiniteTime;

    // All preconditions are checked above, so neither Reset can fail here for
    // generation or range reasons after the other has changed state.
    if (chart_.Reset(generation, start_us) != ChartCursorStatus::ok) {
        return GameplayStatus::InvalidConfig;
    }
    if (notes_.Begin(generation, start_us) != NoteRunStatus::Ok) {
        return GameplayStatus::InvalidConfig;
    }
    if (ResetSounds(start_us) != GameplayStatus::Ok) {
        return GameplayStatus::InvalidConfig;
    }

    queue_head_ = 0;
    queue_count_ = 0;
    recent_head_ = 0;
    recent_count_ = 0;
    has_last_capture_ = false;
    last_capture_us_ = 0.0;
    watermark_us_ = start_us;
    last_now_us_ = start_us;
    generation_ = generation;
    last_begin_generation_ = generation;
    started_ = true;
    return GameplayStatus::Ok;
}

GameplayStatus NativeGameplay::ResetSounds(double start_us) noexcept {
    sound_cursor_ = 0;
    while (sound_cursor_ < sound_crossings_.size() &&
           sound_crossings_[sound_cursor_].at_us < start_us) {
        ++sound_cursor_;
    }
    return GameplayStatus::Ok;
}

GameplayStatus NativeGameplay::SubmitInput(const TimedInputEvent& event) noexcept {
    if (!started_) return GameplayStatus::NotStarted;
    if (event.song_time.generation == 0 ||
        event.song_time.generation != generation_) {
        return GameplayStatus::StaleGeneration;
    }
    if (!FiniteTime(event.song_time.microseconds)) return GameplayStatus::NonFiniteTime;
    if (event.input.lane >= kNativeNoteLaneCount || !ValidEdge(event.input.edge)) {
        return GameplayStatus::InvalidInput;
    }
    if (event.input.stamp.stream_epoch == 0 || event.input.stamp.sequence == 0 ||
        (event.input.synthetic && event.input.edge == InputEdge::Press)) {
        return GameplayStatus::InvalidInput;
    }
    InputIdentity identity;
    identity.stream_epoch = event.input.stamp.stream_epoch;
    identity.sequence = event.input.stamp.sequence;
    identity.ordinal = event.input.stamp.ordinal;
    identity.source_id = event.input.source_id;
    identity.device_generation = event.input.device_generation;
    identity.lane = event.input.lane;
    identity.edge = static_cast<std::uint8_t>(event.input.edge);
    identity.synthetic = event.input.synthetic;
    if (SeenIdentity(identity)) return GameplayStatus::DuplicateInput;
    if (has_last_capture_ && event.song_time.microseconds < last_capture_us_) {
        return GameplayStatus::InputNotChronological;
    }

    if (queue_count_ >= queue_capacity_) return GameplayStatus::InputQueueFull;
    const std::size_t tail = (queue_head_ + queue_count_) % queue_capacity_;
    queue_[tail] = event;
    ++queue_count_;
    RememberIdentity(identity);
    last_capture_us_ = event.song_time.microseconds;
    has_last_capture_ = true;
    return GameplayStatus::Ok;
}

bool NativeGameplay::SeenIdentity(const InputIdentity& identity) const noexcept {
    for (std::size_t i = 0; i < recent_count_; ++i) {
        const InputIdentity& seen = recent_[i];
        if (seen.stream_epoch == identity.stream_epoch &&
            seen.sequence == identity.sequence &&
            seen.ordinal == identity.ordinal &&
            seen.source_id == identity.source_id &&
            seen.device_generation == identity.device_generation &&
            seen.lane == identity.lane && seen.edge == identity.edge &&
            seen.synthetic == identity.synthetic) {
            return true;
        }
    }
    return false;
}

void NativeGameplay::RememberIdentity(const InputIdentity& identity) noexcept {
    recent_[recent_head_] = identity;
    recent_head_ = (recent_head_ + 1) % kRecentIdentityCount;
    if (recent_count_ < kRecentIdentityCount) ++recent_count_;
}

GameplayStatus NativeGameplay::SetInputWatermark(SongTime through) noexcept {
    if (!started_) return GameplayStatus::NotStarted;
    if (through.generation == 0 || through.generation != generation_) {
        return GameplayStatus::StaleGeneration;
    }
    if (!FiniteTime(through.microseconds)) return GameplayStatus::NonFiniteTime;
    if (through.microseconds < watermark_us_) return GameplayStatus::WatermarkRegressed;
    watermark_us_ = through.microseconds;
    return GameplayStatus::Ok;
}

GameplayAdvance NativeGameplay::Advance(SongTime now,
                                        const GameplayOutputs& outputs) noexcept {
    GameplayAdvance result;
    if (!configured_) {
        result.status = GameplayStatus::NotConfigured;
        return result;
    }
    if (!started_) {
        result.status = GameplayStatus::NotStarted;
        return result;
    }
    if (!FiniteTime(now.microseconds)) {
        result.status = GameplayStatus::NonFiniteTime;
        return result;
    }
    if (now.generation != generation_ || generation_ == 0) {
        result.status = GameplayStatus::StaleGeneration;
        return result;
    }
    if (now.microseconds < last_now_us_) {
        result.status = GameplayStatus::BackwardTime;
        return result;
    }
    if (watermark_us_ > now.microseconds) {
        result.status = GameplayStatus::WatermarkAhead;
        return result;
    }
    if (outputs.note_outcomes.size() < MaxOutcomesPerAdvance()) {
        result.status = GameplayStatus::InsufficientCapacity;
        return result;
    }

    NoteAdvance counts;
    const double input_limit = std::min(now.microseconds, watermark_us_);
    while (queue_count_ > 0) {
        const TimedInputEvent& front = queue_[queue_head_];
        if (front.song_time.microseconds > input_limit) break;
        const TimedInputEvent event = front;
        queue_head_ = (queue_head_ + 1) % queue_capacity_;
        --queue_count_;

        if (event.song_time.generation != generation_) {
            ++result.inputs_dropped;
            continue;
        }
        // Resolve note crossings up to the captured time before applying either
        // edge. Releases must not be observed before a pending/auto crossing
        // that is earlier in song time, or the outcome order would depend on
        // the consumer cadence.
        const NoteRunStatus crossed = notes_.Advance(
            event.song_time.microseconds, event.song_time.microseconds,
            outputs.note_outcomes, counts);
        if (crossed != NoteRunStatus::Ok) {
            // BackwardTime means state already advanced beyond this capture.
            // Never apply the edge against newer state; drop it visibly.
            ++result.inputs_dropped;
            continue;
        }
        NoteRunStatus status = NoteRunStatus::Ok;
        if (event.input.edge == InputEdge::Press) {
            status = notes_.Press(event, watermark_us_, outputs.note_outcomes, counts);
        } else {
            status = notes_.Release(event, outputs.note_outcomes, counts);
        }
        if (status == NoteRunStatus::Ok) {
            ++result.inputs_applied;
        } else {
            ++result.inputs_dropped;
        }
    }

    // Input-dependent state may advance only through the producer-certified
    // watermark, not to the render time. Auto/pending/miss decisions that occur
    // later wait for certification; chart/sound time-based crossings below
    // still use `now`.
    const NoteRunStatus note_status = notes_.Advance(watermark_us_, watermark_us_,
                                                     outputs.note_outcomes, counts);
    if (note_status != NoteRunStatus::Ok) {
        result.status = note_status == NoteRunStatus::InsufficientCapacity
                            ? GameplayStatus::InsufficientCapacity
                            : GameplayStatus::InvalidConfig;
        return result;
    }
    result.note_counts = counts;

    const ChartEventDrain drain = chart_.Drain(now, outputs.chart_events);
    if (drain.ok()) {
        result.chart_written = drain.written;
        result.chart_backlog = !drain.drained_through_now && chart_.PendingCount() > 0;
    } else if (drain.status == ChartCursorStatus::empty_destination) {
        result.chart_backlog = chart_.PendingCount() > 0;
    } else {
        result.status = GameplayStatus::InvalidConfig;
        return result;
    }

    if (AdvanceSounds(now.microseconds, outputs.sounds, result.sounds_written,
                      result.sound_backlog) != GameplayStatus::Ok) {
        result.status = GameplayStatus::InvalidConfig;
        return result;
    }

    last_now_us_ = now.microseconds;
    result.status = GameplayStatus::Ok;
    return result;
}

GameplayStatus NativeGameplay::AdvanceSounds(double now_us,
                                             std::span<SoundCommand> out,
                                             std::size_t& written,
                                             bool& backlog) noexcept {
    written = 0;
    backlog = false;
    while (sound_cursor_ < sound_crossings_.size() &&
           sound_crossings_[sound_cursor_].at_us <= now_us) {
        if (written >= out.size()) {
            backlog = true;
            break;
        }
        const SoundCrossing& crossing = sound_crossings_[sound_cursor_];
        out[written++] = SoundCommand{crossing.action, crossing.sound_id,
                                      crossing.schedule_index, crossing.velocity,
                                      crossing.at_us};
        ++sound_cursor_;
    }
    return GameplayStatus::Ok;
}

NativeSeamReport EvaluateNativeSeam(bool feature_enabled,
                                    bool evidence_verified) noexcept {
    if (!feature_enabled) {
        return {NativeSeamState::DisabledByDefault,
                "native gameplay seam is disabled by nosfix.ini"};
    }
    if (!evidence_verified) {
        return {NativeSeamState::WaitingForEvidence,
                "no verified function signatures or layout report; see REQUESTS.txt"};
    }
    if (!native_layout::kAbiAndMatchingVerified) {
        return {NativeSeamState::WaitingForEvidence,
                "normal matching/completion ABI and auto-slot semantics unverified"};
    }
    if (native_layout::kTargetPrefixes.size() != native_layout::kTimingTargets.size()) {
        return {NativeSeamState::WaitingForEvidence,
                "target signature table is incomplete"};
    }
    for (const native_layout::Target& target : native_layout::kTimingTargets) {
        if (!target.signature_known) {
            return {NativeSeamState::WaitingForEvidence,
                    "target signature missing from the verified table"};
        }
    }
    return {NativeSeamState::ReadyNotInstalled,
            "all evidenced targets verified; installation remains the parent's step"};
}

std::span<const native_layout::Target> NativeSeamTargets() noexcept {
    return native_layout::kTimingTargets;
}

} // namespace nosfix
