#pragma once

// Native normal-note MATCHING seam: the per-lane selection rules of the gameplay
// traversal at VA 0x1802F7C10 plus its horizon helpers 0x180303BA0 /
// 0x180303C10, and a shadow comparator that can be validated without enabling
// any hook.
//
// Evidence base (analyze/evidence/, all offsets module-relative RVAs of
// nostalgia.dll PAN-2025020501):
//   verified_0x1802f7c10.c   traversal, 2729 lines of pseudocode
//   verified_0x180303ba0.c   per-lane horizon admission
//   verified_0x180303c10.c   per-lane history write
//   verified_0x180303b50.c   lazily constructed debug/state singleton
//   verified_0x1802f6800.c   judge (called at 0x1802F954D)
//   (no export for 0x1802F79C0; its auto-slot body is inlined in the traversal
//    at 0x1802F93BA..0x1802F93D1 and reproduced here)
//   verified_0x1800cb9a0.c   image initial values of every global used here
//
// The seam is DORMANT and FAILS CLOSED:
//   * `NativeMatchPolicy::Configure` refuses unless the caller supplies a
//     complete evidence record, a validated post-initialization window sample,
//     a sampled horizon parameter set and a sampled layout. A default-constructed
//     policy reports every decision as NotConfigured.
//   * `kNativeMatchModelVerified` is false. It may only be flipped after the
//     coordination answers named in analyze/impl-20261030/judgment-REPORT.md
//     exist and a live shadow run agrees; nothing in this stream sets it.
//   * `NativeNoteState` still refuses NoteSelectionPolicy::Native2F7C10, so no
//     configuration file, ini value or environment variable can reach this code.
//
// No game memory is read or written, no detour is installed, and no allocation
// happens on the decision path. All state is single-owner.

#include "nosfix/native_note_state.hpp"
#include "nosfix/normal_judgment.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace nosfix {

// ===========================================================================
// Judgment window table (0x13668E0 .. 0x1366904)
// ===========================================================================

// The image static block is ZERO before sub_1800CB9A0 runs and is seeded by that
// initializer to 9, -11, 7, -6, 4, -4, 2, -2 plus the +-9999 outer clamps
// (0x1800CC120..0x1800CC184). A zero block is therefore proof that the sample
// was taken too early, and is refused rather than read as "everything is inside
// every window".
enum class WindowReadStatus : std::uint8_t {
    Ok = 0,
    Unavailable = 1,             // span does not cover 0x1366904
    NotPostInitialization = 2,   // caller has not proven the globals were written
    ZeroStaticBlock = 3,         // the eight globals at 0x13668E8..0x1366904 are all zero
    InvalidRate = 4,             // units_per_second not finite/positive
    InvalidUnits = 5,            // sign/nesting/convert failure
    InconsistentEffectBand = 6,  // effect band or outer clamp outside its tier
};

[[nodiscard]] const char* WindowReadStatusName(WindowReadStatus status) noexcept;

struct JudgmentWindowSample final {
    WindowReadStatus status = WindowReadStatus::Unavailable;
    native_layout::NativeWindowUnits units{};
    NormalWindows windows{};
    double units_per_second = 0.0;
    std::int32_t outer_earliest_clamp = 0; // 0x1366900, image init 9999
    std::int32_t outer_latest_clamp = 0;   // 0x1366904, image init -9999
    bool matches_initial_seed = false;     // equal to the initializer's eight values

    [[nodiscard]] bool ok() const noexcept { return status == WindowReadStatus::Ok; }
};

// Reads the eight int32 globals at 0x13668E8..0x1366904 (plus the effect band at
// 0x13668E0/0x13668E4) from a caller-owned image span and converts them with the
// caller's explicit units-per-second rate. The caller may pass
// post_initialization_sample == true only after it has observed the game's
// initializer run; the reader refuses otherwise and refuses an all-zero block.
[[nodiscard]] JudgmentWindowSample SampleJudgmentWindows(
    std::span<const std::byte> image, double units_per_second,
    bool post_initialization_sample) noexcept;

