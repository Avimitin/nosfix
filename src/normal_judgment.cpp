#include "nosfix/normal_judgment.hpp"

#include <cmath>

namespace nosfix {
namespace {

// 2^53: the largest integer magnitude every one of whose neighbours is exactly
// representable as a double. Bounds outside it would be silently rounded by the
// conversion, so they are rejected. The check is made on the integer, because
// static_cast<double>(2^53 + 1) already rounds to 2^53.
constexpr std::int64_t kMaxExactLegacyUnits = 9'007'199'254'740'992LL;

[[nodiscard]] NormalStatus ComputeDelta(double note_us, SongTime captured, double offset_us,
                                      double& delta_us) noexcept {
    if (!std::isfinite(note_us) || !std::isfinite(captured.microseconds) ||
        !std::isfinite(offset_us)) {
        return NormalStatus::NonFiniteInput;
    }
    // Deliberately left-to-right: (note - input) - offset.
    const double delta = (note_us - captured.microseconds) - offset_us;
    if (!std::isfinite(delta)) return NormalStatus::ArithmeticOverflow;
    delta_us = delta;
    return NormalStatus::Ok;
}

// Callers must have rejected out-of-range magnitudes on the integer first.
[[nodiscard]] bool ConvertUnits(double units, double units_per_second,
                               double& microseconds) noexcept {
    const double converted = units * 1'000'000.0 / units_per_second;
    if (!std::isfinite(converted)) return false;
    microseconds = converted;
    return true;
}

} // namespace

const char* NormalRankName(NormalRank rank) noexcept {
    switch (rank) {
    case NormalRank::TooEarly: return "TooEarly";
    case NormalRank::Missed: return "Missed";
    case NormalRank::Good: return "Good";
    case NormalRank::Great: return "Great";
    case NormalRank::Just: return "Just";
    }
    return "Unknown";
}

const char* NormalWindowStatusName(NormalWindowStatus status) noexcept {
    switch (status) {
    case NormalWindowStatus::Ok: return "Ok";
    case NormalWindowStatus::NonFinite: return "NonFinite";
    case NormalWindowStatus::Negative: return "Negative";
    case NormalWindowStatus::NotNested: return "NotNested";
    }
    return "Unknown";
}

const char* NormalStatusName(NormalStatus status) noexcept {
    switch (status) {
    case NormalStatus::Ok: return "Ok";
    case NormalStatus::NotEvaluated: return "NotEvaluated";
    case NormalStatus::GenerationMismatch: return "GenerationMismatch";
    case NormalStatus::NonFiniteInput: return "NonFiniteInput";
    case NormalStatus::InvalidWindows: return "InvalidWindows";
    case NormalStatus::ArithmeticOverflow: return "ArithmeticOverflow";
    case NormalStatus::StaleWatermark: return "StaleWatermark";
    }
    return "Unknown";
}

const char* LegacyWindowStatusName(LegacyWindowStatus status) noexcept {
    switch (status) {
    case LegacyWindowStatus::Ok: return "Ok";
    case LegacyWindowStatus::NonFiniteRate: return "NonFiniteRate";
    case LegacyWindowStatus::NonPositiveRate: return "NonPositiveRate";
    case LegacyWindowStatus::InvalidBoundSign: return "InvalidBoundSign";
    case LegacyWindowStatus::UnrepresentableBound: return "UnrepresentableBound";
    case LegacyWindowStatus::InvalidWindows: return "InvalidWindows";
    }
    return "Unknown";
}

NormalWindowStatus ValidateNormalWindows(const NormalWindows& windows) noexcept {
    const double widths[6] = {
        windows.just_early_us,  windows.just_late_us,  windows.great_early_us,
        windows.great_late_us,  windows.good_early_us,  windows.good_late_us,
    };
    for (const double width : widths) {
        if (!std::isfinite(width)) return NormalWindowStatus::NonFinite;
    }
    for (const double width : widths) {
        if (width < 0.0) return NormalWindowStatus::Negative;
    }
    // Just inside Great inside Good, on each side independently. The late side
    // compares magnitudes, so a wider tier needs a greater late magnitude.
    if (windows.just_early_us > windows.great_early_us ||
        windows.great_early_us > windows.good_early_us) {
        return NormalWindowStatus::NotNested;
    }
    if (windows.just_late_us > windows.great_late_us ||
        windows.great_late_us > windows.good_late_us) {
        return NormalWindowStatus::NotNested;
    }
    return NormalWindowStatus::Ok;
}

NormalRank ClassifyNormalDelta(double delta_us, const NormalWindows& windows) noexcept {
    // Fail closed: malformed input or table can never produce a scoring rank or
    // Missed, so a caller that ignored validation cannot consume garbage.
    if (!std::isfinite(delta_us) || ValidateNormalWindows(windows) != NormalWindowStatus::Ok) {
        return NormalRank::TooEarly;
    }
    // Native precedence: innermost window first, inclusive on both ends.
    if (delta_us <= windows.just_early_us && delta_us >= -windows.just_late_us) {
        return NormalRank::Just;
    }
    if (delta_us <= windows.great_early_us && delta_us >= -windows.great_late_us) {
        return NormalRank::Great;
    }
    if (delta_us <= windows.good_early_us && delta_us >= -windows.good_late_us) {
        return NormalRank::Good;
    }
    return delta_us >= 0.0 ? NormalRank::TooEarly : NormalRank::Missed;
}

NormalPressResult EvaluateNormalPress(double note_us, SongTime captured,
                                      std::uint64_t expected_generation,
                                      double judgment_offset_us,
                                      const NormalWindows& windows) noexcept {
    NormalPressResult result{};

    if (!IsAcceptableNormalGeneration(expected_generation, captured.generation)) {
        result.status = NormalStatus::GenerationMismatch;
        return result;
    }

    double delta_us = 0.0;
    const NormalStatus delta_status =
        ComputeDelta(note_us, captured, judgment_offset_us, delta_us);
    if (delta_status != NormalStatus::Ok) {
        result.status = delta_status;
        return result;
    }

    if (ValidateNormalWindows(windows) != NormalWindowStatus::Ok) {
        result.status = NormalStatus::InvalidWindows;
        return result;
    }

    result.status = NormalStatus::Ok;
    result.delta_us = delta_us;
    result.rank = ClassifyNormalDelta(delta_us, windows);
    result.eligible = IsScoringNormalRank(result.rank);
    // Native press path: an early hit inside the Great early bound is stored as
    // pending instead of being accepted now, and its stored rank collapses to
    // Great unless it was already Just. On-time, late and far-early Good hits
    // are taken immediately at their own rank.
    result.deferred_early = result.eligible && delta_us > 0.0 && delta_us <= windows.great_early_us;
    result.recorded_rank = (result.deferred_early && result.rank != NormalRank::Just)
                               ? NormalRank::Great
                               : result.rank;
    return result;
}

NormalEligibilityResult EvaluateNormalEligibility(double note_us, SongTime captured,
                                                  std::uint64_t expected_generation,
                                                  double eligibility_total_offset_us,
                                                  const NormalWindows& windows) noexcept {
    NormalEligibilityResult result{};

    if (!IsAcceptableNormalGeneration(expected_generation, captured.generation)) {
        result.status = NormalStatus::GenerationMismatch;
        return result;
    }

    double delta_us = 0.0;
    const NormalStatus delta_status =
        ComputeDelta(note_us, captured, eligibility_total_offset_us, delta_us);
    if (delta_status != NormalStatus::Ok) {
        result.status = delta_status;
        return result;
    }

    if (ValidateNormalWindows(windows) != NormalWindowStatus::Ok) {
        result.status = NormalStatus::InvalidWindows;
        return result;
    }

    result.status = NormalStatus::Ok;
    result.delta_us = delta_us;
    result.rank = ClassifyNormalDelta(delta_us, windows);
    result.eligible = IsScoringNormalRank(result.rank);
    return result;
}

NormalNoteEvaluation EvaluateNormalNote(double note_us, SongTime captured,
                                        std::uint64_t expected_generation,
                                        const NormalNoteOffsets& offsets,
                                        const NormalWindows& windows) noexcept {
    NormalNoteEvaluation result{};
    result.press = EvaluateNormalPress(note_us, captured, expected_generation,
                                       offsets.judgment_offset_us, windows);
    result.eligibility = EvaluateNormalEligibility(note_us, captured, expected_generation,
                                                   offsets.eligibility_total_offset_us, windows);
    return result;
}

NormalMissDeadline ComputeNormalMissDeadline(double note_us, double judgment_offset_us,
                                             const NormalWindows& windows) noexcept {
    NormalMissDeadline result{};
    if (!std::isfinite(note_us) || !std::isfinite(judgment_offset_us)) {
        result.status = NormalStatus::NonFiniteInput;
        return result;
    }
    const NormalWindowStatus window_status = ValidateNormalWindows(windows);
    if (window_status != NormalWindowStatus::Ok) {
        result.status = NormalStatus::InvalidWindows;
        return result;
    }
    const double deadline = (note_us - judgment_offset_us) + windows.good_late_us;
    if (!std::isfinite(deadline)) {
        result.status = NormalStatus::ArithmeticOverflow;
        return result;
    }
    result.status = NormalStatus::Ok;
    result.deadline_us = deadline;
    return result;
}

NormalMissDecision DecideNormalMiss(double note_us, SongTime current_song_time,
                                    SongTime processed_input_through,
                                    std::uint64_t expected_generation,
                                    double judgment_offset_us,
                                    const NormalWindows& windows) noexcept {
    NormalMissDecision result{};

    if (!IsAcceptableNormalGeneration(expected_generation, current_song_time.generation) ||
        !IsAcceptableNormalGeneration(expected_generation, processed_input_through.generation)) {
        result.status = NormalStatus::GenerationMismatch;
        return result;
    }

    if (!std::isfinite(note_us) || !std::isfinite(current_song_time.microseconds) ||
        !std::isfinite(processed_input_through.microseconds) ||
        !std::isfinite(judgment_offset_us)) {
        result.status = NormalStatus::NonFiniteInput;
        return result;
    }

    if (ValidateNormalWindows(windows) != NormalWindowStatus::Ok) {
        result.status = NormalStatus::InvalidWindows;
        return result;
    }

    // The watermark is the furthest captured song time already judged. If it
    // claims to be ahead of the song itself it cannot be trusted, and neither
    // can a miss derived from it.
    if (processed_input_through.microseconds > current_song_time.microseconds) {
        result.status = NormalStatus::StaleWatermark;
        return result;
    }

    const double deadline = (note_us - judgment_offset_us) + windows.good_late_us;
    if (!std::isfinite(deadline)) {
        result.status = NormalStatus::ArithmeticOverflow;
        return result;
    }

    result.status = NormalStatus::Ok;
    result.deadline_us = deadline;
    const double current_over = current_song_time.microseconds - deadline;
    const double input_over = processed_input_through.microseconds - deadline;
    // Finite operands can still overflow the differences (e.g. DBL_MAX against
    // -DBL_MAX). An infinite diagnostic would compare "> 0" and finalize a miss
    // from unrepresentable arithmetic, so reject instead.
    if (!std::isfinite(current_over) || !std::isfinite(input_over)) {
        result.status = NormalStatus::ArithmeticOverflow;
        result.finalize_miss = false;
        return result;
    }
    result.current_over_deadline_us = current_over;
    result.input_over_deadline_us = input_over;
    // Strictly past: the deadline itself is still the last Good instant, so a
    // press captured exactly there must not be pre-empted by a finalized miss.
    result.current_past_deadline = current_over > 0.0;
    result.input_past_deadline = input_over > 0.0;
    result.finalize_miss = result.current_past_deadline && result.input_past_deadline;
    return result;
}

LegacyNormalWindows ConvertLegacyNormalWindows(const LegacyNormalWindowUnits& bounds,
                                               double units_per_second) noexcept {
    LegacyNormalWindows result{};
    result.window_status = NormalWindowStatus::Ok;

    if (!std::isfinite(units_per_second)) {
        result.status = LegacyWindowStatus::NonFiniteRate;
        return result;
    }
    if (!(units_per_second > 0.0)) {
        result.status = LegacyWindowStatus::NonPositiveRate;
        return result;
    }

    const std::int64_t earliest[3] = {bounds.just_earliest_units, bounds.great_earliest_units,
                                      bounds.good_earliest_units};
    const std::int64_t latest[3] = {bounds.just_latest_units, bounds.great_latest_units,
                                    bounds.good_latest_units};
    for (const std::int64_t bound : earliest) {
        if (bound < 0) {
            result.status = LegacyWindowStatus::InvalidBoundSign;
            return result;
        }
    }
    for (const std::int64_t bound : latest) {
        if (bound > 0) {
            result.status = LegacyWindowStatus::InvalidBoundSign;
            return result;
        }
    }

    double early_us[3] = {0.0, 0.0, 0.0};
    double late_us[3] = {0.0, 0.0, 0.0};
    for (int i = 0; i < 3; ++i) {
        // Integer comparison first: 2^53 + 1 would round to 2^53 inside a
        // double and pass a double-precision check.
        if (earliest[i] > kMaxExactLegacyUnits || latest[i] < -kMaxExactLegacyUnits) {
            result.status = LegacyWindowStatus::UnrepresentableBound;
            return result;
        }
        // Negate in int64 only after the magnitude check, so -INT64_MIN cannot
        // overflow; the latest bound is <= 0, so this magnitude is nonnegative.
        if (!ConvertUnits(static_cast<double>(earliest[i]), units_per_second, early_us[i]) ||
            !ConvertUnits(static_cast<double>(-latest[i]), units_per_second, late_us[i])) {
            result.status = LegacyWindowStatus::UnrepresentableBound;
            return result;
        }
    }

    NormalWindows windows{};
    windows.just_early_us = early_us[0];
    windows.just_late_us = late_us[0];
    windows.great_early_us = early_us[1];
    windows.great_late_us = late_us[1];
    windows.good_early_us = early_us[2];
    windows.good_late_us = late_us[2];

    const NormalWindowStatus window_status = ValidateNormalWindows(windows);
    result.window_status = window_status;
    if (window_status != NormalWindowStatus::Ok) {
        result.status = LegacyWindowStatus::InvalidWindows;
        return result;
    }

    result.status = LegacyWindowStatus::Ok;
    result.windows = windows;
    return result;
}

} // namespace nosfix
