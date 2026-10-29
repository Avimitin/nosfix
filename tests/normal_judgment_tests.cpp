// Tests for include/nosfix/normal_judgment.hpp.
//
// These checks are ALWAYS compiled: no assert(), no <cassert>, no NDEBUG
// guards. A Release build runs exactly the same checks as a Debug build and
// still returns a non-zero exit code on failure. `cl /O2 /DNDEBUG` must run and
// pass, so the suite cannot be compiled out of the release configuration.

#include "nosfix/normal_judgment.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <limits>
#include <optional>
#include <vector>

namespace {

int g_checks = 0;
int g_failures = 0;

void CheckImpl(bool ok, const char* expr, const char* file, int line) {
    ++g_checks;
    if (!ok) {
        ++g_failures;
        std::printf("FAIL %s(%d): %s\n", file, line, expr);
    }
}

#define CHECK(expr) CheckImpl((expr), #expr, __FILE__, __LINE__)

constexpr double kNaN = std::numeric_limits<double>::quiet_NaN();
constexpr double kInf = std::numeric_limits<double>::infinity();
constexpr double kMax = std::numeric_limits<double>::max();

constexpr std::uint64_t kGeneration = 11;

using nosfix::NormalRank;
using nosfix::NormalStatus;
using nosfix::NormalWindowStatus;
using nosfix::SongTime;

bool Near(double a, double b, double relative = 1e-12) {
    const double scale = std::max({1.0, std::fabs(a), std::fabs(b)});
    return std::fabs(a - b) <= relative * scale;
}

// Deliberately asymmetric: 1.5 ms Just early / 2.0 ms Just late,
// 10 ms Great early / 11 ms Great late, 25 ms Good early / 26.5 ms Good late.
nosfix::NormalWindows BaseWindows() {
    nosfix::NormalWindows w;
    w.just_early_us = 1500.0;
    w.just_late_us = 2000.0;
    w.great_early_us = 10000.0;
    w.great_late_us = 11000.0;
    w.good_early_us = 25000.0;
    w.good_late_us = 26500.0;
    return w;
}

void SetWidth(nosfix::NormalWindows& w, int field, double value) {
    switch (field) {
    case 0: w.just_early_us = value; break;
    case 1: w.just_late_us = value; break;
    case 2: w.great_early_us = value; break;
    case 3: w.great_late_us = value; break;
    case 4: w.good_early_us = value; break;
    default: w.good_late_us = value; break;
    }
}

// delta_us == note - captured - offset, so passing the delta as the note time
// with captured == 0 and offset == 0 reproduces it exactly.
std::optional<NormalRank> RankAt(double delta_us, const nosfix::NormalWindows& w) {
    const auto result =
        nosfix::EvaluateNormalPress(delta_us, SongTime{0.0, kGeneration}, kGeneration, 0.0, w);
    if (!result.ok()) return std::nullopt;
    return result.rank;
}

// ---------------------------------------------------------------------------

void TestWindowValidation() {
    std::printf("[section] window validation\n");

    const auto base = BaseWindows();
    CHECK(nosfix::ValidateNormalWindows(base) == NormalWindowStatus::Ok);

    // Non-finite widths are rejected in every position.
    const double non_finite[3] = {kNaN, kInf, -kInf};
    for (const double bad : non_finite) {
        for (int field = 0; field < 6; ++field) {
            auto w = base;
            SetWidth(w, field, bad);
            CHECK(nosfix::ValidateNormalWindows(w) == NormalWindowStatus::NonFinite);
        }
    }

    // Negative widths are rejected, including sub-microsecond ones.
    const double negative[2] = {-1.0, -1e-9};
    for (const double bad : negative) {
        for (int field = 0; field < 6; ++field) {
            auto w = base;
            SetWidth(w, field, bad);
            CHECK(nosfix::ValidateNormalWindows(w) == NormalWindowStatus::Negative);
        }
    }

    // Negative zero is a zero width, not a negative one.
    {
        auto w = base;
        w.just_late_us = -0.0;
        CHECK(nosfix::ValidateNormalWindows(w) == NormalWindowStatus::Ok);
    }

    // Nesting is required independently on each side.
    {
        auto w = base;
        w.just_early_us = w.great_early_us + 0.5;
        CHECK(nosfix::ValidateNormalWindows(w) == NormalWindowStatus::NotNested);
    }
    {
        auto w = base;
        w.great_early_us = w.good_early_us + 0.5;
        CHECK(nosfix::ValidateNormalWindows(w) == NormalWindowStatus::NotNested);
    }
    {
        auto w = base;
        w.just_late_us = w.great_late_us + 0.5;
        CHECK(nosfix::ValidateNormalWindows(w) == NormalWindowStatus::NotNested);
    }
    {
        auto w = base;
        w.great_late_us = w.good_late_us + 0.5;
        CHECK(nosfix::ValidateNormalWindows(w) == NormalWindowStatus::NotNested);
    }

    // Equal bounds are allowed: a degenerate tier is config, not an error.
    {
        auto w = base;
        w.just_early_us = w.great_early_us;
        w.just_late_us = w.great_late_us;
        CHECK(nosfix::ValidateNormalWindows(w) == NormalWindowStatus::Ok);
        CHECK(RankAt(w.great_early_us, w) == NormalRank::Just);
        CHECK(RankAt(w.great_early_us + 0.001, w) == NormalRank::Good);
    }

    // All-zero windows are valid and still resolve the exact-zero press.
    {
        const nosfix::NormalWindows zero{};
        CHECK(nosfix::ValidateNormalWindows(zero) == NormalWindowStatus::Ok);
        CHECK(RankAt(0.0, zero) == NormalRank::Just);
        CHECK(RankAt(1e-12, zero) == NormalRank::TooEarly);
        CHECK(RankAt(-1e-12, zero) == NormalRank::Missed);
    }

    // Validation never repairs or reorders the caller's table.
    {
        auto w = base;
        w.just_early_us = 20000.0;
        const auto before = w;
        (void)nosfix::ValidateNormalWindows(w);
        CHECK(w == before);
        CHECK(w.just_early_us == 20000.0);
    }

    CHECK(std::strcmp(nosfix::NormalWindowStatusName(NormalWindowStatus::NotNested),
                      "NotNested") == 0);
    CHECK(std::strcmp(nosfix::NormalStatusName(NormalStatus::StaleWatermark),
                      "StaleWatermark") == 0);
    CHECK(std::strcmp(nosfix::LegacyWindowStatusName(
                          nosfix::LegacyWindowStatus::UnrepresentableBound),
                      "UnrepresentableBound") == 0);
    CHECK(std::strcmp(nosfix::NormalRankName(NormalRank::Just), "Just") == 0);
}

// ---------------------------------------------------------------------------

void TestRankClassification() {
    std::printf("[section] rank classification\n");
    const auto w = BaseWindows();

    // Sub-microsecond distinctions around zero.
    CHECK(RankAt(0.0, w) == NormalRank::Just);
    CHECK(RankAt(0.0004, w) == NormalRank::Just); // 0.4 ns early
    CHECK(RankAt(-0.0004, w) == NormalRank::Just);
    CHECK(RankAt(0.4, w) == NormalRank::Just); // 400 ns early
    CHECK(RankAt(-0.4, w) == NormalRank::Just);

    // Exact Just bounds are inclusive; one part in 1e9 outside is not.
    CHECK(RankAt(1500.0, w) == NormalRank::Just);
    CHECK(RankAt(1500.000001, w) == NormalRank::Great);
    CHECK(RankAt(-2000.0, w) == NormalRank::Just);
    CHECK(RankAt(-2000.000001, w) == NormalRank::Great);

    // Great bounds.
    CHECK(RankAt(10000.0, w) == NormalRank::Great);
    CHECK(RankAt(10000.000001, w) == NormalRank::Good);
    CHECK(RankAt(-11000.0, w) == NormalRank::Great);
    CHECK(RankAt(-11000.000001, w) == NormalRank::Good);

    // Good bounds: outside them the rank is too-early or missed.
    CHECK(RankAt(25000.0, w) == NormalRank::Good);
    CHECK(RankAt(25000.000001, w) == NormalRank::TooEarly);
    CHECK(RankAt(-26500.0, w) == NormalRank::Good);
    CHECK(RankAt(-26500.000001, w) == NormalRank::Missed);

    // Asymmetry is preserved: equal magnitudes rank differently per side.
    CHECK(RankAt(1600.0, w) == NormalRank::Great);
    CHECK(RankAt(-1600.0, w) == NormalRank::Just);
    CHECK(RankAt(25500.0, w) == NormalRank::TooEarly);
    CHECK(RankAt(-25500.0, w) == NormalRank::Good);
    CHECK(RankAt(26500.0, w) == NormalRank::TooEarly); // symmetric-looking would be Good
    CHECK(RankAt(-2000.0, w) == NormalRank::Just);

    // Resolution far below one microsecond survives in narrow windows.
    nosfix::NormalWindows tiny{};
    tiny.just_early_us = 0.5;
    tiny.just_late_us = 0.5;
    tiny.great_early_us = 1.0;
    tiny.great_late_us = 1.0;
    tiny.good_early_us = 2.0;
    tiny.good_late_us = 2.0;
    CHECK(RankAt(0.5, tiny) == NormalRank::Just);
    CHECK(RankAt(0.5000001, tiny) == NormalRank::Great);
    CHECK(RankAt(-0.5, tiny) == NormalRank::Just);
    CHECK(RankAt(-0.5000001, tiny) == NormalRank::Great);
    CHECK(RankAt(1.0, tiny) == NormalRank::Great);
    CHECK(RankAt(1.0000001, tiny) == NormalRank::Good);
    CHECK(RankAt(-1.0000001, tiny) == NormalRank::Good);
    CHECK(RankAt(2.0, tiny) == NormalRank::Good);
    CHECK(RankAt(2.0000001, tiny) == NormalRank::TooEarly);

    // The same boundaries hold when the delta comes from realistic times
    // rather than being passed in directly.
    const double note = 1'000'000.0;
    const auto press = [&](double input_us) {
        return nosfix::EvaluateNormalPress(note, SongTime{input_us, kGeneration}, kGeneration,
                                           0.0, w);
    };
    CHECK(press(note).delta_us == 0.0);
    CHECK(press(note - 1500.0).delta_us == 1500.0);
    CHECK(press(note - 1500.0).rank == NormalRank::Just);
    CHECK(press(note - 1500.001).rank == NormalRank::Great);
    CHECK(press(note + 2000.0).rank == NormalRank::Just);
    CHECK(press(note + 2000.001).rank == NormalRank::Great);
    CHECK(press(note + 26500.0).rank == NormalRank::Good);
    CHECK(press(note + 26500.001).rank == NormalRank::Missed);

    // Rank names match the native codes.
    CHECK(static_cast<int>(NormalRank::TooEarly) == 0);
    CHECK(static_cast<int>(NormalRank::Missed) == 1);
    CHECK(static_cast<int>(NormalRank::Good) == 2);
    CHECK(static_cast<int>(NormalRank::Great) == 3);
    CHECK(static_cast<int>(NormalRank::Just) == 4);
    CHECK(!nosfix::IsScoringNormalRank(NormalRank::TooEarly));
    CHECK(!nosfix::IsScoringNormalRank(NormalRank::Missed));
    CHECK(nosfix::IsScoringNormalRank(NormalRank::Good));
    CHECK(nosfix::IsScoringNormalRank(NormalRank::Great));
    CHECK(nosfix::IsScoringNormalRank(NormalRank::Just));
}

// ---------------------------------------------------------------------------

void TestClassifierFailClosed() {
    std::printf("[section] public classifier fail-closed tables\n");
    const auto w = BaseWindows();
    CHECK(nosfix::ClassifyNormalDelta(0.0, w) == NormalRank::Just);
    CHECK(nosfix::ClassifyNormalDelta(-1500.0, w) == NormalRank::Just);
    CHECK(nosfix::ClassifyNormalDelta(5000.0, w) == NormalRank::Great);
    CHECK(nosfix::ClassifyNormalDelta(-30000.0, w) == NormalRank::Missed);

    // Non-finite deltas never produce a rank or a miss.
    CHECK(nosfix::ClassifyNormalDelta(kNaN, w) == NormalRank::TooEarly);
    CHECK(nosfix::ClassifyNormalDelta(kInf, w) == NormalRank::TooEarly);
    CHECK(nosfix::ClassifyNormalDelta(-kInf, w) == NormalRank::TooEarly);

    auto negative = w;
    negative.just_early_us = -1.0;
    CHECK(nosfix::ValidateNormalWindows(negative) == NormalWindowStatus::Negative);

    auto non_nested = w;
    non_nested.just_early_us = 20000.0; // wider than Great.early
    CHECK(nosfix::ValidateNormalWindows(non_nested) == NormalWindowStatus::NotNested);

    auto non_nested_late = w;
    non_nested_late.just_late_us = 20000.0; // wider than Great.late
    CHECK(nosfix::ValidateNormalWindows(non_nested_late) == NormalWindowStatus::NotNested);

    auto non_finite = w;
    non_finite.good_late_us = kInf;
    CHECK(nosfix::ValidateNormalWindows(non_finite) == NormalWindowStatus::NonFinite);

    const nosfix::NormalWindows invalid[] = {negative, non_nested, non_nested_late, non_finite};
    const double deltas[] = {kNaN, kInf, -kInf, 30000.0, 5000.0, 0.0, -1500.0, -30000.0};
    for (const auto& table : invalid) {
        for (const double delta : deltas) {
            // TooEarly is the only fail-closed code: not a scoring rank and not
            // Missed, even for deltas a trusted table would rank.
            CHECK(nosfix::ClassifyNormalDelta(delta, table) == NormalRank::TooEarly);
        }
    }
}

// ---------------------------------------------------------------------------

void TestEarlyDeferPolicy() {
    std::printf("[section] early-defer policy\n");
    const auto w = BaseWindows();
    const auto press = [&](double delta_us) {
        return nosfix::EvaluateNormalPress(delta_us, SongTime{0.0, kGeneration}, kGeneration,
                                           0.0, w);
    };

    // 0 < delta <= Great.early: recorded as pending, not accepted now.
    // A press 1 us early is already inside the Just window, so its captured
    // rank is Just and stays Just.
    auto r = press(1.0);
    CHECK(r.ok());
    CHECK(r.rank == NormalRank::Just);
    CHECK(r.recorded_rank == NormalRank::Just);
    CHECK(r.eligible);
    CHECK(r.deferred_early);
    CHECK(!r.scores_immediately());

    // A Great-ranked press in the defer range collapses to the stored Great.
    r = press(5000.0);
    CHECK(r.rank == NormalRank::Great);
    CHECK(r.recorded_rank == NormalRank::Great);
    CHECK(r.deferred_early);

    // A Just press keeps its rank while pending.
    r = press(1499.5);
    CHECK(r.rank == NormalRank::Just);
    CHECK(r.recorded_rank == NormalRank::Just);
    CHECK(r.deferred_early);
    CHECK(!r.scores_immediately());

    // Exact Just early bound is inside Just and inside the defer range.
    r = press(1500.0);
    CHECK(r.rank == NormalRank::Just);
    CHECK(r.recorded_rank == NormalRank::Just);
    CHECK(r.deferred_early);

    // Exact Great early bound is still deferred, ranked Great.
    r = press(10000.0);
    CHECK(r.rank == NormalRank::Great);
    CHECK(r.recorded_rank == NormalRank::Great);
    CHECK(r.deferred_early);

    // A Good press earlier than Great.early is accepted immediately.
    r = press(10000.5);
    CHECK(r.rank == NormalRank::Good);
    CHECK(r.recorded_rank == NormalRank::Good);
    CHECK(r.eligible);
    CHECK(!r.deferred_early);
    CHECK(r.scores_immediately());

    // On time and late presses are immediate at their captured rank.
    r = press(0.0);
    CHECK(r.rank == NormalRank::Just && !r.deferred_early && r.scores_immediately());
    r = press(-0.5);
    CHECK(r.rank == NormalRank::Just && !r.deferred_early && r.scores_immediately());
    r = press(-2000.0);
    CHECK(r.rank == NormalRank::Just && !r.deferred_early && r.scores_immediately());
    r = press(-11000.0);
    CHECK(r.rank == NormalRank::Great && !r.deferred_early && r.scores_immediately());
    r = press(-26500.0);
    CHECK(r.rank == NormalRank::Good && !r.deferred_early && r.scores_immediately());

    // Too early and missed presses are not hits at all.
    r = press(10001.0);
    CHECK(r.rank == NormalRank::Good && r.eligible && !r.deferred_early);
    r = press(25000.0);
    CHECK(r.rank == NormalRank::Good && r.eligible && !r.deferred_early);
    r = press(25000.5);
    CHECK(r.rank == NormalRank::TooEarly && !r.eligible && !r.deferred_early);
    CHECK(!r.scores_immediately());
    r = press(-26500.5);
    CHECK(r.rank == NormalRank::Missed && !r.eligible && !r.deferred_early);
    CHECK(!r.scores_immediately());

    // The policy is metadata: a deferred press is still "eligible", and the
    // only lifecycle statement made is that the caller stores it pending.
    r = press(500.0);
    CHECK(r.rank == NormalRank::Just);
    CHECK(r.eligible && r.deferred_early);
    CHECK(r.recorded_rank == NormalRank::Just);
    CHECK(!r.scores_immediately());
}

// ---------------------------------------------------------------------------

void TestOffsets() {
    std::printf("[section] judgment and eligibility total offsets\n");
    const auto w = BaseWindows();
    const double note = 2'000'000.0;
    const SongTime captured{2'000'000.0, kGeneration};

    auto r = nosfix::EvaluateNormalPress(note, captured, kGeneration, 0.0, w);
    CHECK(r.delta_us == 0.0 && r.rank == NormalRank::Just);

    // The judgment offset is subtracted, so a positive offset makes the press
    // later.
    r = nosfix::EvaluateNormalPress(note, captured, kGeneration, 5000.0, w);
    CHECK(r.delta_us == -5000.0);
    CHECK(r.rank == NormalRank::Great);
    CHECK(!r.deferred_early);

    r = nosfix::EvaluateNormalPress(note, captured, kGeneration, -20000.0, w);
    CHECK(r.delta_us == 20000.0);
    CHECK(r.rank == NormalRank::Good);
    CHECK(r.eligible && !r.deferred_early);

    // Unknown offset sign is not guessed: the caller's sign is used verbatim.
    r = nosfix::EvaluateNormalPress(note, captured, kGeneration, 12000.0, w);
    CHECK(r.delta_us == -12000.0 && r.rank == NormalRank::Good);

    // The eligibility channel takes the COMPLETE sum of the judgment offset
    // (0x136690C) and the additional visual offset (0x1366918). Both components
    // are nonzero and each partial offset ranks differently from the total, so
    // subtracting only one component is detectable.
    const double judgment_offset = 20'000.0; // judgment channel alone -> -20 ms -> Good
    const double visual_offset = 10'000.0;   // visual channel alone   -> -10 ms -> Great
    const double total_offset = judgment_offset + visual_offset; // -30 ms -> Missed

    const auto total = nosfix::EvaluateNormalEligibility(note, captured, kGeneration,
                                                         total_offset, w);
    CHECK(total.ok());
    CHECK(total.delta_us == -30'000.0);
    CHECK(total.delta_us == -(judgment_offset + visual_offset));
    CHECK(total.rank == NormalRank::Missed); // past good_late 26.5 ms
    CHECK(!total.eligible);

    const auto visual_only = nosfix::EvaluateNormalEligibility(note, captured, kGeneration,
                                                               visual_offset, w);
    CHECK(visual_only.delta_us == -10'000.0 && visual_only.rank == NormalRank::Great);
    const auto judgment_only = nosfix::EvaluateNormalEligibility(note, captured, kGeneration,
                                                                 judgment_offset, w);
    CHECK(judgment_only.delta_us == -20'000.0 && judgment_only.rank == NormalRank::Good);
    CHECK(total.rank != visual_only.rank && total.rank != judgment_only.rank);

    // Both channels through the combined call: the press uses judgment only,
    // eligibility uses the total; neither leaks into the other.
    nosfix::NormalNoteOffsets offsets{};
    offsets.judgment_offset_us = judgment_offset;
    offsets.eligibility_total_offset_us = total_offset;
    auto both = nosfix::EvaluateNormalNote(note, captured, kGeneration, offsets, w);
    CHECK(both.press.ok() && both.press.rank == NormalRank::Good);
    CHECK(both.press.delta_us == -judgment_offset); // judgment stays independent
    CHECK(both.eligibility.ok() && both.eligibility.rank == NormalRank::Missed);
    CHECK(both.eligibility.delta_us == -(judgment_offset + visual_offset));
    CHECK(both.press.delta_us != both.eligibility.delta_us);

    // A total below the judgment offset must not be clamped or reordered.
    offsets.judgment_offset_us = 30'000.0;  // -30 ms -> Missed
    offsets.eligibility_total_offset_us = 0.0; //    0 ms -> Just
    const auto split = nosfix::EvaluateNormalNote(note, captured, kGeneration, offsets, w);
    CHECK(split.press.rank == NormalRank::Missed);
    CHECK(split.eligibility.rank == NormalRank::Just);
    CHECK(split.press.delta_us != split.eligibility.delta_us);

    // Equal values make the two channels agree exactly.
    offsets.judgment_offset_us = 12'000.0;
    offsets.eligibility_total_offset_us = 12'000.0;
    const auto same = nosfix::EvaluateNormalNote(note, captured, kGeneration, offsets, w);
    CHECK(same.press.rank == same.eligibility.rank);
    CHECK(same.press.delta_us == same.eligibility.delta_us);
    CHECK(same.press.eligible == same.eligibility.eligible);
}

// ---------------------------------------------------------------------------

void TestGeneration() {
    std::printf("[section] playback generation\n");
    const auto w = BaseWindows();

    auto r = nosfix::EvaluateNormalPress(0.0, SongTime{0.0, 0}, 0, 0.0, w);
    CHECK(r.status == NormalStatus::GenerationMismatch);
    CHECK(!r.ok() && !r.eligible && !r.deferred_early);
    CHECK(r.delta_us == 0.0);

    r = nosfix::EvaluateNormalPress(0.0, SongTime{0.0, 0}, 5, 0.0, w);
    CHECK(r.status == NormalStatus::GenerationMismatch);

    r = nosfix::EvaluateNormalPress(0.0, SongTime{0.0, 4}, 5, 0.0, w);
    CHECK(r.status == NormalStatus::GenerationMismatch);

    r = nosfix::EvaluateNormalPress(0.0, SongTime{0.0, 5}, 5, 0.0, w);
    CHECK(r.status == NormalStatus::Ok);
    CHECK(r.rank == NormalRank::Just);

    // Large generations are ordinary values, not sentinels.
    r = nosfix::EvaluateNormalPress(0.0, SongTime{0.0, UINT64_MAX}, UINT64_MAX, 0.0, w);
    CHECK(r.status == NormalStatus::Ok);

    CHECK(!nosfix::IsAcceptableNormalGeneration(0, 0));
    CHECK(!nosfix::IsAcceptableNormalGeneration(0, 3));
    CHECK(!nosfix::IsAcceptableNormalGeneration(3, 0));
    CHECK(!nosfix::IsAcceptableNormalGeneration(3, 4));
    CHECK(nosfix::IsAcceptableNormalGeneration(3, 3));

    // Eligibility and the miss decision apply the same rule.
    CHECK(nosfix::EvaluateNormalEligibility(0.0, SongTime{0.0, 4}, 5, 0.0, w).status ==
          NormalStatus::GenerationMismatch);
    CHECK(nosfix::DecideNormalMiss(0.0, SongTime{0.0, 4}, SongTime{0.0, 5}, 5, 0.0, w).status ==
          NormalStatus::GenerationMismatch);
    CHECK(nosfix::DecideNormalMiss(0.0, SongTime{0.0, 5}, SongTime{0.0, 4}, 5, 0.0, w).status ==
          NormalStatus::GenerationMismatch);
    CHECK(nosfix::DecideNormalMiss(0.0, SongTime{0.0, 5}, SongTime{0.0, 5}, 0, 0.0, w).status ==
          NormalStatus::GenerationMismatch);
}

// ---------------------------------------------------------------------------

void TestInvalidArithmetic() {
    std::printf("[section] non-finite and overflowing inputs\n");
    const auto w = BaseWindows();
    const double non_finite[3] = {kNaN, kInf, -kInf};

    for (const double bad : non_finite) {
        CHECK(nosfix::EvaluateNormalPress(bad, SongTime{0.0, kGeneration}, kGeneration, 0.0, w)
                  .status == NormalStatus::NonFiniteInput);
        CHECK(nosfix::EvaluateNormalPress(0.0, SongTime{bad, kGeneration}, kGeneration, 0.0, w)
                  .status == NormalStatus::NonFiniteInput);
        CHECK(nosfix::EvaluateNormalPress(0.0, SongTime{0.0, kGeneration}, kGeneration, bad, w)
                  .status == NormalStatus::NonFiniteInput);
        CHECK(nosfix::EvaluateNormalEligibility(0.0, SongTime{0.0, kGeneration}, kGeneration,
                                                bad, w)
                  .status == NormalStatus::NonFiniteInput);
        CHECK(nosfix::ComputeNormalMissDeadline(bad, 0.0, w).status ==
              NormalStatus::NonFiniteInput);
        CHECK(nosfix::ComputeNormalMissDeadline(0.0, bad, w).status ==
              NormalStatus::NonFiniteInput);
        CHECK(nosfix::DecideNormalMiss(0.0, SongTime{bad, kGeneration},
                                       SongTime{0.0, kGeneration}, kGeneration, 0.0, w)
                  .status == NormalStatus::NonFiniteInput);
        CHECK(nosfix::DecideNormalMiss(0.0, SongTime{0.0, kGeneration},
                                       SongTime{bad, kGeneration}, kGeneration, 0.0, w)
                  .status == NormalStatus::NonFiniteInput);
    }

    // Finite operands whose combination is not representable.
    CHECK(nosfix::EvaluateNormalPress(kMax, SongTime{-kMax, kGeneration}, kGeneration, 0.0, w)
              .status == NormalStatus::ArithmeticOverflow);
    CHECK(nosfix::EvaluateNormalPress(kMax, SongTime{0.0, kGeneration}, kGeneration, -kMax, w)
              .status == NormalStatus::ArithmeticOverflow);
    CHECK(nosfix::EvaluateNormalPress(-kMax, SongTime{0.0, kGeneration}, kGeneration, kMax, w)
              .status == NormalStatus::ArithmeticOverflow);
    CHECK(nosfix::EvaluateNormalEligibility(kMax, SongTime{-kMax, kGeneration}, kGeneration, 0.0,
                                            w)
              .status == NormalStatus::ArithmeticOverflow);
    CHECK(nosfix::ComputeNormalMissDeadline(kMax, -kMax, w).status ==
          NormalStatus::ArithmeticOverflow);
    CHECK(nosfix::DecideNormalMiss(kMax, SongTime{0.0, kGeneration}, SongTime{0.0, kGeneration},
                                   kGeneration, -kMax, w)
              .status == NormalStatus::ArithmeticOverflow);

    // Finite DBL_MAX operands can leave a finite deadline while the diagnostic
    // differences overflow. That must not finalize a miss from infinities.
    const nosfix::NormalWindows zero_windows{};
    CHECK(nosfix::ComputeNormalMissDeadline(-kMax, 0.0, zero_windows).ok());
    const auto huge_miss =
        nosfix::DecideNormalMiss(-kMax, SongTime{kMax, kGeneration},
                                 SongTime{kMax, kGeneration}, kGeneration, 0.0, zero_windows);
    CHECK(huge_miss.status == NormalStatus::ArithmeticOverflow);
    CHECK(!huge_miss.ok());
    CHECK(!huge_miss.finalize_miss);
    CHECK(!huge_miss.current_past_deadline && !huge_miss.input_past_deadline);
    CHECK(huge_miss.current_over_deadline_us == 0.0);
    CHECK(huge_miss.input_over_deadline_us == 0.0);
    CHECK(std::isfinite(huge_miss.current_over_deadline_us));
    CHECK(std::isfinite(huge_miss.input_over_deadline_us));

    // One overflowing diagnostic is enough to reject.
    const auto one_over =
        nosfix::DecideNormalMiss(-kMax, SongTime{kMax, kGeneration},
                                 SongTime{0.0, kGeneration}, kGeneration, 0.0, zero_windows);
    CHECK(one_over.status == NormalStatus::ArithmeticOverflow);
    CHECK(!one_over.finalize_miss);

    // The other direction: a finite current difference but an older watermark
    // whose difference overflows.
    const auto input_overflow =
        nosfix::DecideNormalMiss(kMax, SongTime{kMax, kGeneration},
                                 SongTime{-kMax, kGeneration}, kGeneration, 0.0, zero_windows);
    CHECK(input_overflow.status == NormalStatus::ArithmeticOverflow);
    CHECK(!input_overflow.finalize_miss);

    // An overflowing press reports a default delta, never a NaN that could be
    // compared downstream.
    const auto overflowed =
        nosfix::EvaluateNormalPress(kMax, SongTime{-kMax, kGeneration}, kGeneration, 0.0, w);
    CHECK(overflowed.delta_us == 0.0);
    CHECK(std::isfinite(overflowed.delta_us));

    // Invalid windows fail every entry point.
    auto bad_windows = w;
    bad_windows.just_late_us = 30000.0; // not nested in Great
    CHECK(nosfix::EvaluateNormalPress(0.0, SongTime{0.0, kGeneration}, kGeneration, 0.0,
                                      bad_windows)
              .status == NormalStatus::InvalidWindows);
    CHECK(nosfix::EvaluateNormalEligibility(0.0, SongTime{0.0, kGeneration}, kGeneration, 0.0,
                                            bad_windows)
              .status == NormalStatus::InvalidWindows);
    CHECK(nosfix::ComputeNormalMissDeadline(0.0, 0.0, bad_windows).status ==
          NormalStatus::InvalidWindows);
    CHECK(nosfix::DecideNormalMiss(0.0, SongTime{0.0, kGeneration}, SongTime{0.0, kGeneration},
                                   kGeneration, 0.0, bad_windows)
              .status == NormalStatus::InvalidWindows);

    // Documented failure precedence: generation, then arithmetic, then windows.
    CHECK(nosfix::EvaluateNormalPress(kNaN, SongTime{kNaN, 0}, kGeneration, kNaN, bad_windows)
              .status == NormalStatus::GenerationMismatch);
    CHECK(nosfix::EvaluateNormalPress(kNaN, SongTime{kNaN, kGeneration}, kGeneration, kNaN,
                                      bad_windows)
              .status == NormalStatus::NonFiniteInput);
    CHECK(nosfix::EvaluateNormalPress(0.0, SongTime{0.0, kGeneration}, kGeneration, 0.0,
                                      bad_windows)
              .status == NormalStatus::InvalidWindows);

    // Default-constructed results are not accidentally "Ok".
    const nosfix::NormalPressResult fresh;
    CHECK(fresh.status == NormalStatus::NotEvaluated);
    CHECK(!fresh.ok() && !fresh.eligible && !fresh.deferred_early);
    CHECK(!fresh.scores_immediately());
    CHECK(!nosfix::NormalMissDecision{}.ok());
    CHECK(!nosfix::NormalMissDeadline{}.ok());
    CHECK(!nosfix::LegacyNormalWindows{}.ok());
}

// ---------------------------------------------------------------------------

void TestMissDeadline() {
    std::printf("[section] miss deadline and watermark\n");
    const auto w = BaseWindows();
    const double note = 60'000'000.0;
    const double offset = 2500.0;

    auto computed = nosfix::ComputeNormalMissDeadline(note, offset, w);
    CHECK(computed.ok());
    CHECK(computed.deadline_us == note - offset + w.good_late_us);
    CHECK(computed.deadline_us == 60'024'000.0);

    // An early chart time with a large offset still yields a finite deadline.
    computed = nosfix::ComputeNormalMissDeadline(1000.0, 5000.0, w);
    CHECK(computed.ok() && computed.deadline_us == 22'500.0);

    const double deadline = 60'024'000.0;
    const auto decide = [&](double current, double watermark) {
        return nosfix::DecideNormalMiss(note, SongTime{current, kGeneration},
                                        SongTime{watermark, kGeneration}, kGeneration, offset, w);
    };

    // The deadline is inclusive: neither side may finalize at exactly there.
    auto r = decide(deadline, deadline);
    CHECK(r.ok());
    CHECK(!r.current_past_deadline && !r.input_past_deadline && !r.finalize_miss);
    CHECK(r.deadline_us == deadline);
    CHECK(r.current_over_deadline_us == 0.0 && r.input_over_deadline_us == 0.0);

    // A late render alone must not finalize the miss while the watermark is
    // still at the deadline.
    r = decide(deadline + 0.5, deadline);
    CHECK(r.ok());
    CHECK(r.current_past_deadline);
    CHECK(!r.input_past_deadline);
    CHECK(!r.finalize_miss);
    CHECK(r.current_over_deadline_us == 0.5);

    // Both strictly past is the only accepting case.
    r = decide(deadline + 0.5, deadline + 0.25);
    CHECK(r.ok() && r.finalize_miss);

    // Very late rendering with an older watermark stays pending.
    r = decide(deadline + 5'000'000.0, deadline - 1.0);
    CHECK(r.ok() && r.current_past_deadline && !r.input_past_deadline && !r.finalize_miss);

    // A watermark strictly before the deadline, current strictly after.
    r = decide(deadline + 0.001, deadline - 0.001);
    CHECK(r.ok() && !r.finalize_miss);

    // Both before the deadline.
    r = decide(deadline - 0.001, deadline - 0.001);
    CHECK(r.ok());
    CHECK(!r.current_past_deadline && !r.input_past_deadline && !r.finalize_miss);

    // A watermark ahead of the current song time is not believed.
    r = decide(deadline + 100.0, deadline + 100.001);
    CHECK(r.status == NormalStatus::StaleWatermark);
    CHECK(!r.ok() && !r.finalize_miss);
    r = decide(0.0, 1.0);
    CHECK(r.status == NormalStatus::StaleWatermark);

    // Equal times are not stale.
    r = decide(1.0, 1.0);
    CHECK(r.status == NormalStatus::Ok);
}

// ---------------------------------------------------------------------------

void TestDelayedConsumption() {
    std::printf("[section] delayed consumption past the miss deadline\n");
    const auto w = BaseWindows();
    const double note = 60'000'000.0;
    const double deadline = note + w.good_late_us; // offset 0: 60'026'500
    CHECK(deadline == 60'026'500.0);
    const double render_time = deadline + 900'000.0; // a 900 ms stall

    const SongTime early_press{note - 400.0, kGeneration};  // delta +400 us
    const SongTime late_press{note + 1200.0, kGeneration};  // delta -1200 us
    const SongTime on_deadline{deadline, kGeneration};      // delta -26500 us

    // Both in-window captures still rank Just when judged by their captured
    // time - the render is 0.9 s late and changes nothing.
    auto early = nosfix::EvaluateNormalPress(note, early_press, kGeneration, 0.0, w);
    CHECK(early.ok());
    CHECK(early.delta_us == 400.0);
    CHECK(early.rank == NormalRank::Just);
    CHECK(early.deferred_early && early.recorded_rank == NormalRank::Just);
    CHECK(!early.scores_immediately());

    auto late = nosfix::EvaluateNormalPress(note, late_press, kGeneration, 0.0, w);
    CHECK(late.ok());
    CHECK(late.delta_us == -1200.0);
    CHECK(late.rank == NormalRank::Just);
    CHECK(!late.deferred_early && late.scores_immediately());

    // Judging at the render time instead would be wrong for both.
    CHECK(nosfix::EvaluateNormalPress(note, SongTime{render_time, kGeneration}, kGeneration, 0.0, w)
              .rank == NormalRank::Missed);

    // While that press is the newest processed input the miss stays pending,
    // even though the current song time is far past the deadline.
    auto miss = nosfix::DecideNormalMiss(note, SongTime{render_time, kGeneration}, late_press,
                                         kGeneration, 0.0, w);
    CHECK(miss.ok());
    CHECK(miss.current_past_deadline);
    CHECK(!miss.input_past_deadline);
    CHECK(!miss.finalize_miss);

    miss = nosfix::DecideNormalMiss(note, SongTime{render_time, kGeneration}, early_press,
                                    kGeneration, 0.0, w);
    CHECK(!miss.finalize_miss);

    // A press captured exactly at the inclusive deadline is the last Good hit;
    // it still scores and cannot be pre-empted.
    const auto last_good = nosfix::EvaluateNormalPress(note, on_deadline, kGeneration, 0.0, w);
    CHECK(last_good.ok());
    CHECK(last_good.rank == NormalRank::Good);
    CHECK(last_good.eligible);
    CHECK(last_good.delta_us == -26500.0);
    miss = nosfix::DecideNormalMiss(note, on_deadline, on_deadline, kGeneration, 0.0, w);
    CHECK(!miss.finalize_miss);
    // Consumed 0.9 s late, it is still the same Good hit.
    CHECK(nosfix::EvaluateNormalPress(note, on_deadline, kGeneration, 0.0, w).rank ==
          NormalRank::Good);
    // Half a microsecond later there is no hit left.
    CHECK(nosfix::EvaluateNormalPress(note, SongTime{deadline + 0.5, kGeneration}, kGeneration,
                                      0.0, w)
              .rank == NormalRank::Missed);

    // Only once the input queue is drained through the render time does the
    // miss become final.
    miss = nosfix::DecideNormalMiss(note, SongTime{render_time, kGeneration},
                                    SongTime{render_time, kGeneration}, kGeneration, 0.0, w);
    CHECK(miss.ok());
    CHECK(miss.current_past_deadline && miss.input_past_deadline);
    CHECK(miss.finalize_miss);

    // ... and a retry generation invalidates that decision entirely.
    miss = nosfix::DecideNormalMiss(note, SongTime{render_time, kGeneration},
                                    SongTime{render_time, kGeneration + 1}, kGeneration, 0.0, w);
    CHECK(miss.status == NormalStatus::GenerationMismatch);
    CHECK(!miss.finalize_miss);
}

// ---------------------------------------------------------------------------
// Render-schedule replay
// ---------------------------------------------------------------------------

struct ReplayRow final {
    double note_us = 0.0;
    double captured_us = 0.0;
    double consumed_us = 0.0;
    double delta_us = 0.0;
    NormalRank rank = NormalRank::TooEarly;
    NormalRank recorded = NormalRank::TooEarly;
    NormalRank rank_at_render = NormalRank::TooEarly;
    bool eligible = false;
    bool deferred = false;
    NormalStatus status = NormalStatus::NotEvaluated;
};

// Presentation ticks for one schedule: nominal period, deterministic jitter of
// +/-30 %, and a 900 ms stall every 23rd presentation (the measured class of
// scene-load stall).
std::vector<double> MakeTicks(double hz, std::uint32_t seed, double end_us) {
    const double period = 1'000'000.0 / hz;
    const double stall_us = 900'000.0;
    std::vector<double> ticks;
    ticks.reserve(static_cast<std::size_t>(end_us / period) + 64);

    std::uint32_t state = seed;
    double t = -period * 0.5;
    std::uint64_t index = 0;
    while (t < end_us) {
        ticks.push_back(t);
        state = state * 1664525u + 1013904223u;
        const double unit = static_cast<double>(state >> 8) / 16777216.0; // [0,1)
        double step = period * (1.0 + 0.6 * (unit - 0.5));
        if (step < period * 0.25) step = period * 0.25;
        if (++index % 23 == 0) step += stall_us;
        t += step;
    }
    return ticks;
}

const double kReplayRates[7] = {60.0, 90.0, 120.0, 144.0, 165.0, 240.0, 360.0};

// Fixed captures: the press time follows from the chart time and the intended
// delta, so every schedule replays the very same SongTimes.
std::vector<ReplayRow> Replay(double hz) {
    const auto w = BaseWindows();
    const double pattern[10] = {400.0,      0.0,        -400.0,  1500.0,   1500.5,
                                -2000.0,    -2000.5,    -26500.5, 25000.5, -11000.0};
    constexpr int kNotes = 40;

    std::vector<ReplayRow> rows;
    rows.reserve(kNotes);
    for (int k = 0; k < kNotes; ++k) {
        ReplayRow row;
        row.note_us = 5'000'000.0 + static_cast<double>(k) * 137'000.0;
        row.delta_us = pattern[k % 10];
        row.captured_us = row.note_us - row.delta_us;
        rows.push_back(row);
    }

    const double end_us = rows.back().note_us + 2'000'000.0;
    const auto seed = 0x9E3779B9u ^ static_cast<std::uint32_t>(hz * 7919.0);
    const auto ticks = MakeTicks(hz, seed, end_us);

    std::size_t tick = 0;
    for (auto& row : rows) {
        while (tick < ticks.size() && ticks[tick] < row.captured_us) ++tick;
        row.consumed_us = tick < ticks.size() ? ticks[tick] : ticks.back();

        const auto press = nosfix::EvaluateNormalPress(
            row.note_us, SongTime{row.captured_us, kGeneration}, kGeneration, 0.0, w);
        row.status = press.status;
        row.rank = press.rank;
        row.recorded = press.recorded_rank;
        row.eligible = press.eligible;
        row.deferred = press.deferred_early;
        row.rank_at_render = nosfix::EvaluateNormalPress(
                                 row.note_us, SongTime{row.consumed_us, kGeneration},
                                 kGeneration, 0.0, w)
                                 .rank;
    }
    return rows;
}

void TestRenderScheduleInvariance() {
    std::printf("[section] replay across seven render schedules\n");
    const auto w = BaseWindows();

    std::vector<std::vector<ReplayRow>> replays;
    replays.reserve(7);
    for (const double hz : kReplayRates) replays.push_back(Replay(hz));

    // Every capture was evaluated successfully.
    CHECK(replays[0].size() == 40);
    for (const auto& rows : replays) {
        bool all_ok = true;
        for (const auto& row : rows) all_ok = all_ok && row.status == NormalStatus::Ok;
        CHECK(all_ok);
    }

    // Classification, recorded rank, eligibility, deferral and delta must not
    // depend on the schedule at all.
    for (std::size_t s = 1; s < replays.size(); ++s) {
        std::size_t classification_differences = 0;
        std::size_t delta_differences = 0;
        for (std::size_t i = 0; i < replays[0].size(); ++i) {
            const auto& a = replays[0][i];
            const auto& b = replays[s][i];
            if (a.captured_us != b.captured_us) ++delta_differences;
            if (a.rank != b.rank || a.recorded != b.recorded || a.eligible != b.eligible ||
                a.deferred != b.deferred) {
                ++classification_differences;
            }
            if (a.delta_us != b.delta_us) ++delta_differences;
        }
        if (classification_differences != 0) {
            std::printf("  %g Hz: %zu classification differences\n", kReplayRates[s],
                        classification_differences);
        }
        if (delta_differences != 0) {
            std::printf("  %g Hz: %zu captured/delta differences\n", kReplayRates[s],
                        delta_differences);
        }
        CHECK(classification_differences == 0);
        CHECK(delta_differences == 0);
    }

    // Expected ranks per pattern entry, identical in every schedule.
    const NormalRank expected[10] = {NormalRank::Just,   NormalRank::Just,  NormalRank::Just,
                                     NormalRank::Just,   NormalRank::Great, NormalRank::Just,
                                     NormalRank::Great,  NormalRank::Missed, NormalRank::TooEarly,
                                     NormalRank::Great};
    const bool expected_deferred[10] = {true,  false, false, true,  true, false,
                                        false, false, false, false};
    const NormalRank expected_recorded[10] = {
        NormalRank::Just,  NormalRank::Just,  NormalRank::Just,   NormalRank::Just,
        NormalRank::Great, NormalRank::Just,  NormalRank::Great,  NormalRank::Missed,
        NormalRank::TooEarly, NormalRank::Great};

    for (const auto& rows : replays) {
        for (std::size_t i = 0; i < rows.size(); ++i) {
            const std::size_t slot = i % 10;
            CHECK(rows[i].rank == expected[slot]);
            CHECK(rows[i].recorded == expected_recorded[slot]);
            CHECK(rows[i].deferred == expected_deferred[slot]);
        }
    }

    // Each schedule really is a different consumption lattice, really stalls
    // past the miss deadline, and really would misjudge if the render time
    // were used instead of the captured time.
    for (std::size_t s = 0; s < replays.size(); ++s) {
        const auto& rows = replays[s];
        double max_lag = 0.0;
        std::size_t consumed_after_deadline = 0;
        std::size_t render_time_would_differ = 0;
        for (const auto& row : rows) {
            const double lag = row.consumed_us - row.captured_us;
            CHECK(lag >= 0.0);
            max_lag = std::max(max_lag, lag);
            if (row.consumed_us > row.note_us + w.good_late_us) ++consumed_after_deadline;
            if (row.rank_at_render != row.rank) ++render_time_would_differ;

            // The central invariant: with the press as the newest processed
            // input, no miss may be finalized before the watermark passes the
            // deadline, however late the render is.
            const auto miss = nosfix::DecideNormalMiss(
                row.note_us, SongTime{row.consumed_us, kGeneration},
                SongTime{row.captured_us, kGeneration}, kGeneration, 0.0, w);
            CHECK(miss.ok());
            if (row.captured_us <= row.note_us + w.good_late_us) CHECK(!miss.finalize_miss);
        }
        CHECK(max_lag > 0.0);
        CHECK(consumed_after_deadline > 0);
        CHECK(render_time_would_differ > 0);
        std::printf(
            "  %6.1f Hz: max consumption lag %10.1f us, %2zu/40 consumed past the miss "
            "deadline, %2zu/40 would misrank at render time\n",
            kReplayRates[s], max_lag, consumed_after_deadline, render_time_would_differ);

        // Consumption times are genuinely schedule-dependent.
        std::size_t differing_consumption = 0;
        for (std::size_t i = 0; i < rows.size(); ++i) {
            if (rows[i].consumed_us != replays[0][i].consumed_us) ++differing_consumption;
        }
        if (s != 0) CHECK(differing_consumption > 0);
    }
}

void TestResolutionIsNotWindowWidth() {
    std::printf("[section] render resolution vs scoring resolution\n");
    const auto w = BaseWindows();
    const auto rank_with = [](double delta_us, const nosfix::NormalWindows& windows) {
        return nosfix::EvaluateNormalPress(delta_us, SongTime{0.0, kGeneration}, kGeneration,
                                           0.0, windows)
            .rank;
    };

    for (const double hz : kReplayRates) {
        const double period = 1'000'000.0 / hz;
        // Every schedule is far coarser than the distinctions below.
        CHECK(period > 1000.0);
        CHECK(rank_with(w.just_early_us, w) == NormalRank::Just);
        CHECK(rank_with(w.just_early_us + 0.001, w) == NormalRank::Great);
        CHECK(rank_with(-w.just_late_us, w) == NormalRank::Just);
        CHECK(rank_with(-w.just_late_us - 0.001, w) == NormalRank::Great);
        CHECK(rank_with(w.good_early_us + 0.001, w) == NormalRank::TooEarly);
        CHECK(rank_with(-w.good_late_us - 0.001, w) == NormalRank::Missed);
        // The window table is the caller's and is never adjusted per schedule.
        CHECK(w == BaseWindows());
    }

    // Tightening the windows is a caller decision and changes the ranks even
    // at the identical render rate - the two effects are independent.
    auto narrow = w;
    narrow.just_early_us = 1499.0;
    narrow.just_late_us = 1999.0;
    narrow.great_early_us = 9999.0;
    narrow.great_late_us = 10999.0;
    narrow.good_early_us = 24999.0;
    narrow.good_late_us = 26499.0;
    CHECK(nosfix::ValidateNormalWindows(narrow) == NormalWindowStatus::Ok);
    CHECK(rank_with(1499.5, narrow) == NormalRank::Great);
    CHECK(rank_with(-1999.5, narrow) == NormalRank::Great);
    CHECK(rank_with(24999.5, narrow) == NormalRank::TooEarly);
    CHECK(rank_with(-26499.5, narrow) == NormalRank::Missed);
    // The same deltas remain inside the base windows: only the table changed.
    CHECK(rank_with(1499.5, w) == NormalRank::Just);
    CHECK(rank_with(-1999.5, w) == NormalRank::Just);
    CHECK(rank_with(24999.5, w) == NormalRank::Good);
    CHECK(rank_with(-26499.5, w) == NormalRank::Good);
}

// ---------------------------------------------------------------------------

void TestLegacyAdapter() {
    std::printf("[section] legacy integer window adapter\n");
    using nosfix::LegacyNormalWindowUnits;

    LegacyNormalWindowUnits bounds{};
    bounds.just_earliest_units = 1;
    bounds.just_latest_units = -1;
    bounds.great_earliest_units = 6;
    bounds.great_latest_units = -6;
    bounds.good_earliest_units = 15;
    bounds.good_latest_units = -15;

    auto converted = nosfix::ConvertLegacyNormalWindows(bounds, 60.0);
    CHECK(converted.ok());
    CHECK(converted.window_status == NormalWindowStatus::Ok);
    CHECK(Near(converted.windows.just_early_us, 1'000'000.0 / 60.0));
    CHECK(Near(converted.windows.just_late_us, 1'000'000.0 / 60.0));
    CHECK(converted.windows.great_early_us == 100'000.0);
    CHECK(converted.windows.great_late_us == 100'000.0);
    CHECK(converted.windows.good_early_us == 250'000.0);
    CHECK(converted.windows.good_late_us == 250'000.0);
    CHECK(nosfix::ValidateNormalWindows(converted.windows) == NormalWindowStatus::Ok);

    // The converted table is directly usable and the boundary falls where the
    // native integer comparison put it (one unit == 16.67 ms here).
    auto r = nosfix::EvaluateNormalPress(16'666.0, SongTime{0.0, kGeneration}, kGeneration, 0.0,
                                         converted.windows);
    CHECK(r.rank == NormalRank::Just);
    r = nosfix::EvaluateNormalPress(16'667.0, SongTime{0.0, kGeneration}, kGeneration, 0.0,
                                    converted.windows);
    CHECK(r.rank == NormalRank::Great);
    r = nosfix::EvaluateNormalPress(-250'000.0, SongTime{0.0, kGeneration}, kGeneration, 0.0,
                                    converted.windows);
    CHECK(r.rank == NormalRank::Good);

    // A caller that supplies 1e6 units per second supplies microseconds.
    LegacyNormalWindowUnits micro{};
    micro.just_earliest_units = 200;
    micro.just_latest_units = -210;
    micro.great_earliest_units = 500;
    micro.great_latest_units = -520;
    micro.good_earliest_units = 1000;
    micro.good_latest_units = -1050;
    const auto as_micro = nosfix::ConvertLegacyNormalWindows(micro, 1'000'000.0);
    CHECK(as_micro.ok());
    CHECK(as_micro.windows.just_early_us == 200.0);
    CHECK(as_micro.windows.just_late_us == 210.0);
    CHECK(as_micro.windows.great_early_us == 500.0);
    CHECK(as_micro.windows.great_late_us == 520.0);
    CHECK(as_micro.windows.good_early_us == 1000.0);
    CHECK(as_micro.windows.good_late_us == 1050.0);

    // A different caller rate gives a different table: the kernel itself has
    // no built-in 60 and no monitor rate.
    const auto at_1000 = nosfix::ConvertLegacyNormalWindows(bounds, 1000.0);
    CHECK(at_1000.ok());
    CHECK(Near(at_1000.windows.just_early_us, 1000.0));
    CHECK(at_1000.windows.great_early_us == 6000.0);
    CHECK(at_1000.windows.good_early_us == 15'000.0);
    CHECK(at_1000.windows != converted.windows);

    // Rate rejection.
    CHECK(nosfix::ConvertLegacyNormalWindows(bounds, 0.0).status ==
          nosfix::LegacyWindowStatus::NonPositiveRate);
    CHECK(nosfix::ConvertLegacyNormalWindows(bounds, -60.0).status ==
          nosfix::LegacyWindowStatus::NonPositiveRate);
    CHECK(nosfix::ConvertLegacyNormalWindows(bounds, kNaN).status ==
          nosfix::LegacyWindowStatus::NonFiniteRate);
    CHECK(nosfix::ConvertLegacyNormalWindows(bounds, kInf).status ==
          nosfix::LegacyWindowStatus::NonFiniteRate);

    // Sign convention is checked, not guessed.
    auto bad_sign = bounds;
    bad_sign.just_earliest_units = -1;
    CHECK(nosfix::ConvertLegacyNormalWindows(bad_sign, 60.0).status ==
          nosfix::LegacyWindowStatus::InvalidBoundSign);
    bad_sign = bounds;
    bad_sign.good_latest_units = 1;
    CHECK(nosfix::ConvertLegacyNormalWindows(bad_sign, 60.0).status ==
          nosfix::LegacyWindowStatus::InvalidBoundSign);

    // Bounds that a double cannot carry exactly are refused, not rounded.
    auto huge = bounds;
    huge.just_earliest_units = 9'007'199'254'740'993LL; // 2^53 + 1
    CHECK(nosfix::ConvertLegacyNormalWindows(huge, 60.0).status ==
          nosfix::LegacyWindowStatus::UnrepresentableBound);

    // A tiny caller rate can make even a representable bound overflow.
    auto overflowing = bounds;
    overflowing.just_earliest_units = 9'007'199'254'740'992LL;
    CHECK(nosfix::ConvertLegacyNormalWindows(overflowing, 1e-300).status ==
          nosfix::LegacyWindowStatus::UnrepresentableBound);

    // Converted tables are validated like any other table.
    auto inverted = bounds;
    inverted.just_earliest_units = 10; // 166.7 ms, outside 6 units of Great
    const auto inverted_result = nosfix::ConvertLegacyNormalWindows(inverted, 60.0);
    CHECK(inverted_result.status == nosfix::LegacyWindowStatus::InvalidWindows);
    CHECK(inverted_result.window_status == NormalWindowStatus::NotNested);
    CHECK(!inverted_result.ok());

    // Zero bounds convert to the all-zero table exactly.
    const nosfix::LegacyNormalWindowUnits zeros{};
    const auto zero_result = nosfix::ConvertLegacyNormalWindows(zeros, 60.0);
    CHECK(zero_result.ok());
    CHECK(zero_result.windows == nosfix::NormalWindows{});
}

// ---------------------------------------------------------------------------

void TestStatelessRepeatability() {
    std::printf("[section] stateless repeatability\n");
    const auto w = BaseWindows();
    const auto first = nosfix::EvaluateNormalPress(1'000.0, SongTime{500.0, kGeneration},
                                                   kGeneration, 0.0, w);

    // Interleave unrelated calls, including failing ones, with no state
    // carried between them.
    (void)nosfix::EvaluateNormalPress(kNaN, SongTime{kInf, 0}, 0, kNaN, w);
    (void)nosfix::EvaluateNormalPress(-500.0, SongTime{900'000.0, kGeneration}, kGeneration,
                                      12'000.0, w);
    (void)nosfix::DecideNormalMiss(1.0, SongTime{2.0, kGeneration}, SongTime{3.0, kGeneration},
                                   kGeneration, 0.0, w);
    (void)nosfix::DecideNormalMiss(1.0, SongTime{2.0, kGeneration}, SongTime{2.0, kGeneration},
                                   kGeneration, 0.0, w);
    (void)nosfix::ConvertLegacyNormalWindows(nosfix::LegacyNormalWindowUnits{}, 60.0);

    const auto again = nosfix::EvaluateNormalPress(1'000.0, SongTime{500.0, kGeneration},
                                                   kGeneration, 0.0, w);
    CHECK(first.status == again.status);
    CHECK(first.rank == again.rank);
    CHECK(first.recorded_rank == again.recorded_rank);
    CHECK(first.delta_us == again.delta_us);
    CHECK(first.eligible == again.eligible);
    CHECK(first.deferred_early == again.deferred_early);
    CHECK(first.delta_us == 500.0);
    CHECK(first.rank == NormalRank::Just);
    CHECK(first.deferred_early);
}

} // namespace

int main() {
    std::printf("normal_judgment_tests\n");
#if defined(NDEBUG)
    std::printf("build: NDEBUG defined (release) - checks remain active\n");
#else
    std::printf("build: debug\n");
#endif

    TestWindowValidation();
    TestRankClassification();
    TestClassifierFailClosed();
    TestEarlyDeferPolicy();
    TestOffsets();
    TestGeneration();
    TestInvalidArithmetic();
    TestMissDeadline();
    TestDelayedConsumption();
    TestRenderScheduleInvariance();
    TestResolutionIsNotWindowWidth();
    TestLegacyAdapter();
    TestStatelessRepeatability();

    std::printf("\n%d checks, %d failure(s)\n", g_checks, g_failures);
    if (g_failures == 0) {
        std::printf("normal_judgment_tests: PASS\n");
        return 0;
    }
    std::printf("normal_judgment_tests: FAIL\n");
    return 1;
}