// ===========================================================================
// Horizon / lane-history parameters (0x180303BA0 / 0x180303C10)
// ===========================================================================

// 0x13E4978 is zeroed for 232 bytes by the initializer: 29 entries of
// {int32 stamp, int32 value}. Coordinates request judgment-001 item 1 and the
// 2f7c10 evidence pack s1 prove the array is 29 x 8, that index 0 is NEVER a
// lane (the downward writer stops at lane-4 >= 0 and the upward writer is
// bounded by 0x1813E4A5C = base+228), and that the 28 playable keys are
// numbered 1..28. Lane 0 and lane > 28 are therefore refused, not clamped.
inline constexpr std::size_t kNativeLaneHistorySlots = 29;

// Image initial values (0x1800CC2C4..0x1800CC2E9). These are SEEDS: the seam
// accepts them only as a caller-supplied live sample, never as hardcoded truth.
inline constexpr std::int32_t kInitialHistoryStamp = 3;  // 0x13E4968
inline constexpr std::int32_t kInitialHistoryWindow = 1; // 0x13E496C
inline constexpr std::int32_t kInitialLaneSpread = 4;    // 0x13E4A60
inline constexpr std::int32_t kInitialLaneCooldown = 10; // 0x13E4974

struct NativeHorizonConfig final {
    std::int32_t history_stamp = kInitialHistoryStamp;   // 0x13E4968
    std::int32_t history_window = kInitialHistoryWindow; // 0x13E496C
    std::int32_t lane_spread = kInitialLaneSpread;       // 0x13E4A60
    std::int32_t lane_cooldown = kInitialLaneCooldown;   // 0x13E4974
    bool sampled = false; // true only for a live post-initialization sample

    friend bool operator==(const NativeHorizonConfig&, const NativeHorizonConfig&) = default;
};

// ===========================================================================
// Evidence gate
// ===========================================================================

// One boolean per decoded fact the model below depends on. Every field must be
// true before the policy will configure; see the report for the exact pseudocode
// behind each one and for the ones that are still unproven.
struct NativeMatchEvidence final {
    bool horizon_writers_known = false;      // who writes 0x13E4968/6C/A60 after init
    bool pair_record_layout_known = false;   // the 64-byte 2F7C10 pair records
    bool bucket_table_known = false;         // 0x13DEA20, stride 792, count at +784
    bool linked_object_known = false;        // note+96 object, fields +80 and +96
    bool linked_pairs_never_judged = false;  // 0x1802F9366..0x1802F9413 returns early
    bool slot_priority_known = false;        // the three armed slots at note+1216/1240/1264
    bool per_group_masks_known = false;      // v558 / v563, reset per group
    bool horizon_push_sites_known = false;   // the four 303C10 call sites
    bool auto_slot_rule_known = false;       // note+96 + note+104 in 0x1802F6800
    bool over_filter_rule_known = false;     // lane over-filter at 0x1802F8CB9

    [[nodiscard]] bool Complete() const noexcept {
        return horizon_writers_known && pair_record_layout_known &&
               bucket_table_known && linked_object_known &&
               linked_pairs_never_judged && slot_priority_known &&
               per_group_masks_known && horizon_push_sites_known &&
               auto_slot_rule_known && over_filter_rule_known;
    }
    friend bool operator==(const NativeMatchEvidence&, const NativeMatchEvidence&) = default;
};

// Compile-time readiness. False: the 2F7C10 selection path and its horizon
// globals are not fully proven (coordination request judgment-001 items 1..6).
// Flipping this without a live shadow run would let a partial model judge notes.
inline constexpr bool kNativeMatchModelVerified = false;

// ===========================================================================
// Matching model
// ===========================================================================

