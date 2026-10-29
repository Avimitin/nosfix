#include "nosfix/native_shadow_adapter.hpp"

#include <cmath>

namespace nosfix {

namespace {

constexpr double kMaximumTimeUs = 1.0e12;

bool FiniteTime(double value) noexcept {
    return std::isfinite(value) && std::abs(value) <= kMaximumTimeUs;
}

} // namespace

const char* ShadowFeedStatusName(ShadowFeedStatus status) noexcept {
    switch (status) {
    case ShadowFeedStatus::Ok: return "ok";
    case ShadowFeedStatus::NotConfigured: return "not-configured";
    case ShadowFeedStatus::EvidenceIncomplete: return "evidence-incomplete";
    case ShadowFeedStatus::InvalidMatchConfig: return "invalid-match-config";
    case ShadowFeedStatus::InvalidScoringConfig: return "invalid-scoring-config";
    case ShadowFeedStatus::GroupPhaseNotEvidenced: return "group-phase-not-evidenced";
    case ShadowFeedStatus::NonFiniteTime: return "non-finite-time";
    case ShadowFeedStatus::TimeBackwards: return "time-backwards";
    case ShadowFeedStatus::FrameNotOpen: return "frame-not-open";
    case ShadowFeedStatus::InsufficientCapacity: return "insufficient-capacity";
    case ShadowFeedStatus::InvalidBucket: return "invalid-bucket";
    case ShadowFeedStatus::InvalidStaging: return "invalid-staging";
    case ShadowFeedStatus::DecisionRefused: return "decision-refused";
    case ShadowFeedStatus::ScoreRejected: return "score-rejected";
    }
    return "invalid";
}

NoteDecodeStatus MakeMatchCandidate(const NativeChartFields& chart, const NativeNoteFields& note,
                                    std::uint32_t note_index, double units_per_second,
                                    MatchCandidate& out) noexcept {
    if (!std::isfinite(units_per_second) || units_per_second <= 0.0 ||
        units_per_second > 1.0e6) {
        return NoteDecodeStatus::InvalidSongUnits;
    }
    // chart+8 selects one of the five score rows (clamp(chart+8, 0, 4)); a record
    // outside that range would index a table this model does not have.
    if (chart.group < 0 || chart.group > 4) return NoteDecodeStatus::InvalidGroup;
    // note+168 comes from sub_180301CB0(chart+48, chart+0x30), which is not
    // decoded; the candidate view therefore carries no keysound handle, and
    // NativeNoteFields::keysound_index_known says so instead of claiming one.
    const double note_us = SongUnitsToUs(static_cast<double>(chart.frame), units_per_second);
    if (!FiniteTime(note_us)) return NoteDecodeStatus::InvalidSongUnits;

    MatchCandidate candidate;
    candidate.note = note_index;
    candidate.note_us = note_us;
    candidate.left_key = note.left_key;
    candidate.right_key = note.right_key;
    candidate.mid_key = note.mid_key;
    candidate.auto_note = chart.auto_play;
    candidate.chart_type = static_cast<std::uint8_t>(chart.group);
    candidate.done = false;
    candidate.pending_early = false;
    candidate.event_lane = 0; // SelectSlot takes the lane from the bucket slot
    candidate.event_kind = 0; // idem for the event kind
    candidate.linked = false;
    candidate.linked_open = false;
    candidate.consumed = false;
    candidate.auto_slot_empty = true;
    candidate.keysound_id = chart.keysound_id;
    out = candidate;
    return NoteDecodeStatus::Ok;
}

// ===========================================================================
// NativeShadowFeed
// ===========================================================================

ShadowFeedStatus NativeShadowFeed::Configure(const ShadowFeedConfig& config) noexcept {
    if (!config.evidence.Complete()) return ShadowFeedStatus::EvidenceIncomplete;
    if (config.compact.group_phase_enabled && !config.evidence.group_phase_known) {
        return ShadowFeedStatus::GroupPhaseNotEvidenced;
    }
    const MatchStatus match = policy_.Configure(config.match, config.match_evidence);
    if (match != MatchStatus::Ok) return ShadowFeedStatus::InvalidMatchConfig;
    if (scoring_.Configure(config.scoring) != ScoreStatus::Ok) {
        return ShadowFeedStatus::InvalidScoringConfig;
    }
    config_ = config;
    bucket_.Clear();
    counts_ = ShadowFeedCounts{};
    last_compact_ = NativeBucketCompactReport{};
    records_.fill(ShadowFeedRecord{});
    record_head_ = 0;
    record_count_ = 0;
    record_order_ = 0;
    now_us_ = 0.0;
    frame_open_ = false;
    last_status_ = ShadowFeedStatus::Ok;
    configured_ = true;
    return last_status_;
}

ShadowFeedStatus NativeShadowFeed::BeginFrame(double now_us) noexcept {
    if (!configured_) {
        last_status_ = ShadowFeedStatus::NotConfigured;
        return last_status_;
    }
    if (!FiniteTime(now_us)) {
        frame_open_ = false;
        last_status_ = ShadowFeedStatus::NonFiniteTime;
        return last_status_;
    }
    if (counts_.frames != 0 && now_us < now_us_) {
        frame_open_ = false;
        last_status_ = ShadowFeedStatus::TimeBackwards;
        return last_status_;
    }
    now_us_ = now_us;
    policy_.BeginUpdate(now_us);
    frame_open_ = true;
    ++counts_.frames;
    last_status_ = ShadowFeedStatus::Ok;
    return last_status_;
}

ShadowFeedStatus NativeShadowFeed::FeedFrame(std::span<const NativeBucketSlot> staging,
                                             std::span<const MatchCandidate> notes,
                                             std::span<NativeNoteOutcome> outcomes,
                                             std::size_t& outcome_count,
                                             std::span<ScoreEffect> effects,
                                             std::size_t& effect_count) noexcept {
    if (!configured_) {
        last_status_ = ShadowFeedStatus::NotConfigured;
        return last_status_;
    }
    if (!frame_open_) {
        last_status_ = ShadowFeedStatus::FrameNotOpen;
        return last_status_;
    }
    // Capacity is checked before anything mutates, so a refusal leaves both the
    // bucket table and the counters untouched.
    if (outcomes.size() < kNativeBucketSlotCount ||
        effects.size() < kNativeBucketSlotCount) {
        last_status_ = ShadowFeedStatus::InsufficientCapacity;
        return last_status_;
    }
    if (staging.size() != kNativeBucketSlotCount) {
        last_status_ = ShadowFeedStatus::InvalidStaging;
        return last_status_;
    }
    NativeBucketCompactReport report;
    const BucketStatus compact = bucket_.Advance(staging, config_.compact, report);
    if (compact != BucketStatus::Ok) {
        last_status_ = compact == BucketStatus::InvalidStaging
                           ? ShadowFeedStatus::InvalidStaging
                           : ShadowFeedStatus::InvalidBucket;
        return last_status_;
    }
    last_compact_ = report;
    ++counts_.rotations;
    counts_.staging_slots += static_cast<std::uint32_t>(report.staging_slots);
    counts_.compacted_slots += static_cast<std::uint32_t>(report.appended);
    return EvaluateLiveRecord(notes, outcomes, outcome_count, effects, effect_count);
}

ShadowFeedStatus NativeShadowFeed::FeedLiveRecord(std::span<const MatchCandidate> notes,
                                                  std::span<NativeNoteOutcome> outcomes,
                                                  std::size_t& outcome_count,
                                                  std::span<ScoreEffect> effects,
                                                  std::size_t& effect_count) noexcept {
    if (!configured_) {
        last_status_ = ShadowFeedStatus::NotConfigured;
        return last_status_;
    }
    if (!frame_open_) {
        last_status_ = ShadowFeedStatus::FrameNotOpen;
        return last_status_;
    }
    if (outcomes.size() < kNativeBucketSlotCount ||
        effects.size() < kNativeBucketSlotCount) {
        last_status_ = ShadowFeedStatus::InsufficientCapacity;
        return last_status_;
    }
    return EvaluateLiveRecord(notes, outcomes, outcome_count, effects, effect_count);
}

ShadowFeedStatus NativeShadowFeed::LoadBucket(const NativeBucketTable& table) noexcept {
    if (!configured_) {
        last_status_ = ShadowFeedStatus::NotConfigured;
        return last_status_;
    }
    if (table.Validate() != BucketStatus::Ok) {
        last_status_ = ShadowFeedStatus::InvalidBucket;
        return last_status_;
    }
    bucket_ = table;
    last_status_ = ShadowFeedStatus::Ok;
    return last_status_;
}

ShadowFeedStatus NativeShadowFeed::EvaluateLiveRecord(std::span<const MatchCandidate> notes,
                                                      std::span<NativeNoteOutcome> outcomes,
                                                      std::size_t& outcome_count,
                                                      std::span<ScoreEffect> effects,
                                                      std::size_t& effect_count) noexcept {
    const NativeBucketEntry& live = bucket_.Live();
    if (live.count < 0 ||
        live.count > static_cast<std::int32_t>(kNativeBucketSlotCount) ||
        !live.valid()) {
        last_status_ = ShadowFeedStatus::InvalidBucket;
        return last_status_;
    }
    outcome_count = 0;
    effect_count = 0;
    // 0x1802F9194: the v558/v563 flags are cleared per bucket record, so one
    // record is one group.
    policy_.BeginGroup();

    for (std::int32_t index = 0; index < live.count; ++index) {
        const std::uint32_t slot_index = static_cast<std::uint32_t>(index);
        const NativeBucketSlot& slot = live.slots[static_cast<std::size_t>(index)];
        if (slot.state == static_cast<std::int32_t>(NativeSlotState::Free) ||
            slot.state == static_cast<std::int32_t>(NativeSlotState::Held)) {
            // Idle or still held: the traversal only reads the value byte for
            // these states, which is not a judgement.
            continue;
        }
        if (slot.state == static_cast<std::int32_t>(NativeSlotState::Release)) {
            // State 3 only gates the keysound (0x1802BCAD0 at 0x1802F... 1707);
            // it never reaches a judge.
            NativeNoteOutcome outcome;
            outcome.note_index = 0;
            outcome.lane = static_cast<std::uint8_t>(slot.key);
            outcome.kind = NoteOutcomeKind::KeysoundCut;
            outcome.rank = NormalRank::TooEarly;
            outcome.effect_grade = 0;
            outcome.at_us = now_us_;
            outcomes[outcome_count++] = outcome;
            ++counts_.releases;
            ShadowFeedRecord record;
            record.order = ++record_order_;
            record.lane = static_cast<std::uint8_t>(slot.key);
            record.state = static_cast<std::uint8_t>(slot.state);
            record.at_us = now_us_;
            PushRecord(record);
            continue;
        }

        const MatchDecision decision =
            policy_.SelectSlot(bucket_, static_cast<std::uint32_t>(kNativeLiveBucketEntry),
                               slot_index, notes);
        ++counts_.decisions;
        if (decision.status != MatchStatus::Ok) {
            last_status_ = decision.status == MatchStatus::NotConfigured
                               ? ShadowFeedStatus::NotConfigured
                               : ShadowFeedStatus::DecisionRefused;
            return last_status_;
        }
        if (decision.action == MatchAction::None) {
            ++counts_.no_candidate;
            ShadowFeedRecord record;
            record.order = ++record_order_;
            record.lane = static_cast<std::uint8_t>(slot.key);
            record.state = static_cast<std::uint8_t>(slot.state);
            record.at_us = now_us_;
            PushRecord(record);
            continue;
        }
        if (decision.action == MatchAction::LinkedState) {
            // 0x1802F933F..0x1802F9413: a linked pair records state, arms the
            // auto slot and pushes the horizon; it is never judged or scored.
            ++counts_.linked;
            ShadowFeedRecord record;
            record.order = ++record_order_;
            record.note_index = decision.note;
            record.lane = static_cast<std::uint8_t>(slot.key);
            record.state = static_cast<std::uint8_t>(slot.state);
            record.event_state = decision.event_state;
            record.linked = true;
            record.at_us = decision.at_us;
            PushRecord(record);
            continue;
        }

        // 0x1802F6800 -> 0x1802F79E0 -> 0x180303E80. The window classification
        // uses the captured input time and the judgment offset, exactly as the
        // R1 kernel does.
        const double delta_us =
            decision.at_us - now_us_ - config_.match.judgment_offset_us;
        const NormalRank rank = ClassifyNormalDelta(delta_us, config_.match.windows);
        if (rank == NormalRank::TooEarly) {
            // Non-consuming: the native routine neither latches nor scores it.
            ++counts_.early_ignored;
            ShadowFeedRecord record;
            record.order = ++record_order_;
            record.note_index = decision.note;
            record.lane = static_cast<std::uint8_t>(slot.key);
            record.state = static_cast<std::uint8_t>(slot.state);
            record.event_state = decision.event_state;
            record.at_us = decision.at_us;
            PushRecord(record);
            continue;
        }

        // note+84 > 1 -> grade 5 stays fail-closed: report it, never apply it.
        const SlotOverrideDecision override_decision = DecideSlotOverride(
            decision.event_state, static_cast<std::uint8_t>(rank),
            config_.evidence.slot_override_known);
        const bool override_unproven = override_decision == SlotOverrideDecision::Unverified;
        if (override_unproven) ++counts_.slot_override_unproven;

        ++counts_.judged;
        if (rank == NormalRank::Missed) {
            ++counts_.late_presses;
        } else {
            ++counts_.scored;
        }

        NativeNoteOutcome outcome;
        outcome.note_index = decision.note;
        outcome.lane = static_cast<std::uint8_t>(slot.key);
        outcome.kind = NoteOutcomeKind::Scored;
        outcome.rank = rank;
        // 0x1802F79E0: 0 = Just, 1 = an early non-Just grade, 2 = a late one.
        outcome.effect_grade =
            rank == NormalRank::Just ? 0 : (delta_us > 0.0 ? 1 : 2);
        // chart+48 is the keysound SELECTOR id; the resolved handle at note+168
        // needs sub_180301CB0, so nothing is claimed here.
        outcome.sound_id = 0;
        outcome.at_us = decision.at_us;
        outcome.input_us = now_us_;
        outcomes[outcome_count++] = outcome;

        ScoreInput input;
        input.grade = static_cast<std::uint8_t>(rank);
        input.chart_group = decision.chart_group;
        input.scale = 100;
        // An unproven override is fed as the primary slot so the unproven branch
        // cannot fire; the record above says why.
        input.event_state = override_unproven ? 1u : decision.event_state;
        input.scoring_enabled = ScoreGate(decision.keysound_id, false);
        input.left_key = decision.left_key;
        input.right_key = decision.right_key;
        input.note_index = decision.note;
        input.auto_mode = config_.auto_mode;
        input.auto_play = config_.auto_play;
        input.effects_muted = config_.effects_muted;
        ScoreEffect effect;
        const ScoreStatus score_status = scoring_.Apply(input, effect);
        if (score_status == ScoreStatus::NotScored) {
            // 0x1802F79E0's a9 == 0: the note was still judged, but 0x180303E80
            // returned before touching combo, counters or the score table.
            ++counts_.not_scored;
        } else if (score_status != ScoreStatus::Ok) {
            last_status_ = ShadowFeedStatus::ScoreRejected;
            return last_status_;
        } else {
            effects[effect_count++] = effect;
        }

        // 0x1802F954F: the horizon push of a judged press.
        MatchCandidate judged;
        judged.note = decision.note;
        judged.note_us = decision.at_us;
        judged.left_key = decision.left_key;
        judged.right_key = decision.right_key;
        judged.mid_key = 0.0f;
        judged.auto_note = false;
        judged.chart_type = static_cast<std::uint8_t>(decision.chart_group);
        judged.done = false;
        judged.pending_early = false;
        judged.event_lane = static_cast<std::uint8_t>(slot.key);
        judged.event_kind = 1; // a New slot is a press
        judged.linked = false;
        judged.linked_open = false;
        judged.consumed = false;
        judged.auto_slot_empty = true;
        judged.keysound_id = decision.keysound_id;
        policy_.CommitJudge(judged, true, false);

        ShadowFeedRecord record;
        record.order = ++record_order_;
        record.note_index = decision.note;
        record.lane = static_cast<std::uint8_t>(slot.key);
        record.state = static_cast<std::uint8_t>(slot.state);
        record.rank = static_cast<std::uint8_t>(rank);
        record.event_state = decision.event_state;
        record.slot_override_unproven = override_unproven;
        record.at_us = decision.at_us;
        PushRecord(record);
    }
    last_status_ = ShadowFeedStatus::Ok;
    return last_status_;
}

void NativeShadowFeed::PushRecord(const ShadowFeedRecord& record) noexcept {
    records_[record_head_] = record;
    record_head_ = (record_head_ + 1) % kShadowFeedRingSize;
    if (record_count_ < kShadowFeedRingSize) ++record_count_;
}

std::size_t NativeShadowFeed::RecentRecords(std::span<ShadowFeedRecord> out) const noexcept {
    const std::size_t stored = record_count_;
    if (out.size() < stored) return 0;
    for (std::size_t i = 0; i < stored; ++i) {
        const std::size_t index = (record_head_ + kShadowFeedRingSize - stored + i) %
                                  kShadowFeedRingSize;
        out[i] = records_[index];
    }
    return stored;
}

} // namespace nosfix
