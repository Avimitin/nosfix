#include "nosfix/native_scoring.hpp"

#include "test_support.hpp"

#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <limits>

namespace {

using nosfix::NativeScoringConfig;
using nosfix::NativeScoringState;
using nosfix::ScoreEffect;
using nosfix::ScoreInput;
using nosfix::ScoreStatus;

constexpr std::int32_t kComboLimit = nosfix::kScoreComboLimit;

NativeScoringState MakeState(bool history_enabled = false) {
    NativeScoringState state;
    NativeScoringConfig config;
    config.history_enabled = history_enabled;
    const ScoreStatus status = state.Configure(config);
    CHECK(status == ScoreStatus::Ok);
    return state;
}

ScoreInput MakeInput(std::uint8_t grade, std::int32_t row = 0, std::int32_t scale = 100) {
    ScoreInput input;
    input.grade = grade;
    input.chart_group = row;
    input.scale = scale;
    input.scoring_enabled = true;
    input.left_key = 1;
    input.right_key = 1;
    return input;
}

std::int32_t ColumnSum(const ScoreEffect& effect) {
    std::int32_t sum = 0;
    for (std::int32_t value : effect.column_delta) sum += value;
    return sum;
}

void TestConfigureAndReset() {
    NativeScoringState state;
    ScoreEffect effect;
    ScoreInput input = MakeInput(4);
    CHECK(state.Apply(input, effect) == ScoreStatus::NotConfigured);
    CHECK(effect.status == ScoreStatus::NotConfigured);

    NativeScoringConfig bad;
    bad.combo_limit = 0;
    CHECK(state.Configure(bad) == ScoreStatus::InvalidConfig);
    bad = NativeScoringConfig{};
    bad.counter_limit = kComboLimit + 1;
    CHECK(state.Configure(bad) == ScoreStatus::InvalidConfig);
    bad = NativeScoringConfig{};
    bad.history_capacity = 0;
    CHECK(state.Configure(bad) == ScoreStatus::InvalidConfig);
    bad = NativeScoringConfig{};
    bad.popup_pitch = -1;
    CHECK(state.Configure(bad) == ScoreStatus::InvalidConfig);

    CHECK(state.Configure(NativeScoringConfig{}) == ScoreStatus::Ok);
    CHECK(state.IsConfigured());
    CHECK(state.Combo() == 0);
    CHECK(state.Order() == 0);

    CHECK(state.Apply(MakeInput(4), effect) == ScoreStatus::Ok);
    CHECK(state.Combo() == 1);
    state.Reset();
    CHECK(state.Combo() == 0);
    CHECK(state.MaxCombo() == 0);
    CHECK(state.RankState() == 0);
    CHECK(state.Order() == 0);
    CHECK(state.RowColumn(0, 0) == 0);
    CHECK(state.PerfectCount(0, 0) == 0);
    CHECK(state.GradeCounter(0) == 0);
    CHECK(state.HistoryCount() == 0);
}

void TestScoringGateAndGradeRange() {
    NativeScoringState state = MakeState();
    ScoreEffect effect;
    ScoreInput input = MakeInput(4);
    input.scoring_enabled = false; // a9 == 0: auto-play or chart+48 == 2
    CHECK(state.Apply(input, effect) == ScoreStatus::NotScored);
    CHECK(effect.status == ScoreStatus::NotScored);
    CHECK(state.Combo() == 0);
    CHECK(state.Order() == 0);

    for (std::uint8_t grade : {std::uint8_t{0}, std::uint8_t{6}, std::uint8_t{255}}) {
        input = MakeInput(grade);
        CHECK(state.Apply(input, effect) == ScoreStatus::GradeOutOfRange);
        CHECK(state.Combo() == 0);
        CHECK(state.Order() == 0);
        CHECK(state.RowColumn(0, 0) == 0);
    }
}

void TestComboAndMaxCombo() {
    NativeScoringState state = MakeState();
    ScoreEffect effect;
    CHECK(state.Apply(MakeInput(4), effect) == ScoreStatus::Ok);
    CHECK(effect.combo_before == 0);
    CHECK(effect.combo_after == 1);
    CHECK(effect.combo_incremented);
    CHECK(!effect.combo_reset);
    CHECK(state.Apply(MakeInput(2), effect) == ScoreStatus::Ok);
    CHECK(effect.combo_after == 2);
    CHECK(state.Apply(MakeInput(3), effect) == ScoreStatus::Ok);
    CHECK(effect.combo_after == 3);
    CHECK(state.MaxCombo() == 3);

    // A late miss (grade 1) clears the combo and records the worst rank.
    CHECK(state.Apply(MakeInput(1), effect) == ScoreStatus::Ok);
    CHECK(effect.combo_reset);
    CHECK(!effect.combo_incremented);
    CHECK(effect.combo_after == 0);
    CHECK(state.Combo() == 0);
    CHECK(state.MaxCombo() == 3);
    CHECK(state.RankState() == 1);

    // Saturation at the image-initialized 9999 clamp.
    for (int i = 0; i < kComboLimit + 5; ++i) {
        CHECK(state.Apply(MakeInput(4), effect) == ScoreStatus::Ok);
    }
    CHECK(state.Combo() == kComboLimit);
    CHECK(state.MaxCombo() == kComboLimit);
}

void TestScoreColumns() {
    NativeScoringState state = MakeState();
    ScoreEffect effect;

    CHECK(state.Apply(MakeInput(1), effect) == ScoreStatus::Ok);
    CHECK(ColumnSum(effect) == 0);
    CHECK(effect.column_delta[0] == -100);
    CHECK(effect.column_delta[1] == 100); // a miss moves the points into the lost column
    CHECK(state.RowColumn(0, 0) == -100);
    CHECK(state.RowColumn(0, 1) == 100);

    CHECK(state.Apply(MakeInput(4), effect) == ScoreStatus::Ok);
    CHECK(ColumnSum(effect) == 0);
    CHECK(effect.column_delta[0] == -100);
    CHECK(effect.column_delta[1] == 0);
    CHECK(effect.column_delta[4] == 100);
    CHECK(state.RowColumn(0, 4) == 100);

    // A reduced scale loses the difference into column 1.
    CHECK(state.Apply(MakeInput(3, 0, 50), effect) == ScoreStatus::Ok);
    CHECK(ColumnSum(effect) == 0);
    CHECK(effect.column_delta[0] == -100);
    CHECK(effect.column_delta[1] == 50);
    CHECK(effect.column_delta[3] == 50);
    CHECK(effect.score == 50);
    CHECK(!effect.perfect_counted);

    // Rows are the clamped chart+8 index and stay independent.
    CHECK(state.Apply(MakeInput(2, 4), effect) == ScoreStatus::Ok);
    CHECK(effect.row == 4);
    CHECK(state.RowColumn(4, 2) == 100);
    CHECK(state.RowColumn(0, 2) == 0);
    CHECK(state.Apply(MakeInput(2, 99), effect) == ScoreStatus::Ok);
    CHECK(effect.row == 4);
    CHECK(state.Apply(MakeInput(2, -7), effect) == ScoreStatus::Ok);
    CHECK(effect.row == 0);
}

void TestPerfectCounter() {
    NativeScoringState state = MakeState();
    ScoreEffect effect;
    CHECK(state.Apply(MakeInput(4), effect) == ScoreStatus::Ok);
    CHECK(effect.perfect_counted);
    CHECK(state.PerfectCount(0, 4) == 1);
    CHECK(state.Apply(MakeInput(2), effect) == ScoreStatus::Ok);
    CHECK(effect.perfect_counted);
    CHECK(state.PerfectCount(0, 2) == 1);
    CHECK(state.Apply(MakeInput(2, 0, 99), effect) == ScoreStatus::Ok);
    CHECK(!effect.perfect_counted);
    CHECK(state.PerfectCount(0, 2) == 1);
}

void TestGradeOverride() {
    NativeScoringState state = MakeState();
    ScoreEffect effect;
    ScoreInput input = MakeInput(2);
    input.event_state = 2; // note+84 > 1 and grade > 1: grade := 5, score := 100
    CHECK(state.Apply(input, effect) == ScoreStatus::Ok);
    CHECK(effect.grade_overridden);
    CHECK(effect.grade == 5);
    CHECK(effect.score == 100);
    CHECK(state.RankState() == 5);
    CHECK(effect.column_delta[5] == 100);

    input = MakeInput(2);
    input.event_state = 1; // not > 1
    CHECK(state.Apply(input, effect) == ScoreStatus::Ok);
    CHECK(!effect.grade_overridden);
    CHECK(effect.grade == 2);

    input = MakeInput(1);
    input.event_state = 4; // grade 1 is not > 1
    CHECK(state.Apply(input, effect) == ScoreStatus::Ok);
    CHECK(!effect.grade_overridden);
    CHECK(effect.grade == 1);
}

void TestRankStateMachine() {
    // Literal table of the switch at 0x180304565.
    const std::uint8_t expected[6][6] = {
        // grade:  0  1  2  3  4  5
        /*state 0*/ {0, 1, 2, 3, 4, 5},
        /*state 1*/ {1, 1, 1, 1, 1, 1},
        /*state 2*/ {2, 1, 2, 2, 2, 5},
        /*state 3*/ {3, 1, 2, 3, 3, 5},
        /*state 4*/ {4, 1, 2, 3, 4, 5},
        /*state 5*/ {5, 1, 5, 5, 5, 5},
    };
    for (std::uint8_t state = 0; state < 6; ++state) {
        for (std::uint8_t grade = 0; grade < 6; ++grade) {
            const std::uint8_t actual = nosfix::AdvanceRankState(state, grade);
            if (actual != expected[state][grade]) {
                std::printf("rank state %u grade %u -> %u expected %u\n",
                            static_cast<unsigned>(state), static_cast<unsigned>(grade),
                            static_cast<unsigned>(actual),
                            static_cast<unsigned>(expected[state][grade]));
            }
            CHECK(actual == expected[state][grade]);
        }
    }
}

void TestHistoryRecords() {
    NativeScoringState plain = MakeState(false);
    ScoreEffect effect;
    CHECK(plain.Apply(MakeInput(4), effect) == ScoreStatus::Ok);
    CHECK(!effect.history_appended);
    CHECK(plain.HistoryCount() == 0);

    NativeScoringState state = MakeState(true);
    for (std::uint32_t i = 0; i < 20; ++i) {
        ScoreInput input = MakeInput(3);
        input.note_index = 100 + i;
        CHECK(state.Apply(input, effect) == ScoreStatus::Ok);
        CHECK(effect.history_appended);
        CHECK(effect.history_index == i);
    }
    CHECK(state.HistoryCount() == 20);
    std::array<nosfix::ScoreRingRecord, nosfix::kScoreRingSize> records{};
    const std::size_t copied = state.RecentRecords(records);
    CHECK(copied == nosfix::kScoreRingSize);
    CHECK(records[copied - 1].note_index == 119);
    CHECK(records[0].note_index == 104);
    CHECK(records[copied - 1].grade == 3);

    // A tiny capacity refuses further records without disturbing the counters.
    NativeScoringConfig config;
    config.history_enabled = true;
    config.history_capacity = 2;
    NativeScoringState small;
    CHECK(small.Configure(config) == ScoreStatus::Ok);
    ScoreEffect last;
    for (int i = 0; i < 3; ++i) {
        CHECK(small.Apply(MakeInput(4), last) == ScoreStatus::Ok);
    }
    CHECK(!last.history_appended);
    CHECK(small.HistoryCount() == 2);
    CHECK(small.Combo() == 3);
}

void TestPopupAndOverflow() {
    NativeScoringState state = MakeState();
    ScoreEffect effect;
    ScoreInput input = MakeInput(4);
    input.left_key = 1;
    input.right_key = 1;
    CHECK(state.Apply(input, effect) == ScoreStatus::Ok);
    CHECK(effect.popup_units == 262); // 10 + (515 - 10) / 2

    input.left_key = 3;
    input.right_key = 5;
    CHECK(state.Apply(input, effect) == ScoreStatus::Ok);
    // low = 10 + 505*2 = 1020, high = 10 + 505*5 = 2535, 1020 + 1515/2 = 1777.
    CHECK(effect.popup_units == 1777);

    const std::int32_t combo_before = state.Combo();
    const std::int32_t row_before = state.RowColumn(0, 0);
    input.left_key = std::numeric_limits<std::int32_t>::max();
    input.right_key = std::numeric_limits<std::int32_t>::max();
    CHECK(state.Apply(input, effect) == ScoreStatus::Overflow);
    CHECK(state.Combo() == combo_before);
    CHECK(state.RowColumn(0, 0) == row_before);
}

void TestDeterminism() {
    NativeScoringState first = MakeState(true);
    NativeScoringState second = MakeState(true);
    const std::uint8_t grades[8] = {4, 3, 2, 1, 4, 5, 2, 3};
    for (int round = 0; round < 2; ++round) {
        NativeScoringState& state = round == 0 ? first : second;
        for (int i = 0; i < 8; ++i) {
            ScoreInput input = MakeInput(grades[i], i % 3, i % 2 == 0 ? 100 : 80);
            input.note_index = static_cast<std::uint32_t>(i);
            ScoreEffect effect;
            CHECK(state.Apply(input, effect) == ScoreStatus::Ok);
        }
    }
    CHECK(first.Combo() == second.Combo());
    CHECK(first.MaxCombo() == second.MaxCombo());
    CHECK(first.RankState() == second.RankState());
    CHECK(first.HistoryCount() == second.HistoryCount());
    for (std::size_t row = 0; row < nosfix::kScoreRowCount; ++row) {
        for (std::size_t column = 0; column < nosfix::kScoreColumnCount; ++column) {
            CHECK(first.RowColumn(row, column) == second.RowColumn(row, column));
            CHECK(first.PerfectCount(row, column) == second.PerfectCount(row, column));
        }
    }
}

void TestScoreGateAndEffectDispatch() {
    // a9 = (chart+48 != 2) && (chart+97 == 0).
    CHECK(nosfix::ScoreGate(0, false));
    CHECK(nosfix::ScoreGate(3, false));
    CHECK(!nosfix::ScoreGate(2, false));
    CHECK(!nosfix::ScoreGate(0, true));
    CHECK(!nosfix::ScoreGate(2, true));

    NativeScoringState state = MakeState();
    ScoreEffect effect;
    // Default boot: every gate global is 0, so the popup is dispatched.
    CHECK(state.Apply(MakeInput(4), effect) == ScoreStatus::Ok);
    CHECK(effect.effect_dispatched);

    // In auto mode the popup is suppressed for chart groups 2, 3 and 5 unless
    // grade == 1 or an override global is set.
    ScoreInput input = MakeInput(4, 2);
    input.auto_mode = true;
    CHECK(state.Apply(input, effect) == ScoreStatus::Ok);
    CHECK(!effect.effect_dispatched);
    input = MakeInput(4, 1);
    input.auto_mode = true;
    CHECK(state.Apply(input, effect) == ScoreStatus::Ok);
    CHECK(effect.effect_dispatched);
    input = MakeInput(4, 4);
    input.auto_mode = true;
    CHECK(state.Apply(input, effect) == ScoreStatus::Ok);
    CHECK(effect.effect_dispatched);
    input = MakeInput(1, 2);
    input.auto_mode = true;
    CHECK(state.Apply(input, effect) == ScoreStatus::Ok);
    CHECK(effect.effect_dispatched);
    input = MakeInput(4, 2);
    input.auto_mode = true;
    input.effects_muted = true;
    CHECK(state.Apply(input, effect) == ScoreStatus::Ok);
    CHECK(effect.effect_dispatched);
    input.auto_play = true;
    input.effects_muted = false;
    CHECK(state.Apply(input, effect) == ScoreStatus::Ok);
    CHECK(effect.effect_dispatched);

    // The note+84 override also forces the popup for a non-primary slot.
    input = MakeInput(2, 2);
    input.auto_mode = true;
    input.event_state = 2;
    CHECK(state.Apply(input, effect) == ScoreStatus::Ok);
    CHECK(effect.grade == 5);
    CHECK(!effect.effect_dispatched);
}

void TestAllocationFree() {
    NativeScoringState state = MakeState(true);
    ScoreEffect effect;
    test::BeginAllocationProbe();
    for (int i = 0; i < 64; ++i) {
        ScoreInput input = MakeInput(4, i % 5);
        input.note_index = static_cast<std::uint32_t>(i);
        const ScoreStatus status = state.Apply(input, effect);
        if (status != ScoreStatus::Ok) CHECK(false);
    }
    const long long allocations = test::EndAllocationProbe();
    CHECK(allocations == 0);
}

} // namespace

int main() {
    TestConfigureAndReset();
    TestScoringGateAndGradeRange();
    TestComboAndMaxCombo();
    TestScoreColumns();
    TestPerfectCounter();
    TestGradeOverride();
    TestRankStateMachine();
    TestHistoryRecords();
    TestPopupAndOverflow();
    TestDeterminism();
    TestScoreGateAndEffectDispatch();
    TestAllocationFree();
    return test::Finish("native_scoring");
}
