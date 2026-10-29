#include "nosfix/native_scoring.hpp"

#include <algorithm>
#include <limits>

namespace nosfix {

namespace {

constexpr std::int32_t Clamp(std::int32_t value, std::int32_t low,
                             std::int32_t high) noexcept {
    if (value < low) return low;
    if (value > high) return high;
    return value;
}

// 0x180303EDC: `if (a1 > 1 && a3 > 1) { v16 = 100; a3 = 5; }`
constexpr bool GradeIncrementsCombo(std::int32_t grade) noexcept {
    return grade >= 2 && grade <= 5;
}

} // namespace

const char* ScoreGradeName(ScoreGrade grade) noexcept {
    switch (grade) {
    case ScoreGrade::None: return "none";
    case ScoreGrade::Miss: return "miss";
    case ScoreGrade::Good: return "good";
    case ScoreGrade::Great: return "great";
    case ScoreGrade::Just: return "just";
    case ScoreGrade::Special: return "special";
    }
    return "invalid";
}

const char* ScoreStatusName(ScoreStatus status) noexcept {
    switch (status) {
    case ScoreStatus::Ok: return "ok";
    case ScoreStatus::NotConfigured: return "not-configured";
    case ScoreStatus::NotScored: return "not-scored";
    case ScoreStatus::GradeOutOfRange: return "grade-out-of-range";
    case ScoreStatus::InvalidConfig: return "invalid-config";
    case ScoreStatus::InvalidRow: return "invalid-row";
    case ScoreStatus::Overflow: return "overflow";
    }
    return "invalid";
}

std::uint8_t AdvanceRankState(std::uint8_t state, std::uint8_t grade) noexcept {
    // 0x180304565 switch (a3), transcribed literally. The masks drop bit 2.
    switch (grade) {
    case 1:
        return 1;
    case 2:
        return ((static_cast<std::uint32_t>(state) - 1u) & 0xFFFFFFFBu) != 0u
                   ? static_cast<std::uint8_t>(2)
                   : state;
    case 3:
        return (static_cast<std::uint32_t>(state) & 0xFFFFFFFBu) == 0u
                   ? static_cast<std::uint8_t>(3)
                   : state;
    case 4:
        return state == 0 ? static_cast<std::uint8_t>(4) : state;
    case 5:
        return state != 1 ? static_cast<std::uint8_t>(5) : state;
    default:
        return state;
    }
}

ScoreStatus NativeScoringState::Configure(const NativeScoringConfig& config) noexcept {
    if (config.combo_limit <= 0 || config.combo_limit > kScoreComboLimit) {
        return ScoreStatus::InvalidConfig;
    }
    if (config.counter_limit <= 0 || config.counter_limit > kScoreCounterLimit) {
        return ScoreStatus::InvalidConfig;
    }
    if (config.history_capacity <= 0 || config.history_capacity > kScoreHistoryCapacity) {
        return ScoreStatus::InvalidConfig;
    }
    if (config.popup_base < 0 || config.popup_pitch < 0) {
        return ScoreStatus::InvalidConfig;
    }
    config_ = config;
    configured_ = true;
    Reset();
    return ScoreStatus::Ok;
}

void NativeScoringState::Reset() noexcept {
    grade_counters_.fill(0);
    for (auto& row : columns_) row.fill(0);
    for (auto& row : perfect_) row.fill(0);
    ring_.fill(ScoreRingRecord{});
    combo_ = 0;
    max_combo_ = 0;
    rank_state_ = 0;
    history_count_ = 0;
    order_ = 0;
    ring_head_ = 0;
}

ScoreStatus NativeScoringState::Apply(const ScoreInput& input, ScoreEffect& out) noexcept {
    out = ScoreEffect{};
    if (!configured_) {
        out.status = ScoreStatus::NotConfigured;
        return out.status;
    }
    // 0x1802F79E0: a9 == 0 short-circuits 0x180303E80 before any state change.
    if (!input.scoring_enabled) {
        out.status = ScoreStatus::NotScored;
        return out.status;
    }
    const std::int32_t raw_grade = static_cast<std::int32_t>(input.grade);
    if (raw_grade < 1 || raw_grade > 5) {
        // Grade 0 and grades > 5 reach 0x180303E80 only from a non-normal judge
        // (0x1802F6800 passes 1, 3, 4, or a pending rank of 3/4). Refuse rather
        // than invent the semantics of the zero column.
        out.status = ScoreStatus::GradeOutOfRange;
        return out.status;
    }

    std::int32_t grade = raw_grade;
    std::int32_t score = input.scale;
    bool overridden = false;
    if (input.event_state > 1 && grade > 1) {
        score = 100;
        grade = 5;
        overridden = true;
    }

    // Popup geometry is computed up front so an overflowing key pair cannot
    // leave the counters half-updated.
    const std::int64_t low = static_cast<std::int64_t>(config_.popup_base) +
                             static_cast<std::int64_t>(config_.popup_pitch) *
                                 (static_cast<std::int64_t>(input.left_key) - 1);
    const std::int64_t high = static_cast<std::int64_t>(config_.popup_base) +
                              static_cast<std::int64_t>(config_.popup_pitch) *
                                  static_cast<std::int64_t>(input.right_key);
    const std::int64_t popup = low + (high - low) / 2;
    constexpr std::int64_t kInt32Min = std::numeric_limits<std::int32_t>::min();
    constexpr std::int64_t kInt32Max = std::numeric_limits<std::int32_t>::max();
    if (popup < kInt32Min || popup > kInt32Max) {
        out.status = ScoreStatus::Overflow;
        return out.status;
    }

    out.order = ++order_;
    out.note_index = input.note_index;
    out.grade = static_cast<std::uint8_t>(grade);
    out.grade_overridden = overridden;
    out.score = score;
    // 0x180304379: 54 = clamp(a7[2], 0, 4) selects the score-table row.
    out.row = static_cast<std::uint8_t>(
        Clamp(input.chart_group, 0, static_cast<std::int32_t>(kScoreRowCount) - 1));

    // ---- combo (0x180303EDE..0x180303F2C and 0x18030407C..0x1803040FA) ----
    out.combo_before = combo_;
    if (GradeIncrementsCombo(grade)) {
        std::int32_t next = combo_ + 1;
        if (next > config_.combo_limit) next = config_.combo_limit;
        combo_ = next;
        out.combo_incremented = true;
        // `if ((__int16)v13 <= stored) v13 = stored;` then clamp to 9999.
        std::int32_t candidate = std::max(combo_, max_combo_);
        if ((candidate & 0x8000) != 0) candidate = 0; // the native bit-15 reset
        if (candidate > config_.combo_limit) candidate = config_.combo_limit;
        max_combo_ = candidate;
    } else {
        combo_ = 0;
        out.combo_reset = true;
    }
    out.combo_after = combo_;
    out.max_combo_after = max_combo_;

    // ---- grade histogram (0x180304315..0x18030436B) ----
    const std::size_t column = static_cast<std::size_t>(grade);
    out.histogram_before = grade_counters_[column];
    grade_counters_[0] = std::max<std::int32_t>(0, grade_counters_[0] - 1);
    std::int32_t slot = grade_counters_[column] + 1;
    if (slot < 0) slot = 0;
    if (slot > config_.counter_limit) slot = config_.counter_limit;
    grade_counters_[column] = slot;
    out.histogram_after = slot;

    // ---- score decomposition (0x180304379..0x1803043E4) ----
    const std::int32_t weighted = Clamp(score, 0, 100);
    const std::int32_t credit = grade <= 1 ? 100 : weighted;
    const std::size_t row = out.row;
    out.column_delta[0] -= 100;
    if (grade != 1) out.column_delta[1] += 100 - credit;
    out.column_delta[column] += credit;
    for (std::size_t i = 0; i < kScoreColumnCount; ++i) {
        columns_[row][i] += out.column_delta[i];
    }

    // ---- perfect counter (0x1803043E9..0x180304437) ----
    if (grade != 0 && credit == 100) {
        std::int32_t& count = perfect_[row][column];
        if (count < std::numeric_limits<std::uint16_t>::max()) {
            ++count;
        }
        out.perfect_counted = true;
    }

    // ---- effect/score history record (0x180304457..0x180304559) ----
    if (config_.history_enabled &&
        history_count_ < static_cast<std::uint32_t>(config_.history_capacity)) {
        out.history_index = history_count_;
        out.history_appended = true;
        ScoreRingRecord& record = ring_[ring_head_];
        record = ScoreRingRecord{};
        record.order = out.order;
        record.note_index = input.note_index;
        record.grade = out.grade;
        record.score = score;
        record.combo = combo_;
        record.row = out.row;
        record.appended = true;
        record.history_index = out.history_index;
        ring_head_ = (ring_head_ + 1) % kScoreRingSize;
        ++history_count_;
    }

    // ---- rank tracker (0x180304565) ----
    out.rank_state_before = static_cast<std::uint8_t>(rank_state_);
    rank_state_ = AdvanceRankState(out.rank_state_before, out.grade);
    out.rank_state_after = static_cast<std::uint8_t>(rank_state_);

    out.popup_units = static_cast<std::int32_t>(popup);

    // ---- effect dispatch gates (0x180304619..0x18030465F) ----
    out.effect_dispatched = !input.auto_mode || input.auto_play || input.effects_muted ||
                            input.chart_group == 4 || input.chart_group == 1 ||
                            grade == 1;

    out.status = ScoreStatus::Ok;
    return out.status;
}

std::size_t NativeScoringState::RecentRecords(std::span<ScoreRingRecord> out) const noexcept {
    const std::size_t stored = std::min<std::size_t>(history_count_, kScoreRingSize);
    if (out.size() < stored) return 0;
    for (std::size_t i = 0; i < stored; ++i) {
        const std::size_t index = (ring_head_ + kScoreRingSize - stored + i) % kScoreRingSize;
        out[i] = ring_[index];
    }
    return stored;
}

} // namespace nosfix
