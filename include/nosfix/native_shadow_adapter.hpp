#pragma once

// Native SHADOW FEED: the parent's live-adapter seam for judgement/scoring.
//
// It consumes three caller-decoded views and never touches game memory:
//   * the 28x28 input-state staging buffer published at 0x1813DEA10 by
//     sub_180305B30 (PRODUCER_FINDING.md), as 28 NativeBucketSlot values;
//   * the live input-state bucket table at 0x1813DEA20 (ABI_COMPLETION A1),
//     modelled as NativeBucketTable;
//   * the note table, as MatchCandidate views of the notes a lane can see.
//
// and produces the two record types the rest of the harness already compares:
// NativeNoteOutcome (the adapter's outcome vocabulary) and ScoreEffect (the
// deterministic 0x180303E80 transition of NativeScoringState).
//
// Frame order, matching the native update:
//   1. sub_180307090 rotates the bucket table down by one entry (0x307090);
//   2. sub_180307170 compacts the staging slots into entry 0 (0x307170);
//   3. 0x1802F7C10 consumes entry 0 slot by slot, one group per record.
// BeginFrame() supplies the song time of the input event; FeedFrame() runs the
// three steps; FeedLiveRecord() runs step 3 alone for a table the caller already
// decoded.
//
// Dormancy and refusal rules (nothing here can drive the game):
//   * Configure() refuses unless the caller supplies every evidence flag of
//     NativeShadowEvidence and a complete NativeMatchEvidence, and refuses a
//     group-phase request unless that phase is marked known.
//   * NativeShadowEvidence::slot_override_known and ::group_phase_known are the
//     two rules no sample has proven. They are feature gates, not Configure
//     gates: with them false (the default) the note+84 > 1 override is reported
//     as unproven instead of applied, and the compaction phase 2 stays off.
//   * kNativeMatchModelVerified stays false and NativeNoteState still refuses
//     NoteSelectionPolicy::Native2F7C10; this feed is a comparison source, never
//     a replacement for the game's traversal.
//   * Every call is noexcept and allocation-free: outcomes and effects are
//     written into caller spans whose size is checked up front, so a frame
//     refused before the rotation changes no state at all. Once the bucket stage
//     has run (rotation + compaction) it is not rolled back, and the scoring
//     state keeps whatever it has already applied.

#include "nosfix/native_match.hpp"
#include "nosfix/native_scoring.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace nosfix {

// ===========================================================================
// Evidence beyond NativeMatchEvidence
// ===========================================================================

struct NativeShadowEvidence final {
    bool bucket_layout_known = false;      // A1: 30 x 792, 28 x 7 dwords, count +784
    bool bucket_rotation_known = false;    // A1: 0x180307090
    bool bucket_compaction_known = false;  // A1/B6/PRODUCER: 0x180307170 and its source
    bool candidate_ordering_known = false; // A2: the seven-key comparator
    bool note_field_layout_known = false;  // A4: 0x1802F73B0
    bool song_unit_rate_known = false;     // A5: 1 unit = 1/60 s
    // Feature gates for the two rules that remain unproven. They are never part
    // of Complete(); a caller may enable them deliberately (a test, or the day a
    // live sample proves the rule) and nothing else changes.
    bool slot_override_known = false;      // note+84 > 1 -> grade 5   [live]
    bool group_phase_known = false;        // 0x307170 phase 2 window  [dec]

    [[nodiscard]] bool Complete() const noexcept {
        return bucket_layout_known && bucket_rotation_known &&
               bucket_compaction_known && candidate_ordering_known &&
               note_field_layout_known && song_unit_rate_known;
    }
    friend bool operator==(const NativeShadowEvidence&, const NativeShadowEvidence&) = default;
};

// ===========================================================================
// Feed status
// ===========================================================================

enum class ShadowFeedStatus : std::uint8_t {
    Ok = 0,
    NotConfigured = 1,
    EvidenceIncomplete = 2,
    InvalidMatchConfig = 3,
    InvalidScoringConfig = 4,
    GroupPhaseNotEvidenced = 5, // compact.group_phase_enabled without group_phase_known
    NonFiniteTime = 6,
    TimeBackwards = 7,          // the feed never rewinds: a new capture restarts it
    FrameNotOpen = 8,           // FeedLiveRecord/FeedFrame before BeginFrame
    InsufficientCapacity = 9,   // outcomes or effects span smaller than 28
    InvalidBucket = 10,
    InvalidStaging = 11,
    DecisionRefused = 12,       // SelectSlot refused a slot the feed considered new
    ScoreRejected = 13,         // NativeScoringState::Apply refused the input
};

[[nodiscard]] const char* ShadowFeedStatusName(ShadowFeedStatus status) noexcept;

// ===========================================================================
// Configuration, counters and records
// ===========================================================================

struct ShadowFeedConfig final {
    MatchConfig match{};              // windows, horizon, layout, unit rate
    NativeScoringConfig scoring{};    // 0x303E80 limits and rings
    NativeBucketCompactConfig compact{};   // 0x307170 options
    NativeMatchEvidence match_evidence{};  // R1 runtime gate, forwarded as-is
    NativeShadowEvidence evidence{};
    // 0x180304619..0x18030465F effect-dispatch gates. All three are 0 in a
    // default boot (judgment-303e80 s8.3) and stay explicit.
    bool auto_mode = false;      // HIDWORD(qword_1813E4730) == 1
    bool auto_play = false;      // byte_18136691C != 0
    bool effects_muted = false;  // HIBYTE(word_1813E5720) != 0
};

