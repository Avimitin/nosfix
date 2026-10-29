#pragma once

#include "nosfix/song_clock.hpp"

#include <cstdint>

namespace nosfix {

// ===========================================================================
// Continuous-time NORMAL-note judgment kernel (portable, stateless)
// ===========================================================================
//
// Timing arithmetic of the native normal-note judgment routine (module VA
// 0x1802F6800, RVA 0x2F6800) and the offset channel of its eligibility helper
// (VA 0x1802F7900, RVA 0x2F7900). Given one already-captured press it answers:
//
//   * which native rank code that press gets, and whether the native
//     pending-early policy defers it;
//   * whether a miss may be finalized, given the current song time and the
//     furthest input that has actually been processed.
//
// Scope: NORMAL notes only. No note lifecycle, pending-hit resolution, scoring,
// combo, sound dispatch, auto/miss side effects or repeated-hit behaviour is
// modelled here; long (0x1802F4220), gliss (0x1801A60C0) and trill (0x1801A7AF0)
// use different routines and are out of scope. Nothing here reads a clock,
// hook, frame counter or refresh rate, and this is not evidence that live
// gameplay is replaced. Chart constants, ABI, offset signs and integration
// remain the parent's work.
//
// Invariants: every function is pure and noexcept; there is no mutable global
// state, no half-frame padding, no symmetry assumption and no timing derived
// from a frame count. Ranking depends only on the press's captured SongTime,
// never on the render tick that consumes it, so replaying identical captures at
// any refresh rate, with jitter or stalls, yields identical ranks and deltas. A
// coarser render cadence lowers observation resolution; it does not narrow the
// scoring windows.
//
// Units and sign: all times are double microseconds, matching
// SongTime::microseconds. Delta is
//
//     delta_us = (note_us - captured.microseconds) - judgment_offset_us
//
// evaluated in that order in double. A POSITIVE delta means the press was EARLY
// (the note is still in the future); NEGATIVE means LATE. The native routine
// computes the same note - song - offset quantity, so sign and ordering carry
// over unchanged.
//
// Window EARLY values are magnitudes on the positive side and LATE values are
// magnitudes on the negative side. They are never derived from one another.

// Native rank codes of VA 0x1802F6800, stored by the native path into note+120.
enum class NormalRank : std::uint8_t {
    TooEarly = 0, // Earlier than the outer Good window; never a hit.
    Missed = 1,   // Later than the outer Good window; past the late window.
    Good = 2,
    Great = 3,
    Just = 4,
};

[[nodiscard]] const char* NormalRankName(NormalRank rank) noexcept;

// True for the three scoring ranks (Good, Great, Just).
[[nodiscard]] constexpr bool IsScoringNormalRank(NormalRank rank) noexcept {
    const auto value = static_cast<std::uint8_t>(rank);
    return value >= static_cast<std::uint8_t>(NormalRank::Good) &&
           value <= static_cast<std::uint8_t>(NormalRank::Just);
}

// Inclusive, asymmetric window half-widths in microseconds:
//
//   Just  window covers [-just_late_us,  +just_early_us]
//   Great window covers [-great_late_us, +great_early_us]
//   Good  window covers [-good_late_us,  +good_early_us]
//
// Both ends of every window are INCLUSIVE, matching the native comparisons,
// which reject only strict outliers. The native routine tests Just first, then
// Great, then Good, so the tiers must be nested innermost first:
//
//   0 <= just_early_us  <= great_early_us <= good_early_us
//   0 <= just_late_us   <= great_late_us  <= good_late_us
//
// Equal bounds are allowed (a degenerate or absent tier). Symmetry is not
// required and not assumed; the caller supplies the six values verbatim.
struct NormalWindows final {
    double just_early_us = 0.0;
    double just_late_us = 0.0;
    double great_early_us = 0.0;
    double great_late_us = 0.0;
    double good_early_us = 0.0;
    double good_late_us = 0.0;

    friend bool operator==(const NormalWindows&, const NormalWindows&) = default;
};

enum class NormalWindowStatus : std::uint8_t {
    Ok = 0,
    NonFinite = 1,  // NaN or infinity in any of the six widths.
    Negative = 2,   // A width below zero; widths are magnitudes.
    NotNested = 3,  // Just is not inside Great, or Great is not inside Good.
};

[[nodiscard]] const char* NormalWindowStatusName(NormalWindowStatus status) noexcept;

// Validates shape and nesting. Does not clamp, round or reorder anything: a
// rejected table must be fixed by the caller, never repaired here.
[[nodiscard]] NormalWindowStatus ValidateNormalWindows(const NormalWindows& windows) noexcept;

// Classifies a delta against the windows, failing closed: a non-finite delta or
// a table for which ValidateNormalWindows(windows) != Ok returns TooEarly, the
// non-consuming, non-miss code, never a scoring rank or Missed. For a valid
// table the precedence is the native one (innermost Just, then Great, then
// Good), inclusive on both ends.
[[nodiscard]] NormalRank ClassifyNormalDelta(double delta_us, const NormalWindows& windows) noexcept;

// Failure codes. A default-constructed result is NotEvaluated, never Ok.
enum class NormalStatus : std::uint8_t {
    Ok = 0,
    NotEvaluated = 1,
    GenerationMismatch = 2, // expected_generation == 0, or a time came from another generation.
    NonFiniteInput = 3,     // NaN/inf note time, song time or offset.
    InvalidWindows = 4,     // ValidateNormalWindows(windows) != Ok.
    ArithmeticOverflow = 5, // Finite inputs whose delta, deadline or diagnostic is not representable.
    StaleWatermark = 6,     // Input watermark is later than the current song time.
};

[[nodiscard]] const char* NormalStatusName(NormalStatus status) noexcept;

// A playable generation is never zero (SongClock starts at 1), so zero means
// "no generation" and is rejected on both sides.
[[nodiscard]] constexpr bool IsAcceptableNormalGeneration(std::uint64_t expected,
                                                          std::uint64_t captured) noexcept {
    return expected != 0 && captured == expected;
}

// ---------------------------------------------------------------------------
// Press evaluation
// ---------------------------------------------------------------------------

struct NormalPressResult final {
    NormalStatus status = NormalStatus::NotEvaluated;

