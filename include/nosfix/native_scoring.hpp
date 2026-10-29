#pragma once

// Portable model of the native rank/score/effect dispatch at RVA 0x180303E80
// (VA 0x180303E80, nostalgia.dll PAN-2025020501), reached from the normal-note
// judge 0x1802F6800 through vtable slot +0x68 (0x1802F79E0).
//
// Evidence base (all pseudocode exported under analyze/evidence/, all offsets
// module-relative RVAs):
//   verified_0x180303e80.c          the whole function
//   verified_0x1802f79e0.c          rank_score slot: the only normal-note caller
//   native_normal_judgment_0x1802f6800.c  grade values and call sites
//   verified_0x1800cb9a0.c          image initial values of every global below
//   verified_0x1802f7aa0.c          same popup formula for the effect slot
//
// Nothing here reads or writes game memory, calls a virtual, plays a sound,
// logs, allocates or blocks. Apply() is a pure state transition over
// caller-owned integers: the caller decides whether to forward the returned
// record to the real game. All state is single-owner and value-initialized.
//
// WHAT IS MODELLED (deterministic, byte-cited)
//   1. Scoring gate. 0x1802F79E0 computes a9 = (chart+48 != 2) && !chart+97.
//      a9 == 0 makes 0x180303E80 return before touching any state, so an
//      auto-play or chart+48 == 2 note neither scores nor breaks the combo.
//   2. Grade override. a1 = note+84 (the event state the traversal stores) and
//      a3 = the grade: `if (a1 > 1 && a3 > 1) { score = 100; grade = 5; }`.
//   3. Combo. Low dword of qword_181366908: incremented (saturating at 9999) for
//      grades 2..5, forced to 0 for grades 0 and 1. The running maximum is kept
//      in the 16-bit field at player+20 (0x180304A70()), also clamped at 9999.
//   4. Grade histogram. Six 16-bit counters at player+22: slot 0 is decremented
//      (floor 0) whenever the grade is non-zero, then slot[grade] is incremented
//      (ceiling 9999).
//   5. Score decomposition. player+36 is a row table of five rows (row index =
//      clamp(a7[2], 0, 4)) of six int32 columns. `v56 = grade <= 1 ? 100 :
//      clamp(score, 0, 100)`; if grade != 0 the row loses 100 from column 0 and,
//      when grade != 1, gains `100 - v56` in column 1; then column[grade] gains
//      v56. The six columns therefore sum to a constant across one call, which
//      is what makes the model checkable.
//   6. Perfect counter. A five-by-six table of 16-bit counters at player+156,
//      incremented only when grade != 0 and v56 == 100.
//   7. Rank tracker. High dword of qword_1813668C8, a four-state machine whose
//      transitions are reproduced literally below.
//   8. Effect/score history. When the debug/record byte at 0x13E470C is set and
//      `*(int*)(0x180301980() + 84000) < 3000`, a 28-byte record is appended
//      with {a7[14], a7[2], grade, combo, player+16, score}. The model reports
//      whether a record would be appended and at which index; it stores the
//      payload only in a small fixed ring for inspection.
//   9. Judgment popup geometry. `y = base + pitch*(left_key - 1)`, then
//      `y += (base + pitch*right_key - y) / 2` with base 0x13E4738 (10) and pitch
//      0x13E476C (505), evaluated in integer arithmetic exactly as written.
//      This is the (a2) value handed to 0x1803075A0 together with the grade.
//
// WHAT IS NOT MODELLED (and why the seam stays dormant)
//   * The leading "sustain effect" loop (0x180303F48..0x1803042F0) drives
//     `qword_1813E4A94` / `dword_1813E4A90` / `dword_1813E4ABC` and calls
//     `avs2_core_203()` for a RANDOM branch (`% 1000 < 5`, `% 100 < 10`). It is
//     a visual/particle spawner, not score: it cannot be reproduced
//     deterministically and must be excluded from score equivalence.
//   * `sub_180176790` (sound) and `sub_1803075A0` (popup dispatch) side effects.
//     Only their arguments are modelled. No game call is made here.
//   * Long/gliss/trill judges call the same rank_score slot with grades and
//     scores that are not in the exported evidence, so `row`/`scale` are
//     explicit caller inputs and grade 0 is refused (the normal judge never
//     emits it: 0x1802F6800 only ever passes 1, 3, 4 or a pending rank).
//   * `player+16`, `player+384` and the 3000-entry ring itself are game memory.
//     The caller tags records with its own note index instead.

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace nosfix {

// Grades of 0x180303E80 `a3`. Values 2..4 are the normal judge's Good/Great/
// Just. 1 is the late-miss rank. 5 is only produced by the note+84 override or
// by a non-normal judge.
enum class ScoreGrade : std::uint8_t {
    None = 0,    // reachable only from a non-normal caller; refused here
    Miss = 1,
    Good = 2,
    Great = 3,
    Just = 4,
    Special = 5, // event_state override / non-normal caller
};

[[nodiscard]] const char* ScoreGradeName(ScoreGrade grade) noexcept;

// Row count (event/chart +8 clamped to 0..4) and column count (grade 0..5) of
// the player score table.
inline constexpr std::size_t kScoreRowCount = 5;
inline constexpr std::size_t kScoreColumnCount = 6;

// Image-initialized limits, verified_0x1800cb9a0.c:
//   dword_1813668D8 = 9999  (max combo clamp)
//   dword_1813E470C = 0     (record ring disabled until a debug/config write)
//   dword_1813E4738 = 10    (popup base)
//   dword_1813E476C         (popup pitch; layout constant, not a judgment one)
//   dword_1813E4A88 = 3     (auto-slot timer value)
inline constexpr std::int32_t kScoreComboLimit = 9999;
inline constexpr std::int32_t kScoreCounterLimit = 9999;
inline constexpr std::int32_t kScoreHistoryCapacity = 3000;
inline constexpr std::int32_t kScorePopupBase = 10;
inline constexpr std::int32_t kScorePopupPitch = 505;

// A small fixed ring so a caller can inspect recent records without heap use.
inline constexpr std::size_t kScoreRingSize = 16;

struct NativeScoringConfig final {
    std::int32_t combo_limit = kScoreComboLimit;      // 0x13668D8
    std::int32_t counter_limit = kScoreCounterLimit;  // 0x1366900 sibling clamp
    std::int32_t history_capacity = kScoreHistoryCapacity;
    bool history_enabled = false;  // 0x13E470C; image init 0 == disabled
    std::int32_t popup_base = kScorePopupBase;   // 0x13E4738
    std::int32_t popup_pitch = kScorePopupPitch; // 0x13E476C

    friend bool operator==(const NativeScoringConfig&, const NativeScoringConfig&) = default;
};

enum class ScoreStatus : std::uint8_t {
    Ok = 0,
    NotConfigured = 1,
    NotScored = 2,      // a9 == 0: auto-play or chart+48 == 2; no state change
    GradeOutOfRange = 3, // grade 0 (or > 5): no normal-judge caller emits it
    InvalidConfig = 4,
    InvalidRow = 5,     // row outside 0..kScoreRowCount-1 after clamping is impossible; kept for callers
    Overflow = 6,       // a finite input whose derived value is not representable
};

[[nodiscard]] const char* ScoreStatusName(ScoreStatus status) noexcept;

// One record of the native 28-byte effect/score entry, reduced to the fields
// that are deterministic. `note_index` is the caller's own tag, not game data.
struct ScoreRingRecord final {
    std::uint32_t order = 0;
    std::uint32_t note_index = 0;
    std::uint8_t grade = 0;
    std::int32_t score = 0;
    std::int32_t combo = 0;
    std::int32_t row = 0;
    bool appended = false;
    std::uint32_t history_index = 0;
};

// Everything 0x180303E80 needs, named by the argument it came from.
struct ScoreInput final {
    std::uint8_t grade = 0;        // a3
    std::int32_t chart_group = 0;  // a7[2] = chart+8; the score row is clamp(chart+8, 0, 4)
    std::int32_t scale = 100;      // a8 (0x1802F6800 always passes 100)
    std::uint32_t event_state = 1; // a1 = note+84: the candidate slot that answered, 1..3
    bool scoring_enabled = true;   // a9 from 0x1802F79E0; see ScoreGate below
    std::int32_t left_key = 0;     // a7[22] = chart+88
    std::int32_t right_key = 0;    // a7[23] = chart+92
    std::uint32_t note_index = 0;  // caller tag, echoed into the effect
    // 0x180304619..0x18030465F effect-dispatch inputs. All four globals are 0 in
    // the default boot (judgment-303e80.md s8.3); a live read is still needed,
    // so each one is explicit and defaults to dispatch.
    bool auto_mode = false;      // HIDWORD(qword_1813E4730) == 1
    bool auto_play = false;      // byte_18136691C != 0
    bool effects_muted = false;  // HIBYTE(word_1813E5720) != 0
};

// The deterministic outcome of one 0x180303E80 call. `column_delta` is the
// change applied to the row's six int32 columns; it always sums to zero.
struct ScoreEffect final {
    ScoreStatus status = ScoreStatus::NotConfigured;
    std::uint32_t order = 0;        // monotonic per scoring state, starting at 1
    std::uint32_t note_index = 0;   // echoed from ScoreInput

    std::uint8_t grade = 0;         // grade after the note+84 override
    bool grade_overridden = false;  // the (event_state > 1 && grade > 1) branch
    std::int32_t score = 0;         // v16
    bool combo_incremented = false;
    bool combo_reset = false;
    std::int32_t combo_before = 0;
    std::int32_t combo_after = 0;
    std::int32_t max_combo_after = 0;

    std::uint8_t row = 0;                    // row actually used (clamped)
    std::array<std::int32_t, 6> column_delta{}; // sum == 0 by construction
    std::int32_t histogram_before = 0;       // player+22+2*grade
    std::int32_t histogram_after = 0;
    bool perfect_counted = false;            // player+156 + 2*(6*row+grade)
    std::uint8_t rank_state_before = 0;
    std::uint8_t rank_state_after = 0;
    bool history_appended = false;
    std::uint32_t history_index = 0;
    std::int32_t popup_units = 0;  // a2 handed to 0x1803075A0
    // 0x180304619..0x18030465F: whether the popup is dispatched at all.
    bool effect_dispatched = false;

    [[nodiscard]] bool ok() const noexcept { return status == ScoreStatus::Ok; }
};

// Single-owner scoring state. `Apply` is O(1), allocation-free and noexcept; it
// never throws and never fails to a partially-applied state: a rejected input
// leaves every counter untouched.
class NativeScoringState final {
public:
    NativeScoringState() = default;

    // Validates the limits. Rejected configuration leaves the previous state.
    [[nodiscard]] ScoreStatus Configure(const NativeScoringConfig& config) noexcept;

    // Clears combo, max combo, counters, rank tracker and the record ring.
    // The configuration is retained.
    void Reset() noexcept;

    // Applies one 0x180303E80 call. `out` is written only when the status is
    // Ok; on any rejection it is reset to a default-constructed effect with the
    // failure status and the state is unchanged.
    [[nodiscard]] ScoreStatus Apply(const ScoreInput& input, ScoreEffect& out) noexcept;

    // ---- inspection ----
    [[nodiscard]] bool IsConfigured() const noexcept { return configured_; }
    [[nodiscard]] std::int32_t Combo() const noexcept { return combo_; }
    [[nodiscard]] std::int32_t MaxCombo() const noexcept { return max_combo_; }
    [[nodiscard]] std::int32_t RankState() const noexcept { return rank_state_; }
    [[nodiscard]] std::int32_t GradeCounter(std::uint8_t grade) const noexcept {
        return grade < kScoreColumnCount ? grade_counters_[grade] : 0;
    }
    [[nodiscard]] std::int32_t RowColumn(std::size_t row, std::size_t column) const noexcept {
        return row < kScoreRowCount && column < kScoreColumnCount ? columns_[row][column] : 0;
    }
    [[nodiscard]] std::int32_t PerfectCount(std::size_t row, std::size_t column) const noexcept {
        return row < kScoreRowCount && column < kScoreColumnCount ? perfect_[row][column] : 0;
    }
    [[nodiscard]] std::uint32_t HistoryCount() const noexcept { return history_count_; }
    [[nodiscard]] std::uint32_t Order() const noexcept { return order_; }

    // Most recent appended records, newest last. Returns how many were copied.
    [[nodiscard]] std::size_t RecentRecords(std::span<ScoreRingRecord> out) const noexcept;

private:
    NativeScoringConfig config_{};
    std::array<std::int32_t, kScoreColumnCount> grade_counters_{};
    std::array<std::array<std::int32_t, kScoreColumnCount>, kScoreRowCount> columns_{};
    std::array<std::array<std::int32_t, kScoreColumnCount>, kScoreRowCount> perfect_{};
    std::array<ScoreRingRecord, kScoreRingSize> ring_{};
    std::int32_t combo_ = 0;
    std::int32_t max_combo_ = 0;
    std::int32_t rank_state_ = 0;
    std::uint32_t history_count_ = 0;
    std::uint32_t order_ = 0;
    std::size_t ring_head_ = 0;
    bool configured_ = false;
};

// 0x1802F79E0: a9 = (chart+48 != 2) && (chart+97 == 0). A false value makes
// 0x180303E80 return before touching combo, counters, score or rank state.
[[nodiscard]] constexpr bool ScoreGate(std::int32_t chart_field48,
                                      bool chart_auto) noexcept {
    return chart_field48 != 2 && !chart_auto;
}

// The rank tracker's transition, exposed for tests and for the shadow report.
// `grade` is the post-override grade; `state` is the current high dword of
// qword_1813668C8. Literal transcription of the switch at 0x180304565.
[[nodiscard]] std::uint8_t AdvanceRankState(std::uint8_t state, std::uint8_t grade) noexcept;

} // namespace nosfix
