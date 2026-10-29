#include "nosfix/native_note_types.hpp"

#include <algorithm>
#include <cmath>

namespace nosfix {

using namespace nonnormal_layout;

namespace {

bool FiniteTime(double value) noexcept {
    return std::isfinite(value) && std::abs(value) <= kNoteTypeMaximumTimeUs;
}

// Shared configuration validation. `unit_us` is 1e6 / units_per_second.
NoteTypeStatus ValidateTypeConfig(const NativeTypeConfig& config, double& unit_us) noexcept {
    if (!config.windows_sampled) return NoteTypeStatus::WindowsNotSampled;
    if (ValidateNormalWindows(config.windows) != NormalWindowStatus::Ok) {
        return NoteTypeStatus::InvalidWindows;
    }
    if (!FiniteTime(config.judgment_offset_us)) return NoteTypeStatus::InvalidOffset;
    if (!std::isfinite(config.units_per_second) || config.units_per_second <= 0.0 ||
        config.units_per_second > 1.0e6) {
        return NoteTypeStatus::InvalidRate;
    }
    if (config.sound_serial_start == 0 ||
        config.sound_serial_start > config.sound_serial_limit) {
        return NoteTypeStatus::InvalidSpec;
    }
    unit_us = 1'000'000.0 / config.units_per_second;
    if (!FiniteTime(unit_us)) return NoteTypeStatus::InvalidRate;
    return NoteTypeStatus::Ok;
}

NormalRank Classify(double delta_us, const NormalWindows& windows) noexcept {
    return ClassifyNormalDelta(delta_us, windows);
}

// 0x3053E0 is not decoded. The one proven rule is the completion fix-up shared
// by all three judges: when the forwarded scale is not 100 and the effect index
// is still 0, it becomes 1. effect_grade is therefore 1 for a penalised finish
// and 0 otherwise, and every outcome flags it as unverified.
constexpr std::uint8_t EffectGradeForScale(std::uint16_t scale) noexcept {
    return scale == kScaleNormal ? 0 : 1;
}

NoteTypeOutcome& Append(NoteTypeResult& result) noexcept {
    // Callers never append more than kNoteTypeMaxOutcomes; the guard keeps a
    // future edit from writing out of bounds.
    if (result.count >= kNoteTypeMaxOutcomes) return result.outcomes[kNoteTypeMaxOutcomes - 1];
    return result.outcomes[result.count++];
}

NoteTypeOutcome MakeOutcome(NoteTypeOutcomeKind kind, std::uint8_t lane, double at_us) noexcept {
    NoteTypeOutcome outcome{};
    outcome.kind = kind;
    outcome.lane = lane;
    outcome.at_us = at_us;
    outcome.scale = kScaleNormal;
    return outcome;
}

// Copies the outcomes of `from` into `into`, preserving the order and stopping
// at the fixed capacity.
void MergeOutcomes(NoteTypeResult& into, const NoteTypeResult& from) noexcept {
    for (std::size_t i = 0; i < from.count && into.count < kNoteTypeMaxOutcomes; ++i) {
        into.outcomes[into.count++] = from.outcomes[i];
    }
    into.cuts += from.cuts;
    into.penalties += from.penalties;
    into.ignored += from.ignored;
    into.unsupported += from.unsupported;
}

} // namespace

const char* NativeEventKindName(NativeEventKind kind) noexcept {
    switch (kind) {
    case NativeEventKind::Press: return "press";
    case NativeEventKind::Repeat: return "repeat";
    case NativeEventKind::Release: return "release";
    }
    return "invalid";
}

const char* NoteTypeStatusName(NoteTypeStatus status) noexcept {
    switch (status) {
    case NoteTypeStatus::Ok: return "ok";
    case NoteTypeStatus::NotConfigured: return "not-configured";
    case NoteTypeStatus::NotStarted: return "not-started";
    case NoteTypeStatus::GenerationZero: return "generation-zero";
    case NoteTypeStatus::GenerationNotNew: return "generation-not-new";
    case NoteTypeStatus::GenerationMismatch: return "generation-mismatch";
    case NoteTypeStatus::NonFiniteTime: return "non-finite-time";
    case NoteTypeStatus::BackwardTime: return "backward-time";
    case NoteTypeStatus::WatermarkBehindCapture: return "watermark-behind-capture";
    case NoteTypeStatus::WatermarkAhead: return "watermark-ahead";
    case NoteTypeStatus::InvalidSpec: return "invalid-spec";
    case NoteTypeStatus::WindowsNotSampled: return "windows-not-sampled";
    case NoteTypeStatus::InvalidWindows: return "invalid-windows";
    case NoteTypeStatus::InvalidOffset: return "invalid-offset";
    case NoteTypeStatus::InvalidRate: return "invalid-rate";
    }
    return "invalid";
}

const char* NoteTypeOutcomeKindName(NoteTypeOutcomeKind kind) noexcept {
    switch (kind) {
    case NoteTypeOutcomeKind::HeadCommitted: return "head-committed";
    case NoteTypeOutcomeKind::PendingEarly: return "pending-early";
    case NoteTypeOutcomeKind::Scored: return "scored";
    case NoteTypeOutcomeKind::Auto: return "auto";
    case NoteTypeOutcomeKind::Missed: return "missed";
    case NoteTypeOutcomeKind::KeysoundCut: return "keysound-cut";
    case NoteTypeOutcomeKind::RepeatIgnored: return "repeat-ignored";
    case NoteTypeOutcomeKind::ConnectorReleased: return "connector-released";
    case NoteTypeOutcomeKind::Unsupported: return "unsupported";
    }
    return "invalid";
}

namespace nonnormal_layout {

ChartNoteType ChartNoteTypeFromFlags(std::int32_t flags) noexcept {
    if ((flags & kChartFlagGliss) != 0) return ChartNoteType::Gliss;
    if ((flags & kChartFlagTrill) != 0) return ChartNoteType::Trill;
    if ((flags & kChartFlagLong) != 0) return ChartNoteType::Long;
    return ChartNoteType::Normal;
}

} // namespace nonnormal_layout

bool LongEligible(bool held, double delta_us, const NormalWindows& windows) noexcept {
    if (held) return true; // 2F4E29: state(+300) == 1 short-circuits the windows
    return IsScoringNormalRank(ClassifyNormalDelta(delta_us, windows));
}

bool TrillEligible(bool tail, double delta_us, const NormalWindows& windows) noexcept {
    if (tail) return true; // 1A8499: state(+316) == 1
    return IsScoringNormalRank(ClassifyNormalDelta(delta_us, windows));
}

// ===========================================================================
// NativeLongState
// ===========================================================================

NoteTypeStatus NativeLongState::Configure(const NativeLongSpec& spec,
                                          const NativeTypeConfig& config) noexcept {
    double unit_us = 0.0;
    const NoteTypeStatus status = ValidateTypeConfig(config, unit_us);
    if (status != NoteTypeStatus::Ok) return status;
    if (spec.lane >= kNativeTypeLaneCount) return NoteTypeStatus::InvalidSpec;
    if (!FiniteTime(spec.start_us) || !FiniteTime(spec.end_us)) {
        return NoteTypeStatus::NonFiniteTime;
    }
    if (spec.end_us < spec.start_us) return NoteTypeStatus::InvalidSpec;

    spec_ = spec;
    config_ = config;
    unit_us_ = unit_us;
    sound_serial_ = config.sound_serial_start;
    pending_resolve_us_ = spec.start_us - config.judgment_offset_us;
    deadline_us_ = pending_resolve_us_ + config.windows.good_late_us;
    if (!FiniteTime(pending_resolve_us_) || !FiniteTime(deadline_us_)) {
        return NoteTypeStatus::InvalidOffset;
    }
    phase_ = LongPhase::Idle;
    head_rank_ = NormalRank::TooEarly;
    pending_rank_ = NormalRank::TooEarly;
    final_rank_ = NormalRank::TooEarly;
    recorded_delta_us_ = 0.0;
    sound_id_ = 0;
    pending_ = false;
    done_ = false;
    advanced_us_ = 0.0;
    configured_ = true;
    started_ = false;
    return NoteTypeStatus::Ok;
}

NoteTypeStatus NativeLongState::Begin(std::uint64_t generation, double position_us) noexcept {
    if (!configured_) return NoteTypeStatus::NotConfigured;
    if (generation == 0) return NoteTypeStatus::GenerationZero;
    if (generation <= last_begin_generation_) return NoteTypeStatus::GenerationNotNew;
    if (!FiniteTime(position_us)) return NoteTypeStatus::NonFiniteTime;

    phase_ = spec_.start_us < position_us ? LongPhase::Skipped : LongPhase::Waiting;
    head_rank_ = NormalRank::TooEarly;
    pending_rank_ = NormalRank::TooEarly;
    final_rank_ = NormalRank::TooEarly;
    recorded_delta_us_ = 0.0;
    sound_id_ = 0;
    pending_ = false;
    done_ = false;
    advanced_us_ = position_us;
    generation_ = generation;
    last_begin_generation_ = generation;
    started_ = true;
    return NoteTypeStatus::Ok;
}

bool NativeLongState::Ready(NoteTypeResult& result) const noexcept {
    if (!configured_) {
        result.status = NoteTypeStatus::NotConfigured;
        return false;
    }
    if (!started_) {
        result.status = NoteTypeStatus::NotStarted;
        return false;
    }
    result.status = NoteTypeStatus::Ok;
    return true;
}

NoteTypeStatus NativeLongState::CheckEvent(const NativeNoteEvent& event) const noexcept {
    if (generation_ == 0 || event.song_time.generation != generation_) {
        return NoteTypeStatus::GenerationMismatch;
    }
    if (!FiniteTime(event.song_time.microseconds)) return NoteTypeStatus::NonFiniteTime;
    return NoteTypeStatus::Ok;
}

double NativeLongState::DeltaUs(const NativeNoteEvent& event) const noexcept {
    return (spec_.start_us - event.song_time.microseconds) - config_.judgment_offset_us;
}

std::uint32_t NativeLongState::NextSerial() noexcept {
    ++sound_serial_;
    if (sound_serial_ > config_.sound_serial_limit) sound_serial_ = config_.sound_serial_start;
    return sound_serial_;
}

double NativeLongState::RemainingUs(double captured_us) const noexcept {
    // Native operand order: end - recorded - music_frame - judgment offset.
    return ((spec_.end_us - recorded_delta_us_) - captured_us) - config_.judgment_offset_us;
}

void NativeLongState::CommitStart(NormalRank rank, bool have_event, double captured_us,
                                  double delta_us, NoteTypeResult& result) noexcept {
    // 2F4943: a deferred press keeps its recorded 3/4 rank when the start is
    // finally committed, even if a later event is what commits it.
    if (pending_) rank = pending_rank_;
    head_rank_ = rank;
    recorded_delta_us_ = delta_us;
    pending_ = false;
    phase_ = LongPhase::Held;
    // 2F4AC7/2F4B03: the serial is drawn only for a non-auto note whose start was
    // committed by a real event. A null-update commit stores -1 (no sound).
    if (!spec_.auto_play && have_event) {
        sound_id_ = NextSerial();
    } else {
        sound_id_ = 0;
    }
    NoteTypeOutcome& outcome = Append(result);
    outcome = MakeOutcome(NoteTypeOutcomeKind::HeadCommitted, spec_.lane, captured_us);
    outcome.rank = rank;
    outcome.delta_us = delta_us;
    outcome.sound_id = sound_id_;
    outcome.sound_started = sound_id_ != 0;
}

void NativeLongState::Finish(NormalRank rank, std::uint16_t scale, NoteTypeOutcomeKind kind,
                             double captured_us, double delta_us,
                             NoteTypeResult& result) noexcept {
    final_rank_ = rank;
    done_ = true;
    phase_ = kind == NoteTypeOutcomeKind::Missed ? LongPhase::Missed : LongPhase::Finished;

    NoteTypeOutcome& scored = Append(result);
    scored = MakeOutcome(kind, spec_.lane, captured_us);
    scored.rank = rank;
    scored.scale = scale;
    scored.effect_grade = EffectGradeForScale(scale);
    scored.sound_id = sound_id_;
    scored.judged = true;
    scored.delta_us = delta_us;
    scored.remaining_us = RemainingUs(captured_us);

    // 2F4C0C: a finished long note fades its sound only for rank < Great. Great
    // and Just leave the keysound running, which is why the release cut above is
    // the only other place it is stopped.
    if (rank < NormalRank::Great && sound_id_ != 0) {
        NoteTypeOutcome& cut = Append(result);
        cut = MakeOutcome(NoteTypeOutcomeKind::KeysoundCut, spec_.lane, captured_us);
        cut.sound_id = sound_id_;
        cut.sound_cut = true;
        scored.sound_cut = true;
        sound_id_ = 0;
        ++result.cuts;
    }
}

NoteTypeResult NativeLongState::Press(const NativeNoteEvent& event, double decision_us) noexcept {
    NoteTypeResult result;
    if (!Ready(result)) return result;
    const NoteTypeStatus event_status = CheckEvent(event);
    if (event_status != NoteTypeStatus::Ok) {
        result.status = event_status;
        return result;
    }
    const double captured = event.song_time.microseconds;
    if (!FiniteTime(decision_us)) {
        result.status = NoteTypeStatus::NonFiniteTime;
        return result;
    }
    if (decision_us < captured) {
        result.status = NoteTypeStatus::WatermarkBehindCapture;
        return result;
    }
    if (event.lane >= kNativeTypeLaneCount) {
        ++result.ignored;
        return result;
    }
    if (captured < advanced_us_) {
        result.status = NoteTypeStatus::BackwardTime;
        return result;
    }
    if (done_ || phase_ == LongPhase::Skipped || phase_ == LongPhase::Idle) {
        ++result.ignored;
        return result;
    }
    advanced_us_ = std::max(advanced_us_, captured);

    if (event.kind == NativeEventKind::Release) {
        NoteTypeResult release = Release(event, decision_us);
        release.status = NoteTypeStatus::Ok;
        return release;
    }

    const double delta = DeltaUs(event);
    const double reached = static_cast<double>(kEndReachedUnits) * unit_us_;
    if (spec_.auto_play) {
        // 2F4524: an auto long note skips the whole manual path. Before the note
        // it does nothing; at the note it commits the head as Just with no sound.
        if (delta < reached) {
            CommitStart(NormalRank::Just, false, captured, delta, result);
        } else {
            ++result.ignored;
        }
        return result;
    }
    if (pending_ && delta <= 0.0) {
        // 2F4745: a pending early press is resolved by the first event or update
        // that reaches the note time and is judged with the recorded rank.
        CommitStart(pending_rank_, true, captured, delta, result);
        return result;
    }
    if (event.kind == NativeEventKind::Repeat) {
        // 2F432D: a kind-2 event is only appended to the shared id list. It is
        // never judged and never changes the hold state.
        ++result.repeats;
        NoteTypeOutcome& outcome = Append(result);
        outcome = MakeOutcome(NoteTypeOutcomeKind::RepeatIgnored, spec_.lane, captured);
        outcome.event_id = event.event_id;
        return result;
    }

    NoteTypeResult head = JudgeHead(&event, captured, true);
    head.status = NoteTypeStatus::Ok;
    return head;
}

NoteTypeResult NativeLongState::JudgeHead(const NativeNoteEvent* event, double captured_us,
                                          bool have_event) noexcept {
    NoteTypeResult result;
    result.status = NoteTypeStatus::Ok;
    const double delta = have_event && event != nullptr
                             ? DeltaUs(*event)
                             : (spec_.start_us - captured_us) - config_.judgment_offset_us;
    const NormalRank rank = Classify(delta, config_.windows);

    if (rank == NormalRank::TooEarly) {
        // 2F4733 (helper returns 0): nothing is stored and nothing is consumed.
        ++result.ignored;
        return result;
    }
    if (rank == NormalRank::Missed) {
        // 2F4553: a press past the inclusive Good-late window commits the start
        // and finishes the whole note as Missed with scale 100 in one call.
        CommitStart(rank, have_event, captured_us, delta, result);
        Finish(rank, kScaleNormal, NoteTypeOutcomeKind::Missed, captured_us, delta, result);
        return result;
    }
    if (delta > 0.0 && delta <= config_.windows.great_early_us) {
        // 2F478D / 2F48B4: deferred early press, recorded rank 3 + (rank == Just).
        pending_ = true;
        pending_rank_ = rank == NormalRank::Just ? NormalRank::Just : NormalRank::Great;
        phase_ = LongPhase::PendingEarly;
        NoteTypeOutcome& outcome = Append(result);
        outcome = MakeOutcome(NoteTypeOutcomeKind::PendingEarly, spec_.lane, captured_us);
        outcome.rank = pending_rank_;
        outcome.delta_us = delta;
        return result;
    }
    CommitStart(rank, have_event, captured_us, delta, result);
    return result;
}

NoteTypeResult NativeLongState::HeldStep(bool is_release, bool commit_missed,
                                         double captured_us, double delta_us) noexcept {
    NoteTypeResult result;
    result.status = NoteTypeStatus::Ok;
    const double remaining = RemainingUs(captured_us);
    const double tolerance = static_cast<double>(kEndToleranceUnits) * unit_us_;
    const double reached = static_cast<double>(kEndReachedUnits) * unit_us_;

    if (spec_.auto_play) {
        // 2F44C6: the auto branch ignores every event and completes at the end
        // with the committed head rank and scale 100.
        if (remaining < reached) {
            Finish(head_rank_, kScaleNormal, NoteTypeOutcomeKind::Auto, captured_us, delta_us,
                   result);
        }
        return result;
    }

    NormalRank rank = head_rank_;
    std::uint16_t scale = kScaleNormal;
    bool finish = false;

    if (commit_missed || remaining < reached) {
        // 2F44D4 / 2F42E9: the stale-hold arm, or the end of the hold.
        rank = remaining > tolerance ? NormalRank::Good : head_rank_;
        scale = remaining <= tolerance ? kScaleNormal : kScaleLongPenalty;
        finish = true;
    } else if (is_release) {
        // 2F43B8: a release while held, either inside the tolerance band or too
        // early. Only the tolerance band keeps the head rank and scale 100.
        rank = remaining > tolerance ? NormalRank::Good : head_rank_;
        scale = remaining > tolerance ? kScaleLongPenalty : kScaleNormal;
        finish = true;
    }
    if (finish && rank == NormalRank::Good) {
        // 2F431C / 2F43E9: the native counter follows the assigned rank, not the
        // reason, so a Good head also increments it at completion.
        ++penalties_;
        ++result.penalties;
    }
    if (is_release && sound_id_ != 0) {
        NoteTypeOutcome& cut = Append(result);
        cut = MakeOutcome(NoteTypeOutcomeKind::KeysoundCut, spec_.lane, captured_us);
        cut.sound_id = sound_id_;
        cut.sound_cut = true;
        sound_id_ = 0;
        ++result.cuts;
    }
    if (finish) {
        Finish(rank, scale, NoteTypeOutcomeKind::Scored, captured_us, delta_us, result);
    }
    return result;
}

NoteTypeResult NativeLongState::Release(const NativeNoteEvent& event, double decision_us) noexcept {
    NoteTypeResult result;
    if (!Ready(result)) return result;
    const NoteTypeStatus event_status = CheckEvent(event);
    if (event_status != NoteTypeStatus::Ok) {
        result.status = event_status;
        return result;
    }
    const double captured = event.song_time.microseconds;
    if (!FiniteTime(decision_us)) {
        result.status = NoteTypeStatus::NonFiniteTime;
        return result;
    }
    if (decision_us < captured) {
        result.status = NoteTypeStatus::WatermarkBehindCapture;
        return result;
    }
    if (event.lane >= kNativeTypeLaneCount) {
        ++result.ignored;
        return result;
    }
    if (captured < advanced_us_) {
        result.status = NoteTypeStatus::BackwardTime;
        return result;
    }
    if (done_ || phase_ == LongPhase::Skipped || phase_ == LongPhase::Idle) {
        ++result.ignored;
        return result;
    }
    advanced_us_ = std::max(advanced_us_, captured);

    if (phase_ != LongPhase::Held) {
        // 2F49D6: a release outside the hold only fades a sound-table entry the
        // note does not own; no state, rank or sound changes.
        ++result.ignored;
        return result;
    }
    if (!config_.release_gate_proven) {
        // 0x2F4060 gates the native release; its result is not decoded, so the
        // fail-closed answer is to refuse the release entirely.
        NoteTypeOutcome& outcome = Append(result);
        outcome = MakeOutcome(NoteTypeOutcomeKind::Unsupported, spec_.lane, captured);
        outcome.reason = "long release gate 0x2F4060 not decoded";
        ++result.unsupported;
        return result;
    }
    return HeldStep(true, false, captured, DeltaUs(event));
}

NoteTypeResult NativeLongState::Expire(double up_to_us, double decision_us) noexcept {
    NoteTypeResult result;
    result.status = NoteTypeStatus::Ok;
    if (!config_.expire_unpressed_heads) return result;
    if (!(up_to_us > deadline_us_ && decision_us > deadline_us_)) return result;
    NoteTypeOutcome& outcome = Append(result);
    outcome = MakeOutcome(NoteTypeOutcomeKind::Missed, spec_.lane, up_to_us);
    outcome.rank = NormalRank::Missed;
    outcome.judged = false; // the native dispatcher owns the drop
    outcome.reason = "adapter head expiry; native dispatcher owns the drop";
    final_rank_ = NormalRank::Missed;
    done_ = true;
    phase_ = LongPhase::Missed;
    return result;
}

NoteTypeResult NativeLongState::Advance(double up_to_us, double decision_us,
                                        bool hold_commit_missed) noexcept {
    NoteTypeResult result;
    if (!Ready(result)) return result;
    if (!FiniteTime(up_to_us) || !FiniteTime(decision_us)) {
        result.status = NoteTypeStatus::NonFiniteTime;
        return result;
    }
    if (up_to_us < advanced_us_) {
        result.status = NoteTypeStatus::BackwardTime;
        return result;
    }
    if (decision_us > up_to_us) {
        result.status = NoteTypeStatus::WatermarkAhead;
        return result;
    }
    advanced_us_ = up_to_us;
    switch (phase_) {
    case LongPhase::PendingEarly:
        if (pending_resolve_us_ <= up_to_us) {
            CommitStart(pending_rank_, false, up_to_us, 0.0, result);
        }
        return result;
    case LongPhase::Waiting: {
        if (spec_.auto_play) {
            // 2F4524: an auto note commits its head as Just at the note time.
            const double delta = (spec_.start_us - up_to_us) - config_.judgment_offset_us;
            const double reached = static_cast<double>(kEndReachedUnits) * unit_us_;
            if (delta < reached) {
                CommitStart(NormalRank::Just, false, up_to_us, delta, result);
                return result;
            }
        }
        NoteTypeResult expiry = Expire(up_to_us, decision_us);
        if (expiry.count != 0) return expiry;
        ++result.ignored;
        return result;
    }
    case LongPhase::Held:
        return HeldStep(false, hold_commit_missed, up_to_us, 0.0);
    case LongPhase::Idle:
    case LongPhase::Skipped:
    case LongPhase::Finished:
    case LongPhase::Missed:
        ++result.ignored;
        return result;
    }
    ++result.ignored;
    return result;
}

// ===========================================================================
// NativeGlissState
// ===========================================================================

NoteTypeStatus NativeGlissState::Configure(const NativeGlissSpec& spec,
                                           const NativeTypeConfig& config) noexcept {
    double unit_us = 0.0;
    const NoteTypeStatus status = ValidateTypeConfig(config, unit_us);
    if (status != NoteTypeStatus::Ok) return status;
    if (spec.lane >= kNativeTypeLaneCount) return NoteTypeStatus::InvalidSpec;
    if (spec.connector_count > 2) return NoteTypeStatus::InvalidSpec;
    if (!FiniteTime(spec.start_us)) return NoteTypeStatus::NonFiniteTime;

    spec_ = spec;
    config_ = config;
    unit_us_ = unit_us;
    resolve_us_ = spec.start_us - config.judgment_offset_us;
    if (!FiniteTime(resolve_us_)) return NoteTypeStatus::InvalidOffset;
    phase_ = GlissPhase::Idle;
    pending_rank_ = NormalRank::Just;
    final_rank_ = NormalRank::TooEarly;
    sound_id_ = 0;
    pending_ = false;
    done_ = false;
    scan_reported_ = false;
    advanced_us_ = 0.0;
    configured_ = true;
    started_ = false;
    return NoteTypeStatus::Ok;
}

NoteTypeStatus NativeGlissState::Begin(std::uint64_t generation, double position_us) noexcept {
    if (!configured_) return NoteTypeStatus::NotConfigured;
    if (generation == 0) return NoteTypeStatus::GenerationZero;
    if (generation <= last_begin_generation_) return NoteTypeStatus::GenerationNotNew;
    if (!FiniteTime(position_us)) return NoteTypeStatus::NonFiniteTime;

    phase_ = spec_.start_us < position_us ? GlissPhase::Skipped : GlissPhase::Waiting;
    pending_rank_ = NormalRank::Just;
    final_rank_ = NormalRank::TooEarly;
    sound_id_ = 0;
    pending_ = false;
    done_ = false;
    scan_reported_ = false;
    advanced_us_ = position_us;
    generation_ = generation;
    last_begin_generation_ = generation;
    started_ = true;
    return NoteTypeStatus::Ok;
}

bool NativeGlissState::Ready(NoteTypeResult& result) const noexcept {
    if (!configured_) {
        result.status = NoteTypeStatus::NotConfigured;
        return false;
    }
    if (!started_) {
        result.status = NoteTypeStatus::NotStarted;
        return false;
    }
    result.status = NoteTypeStatus::Ok;
    return true;
}

NoteTypeStatus NativeGlissState::CheckEvent(const NativeNoteEvent& event) const noexcept {
    if (generation_ == 0 || event.song_time.generation != generation_) {
        return NoteTypeStatus::GenerationMismatch;
    }
    if (!FiniteTime(event.song_time.microseconds)) return NoteTypeStatus::NonFiniteTime;
    return NoteTypeStatus::Ok;
}

double NativeGlissState::DeltaUs(const NativeNoteEvent& event) const noexcept {
    return (spec_.start_us - event.song_time.microseconds) - config_.judgment_offset_us;
}

void NativeGlissState::EmitConnectors(double captured_us, NoteTypeResult& result) noexcept {
    if (spec_.connector_count == 0) return;
    // 1A66D0: chart+64 / chart+72 are only tested for non-null. A non-null pair
    // closes the box primitive at note+216 through its vtable slots +24 / +32:
    // render 0x1A6820 appends one 120-byte record per connector between exactly
    // those slots, and the judge calls both again (and returns 1, consumed). The
    // connector element contents never reach the judgement or the rank.
    NoteTypeOutcome& outcome = Append(result);
    outcome = MakeOutcome(NoteTypeOutcomeKind::ConnectorReleased, spec_.lane, captured_us);
    outcome.connectors = spec_.connector_count;
}

NoteTypeResult NativeGlissState::Judge(double captured_us, double delta_us, NormalRank rank,
                                       std::uint32_t event_id,
                                       NoteTypeOutcomeKind kind) noexcept {
    NoteTypeResult result;
    result.status = NoteTypeStatus::Ok;
    final_rank_ = rank;
    done_ = true;
    pending_ = false;
    phase_ = rank == NormalRank::Missed ? GlissPhase::Missed : GlissPhase::Finished;
    NoteTypeOutcome& outcome = Append(result);
    outcome = MakeOutcome(kind, spec_.lane, captured_us);
    outcome.rank = rank;
    outcome.scale = kScaleNormal;
    outcome.effect_grade = EffectGradeForScale(kScaleNormal);
    outcome.judged = true;
    outcome.event_id = event_id;
    outcome.delta_us = delta_us;
    EmitConnectors(captured_us, result);
    return result;
}

NoteTypeResult NativeGlissState::HandleJudgeEvent(const NativeNoteEvent& event,
                                                  double captured_us, double delta_us,
                                                  NormalRank rank) noexcept {
    NoteTypeResult result;
    result.status = NoteTypeStatus::Ok;
    if (!(spec_.group_id == kWildcardGroupId || spec_.group_id == event.group_id)) {
        // 1A62CD: the note's group id (note+272) must match event+5 unless it is
        // the wildcard 1.
        ++result.ignored;
        return result;
    }
    if (delta_us > 0.0) {
        // 1A62EA: a press inside a window defers to the note time and always
        // records Just(4), independent of the rank at the captured time.
        pending_ = true;
        pending_rank_ = NormalRank::Just;
        phase_ = GlissPhase::PendingEarly;
        NoteTypeOutcome& outcome = Append(result);
        outcome = MakeOutcome(NoteTypeOutcomeKind::PendingEarly, spec_.lane, captured_us);
        outcome.rank = pending_rank_;
        outcome.delta_us = delta_us;
        return result;
    }
    return Judge(captured_us, delta_us, rank, event.event_id, NoteTypeOutcomeKind::Scored);
}

NoteTypeResult NativeGlissState::Press(const NativeNoteEvent& event, double decision_us) noexcept {
    NoteTypeResult result;
    if (!Ready(result)) return result;
    const NoteTypeStatus event_status = CheckEvent(event);
    if (event_status != NoteTypeStatus::Ok) {
        result.status = event_status;
        return result;
    }
    const double captured = event.song_time.microseconds;
    if (!FiniteTime(decision_us)) {
        result.status = NoteTypeStatus::NonFiniteTime;
        return result;
    }
    if (decision_us < captured) {
        result.status = NoteTypeStatus::WatermarkBehindCapture;
        return result;
    }
    if (event.lane >= kNativeTypeLaneCount) {
        ++result.ignored;
        return result;
    }
    if (captured < advanced_us_) {
        result.status = NoteTypeStatus::BackwardTime;
        return result;
    }
    if (done_ || phase_ == GlissPhase::Skipped || phase_ == GlissPhase::Idle) {
        ++result.ignored;
        return result;
    }
    advanced_us_ = std::max(advanced_us_, captured);

    if (event.kind == NativeEventKind::Release) {
        NoteTypeResult release = Release(event, decision_us);
        release.status = NoteTypeStatus::Ok;
        return release;
    }
    if (spec_.auto_play) {
        // 1A60E7: the auto branch wraps the whole manual path, so no event is
        // judged for an auto glissando; Advance completes it as Just.
        ++result.ignored;
        return result;
    }
    if (event.kind == NativeEventKind::Repeat && !config_.gliss_repeat_as_press) {
        // 1A62B4: the repeat path first searches the shared id list at
        // 0x1813E46F8 and only falls through to the press path for a new id.
        // With the list undecoded the branch is refused, not guessed.
        ++result.repeats;
        NoteTypeOutcome& outcome = Append(result);
        outcome = MakeOutcome(NoteTypeOutcomeKind::Unsupported, spec_.lane, captured);
        outcome.event_id = event.event_id;
        outcome.reason = "glissando repeat list 0x1813E46F8 not decoded";
        ++result.unsupported;
        return result;
    }
    if (event.kind == NativeEventKind::Repeat) ++result.repeats;

    const double delta = DeltaUs(event);
    if (pending_ && delta <= 0.0) {
        // 1A63E0 / 1A64E5: the deferred press is judged with the recorded rank.
        return Judge(captured, delta, pending_rank_, event.event_id,
                     NoteTypeOutcomeKind::Scored);
    }
    const NormalRank rank = Classify(delta, config_.windows);
    if (rank == NormalRank::Missed) {
        // 1A619D: a press past the outer late window finishes as Missed(1) with
        // mode 0 and scale 100.
        return Judge(captured, delta, NormalRank::Missed, event.event_id,
                     NoteTypeOutcomeKind::Missed);
    }
    if (rank == NormalRank::TooEarly) {
        ++result.ignored;
        return result;
    }
    return HandleJudgeEvent(event, captured, delta, rank);
}

NoteTypeResult NativeGlissState::Release(const NativeNoteEvent& event,
                                         double decision_us) noexcept {
    NoteTypeResult result;
    if (!Ready(result)) return result;
    const NoteTypeStatus event_status = CheckEvent(event);
    if (event_status != NoteTypeStatus::Ok) {
        result.status = event_status;
        return result;
    }
    const double captured = event.song_time.microseconds;
    if (!FiniteTime(decision_us)) {
        result.status = NoteTypeStatus::NonFiniteTime;
        return result;
    }
    if (decision_us < captured) {
        result.status = NoteTypeStatus::WatermarkBehindCapture;
        return result;
    }
    if (done_ || phase_ == GlissPhase::Skipped || phase_ == GlissPhase::Idle) {
        ++result.ignored;
        return result;
    }
    advanced_us_ = std::max(advanced_us_, captured);
    // 1A6337: the release fades the sound-table entry named by the event id. The
    // glissando judge never starts a keysound of its own, so this is a host cut
    // of an event-owned sound.
    if (event.event_id == 0) {
        ++result.ignored;
        return result;
    }
    NoteTypeOutcome& outcome = Append(result);
    outcome = MakeOutcome(NoteTypeOutcomeKind::KeysoundCut, spec_.lane, captured);
    outcome.sound_id = event.event_id;
    outcome.event_id = event.event_id;
    outcome.sound_cut = true;
    ++result.cuts;
    return result;
}

NoteTypeResult NativeGlissState::Expire(double up_to_us, double decision_us) noexcept {
    NoteTypeResult result;
    result.status = NoteTypeStatus::Ok;
    if (!config_.expire_unpressed_heads) return result;
    const double deadline = resolve_us_ + config_.windows.good_late_us;
    if (!(up_to_us > deadline && decision_us > deadline)) return result;
    NoteTypeOutcome& outcome = Append(result);
    outcome = MakeOutcome(NoteTypeOutcomeKind::Missed, spec_.lane, up_to_us);
    outcome.rank = NormalRank::Missed;
    outcome.judged = false;
    outcome.reason = "adapter head expiry; native dispatcher owns the drop";
    final_rank_ = NormalRank::Missed;
    done_ = true;
    phase_ = GlissPhase::Missed;
    return result;
}

NoteTypeResult NativeGlissState::Advance(double up_to_us, double decision_us,
                                         bool connector_scan_hit) noexcept {
    NoteTypeResult result;
    if (!Ready(result)) return result;
    if (!FiniteTime(up_to_us) || !FiniteTime(decision_us)) {
        result.status = NoteTypeStatus::NonFiniteTime;
        return result;
    }
    if (up_to_us < advanced_us_) {
        result.status = NoteTypeStatus::BackwardTime;
        return result;
    }
    if (decision_us > up_to_us) {
        result.status = NoteTypeStatus::WatermarkAhead;
        return result;
    }
    if (phase_ == GlissPhase::Idle || phase_ == GlissPhase::Skipped ||
        phase_ == GlissPhase::Finished || phase_ == GlissPhase::Missed) {
        advanced_us_ = up_to_us;
        ++result.ignored;
        return result;
    }
    advanced_us_ = up_to_us;
    const double delta = (spec_.start_us - up_to_us) - config_.judgment_offset_us;
    if (phase_ == GlissPhase::Waiting) {
        // The adapter drop runs before the post-note scan so an unpressed
        // glissando cannot stay Waiting past its inclusive Good-late deadline.
        NoteTypeResult expiry = Expire(up_to_us, decision_us);
        if (expiry.count != 0) return expiry;
    }
    if (pending_) {
        if (resolve_us_ <= up_to_us) {
            return Judge(up_to_us, delta, pending_rank_, 0, NoteTypeOutcomeKind::Scored);
        }
        ++result.ignored;
        return result;
    }
    const double reached = static_cast<double>(kEndReachedUnits) * unit_us_;
    if (spec_.auto_play) {
        // 1A65CF: an auto glissando completes at the note time as Just(4)/100.
        if (delta < reached) {
            return Judge(up_to_us, delta, NormalRank::Just, 0, NoteTypeOutcomeKind::Auto);
        }
        ++result.ignored;
        return result;
    }
    if (delta <= 0.0) {
        if (config_.gliss_connector_scan_proven) {
            if (connector_scan_hit) {
                return Judge(up_to_us, delta, NormalRank::Just, 0,
                             NoteTypeOutcomeKind::Scored);
            }
            ++result.ignored;
            return result;
        }
        if (!scan_reported_) {
            // 1A63E6: the native scans the runtime table at 0x1813DF048 for a
            // connector whose key is inside [chart+88, chart+92] and whose id
            // matches note+272, then judges Just. The table is not decoded.
            scan_reported_ = true;
            NoteTypeOutcome& outcome = Append(result);
            outcome = MakeOutcome(NoteTypeOutcomeKind::Unsupported, spec_.lane, up_to_us);
            outcome.reason = "glissando connector scan 0x1813DF048 not decoded";
            ++result.unsupported;
            return result;
        }
        ++result.ignored;
        return result;
    }
    ++result.ignored;
    return result;
}

// ===========================================================================
// NativeTrillState
// ===========================================================================

NoteTypeStatus NativeTrillState::Configure(const NativeTrillSpec& spec,
                                           const NativeTypeConfig& config) noexcept {
    double unit_us = 0.0;
    const NoteTypeStatus status = ValidateTypeConfig(config, unit_us);
    if (status != NoteTypeStatus::Ok) return status;
    if (spec.lane >= kNativeTypeLaneCount) return NoteTypeStatus::InvalidSpec;
    if (!FiniteTime(spec.start_us) || !FiniteTime(spec.end_us)) {
        return NoteTypeStatus::NonFiniteTime;
    }
    if (spec.end_us < spec.start_us) return NoteTypeStatus::InvalidSpec;

    spec_ = spec;
    config_ = config;
    unit_us_ = unit_us;
    sound_serial_ = config.sound_serial_start;
    pending_resolve_us_ = spec.start_us - config.judgment_offset_us;
    if (!FiniteTime(pending_resolve_us_)) return NoteTypeStatus::InvalidOffset;
    phase_ = TrillPhase::Idle;
    head_rank_ = NormalRank::TooEarly;
    pending_rank_ = NormalRank::TooEarly;
    final_rank_ = NormalRank::TooEarly;
    recorded_delta_us_ = 0.0;
    sound_id_ = 0;
    counter_ = kTrillCounterInitial;
    pending_ = false;
    done_ = false;
    advanced_us_ = 0.0;
    configured_ = true;
    started_ = false;
    return NoteTypeStatus::Ok;
}

NoteTypeStatus NativeTrillState::Begin(std::uint64_t generation, double position_us) noexcept {
    if (!configured_) return NoteTypeStatus::NotConfigured;
    if (generation == 0) return NoteTypeStatus::GenerationZero;
    if (generation <= last_begin_generation_) return NoteTypeStatus::GenerationNotNew;
    if (!FiniteTime(position_us)) return NoteTypeStatus::NonFiniteTime;

    phase_ = spec_.start_us < position_us ? TrillPhase::Skipped : TrillPhase::Head;
    head_rank_ = NormalRank::TooEarly;
    pending_rank_ = NormalRank::TooEarly;
    final_rank_ = NormalRank::TooEarly;
    recorded_delta_us_ = 0.0;
    sound_id_ = 0;
    counter_ = kTrillCounterInitial;
    pending_ = false;
    done_ = false;
    advanced_us_ = position_us;
    generation_ = generation;
    last_begin_generation_ = generation;
    started_ = true;
    return NoteTypeStatus::Ok;
}

bool NativeTrillState::Ready(NoteTypeResult& result) const noexcept {
    if (!configured_) {
        result.status = NoteTypeStatus::NotConfigured;
        return false;
    }
    if (!started_) {
        result.status = NoteTypeStatus::NotStarted;
        return false;
    }
    result.status = NoteTypeStatus::Ok;
    return true;
}

NoteTypeStatus NativeTrillState::CheckEvent(const NativeNoteEvent& event) const noexcept {
    if (generation_ == 0 || event.song_time.generation != generation_) {
        return NoteTypeStatus::GenerationMismatch;
    }
    if (!FiniteTime(event.song_time.microseconds)) return NoteTypeStatus::NonFiniteTime;
    return NoteTypeStatus::Ok;
}

double NativeTrillState::DeltaUs(const NativeNoteEvent& event) const noexcept {
    return (spec_.start_us - event.song_time.microseconds) - config_.judgment_offset_us;
}

double NativeTrillState::RemainingUs(double captured_us) const noexcept {
    return ((spec_.end_us - recorded_delta_us_) - captured_us) - config_.judgment_offset_us;
}

void NativeTrillState::CommitStart(NormalRank rank, bool have_event, double captured_us,
                                   double delta_us, NoteTypeResult& result) noexcept {
    if (pending_) rank = pending_rank_;
    head_rank_ = rank;
    recorded_delta_us_ = delta_us;
    pending_ = false;
    phase_ = TrillPhase::Tail;
    if (!spec_.auto_play && have_event) {
        ++sound_serial_;
        if (sound_serial_ > config_.sound_serial_limit) {
            sound_serial_ = config_.sound_serial_start;
        }
        sound_id_ = sound_serial_;
    } else {
        sound_id_ = 0;
    }
    NoteTypeOutcome& outcome = Append(result);
    outcome = MakeOutcome(NoteTypeOutcomeKind::HeadCommitted, spec_.lane, captured_us);
    outcome.rank = rank;
    outcome.delta_us = delta_us;
    outcome.sound_id = sound_id_;
    outcome.sound_started = sound_id_ != 0;
}

void NativeTrillState::Finish(NormalRank rank, std::uint16_t scale, NoteTypeOutcomeKind kind,
                              double captured_us, double delta_us,
                              NoteTypeResult& result) noexcept {
    final_rank_ = rank;
    done_ = true;
    phase_ = kind == NoteTypeOutcomeKind::Missed ? TrillPhase::Missed : TrillPhase::Finished;

    NoteTypeOutcome& scored = Append(result);
    scored = MakeOutcome(kind, spec_.lane, captured_us);
    scored.rank = rank;
    scored.scale = scale;
    scored.effect_grade = EffectGradeForScale(scale);
    scored.sound_id = sound_id_;
    scored.judged = true;
    scored.delta_us = delta_us;
    scored.remaining_us = RemainingUs(captured_us);

    if (rank < NormalRank::Great && sound_id_ != 0) {
        NoteTypeOutcome& cut = Append(result);
        cut = MakeOutcome(NoteTypeOutcomeKind::KeysoundCut, spec_.lane, captured_us);
        cut.sound_id = sound_id_;
        cut.sound_cut = true;
        scored.sound_cut = true;
        sound_id_ = 0;
        ++result.cuts;
    }
}

NoteTypeResult NativeTrillState::TailStep(bool is_release, bool commit_missed,
                                          double captured_us, double delta_us) noexcept {
    NoteTypeResult result;
    result.status = NoteTypeStatus::Ok;
    const double remaining = RemainingUs(captured_us);
    const double reached = static_cast<double>(kEndReachedUnits) * unit_us_;
    if (spec_.auto_play) {
        // 1A7CAB: the auto tail ignores events, sets the commit pulse and
        // completes at the end with the head rank and scale 100.
        if (remaining < reached) {
            Finish(head_rank_, kScaleNormal, NoteTypeOutcomeKind::Auto, captured_us, delta_us,
                   result);
        }
        return result;
    }
    // 1A7B9B: the tail only completes on the stale arm or at the end time; there
    // is no 15-unit band and no early-release penalty grade for a trill.
    const bool finish = commit_missed || remaining < reached;

    if (is_release) {
        // 1A7C01: the tail release increments the native counter and fades the
        // sound unconditionally; 0x2F4060 does not gate it.
        ++result.penalties;
        if (sound_id_ != 0) {
            NoteTypeOutcome& cut = Append(result);
            cut = MakeOutcome(NoteTypeOutcomeKind::KeysoundCut, spec_.lane, captured_us);
            cut.sound_id = sound_id_;
            cut.sound_cut = true;
            sound_id_ = 0;
            ++result.cuts;
        }
    }
    if (finish) {
        // 1A7B9D: rank and scale come from the alternation counter. A positive
        // counter keeps the head rank with scale 100; otherwise Good(2)/40.
        const NormalRank rank = counter_ <= 0 ? NormalRank::Good : head_rank_;
        const std::uint16_t scale = counter_ > 0 ? kScaleNormal : kScaleTrillPenalty;
        Finish(rank, scale, NoteTypeOutcomeKind::Scored, captured_us, delta_us, result);
    }
    return result;
}

NoteTypeResult NativeTrillState::Press(const NativeNoteEvent& event, double decision_us) noexcept {
    NoteTypeResult result;
    if (!Ready(result)) return result;
    const NoteTypeStatus event_status = CheckEvent(event);
    if (event_status != NoteTypeStatus::Ok) {
        result.status = event_status;
        return result;
    }
    const double captured = event.song_time.microseconds;
    if (!FiniteTime(decision_us)) {
        result.status = NoteTypeStatus::NonFiniteTime;
        return result;
    }
    if (decision_us < captured) {
        result.status = NoteTypeStatus::WatermarkBehindCapture;
        return result;
    }
    if (event.lane >= kNativeTypeLaneCount) {
        ++result.ignored;
        return result;
    }
    if (captured < advanced_us_) {
        result.status = NoteTypeStatus::BackwardTime;
        return result;
    }
    if (done_ || phase_ == TrillPhase::Skipped || phase_ == TrillPhase::Idle) {
        ++result.ignored;
        return result;
    }
    advanced_us_ = std::max(advanced_us_, captured);

    if (event.kind == NativeEventKind::Release) {
        NoteTypeResult release = Release(event, decision_us);
        release.status = NoteTypeStatus::Ok;
        return release;
    }

    const double delta = DeltaUs(event);
    const double reached = static_cast<double>(kEndReachedUnits) * unit_us_;
    if (spec_.auto_play) {
        // 1A7D07: the auto trill skips the manual path and every event. At the
        // note time it commits the head as Just with no sound.
        if (phase_ != TrillPhase::Tail && delta < reached) {
            CommitStart(NormalRank::Just, false, captured, delta, result);
        } else {
            ++result.ignored;
        }
        return result;
    }
    if (phase_ == TrillPhase::Tail) {
        if (event.kind == NativeEventKind::Repeat) {
            // The tail branch only handles kind 1 and kind 3; a kind-2 event is
            // inert both at 1A7BE5 and at 1A7C01.
            ++result.repeats;
            NoteTypeOutcome& outcome = Append(result);
            outcome = MakeOutcome(NoteTypeOutcomeKind::RepeatIgnored, spec_.lane, captured);
            outcome.event_id = event.event_id;
            return result;
        }
        // 1A7BE5: a tail press is handed to 0x1A7910, which owns the alternating
        // hit bookkeeping. Without it the press is reported and only the tail's
        // own completion rule is applied.
        if (!config_.trill_tail_handler_proven) {
            NoteTypeOutcome& outcome = Append(result);
            outcome = MakeOutcome(NoteTypeOutcomeKind::Unsupported, spec_.lane, captured);
            outcome.reason = "trill tail handler 0x1A7910 not decoded";
            ++result.unsupported;
        }
        NoteTypeResult step = TailStep(false, false, captured, delta);
        MergeOutcomes(result, step);
        return result;
    }
    if (event.kind == NativeEventKind::Repeat) {
        // The trill judge has no kind-2 branch; the event is inert.
        ++result.repeats;
        NoteTypeOutcome& outcome = Append(result);
        outcome = MakeOutcome(NoteTypeOutcomeKind::RepeatIgnored, spec_.lane, captured);
        outcome.event_id = event.event_id;
        return result;
    }

    if (pending_ && delta <= 0.0) {
        CommitStart(pending_rank_, true, captured, delta, result);
        return result;
    }
    const NormalRank rank = Classify(delta, config_.windows);
    if (rank == NormalRank::TooEarly) {
        ++result.ignored;
        return result;
    }
    if (rank == NormalRank::Missed) {
        // 1A7D36: a press past the late window commits and finishes as Missed.
        CommitStart(rank, true, captured, delta, result);
        Finish(rank, kScaleNormal, NoteTypeOutcomeKind::Missed, captured, delta, result);
        return result;
    }
    if (delta > 0.0 && delta <= config_.windows.great_early_us) {
        pending_ = true;
        pending_rank_ = rank == NormalRank::Just ? NormalRank::Just : NormalRank::Great;
        phase_ = TrillPhase::PendingEarly;
        NoteTypeOutcome& outcome = Append(result);
        outcome = MakeOutcome(NoteTypeOutcomeKind::PendingEarly, spec_.lane, captured);
        outcome.rank = pending_rank_;
        outcome.delta_us = delta;
        return result;
    }
    CommitStart(rank, true, captured, delta, result);
    return result;
}

NoteTypeResult NativeTrillState::Release(const NativeNoteEvent& event,
                                         double decision_us) noexcept {
    NoteTypeResult result;
    if (!Ready(result)) return result;
    const NoteTypeStatus event_status = CheckEvent(event);
    if (event_status != NoteTypeStatus::Ok) {
        result.status = event_status;
        return result;
    }
    const double captured = event.song_time.microseconds;
    if (!FiniteTime(decision_us)) {
        result.status = NoteTypeStatus::NonFiniteTime;
        return result;
    }
    if (decision_us < captured) {
        result.status = NoteTypeStatus::WatermarkBehindCapture;
        return result;
    }
    if (event.lane >= kNativeTypeLaneCount) {
        ++result.ignored;
        return result;
    }
    if (captured < advanced_us_) {
        result.status = NoteTypeStatus::BackwardTime;
        return result;
    }
    if (done_ || phase_ == TrillPhase::Skipped || phase_ == TrillPhase::Idle) {
        ++result.ignored;
        return result;
    }
    advanced_us_ = std::max(advanced_us_, captured);
    if (phase_ != TrillPhase::Tail) {
        // 1A8119: a release before the tail only fades an event-owned entry.
        ++result.ignored;
        return result;
    }
    return TailStep(true, false, captured, DeltaUs(event));
}

NoteTypeResult NativeTrillState::Expire(double up_to_us, double decision_us) noexcept {
    NoteTypeResult result;
    result.status = NoteTypeStatus::Ok;
    if (!config_.expire_unpressed_heads) return result;
    const double deadline = pending_resolve_us_ + config_.windows.good_late_us;
    if (!(up_to_us > deadline && decision_us > deadline)) return result;
    NoteTypeOutcome& outcome = Append(result);
    outcome = MakeOutcome(NoteTypeOutcomeKind::Missed, spec_.lane, up_to_us);
    outcome.rank = NormalRank::Missed;
    outcome.judged = false;
    outcome.reason = "adapter head expiry; native dispatcher owns the drop";
    final_rank_ = NormalRank::Missed;
    done_ = true;
    phase_ = TrillPhase::Missed;
    return result;
}

NoteTypeResult NativeTrillState::Advance(double up_to_us, double decision_us,
                                         bool hold_commit_missed) noexcept {
    NoteTypeResult result;
    if (!Ready(result)) return result;
    if (!FiniteTime(up_to_us) || !FiniteTime(decision_us)) {
        result.status = NoteTypeStatus::NonFiniteTime;
        return result;
    }
    if (up_to_us < advanced_us_) {
        result.status = NoteTypeStatus::BackwardTime;
        return result;
    }
    if (decision_us > up_to_us) {
        result.status = NoteTypeStatus::WatermarkAhead;
        return result;
    }
    advanced_us_ = up_to_us;
    if (spec_.auto_play &&
        (phase_ == TrillPhase::Head || phase_ == TrillPhase::PendingEarly)) {
        // 1A7D07: an auto trill commits its head as Just at the note time.
        const double delta = (spec_.start_us - up_to_us) - config_.judgment_offset_us;
        const double reached = static_cast<double>(kEndReachedUnits) * unit_us_;
        if (delta < reached) {
            CommitStart(NormalRank::Just, false, up_to_us, delta, result);
        } else {
            ++result.ignored;
        }
        return result;
    }
    switch (phase_) {
    case TrillPhase::PendingEarly:
        if (pending_resolve_us_ <= up_to_us) {
            CommitStart(pending_rank_, false, up_to_us, 0.0, result);
        }
        return result;
    case TrillPhase::Head: {
        NoteTypeResult expiry = Expire(up_to_us, decision_us);
        if (expiry.count != 0) return expiry;
        ++result.ignored;
        return result;
    }
    case TrillPhase::Tail: {
        const double delta = (spec_.start_us - up_to_us) - config_.judgment_offset_us;
        return TailStep(false, hold_commit_missed, up_to_us, delta);
    }
    case TrillPhase::Idle:
    case TrillPhase::Skipped:
    case TrillPhase::Finished:
    case TrillPhase::Missed:
        ++result.ignored;
        return result;
    }
    ++result.ignored;
    return result;
}

} // namespace nosfix