    // Valid only when status == Ok.
    NormalRank rank = NormalRank::TooEarly;          // rank at the captured time
    NormalRank recorded_rank = NormalRank::TooEarly; // rank the native path stores
    double delta_us = 0.0;                           // positive = early
    bool eligible = false;                           // rank is Good/Great/Just
    bool deferred_early = false;                     // native pending-early policy applies

    [[nodiscard]] bool ok() const noexcept { return status == NormalStatus::Ok; }

    // Scoring hit that the legacy path stores immediately, as opposed to
    // deferring it as a pending early hit.
    [[nodiscard]] bool scores_immediately() const noexcept {
        return ok() && eligible && !deferred_early;
    }
};

// Evaluates one press against the windows at its CAPTURED song time.
//
// Failure precedence: generation, then non-finite/overflowing delta, then
// windows.
//
// Legacy policy metadata (evidence: VA 0x1802F6800, press path after ranking):
// for 0 < delta_us <= great_early_us the native routine does not accept the hit
// yet. It sets a pending-early flag on the note and records `3 + (rank == Just)`
// (Just stays Just, every other rank is stored as Great). Those presses report
// deferred_early == true with recorded_rank; `rank` still reports the
// classification at the captured time. Every other in-window press (delta <= 0,
// i.e. on time or late, or delta > great_early_us, a Good press earlier than the
// Great window) is taken immediately at its own rank. TooEarly presses never
// hit; Missed presses are past the late window and belong to the caller's miss
// handling. deferred_early creates no note state, pending slot, score, sound or
// repeated-hit rule; resolving a pending hit remains the caller's state machine.
[[nodiscard]] NormalPressResult EvaluateNormalPress(double note_us, SongTime captured,
                                                    std::uint64_t expected_generation,
                                                    double judgment_offset_us,
                                                    const NormalWindows& windows) noexcept;

// ---------------------------------------------------------------------------
// Eligibility
// ---------------------------------------------------------------------------

struct NormalEligibilityResult final {
    NormalStatus status = NormalStatus::NotEvaluated;

    NormalRank rank = NormalRank::TooEarly; // valid only when status == Ok
    double delta_us = 0.0;
    bool eligible = false; // rank is Good/Great/Just under the eligibility total offset

    [[nodiscard]] bool ok() const noexcept { return status == NormalStatus::Ok; }
};

// Same evaluator and windows, with the caller's ELIGIBILITY TOTAL offset.
//
// The native normal eligibility helper (vtable slot +56, VA 0x1802F7900,
// RVA 0x2F7900) subtracts BOTH the judgment offset (global RVA 0x136690C) and an
// ADDITIONAL visual offset (global RVA 0x1366918). eligibility_total_offset_us
// is the complete, separately provided sum of those two channels -- never
// assumed to be the visual offset alone and never derived from
// judgment_offset_us here. The judgment and eligibility channels stay
// independent: callers evaluating both pass each value to its own evaluator.
//
// The native helper accepts the inclusive union of Just, Great and Good.
// Evidence: analyze/evidence/native_normal_eligibility_0x1802f7900.c.
// Note selection and lifetime are still the caller's responsibility.
[[nodiscard]] NormalEligibilityResult EvaluateNormalEligibility(
    double note_us, SongTime captured, std::uint64_t expected_generation,
    double eligibility_total_offset_us, const NormalWindows& windows) noexcept;

// Both offset channels kept independent in one call.
struct NormalNoteOffsets final {
    double judgment_offset_us = 0.0; // judgment channel: global RVA 0x136690C
    // Complete eligibility sum: judgment 0x136690C + additional visual 0x1366918.
    double eligibility_total_offset_us = 0.0;
};

struct NormalNoteEvaluation final {
    NormalPressResult press;             // judged with judgment_offset_us
    NormalEligibilityResult eligibility; // judged with eligibility_total_offset_us
};

[[nodiscard]] NormalNoteEvaluation EvaluateNormalNote(double note_us, SongTime captured,
                                                      std::uint64_t expected_generation,
                                                      const NormalNoteOffsets& offsets,
                                                      const NormalWindows& windows) noexcept;

// ---------------------------------------------------------------------------
// Miss deadline
// ---------------------------------------------------------------------------

// deadline_us = (note_us - judgment_offset_us) + good_late_us, evaluated in
// that order. The deadline is INCLUSIVE: a press captured at exactly the
// deadline is still the latest possible Good hit, so a miss may only be
// finalized strictly after it.
struct NormalMissDeadline final {
    NormalStatus status = NormalStatus::NotEvaluated;
    double deadline_us = 0.0; // valid only when status == Ok