enum class MatchStatus : std::uint8_t {
    Ok = 0,
    NotConfigured = 1,      // Configure() was never accepted, or BeginUpdate missing
    EvidenceIncomplete = 2,
    WindowsNotSampled = 3,
    InvalidWindows = 4,
    HorizonNotSampled = 5,
    InvalidHorizon = 6,
    LayoutNotSampled = 7,   // 0x13E5724 / 0x13E4A84 were not sampled
    InvalidRate = 8,
    NonFiniteTime = 9,
    InvalidLane = 10,
};

[[nodiscard]] const char* MatchStatusName(MatchStatus status) noexcept;

// Which of the three armed slots of a bucket entry a candidate belongs to. The
// native code arms v1+1216 (inside the widened key range), v1+1240 (left of it)
// and v1+1264 (right of it) and then evaluates the slots in that order.
enum class MatchSlot : std::uint8_t {
    SameKey = 0,
    LeftOfRange = 1,
    RightOfRange = 2,
};

[[nodiscard]] const char* MatchSlotName(MatchSlot slot) noexcept;

// One (note, event) pair of one bucket, as the caller decoded it from game
// memory. Field names carry the note/event offset each value came from. Nothing
// here is dereferenced by this module.
struct MatchCandidate final {
    std::uint32_t note = 0;         // caller note index (echoed back)
    double note_us = 0.0;           // note+24 song units, converted to microseconds
    std::int32_t left_key = 0;      // note+44 (vtable+0x40)
    std::int32_t right_key = 0;     // note+48 (vtable+0x48)
    float mid_key = 0.0f;           // note+56 (vtable+0x58, returned as float)
    bool auto_note = false;         // chart+97 (vtable+0xC0)
    std::uint8_t chart_type = 0;    // chart+8
    bool done = false;              // note+68 (vtable+0xB8)
    bool pending_early = false;     // note+76 (vtable+0xE0)
    std::uint8_t event_lane = 0;    // event+0
    std::uint8_t event_kind = 0;    // event+4: 1 press, 3 release
    bool linked = false;            // pair+48 partner note pointer present
    bool linked_open = false;       // *(partner + 80) <= 0 (partner still open)
    bool consumed = false;          // chart+84 > 0: already taken by a judge
    bool auto_slot_empty = true;    // note+96 == 0 (armed only when empty)
};

enum class MatchAction : std::uint8_t {
    None = 0,
    Judge = 1,       // 0x1802F954D is reached: the judge virtual may run
    LinkedState = 2, // linked pair: event state + auto slot + horizon, never judged
};

struct MatchDecision final {
    MatchStatus status = MatchStatus::NotConfigured;
    MatchAction action = MatchAction::None;
    std::uint32_t note = 0;   // candidate index chosen (0 when action == None)
    MatchSlot slot = MatchSlot::SameKey;
    std::uint8_t lane = 0;    // bucket entry lane
    double at_us = 0.0;       // song time of the decision (note time)

    bool horizon_ok = false;    // 0x180303BA0 returned non-zero
    bool lane_masked = false;   // v558[event lane] already set in this group
    bool lane_resolved = false; // v563[event lane] already set in this group
    bool skipped_done = false;  // note+68 != 0
    bool skipped_pending_early = false; // note+76 == 1 && event_state != 1
    bool skipped_consumed = false;      // chart+84 > 0
    bool skipped_invalid_lane = false;  // event lane outside 1..28
    bool auto_slot_armed = false;       // note+96 / note+104 would be stored
    // The record's slot index, 1..3, stored into note+84 through vtable+0xE8.
    // judgment-001 item 6: this is the candidate slot that answered, so 1 means
    // the primary slot and 2/3 mean a secondary one.
    std::uint32_t event_state = 1;

    bool horizon_pushed = false;
    double horizon_push_us = 0.0;
    std::uint8_t horizon_push_first_lane = 0;
    std::uint8_t horizon_push_last_lane = 0;

    [[nodiscard]] bool ok() const noexcept { return status == MatchStatus::Ok; }
};