struct ShadowFeedCounts final {
    std::uint32_t frames = 0;
    std::uint32_t rotations = 0;
    std::uint32_t staging_slots = 0;
    std::uint32_t compacted_slots = 0;
    std::uint32_t decisions = 0;
    std::uint32_t judged = 0;
    std::uint32_t linked = 0;
    std::uint32_t scored = 0;         // rank Good/Great/Just
    std::uint32_t late_presses = 0;   // rank Missed: consumes, combo resets
    std::uint32_t early_ignored = 0;  // rank TooEarly: consumes nothing
    std::uint32_t releases = 0;       // state 3 keysound cuts
    std::uint32_t no_candidate = 0;   // a new slot that armed nothing
    std::uint32_t not_scored = 0;     // a9 == 0: chart+48 == 2, 0x303E80 returns early
    std::uint32_t slot_override_unproven = 0; // note+84 > 1 left unapplied
};

// One evaluated live-record slot, kept in a small fixed ring for inspection and
// for the report. `slot_override_unproven` marks exactly the case the scoring
// rule cannot decide yet.
struct ShadowFeedRecord final {
    std::uint32_t order = 0;
    std::uint32_t note_index = 0;
    std::uint8_t lane = 0;
    std::uint8_t state = 0;
    std::uint8_t rank = 0;
    std::uint32_t event_state = 1;
    bool linked = false;
    bool slot_override_unproven = false;
    double at_us = 0.0;
};

inline constexpr std::size_t kShadowFeedRingSize = 32;

class NativeShadowFeed final {
public:
    NativeShadowFeed() = default;

    [[nodiscard]] ShadowFeedStatus Configure(const ShadowFeedConfig& config) noexcept;

    // Opens one frame at the song time of the input event. Non-decreasing times
    // only; the frame is opened only when the call returns Ok.
    [[nodiscard]] ShadowFeedStatus BeginFrame(double now_us) noexcept;

    // Rotates, compacts `staging` into entry 0 and evaluates that record.
    // `outcomes` and `effects` must each hold kNativeBucketSlotCount entries;
    // otherwise the call is a no-op with InsufficientCapacity.
    [[nodiscard]] ShadowFeedStatus FeedFrame(std::span<const NativeBucketSlot> staging,
                                             std::span<const MatchCandidate> notes,
                                             std::span<NativeNoteOutcome> outcomes,
                                             std::size_t& outcome_count,
                                             std::span<ScoreEffect> effects,
                                             std::size_t& effect_count) noexcept;

    // Evaluates entry 0 exactly as the traversal would, without rotating or
    // compacting. Same capacity contract.
    [[nodiscard]] ShadowFeedStatus FeedLiveRecord(std::span<const MatchCandidate> notes,
                                                  std::span<NativeNoteOutcome> outcomes,
                                                  std::size_t& outcome_count,
                                                  std::span<ScoreEffect> effects,
                                                  std::size_t& effect_count) noexcept;

    // Replaces the modelled table with a caller-decoded sample so a captured
    // bucket can be replayed without compaction. Refuses (and keeps the current
    // table) unless the sample validates.
    [[nodiscard]] ShadowFeedStatus LoadBucket(const NativeBucketTable& table) noexcept;

    // ---- inspection ----
    [[nodiscard]] bool IsConfigured() const noexcept { return configured_; }
    [[nodiscard]] bool ModelVerified() const noexcept { return kNativeMatchModelVerified; }
    [[nodiscard]] double Now() const noexcept { return now_us_; }
    [[nodiscard]] ShadowFeedStatus LastStatus() const noexcept { return last_status_; }
    [[nodiscard]] const NativeBucketTable& Bucket() const noexcept { return bucket_; }
    [[nodiscard]] NativeMatchPolicy& Policy() noexcept { return policy_; }
    [[nodiscard]] const NativeMatchPolicy& Policy() const noexcept { return policy_; }
    [[nodiscard]] const NativeScoringState& Scoring() const noexcept { return scoring_; }
    [[nodiscard]] const ShadowFeedCounts& Counts() const noexcept { return counts_; }
    [[nodiscard]] const NativeBucketCompactReport& LastCompact() const noexcept {
        return last_compact_;
    }

    // Copies the most recent records, oldest first. Returns how many were copied.
    [[nodiscard]] std::size_t RecentRecords(std::span<ShadowFeedRecord> out) const noexcept;

private:
    [[nodiscard]] ShadowFeedStatus EvaluateLiveRecord(std::span<const MatchCandidate> notes,
                                                      std::span<NativeNoteOutcome> outcomes,
                                                      std::size_t& outcome_count,
                                                      std::span<ScoreEffect> effects,
                                                      std::size_t& effect_count) noexcept;
    void PushRecord(const ShadowFeedRecord& record) noexcept;

    ShadowFeedConfig config_{};
    NativeBucketTable bucket_{};
    NativeMatchPolicy policy_{};
    NativeScoringState scoring_{};
    ShadowFeedCounts counts_{};
    NativeBucketCompactReport last_compact_{};
    std::array<ShadowFeedRecord, kShadowFeedRingSize> records_{};
    std::size_t record_head_ = 0;
    std::size_t record_count_ = 0;
    std::uint32_t record_order_ = 0;
    double now_us_ = 0.0;
    ShadowFeedStatus last_status_ = ShadowFeedStatus::NotConfigured;
    bool configured_ = false;
    bool frame_open_ = false;
};

// ===========================================================================
// Note view builder
// ===========================================================================

// Fills the feed's candidate view of one normal note from the decoded A4 fields.
// `event_lane` and `event_kind` stay 0: SelectSlot takes both from the bucket
// slot, which IS the native event descriptor.
[[nodiscard]] NoteDecodeStatus MakeMatchCandidate(const NativeChartFields& chart,
                                                  const NativeNoteFields& note,
                                                  std::uint32_t note_index,
                                                  double units_per_second,
                                                  MatchCandidate& out) noexcept;

} // namespace nosfix
