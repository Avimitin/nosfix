#include "nosfix/native_note_state.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>

namespace nosfix {

using namespace native_layout;

const char* NoteKindName(NoteKind kind) noexcept {
    switch (kind) {
    case NoteKind::Normal: return "normal";
    case NoteKind::Long: return "long";
    case NoteKind::Gliss: return "gliss";
    case NoteKind::Trill: return "trill";
    }
    return "invalid";
}

const char* NoteConfigStatusName(NoteConfigStatus status) noexcept {
    switch (status) {
    case NoteConfigStatus::Accepted: return "accepted";
    case NoteConfigStatus::TooManyNotes: return "too-many-notes";
    case NoteConfigStatus::NonFiniteTime: return "non-finite-time";
    case NoteConfigStatus::InvalidTimeRange: return "invalid-time-range";
    case NoteConfigStatus::InvalidLane: return "invalid-lane";
    case NoteConfigStatus::UnsupportedKind: return "unsupported-kind";
    case NoteConfigStatus::DuplicateChartIndex: return "duplicate-chart-index";
    case NoteConfigStatus::InvalidWindows: return "invalid-windows";
    case NoteConfigStatus::WindowsNotSampled: return "windows-not-sampled";
    case NoteConfigStatus::DegenerateWindows: return "degenerate-windows";
    case NoteConfigStatus::InvalidOffset: return "invalid-offset";
    case NoteConfigStatus::InvalidRate: return "invalid-rate";
    case NoteConfigStatus::UnsupportedSelection: return "unsupported-selection";
    }
    return "invalid";
}

const char* NoteRunStatusName(NoteRunStatus status) noexcept {
    switch (status) {
    case NoteRunStatus::Ok: return "ok";
    case NoteRunStatus::NotConfigured: return "not-configured";
    case NoteRunStatus::NotStarted: return "not-started";
    case NoteRunStatus::GenerationZero: return "generation-zero";
    case NoteRunStatus::GenerationNotNew: return "generation-not-new";
    case NoteRunStatus::NonFiniteTime: return "non-finite-time";
    case NoteRunStatus::BackwardTime: return "backward-time";
    case NoteRunStatus::GenerationMismatch: return "generation-mismatch";
    case NoteRunStatus::WatermarkBehindCapture: return "watermark-behind-capture";
    case NoteRunStatus::WatermarkAhead: return "watermark-ahead";
    case NoteRunStatus::InsufficientCapacity: return "insufficient-capacity";
    }
    return "invalid";
}

namespace {

bool FiniteTime(double value) noexcept {
    return std::isfinite(value) && std::abs(value) <= kNativeMaximumTimeUs;
}

std::int32_t ReadI32(const std::byte* bytes, std::size_t offset) noexcept {
    std::int32_t value = 0;
    std::memcpy(&value, bytes + offset, sizeof(value));
    return value;
}

std::int32_t ReadI16(const std::byte* bytes, std::size_t offset) noexcept {
    std::int16_t value = 0;
    std::memcpy(&value, bytes + offset, sizeof(value));
    return value;
}

} // namespace

namespace native_layout {

bool DecodeChartNote(std::span<const std::byte> record, ChartNoteView& out) noexcept {
    if (record.size() < kChartNoteBytes) return false;
    out.start_ms = ReadI32(record.data(), kChartNoteStartMs);
    out.end_ms = ReadI32(record.data(), kChartNoteEndMs);
    out.start_frame = ReadI32(record.data(), kChartNoteStartFrame);
    out.end_frame = ReadI32(record.data(), kChartNoteEndFrame);
    out.lane = ReadI16(record.data(), kChartNoteLane);
    out.left_key = ReadI32(record.data(), kChartNoteLeftKey);
    out.right_key = ReadI32(record.data(), kChartNoteRightKey);
    out.mode = ReadI32(record.data(), kChartNoteMode);
    out.effect_kind = ReadI32(record.data(), kChartNoteEffectKind);
    out.auto_flag = static_cast<std::uint8_t>(record[kChartNoteAutoByte]) != 0;
    return true;
}

namespace {

bool SameWindowUnits(const NativeWindowUnits& a, const NativeWindowUnits& b) noexcept {
    return a.effect_early == b.effect_early && a.effect_late == b.effect_late &&
           a.good_earliest == b.good_earliest && a.good_latest == b.good_latest &&
           a.great_earliest == b.great_earliest &&
           a.great_latest == b.great_latest &&
           a.just_earliest == b.just_earliest && a.just_latest == b.just_latest;
}

} // namespace

SampledNormalWindows ValidateNormalWindowUnits(
    const NativeWindowUnits& units, double units_per_second) noexcept {
    SampledNormalWindows result;
    result.units = units;
    result.matches_initial_seed = SameWindowUnits(units, kInitialWindowSeeds);
    if (!std::isfinite(units_per_second) || units_per_second <= 0.0) {
        result.status = WindowSampleStatus::InvalidRate;
        return result;
    }
    if (units.effect_early < units.good_earliest ||
        units.effect_late > units.good_latest) {
        result.status = WindowSampleStatus::InconsistentEffectBand;
        return result;
    }
    LegacyNormalWindowUnits legacy;
    legacy.just_earliest_units = units.just_earliest;
    legacy.just_latest_units = units.just_latest;
    legacy.great_earliest_units = units.great_earliest;
    legacy.great_latest_units = units.great_latest;
    legacy.good_earliest_units = units.good_earliest;
    legacy.good_latest_units = units.good_latest;
    const LegacyNormalWindows converted =
        ConvertLegacyNormalWindows(legacy, units_per_second);
    if (!converted.ok()) {
        result.status = WindowSampleStatus::InvalidUnits;
        return result;
    }
    result.windows = converted.windows;
    result.status = WindowSampleStatus::Ok;
    return result;
}

SampledNormalWindows ReadNormalWindowUnits(std::span<const std::byte> image,
                                           double units_per_second) noexcept {
    constexpr std::size_t kRequired =
        static_cast<std::size_t>(kEffectBandEarlyRva) + sizeof(std::int32_t) * 8;
    if (image.size() < kRequired) {
        return SampledNormalWindows{};
    }
    NativeWindowUnits units;
    units.effect_early = ReadI32(image.data(), kEffectBandEarlyRva);
    units.effect_late = ReadI32(image.data(), kEffectBandLateRva);
    units.good_earliest = ReadI32(image.data(), kEffectBandEarlyRva + 8);
    units.good_latest = ReadI32(image.data(), kEffectBandEarlyRva + 12);
    units.great_earliest = ReadI32(image.data(), kEffectBandEarlyRva + 16);
    units.great_latest = ReadI32(image.data(), kEffectBandEarlyRva + 20);
    units.just_earliest = ReadI32(image.data(), kEffectBandEarlyRva + 24);
    units.just_latest = ReadI32(image.data(), kEffectBandEarlyRva + 28);
    return ValidateNormalWindowUnits(units, units_per_second);
}

} // namespace native_layout

NoteConfigStatus NativeNoteState::Configure(std::span<const NativeNoteSpec> notes,
                                            const NativeNoteConfig& config) {
    if (notes.size() > kNativeMaxNotes) return NoteConfigStatus::TooManyNotes;
    if (config.selection != NoteSelectionPolicy::AdapterEarliestPending) {
        return NoteConfigStatus::UnsupportedSelection;
    }
    if (!config.windows_sampled) return NoteConfigStatus::WindowsNotSampled;
    if (ValidateNormalWindows(config.windows) != NormalWindowStatus::Ok) {
        return NoteConfigStatus::InvalidWindows;
    }
    const NormalWindows& windows = config.windows;
    if (windows.just_early_us == 0.0 && windows.just_late_us == 0.0 &&
        windows.great_early_us == 0.0 && windows.great_late_us == 0.0 &&
        windows.good_early_us == 0.0 && windows.good_late_us == 0.0) {
        return NoteConfigStatus::DegenerateWindows;
    }
    if (!FiniteTime(config.judgment_offset_us) ||
        !FiniteTime(config.eligibility_total_offset_us)) {
        return NoteConfigStatus::InvalidOffset;
    }
    if (!std::isfinite(config.units_per_second) || config.units_per_second <= 0.0 ||
        config.units_per_second > 1.0e6) {
        return NoteConfigStatus::InvalidRate;
    }

    std::vector<NativeNoteSpec> specs;
    std::vector<Runtime> runtime;
    std::vector<std::uint32_t> order;
    std::vector<std::uint32_t> chart_ids;
    specs.reserve(notes.size());
    runtime.reserve(notes.size());
    order.reserve(notes.size());
    chart_ids.reserve(notes.size());

    const double unit_us = 1'000'000.0 / config.units_per_second;
    for (const NativeNoteSpec& note : notes) {
        if (note.kind != NoteKind::Normal) return NoteConfigStatus::UnsupportedKind;
        if (note.lane >= kNativeNoteLaneCount) return NoteConfigStatus::InvalidLane;
        if (!FiniteTime(note.start_us) || !FiniteTime(note.end_us)) {
            return NoteConfigStatus::NonFiniteTime;
        }
        if (note.end_us < note.start_us) return NoteConfigStatus::InvalidTimeRange;

        Runtime state;
        state.phase = NativeNotePhase::Idle;
        state.deadline_us = (note.start_us - config.judgment_offset_us) +
                            config.windows.good_late_us;
        state.resolve_us = note.start_us - config.judgment_offset_us;
        state.auto_us = state.resolve_us - unit_us;
        if (!FiniteTime(state.deadline_us) || !FiniteTime(state.resolve_us) ||
            !FiniteTime(state.auto_us)) {
            return NoteConfigStatus::InvalidOffset;
        }
        specs.push_back(note);
        runtime.push_back(state);
        chart_ids.push_back(note.chart_index);
        order.push_back(static_cast<std::uint32_t>(specs.size() - 1));
    }

    std::sort(chart_ids.begin(), chart_ids.end());
    for (std::size_t i = 1; i < chart_ids.size(); ++i) {
        if (chart_ids[i] == chart_ids[i - 1]) {
            return NoteConfigStatus::DuplicateChartIndex;
        }
    }

    std::sort(order.begin(), order.end(), [&specs](std::uint32_t a, std::uint32_t b) {
        if (specs[a].start_us != specs[b].start_us) {
            return specs[a].start_us < specs[b].start_us;
        }
        return specs[a].chart_index < specs[b].chart_index;
    });

    std::vector<Due> due;
    due.reserve(notes.size());

    specs_ = std::move(specs);
    runtime_ = std::move(runtime);
    order_ = std::move(order);
    due_ = std::move(due);
    config_ = config;
    ResetRuntime();
    configured_ = true;
    return NoteConfigStatus::Accepted;
}

void NativeNoteState::ResetRuntime() noexcept {
    for (Runtime& state : runtime_) {
        state.phase = NativeNotePhase::Idle;
        state.rank = NormalRank::TooEarly;
        state.pending_rank = NormalRank::TooEarly;
        state.pending_grade = 0;
        state.pending_input_us = 0.0;
    }
    active_sound_.fill(0);
    active_sound_note_.fill(0);
    generation_ = 0;
    started_ = false;
    advanced_us_ = 0.0;
}

NoteRunStatus NativeNoteState::Begin(std::uint64_t generation, double start_us) noexcept {
    if (!configured_) return NoteRunStatus::NotConfigured;
    if (generation == 0) return NoteRunStatus::GenerationZero;
    if (generation <= last_begin_generation_) return NoteRunStatus::GenerationNotNew;
    if (!FiniteTime(start_us)) return NoteRunStatus::NonFiniteTime;

    for (std::uint32_t index : order_) {
        Runtime& state = runtime_[index];
        state.phase = specs_[index].start_us < start_us
                          ? NativeNotePhase::Skipped
                          : NativeNotePhase::Pending;
        state.rank = NormalRank::TooEarly;
        state.pending_rank = NormalRank::TooEarly;
        state.pending_grade = 0;
        state.pending_input_us = 0.0;
    }
    active_sound_.fill(0);
    active_sound_note_.fill(0);
    generation_ = generation;
    last_begin_generation_ = generation;
    advanced_us_ = start_us;
    started_ = true;
    return NoteRunStatus::Ok;
}

NoteRunStatus NativeNoteState::Advance(double up_to_us, double decision_us,
                                       std::span<NativeNoteOutcome> out,
                                       NoteAdvance& counts) noexcept {
    if (!configured_) return NoteRunStatus::NotConfigured;
    if (!started_) return NoteRunStatus::NotStarted;
    if (!CapacityOk(out)) return NoteRunStatus::InsufficientCapacity;
    if (!FiniteTime(up_to_us) || !FiniteTime(decision_us)) {
        return NoteRunStatus::NonFiniteTime;
    }
    if (up_to_us < advanced_us_) return NoteRunStatus::BackwardTime;
    if (decision_us > up_to_us) return NoteRunStatus::WatermarkAhead;
    return AdvanceLocked(up_to_us, decision_us, out, counts);
}

NoteRunStatus NativeNoteState::AdvanceLocked(double up_to_us, double decision_us,
                                             std::span<NativeNoteOutcome> out,
                                             NoteAdvance& counts) noexcept {
    // Automatic crossings use current song time; a miss additionally needs the
    // certified input watermark strictly past its inclusive deadline.
    const double miss_limit = std::min(up_to_us, decision_us);
    due_.clear();
    for (std::uint32_t index : order_) {
        const NativeNoteSpec& spec = specs_[index];
        const Runtime& state = runtime_[index];
        if (state.phase == NativeNotePhase::PendingEarly) {
            if (state.resolve_us <= up_to_us) {
                due_.push_back(Due{state.resolve_us, spec.chart_index, index,
                                   NoteOutcomeKind::Scored, state.pending_rank,
                                   state.pending_grade, state.pending_input_us});
            }
            continue;
        }
        if (state.phase != NativeNotePhase::Pending) continue;
        if (spec.auto_play && state.auto_us < up_to_us) {
            due_.push_back(Due{state.auto_us, spec.chart_index, index,
                               NoteOutcomeKind::Auto, NormalRank::Just, 0, 0.0});
            continue;
        }
        if (state.deadline_us < miss_limit) {
            due_.push_back(Due{state.deadline_us, spec.chart_index, index,
                               NoteOutcomeKind::Missed, NormalRank::Missed, 0, 0.0});
        }
    }

    std::sort(due_.begin(), due_.end(), [](const Due& a, const Due& b) {
        if (a.at_us != b.at_us) return a.at_us < b.at_us;
        return a.chart_index < b.chart_index;
    });
    for (const Due& item : due_) Emit(item, out, counts);
    advanced_us_ = up_to_us;
    return NoteRunStatus::Ok;
}

void NativeNoteState::Emit(const Due& due, std::span<NativeNoteOutcome> out,
                           NoteAdvance& counts) noexcept {
    if (counts.written >= out.size()) return;
    if (due.kind == NoteOutcomeKind::Missed) {
        Miss(due.note, counts);
    } else {
        Complete(due.note, due.rank, due.kind, counts);
    }
    NativeNoteOutcome& outcome = out[counts.written++];
    outcome = NativeNoteOutcome{};
    outcome.note_index = due.note;
    outcome.lane = specs_[due.note].lane;
    outcome.kind = due.kind;
    outcome.rank = due.kind == NoteOutcomeKind::Missed ? NormalRank::Missed : due.rank;
    outcome.effect_grade = due.kind == NoteOutcomeKind::Missed ? 0 : due.grade;
    outcome.sound_id = (due.kind == NoteOutcomeKind::Missed) ? 0 : specs_[due.note].sound_id;
    outcome.at_us = due.at_us;
    outcome.input_us = due.kind == NoteOutcomeKind::Scored ? due.input_us : 0.0;
}

void NativeNoteState::Complete(std::size_t index, NormalRank rank, NoteOutcomeKind kind,
                               NoteAdvance& counts) noexcept {
    Runtime& state = runtime_[index];
    state.phase = NativeNotePhase::Completed;
    state.rank = rank;
    state.pending_rank = NormalRank::TooEarly;
    state.pending_grade = 0;
    state.pending_input_us = 0.0;
    const NativeNoteSpec& spec = specs_[index];
    if (spec.sound_id != 0) {
        active_sound_[spec.lane] = spec.sound_id;
        active_sound_note_[spec.lane] = static_cast<std::uint32_t>(index);
    }
    if (kind == NoteOutcomeKind::Auto) {
        ++counts.auto_completed;
    } else {
        ++counts.scored;
    }
}

void NativeNoteState::Miss(std::size_t index, NoteAdvance& counts) noexcept {
    Runtime& state = runtime_[index];
    state.phase = NativeNotePhase::Missed;
    state.rank = NormalRank::Missed;
    state.pending_rank = NormalRank::TooEarly;
    state.pending_grade = 0;
    ++counts.missed;
}

int NativeNoteState::SelectPending(std::uint8_t lane, const SongTime& captured) const noexcept {
    for (std::uint32_t index : order_) {
        const NativeNoteSpec& spec = specs_[index];
        if (spec.lane != lane || spec.auto_play) continue;
        const NativeNotePhase phase = runtime_[index].phase;
        if (phase != NativeNotePhase::Pending && phase != NativeNotePhase::PendingEarly) {
            continue;
        }
        const NormalEligibilityResult eligibility = EvaluateNormalEligibility(
            spec.start_us, captured, generation_, config_.eligibility_total_offset_us,
            config_.windows);
        if (eligibility.ok() && eligibility.eligible) return static_cast<int>(index);
    }
    return -1;
}

NoteRunStatus NativeNoteState::Press(const TimedInputEvent& event, double decision_us,
                                     std::span<NativeNoteOutcome> out,
                                     NoteAdvance& counts) noexcept {
    if (!configured_) return NoteRunStatus::NotConfigured;
    if (!started_) return NoteRunStatus::NotStarted;
    if (!CapacityOk(out)) return NoteRunStatus::InsufficientCapacity;
    if (event.song_time.generation != generation_ || generation_ == 0) {
        return NoteRunStatus::GenerationMismatch;
    }
    const double captured = event.song_time.microseconds;
    if (!FiniteTime(captured) || !FiniteTime(decision_us)) {
        return NoteRunStatus::NonFiniteTime;
    }
    if (decision_us < captured) return NoteRunStatus::WatermarkBehindCapture;
    if (event.input.lane >= kNativeNoteLaneCount) {
        ++counts.ignored_presses;
        return NoteRunStatus::Ok;
    }
    if (captured < advanced_us_) return NoteRunStatus::BackwardTime;
    if (captured > advanced_us_) {
        const NoteRunStatus advanced = AdvanceLocked(captured, decision_us, out, counts);
        if (advanced != NoteRunStatus::Ok) return advanced;
    }

    const int selected = SelectPending(event.input.lane, event.song_time);
    if (selected < 0) {
        ++counts.ignored_presses;
        return NoteRunStatus::Ok;
    }

    const NativeNoteSpec& spec = specs_[static_cast<std::size_t>(selected)];
    const NormalPressResult result = EvaluateNormalPress(
        spec.start_us, event.song_time, generation_, config_.judgment_offset_us,
        config_.windows);
    if (!result.ok() || !IsScoringNormalRank(result.rank)) {
        ++counts.ignored_presses;
        return NoteRunStatus::Ok;
    }

    const std::uint8_t grade = result.rank == NormalRank::Just
                                   ? 0
                                   : (result.delta_us > 0.0 ? 1 : 2);
    Runtime& state = runtime_[static_cast<std::size_t>(selected)];
    if (result.deferred_early) {
        if (state.phase == NativeNotePhase::PendingEarly) ++counts.early_overwritten;
        state.phase = NativeNotePhase::PendingEarly;
        state.pending_rank = result.recorded_rank;
        state.pending_grade = grade;
        state.pending_input_us = captured;
        ++counts.early_deferred;
        return NoteRunStatus::Ok;
    }

    if (counts.written >= out.size()) return NoteRunStatus::InsufficientCapacity;
    Complete(static_cast<std::size_t>(selected), result.rank,
             NoteOutcomeKind::Scored, counts);
    NativeNoteOutcome& outcome = out[counts.written++];
    outcome = NativeNoteOutcome{};
    outcome.note_index = static_cast<std::uint32_t>(selected);
    outcome.lane = spec.lane;
    outcome.kind = NoteOutcomeKind::Scored;
    outcome.rank = result.rank;
    outcome.effect_grade = grade;
    outcome.sound_id = spec.sound_id;
    outcome.at_us = captured;
    outcome.input_us = captured;
    return NoteRunStatus::Ok;
}

NoteRunStatus NativeNoteState::Release(const TimedInputEvent& event,
                                       std::span<NativeNoteOutcome> out,
                                       NoteAdvance& counts) noexcept {
    if (!configured_) return NoteRunStatus::NotConfigured;
    if (!started_) return NoteRunStatus::NotStarted;
    if (!CapacityOk(out)) return NoteRunStatus::InsufficientCapacity;
    if (event.song_time.generation != generation_ || generation_ == 0) {
        return NoteRunStatus::GenerationMismatch;
    }
    const double captured = event.song_time.microseconds;
    if (!FiniteTime(captured)) return NoteRunStatus::NonFiniteTime;
    if (event.input.lane >= kNativeNoteLaneCount) {
        ++counts.ignored_releases;
        return NoteRunStatus::Ok;
    }
    const std::uint8_t lane = event.input.lane;
    const std::uint32_t sound = active_sound_[lane];
    if (sound == 0) {
        ++counts.ignored_releases;
        return NoteRunStatus::Ok;
    }
    if (counts.written >= out.size()) return NoteRunStatus::InsufficientCapacity;
    NativeNoteOutcome& outcome = out[counts.written++];
    outcome = NativeNoteOutcome{};
    outcome.note_index = active_sound_note_[lane];
    outcome.lane = lane;
    outcome.kind = NoteOutcomeKind::KeysoundCut;
    outcome.rank = NormalRank::TooEarly;
    outcome.effect_grade = 0;
    outcome.sound_id = sound;
    outcome.at_us = captured;
    outcome.input_us = captured;
    active_sound_[lane] = 0;
    active_sound_note_[lane] = 0;
    ++counts.cuts;
    return NoteRunStatus::Ok;
}

NativeNotePhase NativeNoteState::Phase(std::size_t index) const noexcept {
    return index < runtime_.size() ? runtime_[index].phase : NativeNotePhase::Idle;
}

NormalRank NativeNoteState::FinalRank(std::size_t index) const noexcept {
    return index < runtime_.size() ? runtime_[index].rank : NormalRank::TooEarly;
}

std::size_t NativeNoteState::PendingCount() const noexcept {
    return static_cast<std::size_t>(std::count_if(
        runtime_.begin(), runtime_.end(), [](const Runtime& state) {
            return state.phase == NativeNotePhase::Pending ||
                   state.phase == NativeNotePhase::PendingEarly;
        }));
}

std::size_t NativeNoteState::CompletedCount() const noexcept {
    return static_cast<std::size_t>(std::count_if(
        runtime_.begin(), runtime_.end(), [](const Runtime& state) {
            return state.phase == NativeNotePhase::Completed;
        }));
}

std::size_t NativeNoteState::MissedCount() const noexcept {
    return static_cast<std::size_t>(std::count_if(
        runtime_.begin(), runtime_.end(), [](const Runtime& state) {
            return state.phase == NativeNotePhase::Missed;
        }));
}

std::size_t NativeNoteState::SkippedCount() const noexcept {
    return static_cast<std::size_t>(std::count_if(
        runtime_.begin(), runtime_.end(), [](const Runtime& state) {
            return state.phase == NativeNotePhase::Skipped;
        }));
}

std::uint32_t NativeNoteState::ActiveSound(std::uint8_t lane) const noexcept {
    return lane < kNativeNoteLaneCount ? active_sound_[lane] : 0;
}

} // namespace nosfix