struct MatchConfig final {
    NativeHorizonConfig horizon{};
    NormalWindows windows{};             // sampled 0x13668E8..0x1366904
    double judgment_offset_us = 0.0;     // 0x136690C
    double visual_offset_us = 0.0;       // 0x1366918 (only for the type-4 push)
    double units_per_second = 60.0;      // explicit legacy unit rate
    std::int32_t effect_early_units = 9; // 0x13668E0, used by the type-4 push
    std::int32_t lane_over_filter = 3;   // 0x13E4A84
    float key_spread = 0.4f;             // 0x13E5724 (not in the exported initializer)
    bool layout_sampled = false;         // caller read 0x13E5724 / 0x13E4A84 live
};

class NativeMatchPolicy final {
public:
    NativeMatchPolicy() = default;

    // Refuses unless evidence.Complete(), the window table is sampled and
    // nested, the horizon parameters are sampled and in range, the layout is
    // sampled and the rate is explicit. A refusal leaves the policy unconfigured.
    [[nodiscard]] MatchStatus Configure(const MatchConfig& config,
                                        const NativeMatchEvidence& evidence) noexcept;

    // Opens one traversal pass. The per-lane flags are NOT reset here; the
    // native code clears them inside the bucket loop (0x1802F9194).
    void BeginUpdate(double now_us) noexcept;

    // Resets the two 29-byte per-lane flag arrays (v558 / v563), matching the
    // `for i in 0..28 { v558[i] = 0; v563[i] = 0; }` loop at 0x1802F9194. The
    // evidence pack reads them as per-event-batch flags; the decompiler clears
    // them inside the bucket loop, so a caller that models one bucket per batch
    // may call this once per batch and get the same behaviour.
    void BeginGroup() noexcept;

    // Evaluates one bucket entry (one lane) against its candidate pairs and
    // returns the decision the traversal would take. Candidates must already be
    // in the bucket's recorded order. The closest-in-time candidate per slot
    // wins (ties: smaller note+56), then the slots are tried in order SameKey,
    // LeftOfRange, RightOfRange.
    [[nodiscard]] MatchDecision Select(std::uint8_t lane,
                                       std::span<const MatchCandidate> candidates) noexcept;

    // Applies the post-judge state transition for the pair a Select() returned:
    // pushes the lane horizon when the judge consumed the note (or post_judge()
    // reported one) and the event is a press. Nothing is pushed for a release,
    // for a miss and for a TooEarly/ignored press.
    void CommitJudge(const MatchCandidate& candidate, bool scored,
                     bool post_judged) noexcept;

    // ---- inspection ----
    [[nodiscard]] bool IsConfigured() const noexcept { return configured_; }
    [[nodiscard]] bool ModelVerified() const noexcept { return kNativeMatchModelVerified; }
    [[nodiscard]] double Now() const noexcept { return now_us_; }
    [[nodiscard]] std::uint32_t GroupCount() const noexcept { return groups_; }
    [[nodiscard]] std::array<std::int32_t, kNativeLaneHistorySlots> HistoryStamps()
        const noexcept { return history_stamp_; }
    [[nodiscard]] std::array<double, kNativeLaneHistorySlots> HistoryValues()
        const noexcept { return history_value_; }
    [[nodiscard]] bool LaneMasked(std::uint8_t lane) const noexcept {
        return lane < kNativeLaneHistorySlots ? masked_[lane] : false;
    }
    [[nodiscard]] bool LaneResolved(std::uint8_t lane) const noexcept {
        return lane < kNativeLaneHistorySlots ? resolved_[lane] : false;
    }