    [[nodiscard]] bool ok() const noexcept { return status == NormalStatus::Ok; }
};

[[nodiscard]] NormalMissDeadline ComputeNormalMissDeadline(double note_us,
                                                           double judgment_offset_us,
                                                           const NormalWindows& windows) noexcept;

struct NormalMissDecision final {
    NormalStatus status = NormalStatus::NotEvaluated;

    // True only when the current song time AND the processed-input watermark
    // are both strictly past the deadline. Otherwise the note stays pending.
    bool finalize_miss = false;
    bool current_past_deadline = false;
    bool input_past_deadline = false;
    double deadline_us = 0.0;              // valid only when status == Ok
    double current_over_deadline_us = 0.0; // current - deadline, finite diagnostic
    double input_over_deadline_us = 0.0;   // watermark - deadline, finite diagnostic

    [[nodiscard]] bool ok() const noexcept { return status == NormalStatus::Ok; }
};

// Decides whether a miss may be finalized.
//
// `current_song_time` is the song position at the decision point.
// `processed_input_through` is the furthest CAPTURED song time whose input has
// actually been drained and judged. Both must carry expected_generation, which
// must be non-zero.
//
// A late render alone can never finalize a miss: while old input is still
// queued the watermark stays behind the deadline and the decision stays pending,
// so an in-window press captured before the deadline but consumed after it is
// still judged by its captured time via EvaluateNormalPress.
//
// Fail-closed rejections (all leave finalize_miss == false): zero or mismatched
// generation; non-finite song times or offset; invalid windows; non-finite
// deadline; a watermark later than the current song time; or either diagnostic
// difference (time - deadline) not finite even though the operands are finite.
// Precedence matches the press evaluator: generation, non-finite input, windows,
// then watermark and arithmetic.
[[nodiscard]] NormalMissDecision DecideNormalMiss(double note_us, SongTime current_song_time,
                                                  SongTime processed_input_through,
                                                  std::uint64_t expected_generation,
                                                  double judgment_offset_us,
                                                  const NormalWindows& windows) noexcept;

// ---------------------------------------------------------------------------
// Legacy integer window adapter
// ---------------------------------------------------------------------------

// The native tables (0x13668E8..0x13668FC) are signed integer song units, not
// microseconds, and the unit rate is a runtime value (the temporary 60 that the
// monitor-check bypass installs is a caller decision, not a constant of this
// kernel). This adapter is the only place that knows about a units-per-second
// rate, and the caller passes it.
//
// The bounds are the native comparisons' signed INCLUSIVE limits, so the
// earliest (maximum) bound of a tier is >= 0 and the latest (minimum) bound is
// <= 0. A table that stores late bounds as magnitudes must negate them before
// calling; the sign convention is checked, not guessed.
struct LegacyNormalWindowUnits final {
    std::int64_t just_earliest_units = 0; // >= 0
    std::int64_t just_latest_units = 0;   // <= 0
    std::int64_t great_earliest_units = 0;
    std::int64_t great_latest_units = 0;
    std::int64_t good_earliest_units = 0;
    std::int64_t good_latest_units = 0;
};

enum class LegacyWindowStatus : std::uint8_t {
    Ok = 0,
    NonFiniteRate = 1,        // NaN/inf units_per_second.
    NonPositiveRate = 2,      // Zero or negative units_per_second.
    InvalidBoundSign = 3,     // An earliest bound < 0 or a latest bound > 0.
    UnrepresentableBound = 4, // |bound| > 2^53, or the converted value overflows.
    InvalidWindows = 5,       // Converted table fails ValidateNormalWindows.
};

[[nodiscard]] const char* LegacyWindowStatusName(LegacyWindowStatus status) noexcept;

struct LegacyNormalWindows final {
    LegacyWindowStatus status = LegacyWindowStatus::InvalidWindows;
    NormalWindowStatus window_status = NormalWindowStatus::Ok; // set only when validation ran
    NormalWindows windows{};

    [[nodiscard]] bool ok() const noexcept { return status == LegacyWindowStatus::Ok; }
};

// Converts each bound with `units * 1'000'000.0 / units_per_second`, evaluated
// in that order in double. Integer bounds above 2^53 are rejected rather than
// silently rounded; a non-finite result is rejected. The converted table is
// validated for nesting before it is returned.
[[nodiscard]] LegacyNormalWindows ConvertLegacyNormalWindows(const LegacyNormalWindowUnits& bounds,
                                                            double units_per_second) noexcept;

} // namespace nosfix