    // 0x180303BA0, exposed: true when the candidate may be judged in this lane.
    [[nodiscard]] bool HorizonAllows(double candidate_us, std::uint8_t lane) const noexcept;

private:
    void PushHorizon(double value_us, std::uint8_t lane) noexcept;
    void ResetGroupMasks() noexcept;
    [[nodiscard]] double JudgmentNowUs() const noexcept {
        return config_.judgment_offset_us + config_.visual_offset_us + now_us_;
    }
    [[nodiscard]] double Type4PushUs() const noexcept;
    [[nodiscard]] std::uint8_t LanesTouchedFirst(std::uint8_t lane) const noexcept;
    [[nodiscard]] std::uint8_t LanesTouchedLast(std::uint8_t lane) const noexcept;

    MatchConfig config_{};
    NativeMatchEvidence evidence_{};
    std::array<std::int32_t, kNativeLaneHistorySlots> history_stamp_{};
    std::array<double, kNativeLaneHistorySlots> history_value_{};
    std::array<bool, kNativeLaneHistorySlots> masked_{};
    std::array<bool, kNativeLaneHistorySlots> resolved_{};
    double now_us_ = 0.0;
    std::uint32_t groups_ = 0;
    bool configured_ = false;
    bool update_open_ = false;
};

// ===========================================================================
// Shadow comparison
// ===========================================================================

enum class ShadowField : std::uint8_t {
    None = 0,
    Kind = 1,
    NoteIndex = 2,
    Lane = 3,
    Rank = 4,
    Score = 5,
    Effect = 6,
    Time = 7,
    Order = 8,
};

[[nodiscard]] const char* ShadowFieldName(ShadowField field) noexcept;

// One outcome of either engine, normalized to comparable integers. `score` is
// the running score after the outcome for engines that track one, or 0.
struct ShadowOutcome final {
    std::uint32_t note_index = 0;
    std::uint8_t lane = 0;
    std::uint8_t kind = 0;          // NoteOutcomeKind / MatchAction
    std::uint8_t rank = 0;          // NormalRank / ScoreGrade
    std::uint8_t effect_grade = 0;
    std::int32_t score = 0;
    double at_us = 0.0;             // compared with an explicit tolerance
};

struct ShadowDivergence final {
    bool diverged = false;
    std::uint64_t index = 0;          // ordinal of the first differing outcome
    ShadowField field = ShadowField::None;
    double at_us = 0.0;               // song time where the two first differ
    std::uint32_t note_index = 0;
    std::uint8_t adapter_kind = 0;
    std::uint8_t native_kind = 0;
    std::uint8_t adapter_rank = 0;
    std::uint8_t native_rank = 0;
    std::int32_t adapter_score = 0;
    std::int32_t native_score = 0;
    std::uint8_t adapter_effect = 0;
    std::uint8_t native_effect = 0;

    [[nodiscard]] bool ok() const noexcept { return !diverged; }
};

// Compares two outcome streams produced from the same captured input and reports
// the FIRST divergence. The parent's live-validation seam feeds the existing
// adapter stream and the native stream through Observe() in order; it never
// needs to enable a detour.
class NativeShadowComparison final {
public:
    // Time tolerance in microseconds. 0 compares times exactly.
    void Configure(double time_tolerance_us = 0.0) noexcept;
    void Reset() noexcept;

    // Pushes one outcome from each engine. Once a divergence is recorded the
    // comparator latches: further calls return the same record and compare
    // nothing. Returns the current state.
    ShadowDivergence Observe(const ShadowOutcome& adapter,
                             const ShadowOutcome& native) noexcept;

    [[nodiscard]] const ShadowDivergence& Divergence() const noexcept { return divergence_; }
    [[nodiscard]] std::uint64_t Compared() const noexcept { return compared_; }
    [[nodiscard]] bool Diverged() const noexcept { return divergence_.diverged; }

    // Normalizes the adapter's own outcome type so callers do not repeat it.
    [[nodiscard]] static ShadowOutcome FromAdapter(const NativeNoteOutcome& outcome,
                                                   std::int32_t running_score = 0) noexcept;

private:
    ShadowDivergence divergence_{};
    double time_tolerance_us_ = 0.0;
    std::uint64_t compared_ = 0;
};

} // namespace nosfix
