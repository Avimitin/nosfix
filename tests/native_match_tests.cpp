#include "nosfix/native_match.hpp"

#include "test_support.hpp"

#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <limits>
#include <vector>

namespace {

using nosfix::JudgmentWindowSample;
using nosfix::MatchAction;
using nosfix::MatchCandidate;
using nosfix::MatchConfig;
using nosfix::MatchDecision;
using nosfix::MatchSlot;
using nosfix::MatchStatus;
using nosfix::NativeMatchEvidence;
using nosfix::NativeMatchPolicy;
using nosfix::NativeShadowComparison;
using nosfix::NormalWindows;
using nosfix::ShadowField;
using nosfix::ShadowOutcome;
using nosfix::WindowReadStatus;

constexpr std::size_t kWindowRva = 0x13668E0;
constexpr std::size_t kImageSpan = 0x1367000;

// Builds an image span that carries the eight judgment-window globals plus the
// effect band, using the values the initializer writes (verified_0x1800cb9a0.c).
std::vector<std::byte> SeededImage(std::int32_t effect_early = 9,
                                   std::int32_t effect_late = -11,
                                   std::int32_t good_early = 7,
                                   std::int32_t good_late = -6,
                                   std::int32_t great_early = 4,
                                   std::int32_t great_late = -4,
                                   std::int32_t just_early = 2,
                                   std::int32_t just_late = -2,
                                   std::int32_t outer_early = 9999,
                                   std::int32_t outer_late = -9999) {
    std::vector<std::byte> image(kImageSpan, std::byte{0});
    const std::int32_t values[10] = {effect_early, effect_late, good_early,  good_late,
                                     great_early, great_late, just_early,  just_late,
                                     outer_early, outer_late};
    for (std::size_t i = 0; i < 10; ++i) {
        std::memcpy(image.data() + kWindowRva + i * sizeof(std::int32_t), &values[i],
                    sizeof(std::int32_t));
    }
    return image;
}

NormalWindows UsWindows() {
    // 60 units/s. 7 units = 116666.67 us, 6 = 100000, 4 = 66666.67, 2 = 33333.33.
    NormalWindows windows;
    windows.just_early_us = 2.0 * 1'000'000.0 / 60.0;
    windows.just_late_us = 2.0 * 1'000'000.0 / 60.0;
    windows.great_early_us = 4.0 * 1'000'000.0 / 60.0;
    windows.great_late_us = 4.0 * 1'000'000.0 / 60.0;
    windows.good_early_us = 7.0 * 1'000'000.0 / 60.0;
    windows.good_late_us = 6.0 * 1'000'000.0 / 60.0;
    return windows;
}

double Us(std::int32_t units) {
    return static_cast<double>(units) * 1'000'000.0 / 60.0;
}

NativeMatchEvidence CompleteEvidence() {
    NativeMatchEvidence evidence;
    evidence.horizon_writers_known = true;
    evidence.pair_record_layout_known = true;
    evidence.bucket_table_known = true;
    evidence.linked_object_known = true;
    evidence.linked_pairs_never_judged = true;
    evidence.slot_priority_known = true;
    evidence.per_group_masks_known = true;
    evidence.horizon_push_sites_known = true;
    evidence.auto_slot_rule_known = true;
    evidence.over_filter_rule_known = true;
    return evidence;
}

MatchConfig GoodConfig() {
    MatchConfig config;
    config.windows = UsWindows();
    config.horizon.sampled = true;
    config.layout_sampled = true;
    config.units_per_second = 60.0;
    config.lane_over_filter = 3;
    config.key_spread = 0.4f;
    return config;
}

NativeMatchPolicy Configured() {
    NativeMatchPolicy policy;
    const MatchStatus status = policy.Configure(GoodConfig(), CompleteEvidence());
    CHECK(status == MatchStatus::Ok);
    return policy;
}

MatchCandidate Candidate(std::uint32_t note, double note_us, std::uint8_t lane) {
    MatchCandidate candidate;
    candidate.note = note;
    candidate.note_us = note_us;
    // A key range that covers the entry lane, so the candidate lands in the
    // SameKey slot; the non-type-2 over-filter still accepts lanes within +-3.
    candidate.left_key = lane;
    candidate.right_key = lane;
    candidate.mid_key = static_cast<float>(lane);
    candidate.event_lane = lane;
    candidate.event_kind = 1;
    candidate.chart_type = 0;
    return candidate;
}

MatchCandidate CandidateSpan(std::uint32_t note, double note_us, std::uint8_t lane,
                             std::int32_t left_key, std::int32_t right_key) {
    MatchCandidate candidate = Candidate(note, note_us, lane);
    candidate.left_key = left_key;
    candidate.right_key = right_key;
    candidate.mid_key = static_cast<float>(left_key + right_key) * 0.5f;
    return candidate;
}

void TestWindowSampler() {
    const std::vector<std::byte> seeded = SeededImage();
    JudgmentWindowSample sample = nosfix::SampleJudgmentWindows(seeded, 60.0, true);
    CHECK(sample.status == WindowReadStatus::Ok);
    CHECK(sample.ok());
    CHECK(sample.matches_initial_seed);
    CHECK(sample.units.good_earliest == 7);
    CHECK(sample.units.just_latest == -2);
    CHECK(sample.outer_earliest_clamp == 9999);
    CHECK(sample.outer_latest_clamp == -9999);
    CHECK(std::abs(sample.windows.good_early_us - Us(7)) < 1e-6);
    CHECK(std::abs(sample.windows.good_late_us - Us(6)) < 1e-6);

    // A different rate scales the same units.
    JudgmentWindowSample faster = nosfix::SampleJudgmentWindows(seeded, 120.0, true);
    CHECK(faster.status == WindowReadStatus::Ok);
    CHECK(std::abs(faster.windows.good_early_us - Us(7) / 2.0) < 1e-6);

    // Not post-initialization: refused even for a perfect-looking table.
    CHECK(nosfix::SampleJudgmentWindows(seeded, 60.0, false).status ==
          WindowReadStatus::NotPostInitialization);

    // The zero static block is never valid.
    const std::vector<std::byte> zero(kImageSpan, std::byte{0});
    CHECK(nosfix::SampleJudgmentWindows(zero, 60.0, true).status ==
          WindowReadStatus::ZeroStaticBlock);

    // Rate and span validation.
    CHECK(nosfix::SampleJudgmentWindows(seeded, 0.0, true).status ==
          WindowReadStatus::InvalidRate);
    CHECK(nosfix::SampleJudgmentWindows(seeded, -60.0, true).status ==
          WindowReadStatus::InvalidRate);
    CHECK(nosfix::SampleJudgmentWindows(
              seeded, std::numeric_limits<double>::quiet_NaN(), true)
              .status == WindowReadStatus::InvalidRate);
    const std::vector<std::byte> shortImage(kWindowRva + 4, std::byte{1});
    CHECK(nosfix::SampleJudgmentWindows(shortImage, 60.0, true).status ==
          WindowReadStatus::Unavailable);

    // A band narrower than the Good tier is inconsistent: the outer thresholds
    // at 0x13668E0/0x13668E4 must bracket Good.
    const std::vector<std::byte> wide = SeededImage(3, -4);
    CHECK(nosfix::SampleJudgmentWindows(wide, 60.0, true).status ==
          WindowReadStatus::InconsistentEffectBand);

    // Outer clamp inside the Good tier is inconsistent.
    const std::vector<std::byte> clamp = SeededImage(9, -11, 7, -6, 4, -4, 2, -2, 3, -9);
    CHECK(nosfix::SampleJudgmentWindows(clamp, 60.0, true).status ==
          WindowReadStatus::InconsistentEffectBand);

    // Sign violation: a "latest" bound above zero cannot be a late magnitude.
    const std::vector<std::byte> signs = SeededImage(9, -11, 7, 6, 4, -4, 2, -2);
    CHECK(nosfix::SampleJudgmentWindows(signs, 60.0, true).status ==
          WindowReadStatus::InvalidUnits);
}

void TestConfigureRefusals() {
    NativeMatchPolicy policy;
    MatchConfig config = GoodConfig();
    NativeMatchEvidence evidence = CompleteEvidence();

    CHECK(policy.Configure(config, NativeMatchEvidence{}) == MatchStatus::EvidenceIncomplete);
    evidence.over_filter_rule_known = false;
    CHECK(policy.Configure(config, evidence) == MatchStatus::EvidenceIncomplete);
    evidence = CompleteEvidence();

    config.horizon.sampled = false;
    CHECK(policy.Configure(config, evidence) == MatchStatus::HorizonNotSampled);
    config = GoodConfig();
    config.layout_sampled = false;
    CHECK(policy.Configure(config, evidence) == MatchStatus::LayoutNotSampled);

    config = GoodConfig();
    config.units_per_second = 0.0;
    CHECK(policy.Configure(config, evidence) == MatchStatus::InvalidRate);
    config = GoodConfig();
    config.horizon.lane_spread = 40;
    CHECK(policy.Configure(config, evidence) == MatchStatus::InvalidHorizon);

    config = GoodConfig();
    config.windows.good_early_us = 1.0; // not nested inside Great
    CHECK(policy.Configure(config, evidence) == MatchStatus::InvalidWindows);
    config = GoodConfig();
    config.windows = NormalWindows{};
    CHECK(policy.Configure(config, evidence) == MatchStatus::InvalidWindows);
    config = GoodConfig();
    config.judgment_offset_us = std::numeric_limits<double>::infinity();
    CHECK(policy.Configure(config, evidence) == MatchStatus::NonFiniteTime);

    // Every refusal left the policy unconfigured and unable to answer.
    CHECK(!policy.IsConfigured());
    policy.BeginUpdate(0.0);
    CHECK(policy.Select(0, {}).status == MatchStatus::NotConfigured);
}

void TestAcceptStaysDormant() {
    NativeMatchPolicy policy = Configured();
    CHECK(policy.IsConfigured());
    CHECK(!policy.ModelVerified()); // compile-time gate is still false
    CHECK(policy.HorizonAllows(0.0, 1));
    CHECK(!policy.HorizonAllows(0.0, 0));
    CHECK(!policy.HorizonAllows(0.0, 200));

    // A fresh policy needs BeginUpdate before it will decide anything.
    NativeMatchPolicy warm;
    CHECK(warm.Configure(GoodConfig(), CompleteEvidence()) == MatchStatus::Ok);
    const std::array<MatchCandidate, 1> candidates{Candidate(1, Us(100), 1)};
    CHECK(warm.Select(1, candidates).status == MatchStatus::NotConfigured);
    warm.BeginUpdate(Us(100));
    CHECK(warm.Select(1, candidates).status == MatchStatus::Ok);
    // Lanes are 1..28: index 0 and index 29 are refused, not clamped.
    CHECK(warm.Select(0, candidates).status == MatchStatus::InvalidLane);
    CHECK(warm.Select(nosfix::kNativeLaneHistorySlots, candidates).status ==
          MatchStatus::InvalidLane);
}

void TestExactMatchChoice() {
    NativeMatchPolicy policy = Configured();
    policy.BeginUpdate(Us(100));
    policy.BeginGroup();
    // Three candidates in the same slot: the closest note wins.
    const std::array<MatchCandidate, 3> candidates{
        Candidate(1, Us(104), 3), Candidate(2, Us(101), 3), Candidate(3, Us(109), 3)};
    const MatchDecision decision = policy.Select(3, candidates);
    CHECK(decision.status == MatchStatus::Ok);
    CHECK(decision.action == MatchAction::Judge);
    CHECK(decision.note == 2);
    CHECK(decision.slot == MatchSlot::SameKey);
    CHECK(decision.at_us == Us(101));
    CHECK(policy.LaneMasked(3));
}

void TestTies() {
    NativeMatchPolicy policy = Configured();
    policy.BeginUpdate(Us(100));
    policy.BeginGroup();
    // Equal note time: the smaller note+56 key float wins (0x1802F8EB5).
    MatchCandidate low = Candidate(1, Us(102), 3);
    MatchCandidate high = Candidate(2, Us(102), 3);
    low.mid_key = 3.0f;
    high.mid_key = 5.0f;
    const std::array<MatchCandidate, 2> candidates{high, low};
    const MatchDecision decision = policy.Select(3, candidates);
    CHECK(decision.note == 1);
    CHECK(decision.slot == MatchSlot::SameKey);
}

void TestSameTimeEvents() {
    NativeMatchPolicy policy = Configured();
    policy.BeginUpdate(Us(200));
    policy.BeginGroup();
    // Two notes at the same song time in different lanes: each lane decides on
    // its own entry and neither shadows the other.
    const std::array<MatchCandidate, 1> left{Candidate(1, Us(200), 1)};
    const std::array<MatchCandidate, 1> right{Candidate(2, Us(200), 2)};
    const MatchDecision first = policy.Select(1, left);
    const MatchDecision second = policy.Select(2, right);
    CHECK(first.action == MatchAction::Judge);
    CHECK(second.action == MatchAction::Judge);
    CHECK(first.note == 1);
    CHECK(second.note == 2);
    // The same lane twice in one group is masked after the first attempt.
    const MatchDecision again = policy.Select(1, left);
    CHECK(again.action == MatchAction::None);
    CHECK(again.lane_masked);
    // A new group clears the masks.
    policy.BeginGroup();
    const MatchDecision fresh = policy.Select(1, left);
    CHECK(fresh.action == MatchAction::Judge);
}

void TestPendingEarlyHits() {
    NativeMatchPolicy policy = Configured();
    policy.BeginUpdate(Us(300));
    // A pending early hit is only resolved from the primary candidate slot. A
    // candidate reaching the entry through the left slot gets record slot 2 and
    // is refused (0x1802F9336).
    policy.BeginGroup();
    MatchCandidate pending = CandidateSpan(1, Us(302), 2, 5, 5);
    pending.pending_early = true;
    std::array<MatchCandidate, 1> candidates{pending};
    MatchDecision decision = policy.Select(2, candidates);
    CHECK(decision.action == MatchAction::None);
    CHECK(decision.skipped_pending_early);

    // From the entry lane the record's slot is 1, so the judge may run.
    policy.BeginGroup();
    candidates[0] = pending;
    decision = policy.Select(5, candidates);
    CHECK(decision.action == MatchAction::Judge);
    CHECK(decision.slot == MatchSlot::SameKey);
    CHECK(decision.event_state == 1);
}

void TestAutoNotesAndMisses() {
    NativeMatchPolicy policy = Configured();
    policy.BeginUpdate(Us(400));
    policy.BeginGroup();
    MatchCandidate automatic = Candidate(1, Us(400), 4);
    automatic.auto_note = true;
    std::array<MatchCandidate, 1> candidates{automatic};
    CHECK(policy.Select(4, candidates).action == MatchAction::None);

    MatchCandidate done = Candidate(2, Us(400), 4);
    done.done = true;
    candidates[0] = done;
    MatchDecision decision = policy.Select(4, candidates);
    CHECK(decision.action == MatchAction::None);
    CHECK(decision.skipped_done);

    // chart_type 3 and 5 are excluded before the slots are armed.
    MatchCandidate excluded = Candidate(3, Us(400), 4);
    excluded.chart_type = 3;
    candidates[0] = excluded;
    CHECK(policy.Select(4, candidates).action == MatchAction::None);
    excluded.chart_type = 5;
    candidates[0] = excluded;
    CHECK(policy.Select(4, candidates).action == MatchAction::None);

    // A consumed chart record is refused.
    policy.BeginGroup();
    MatchCandidate consumed = Candidate(4, Us(400), 4);
    consumed.consumed = true;
    candidates[0] = consumed;
    decision = policy.Select(4, candidates);
    CHECK(decision.action == MatchAction::None);
    CHECK(decision.skipped_consumed);
}

void TestLinkedPairs() {
    NativeMatchPolicy policy = Configured();
    policy.BeginUpdate(Us(500));
    policy.BeginGroup();
    MatchCandidate linked = Candidate(1, Us(500), 5);
    linked.linked = true;
    linked.linked_open = true;
    linked.auto_slot_empty = true;
    std::array<MatchCandidate, 1> candidates{linked};
    const MatchDecision decision = policy.Select(5, candidates);
    CHECK(decision.action == MatchAction::LinkedState);
    CHECK(decision.auto_slot_armed);
    CHECK(decision.horizon_pushed);
    CHECK(decision.horizon_push_us == Us(500));
    CHECK(policy.LaneResolved(5));
    CHECK(!policy.LaneMasked(5));
    // The horizon now covers lanes 1..9 (spread 4) with the note time.
    for (std::uint8_t lane = 1; lane <= 9; ++lane) {
        CHECK(policy.HistoryValues()[lane] == Us(500));
        CHECK(policy.HistoryStamps()[lane] == nosfix::kInitialHistoryStamp);
    }
    CHECK(policy.HistoryValues()[0] == 0.0);
    CHECK(policy.HistoryValues()[10] == 0.0);
    // A resolved lane blocks a later judgment in the same group.
    MatchCandidate plain = Candidate(2, Us(500), 5);
    candidates[0] = plain;
    const MatchDecision blocked = policy.Select(5, candidates);
    CHECK(blocked.action == MatchAction::None);
    CHECK(blocked.lane_resolved);
}

void TestHorizonAdmission() {
    NativeMatchPolicy policy = Configured();
    policy.BeginUpdate(Us(600));
    policy.BeginGroup();
    MatchCandidate seed = Candidate(1, Us(600), 10);
    seed.linked = true;
    seed.linked_open = true;
    std::array<MatchCandidate, 1> candidates{seed};
    CHECK(policy.Select(10, candidates).action == MatchAction::LinkedState);

    // The pushed value is 600 units; the admission limit is value + 3 + 1 units.
    CHECK(policy.HorizonAllows(Us(604), 10));
    CHECK(!policy.HorizonAllows(Us(605), 10));
    // Only the spread window carries the value.
    CHECK(policy.HorizonAllows(Us(100000), 16));

    // A linked pair whose horizon rejects it is still admitted through the
    // linked_open path, but a plain one is dropped.
    policy.BeginUpdate(Us(700));
    policy.BeginGroup();
    MatchCandidate far = Candidate(2, Us(900), 10);
    candidates[0] = far;
    MatchDecision decision = policy.Select(10, candidates);
    CHECK(decision.action == MatchAction::None);
    CHECK(!decision.horizon_ok);
    far.linked_open = true;
    candidates[0] = far;
    decision = policy.Select(10, candidates);
    CHECK(decision.action == MatchAction::Judge);
    CHECK(!decision.horizon_ok);
}

void TestCommitJudge() {
    NativeMatchPolicy policy = Configured();
    policy.BeginUpdate(Us(800));
    policy.BeginGroup();
    const MatchCandidate candidate = Candidate(1, Us(800), 12);
    policy.CommitJudge(candidate, false, false);
    CHECK(policy.HistoryValues()[12] == 0.0);
    policy.CommitJudge(candidate, true, false);
    CHECK(policy.HistoryValues()[12] == Us(800));

    // A release never pushes.
    policy.BeginUpdate(Us(900));
    MatchCandidate release = Candidate(2, Us(900), 20);
    release.event_kind = 3;
    policy.CommitJudge(release, true, false);
    CHECK(policy.HistoryValues()[20] == 0.0);

    // chart type 4 pushes "now + judgment + visual + effect_early" instead of the
    // note time (0x1802F954F..0x1802F959A).
    policy.BeginUpdate(Us(1000));
    MatchCandidate type4 = Candidate(3, Us(1050), 25);
    type4.chart_type = 4;
    const std::array<MatchCandidate, 1> candidates{type4};
    const MatchDecision decision = policy.Select(25, candidates);
    CHECK(decision.action == MatchAction::Judge);
    CHECK(std::abs(decision.horizon_push_us - Us(1009)) < 1e-6);
    policy.CommitJudge(type4, true, false);
    CHECK(std::abs(policy.HistoryValues()[25] - Us(1009)) < 1e-6);
}

void TestOverFilterAndSlots() {
    NativeMatchPolicy policy = Configured();
    policy.BeginUpdate(Us(1100));
    policy.BeginGroup();
    // A 2-key wide note at keys 10..12 accepts lanes 10..12 plus, for a
    // non-type-2 chart, a further 0x13E4A84 == 3 keys on each side.
    std::array<MatchCandidate, 1> candidates{CandidateSpan(1, Us(1100), 7, 10, 12)};
    candidates[0].chart_type = 2;
    CHECK(policy.Select(7, candidates).action == MatchAction::None); // 7 < 10

    policy.BeginGroup();
    candidates[0] = CandidateSpan(2, Us(1100), 7, 10, 12);
    candidates[0].chart_type = 0; // over-filter widens to 7..15
    MatchDecision decision = policy.Select(7, candidates);
    CHECK(decision.action == MatchAction::Judge);
    CHECK(decision.slot == MatchSlot::LeftOfRange);

    policy.BeginGroup();
    candidates[0] = CandidateSpan(3, Us(1100), 16, 10, 12);
    candidates[0].chart_type = 2;
    CHECK(policy.Select(16, candidates).action == MatchAction::None); // 16 > 12

    policy.BeginGroup();
    candidates[0] = CandidateSpan(4, Us(1100), 14, 10, 12);
    candidates[0].chart_type = 0;
    decision = policy.Select(14, candidates);
    CHECK(decision.action == MatchAction::Judge);
    CHECK(decision.slot == MatchSlot::RightOfRange);

    policy.BeginGroup();
    candidates[0] = CandidateSpan(5, Us(1100), 11, 10, 12);
    decision = policy.Select(11, candidates);
    CHECK(decision.action == MatchAction::Judge);
    CHECK(decision.slot == MatchSlot::SameKey);
}

// The same captured presses and notes must produce the same decisions at every
// presentation cadence, including stalls, because the seam never reads a frame
// count. The driver below calls BeginUpdate with a wandering "now" and feeds the
// same candidate set.
void TestCadenceInvariance() {
    struct Scenario final {
        double press_us;
        std::uint8_t lane;
        std::uint32_t note;
        double note_us;
    };
    const Scenario scenario[6] = {
        {Us(100), 1, 10, Us(100)}, {Us(250), 2, 20, Us(260)}, {Us(400), 3, 30, Us(395)},
        {Us(550), 4, 40, Us(560)}, {Us(700), 5, 50, Us(700)}, {Us(850), 6, 60, Us(846)},
    };
    const double rates[7] = {60.0, 90.0, 120.0, 144.0, 165.0, 240.0, 360.0};

    std::array<MatchDecision, 6> reference{};
    for (double rate : rates) {
        NativeMatchPolicy policy = Configured();
        const double frame_us = 1'000'000.0 / rate;
        std::size_t next = 0;
        double now = 0.0;
        // Jitter and one long stall inside the run.
        const double jitter[8] = {0.0, 0.37, -0.21, 0.9, -0.55, 1.4, -0.9, 0.15};
        std::size_t step = 0;
        std::array<MatchDecision, 6> decisions{};
        while (next < 6) {
            now += frame_us * (0.6 + 0.8 * std::abs(jitter[step % 8]));
            if (step == 40) now += frame_us * 30.0; // a 30-frame stall
            ++step;
            policy.BeginUpdate(now);
            while (next < 6 && scenario[next].press_us <= now) {
                policy.BeginGroup();
                const MatchCandidate candidate =
                    Candidate(scenario[next].note, scenario[next].note_us,
                              scenario[next].lane);
                const std::array<MatchCandidate, 1> candidates{candidate};
                decisions[next] = policy.Select(scenario[next].lane, candidates);
                CHECK(decisions[next].status == MatchStatus::Ok);
                CHECK(decisions[next].action == MatchAction::Judge);
                CHECK(decisions[next].note == scenario[next].note);
                CHECK(decisions[next].at_us == scenario[next].note_us);
                if (rate == 60.0) {
                    reference[next] = decisions[next];
                } else {
                    CHECK(decisions[next].note == reference[next].note);
                    CHECK(decisions[next].slot == reference[next].slot);
                    CHECK(decisions[next].at_us == reference[next].at_us);
                    CHECK(decisions[next].horizon_push_us == reference[next].horizon_push_us);
                    CHECK(decisions[next].action == reference[next].action);
                }
                ++next;
            }
        }
        CHECK(next == 6);
    }
}

void TestShadowComparison() {
    NativeShadowComparison shadow;
    shadow.Configure(1.0);
    ShadowOutcome adapter;
    adapter.note_index = 7;
    adapter.lane = 2;
    adapter.kind = 0;
    adapter.rank = 4;
    adapter.effect_grade = 0;
    adapter.score = 100;
    adapter.at_us = Us(500);
    ShadowOutcome native = adapter;
    CHECK(!shadow.Observe(adapter, native).diverged);
    CHECK(!shadow.Observe(adapter, native).diverged);
    CHECK(shadow.Compared() == 2);
    CHECK(!shadow.Diverged());

    // Rank divergence at ordinal 2.
    native.rank = 3;
    native.effect_grade = 2;
    const nosfix::ShadowDivergence divergence = shadow.Observe(adapter, native);
    CHECK(divergence.diverged);
    CHECK(divergence.field == ShadowField::Rank);
    CHECK(divergence.index == 2);
    CHECK(divergence.note_index == 7);
    CHECK(divergence.at_us == Us(500));
    CHECK(divergence.adapter_rank == 4);
    CHECK(divergence.native_rank == 3);
    // Latched: later observations return the same record.
    const nosfix::ShadowDivergence again = shadow.Observe(adapter, adapter);
    CHECK(again.diverged);
    CHECK(again.index == 2);
    shadow.Reset();
    CHECK(!shadow.Diverged());
    CHECK(shadow.Compared() == 0);

    // Score divergence.
    native = adapter;
    native.score = 50;
    CHECK(shadow.Observe(adapter, native).field == ShadowField::Score);
    shadow.Reset();

    // Time divergence respects the configured tolerance.
    native = adapter;
    native.at_us = Us(500) + 0.5;
    CHECK(!shadow.Observe(adapter, native).diverged);
    shadow.Reset();
    shadow.Configure(0.0);
    CHECK(shadow.Observe(adapter, native).field == ShadowField::Time);

    // Kind, lane, note index and effect are all compared.
    shadow.Reset();
    native = adapter;
    native.kind = 1;
    CHECK(shadow.Observe(adapter, native).field == ShadowField::Kind);
    shadow.Reset();
    native = adapter;
    native.lane = 3;
    CHECK(shadow.Observe(adapter, native).field == ShadowField::Lane);
    shadow.Reset();
    native = adapter;
    native.note_index = 9;
    CHECK(shadow.Observe(adapter, native).field == ShadowField::NoteIndex);
    shadow.Reset();
    native = adapter;
    native.effect_grade = 1;
    CHECK(shadow.Observe(adapter, native).field == ShadowField::Effect);
}

void TestShadowFromAdapter() {
    nosfix::NativeNoteOutcome outcome;
    outcome.note_index = 12;
    outcome.lane = 3;
    outcome.kind = nosfix::NoteOutcomeKind::Scored;
    outcome.rank = nosfix::NormalRank::Just;
    outcome.effect_grade = 0;
    outcome.at_us = Us(42);
    const ShadowOutcome shadow = NativeShadowComparison::FromAdapter(outcome, 300);
    CHECK(shadow.note_index == 12);
    CHECK(shadow.lane == 3);
    CHECK(shadow.kind == static_cast<std::uint8_t>(nosfix::NoteOutcomeKind::Scored));
    CHECK(shadow.rank == static_cast<std::uint8_t>(nosfix::NormalRank::Just));
    CHECK(shadow.score == 300);
    CHECK(shadow.at_us == Us(42));
}

// The adapter still refuses the native selection, so no configuration can reach
// the new policy.
void TestAdapterRefusesNativeSelection() {
    nosfix::NativeNoteState state;
    std::array<nosfix::NativeNoteSpec, 1> notes{};
    notes[0].start_us = 0.0;
    notes[0].end_us = 0.0;
    notes[0].chart_index = 1;
    notes[0].lane = 0;
    notes[0].kind = nosfix::NoteKind::Normal;
    nosfix::NativeNoteConfig config;
    config.windows = UsWindows();
    config.windows_sampled = true;
    config.selection = nosfix::NoteSelectionPolicy::Native2F7C10;
    CHECK(state.Configure(notes, config) == nosfix::NoteConfigStatus::UnsupportedSelection);
    config.selection = nosfix::NoteSelectionPolicy::AdapterEarliestPending;
    CHECK(state.Configure(notes, config) == nosfix::NoteConfigStatus::Accepted);
}

} // namespace

int main() {
    TestWindowSampler();
    TestConfigureRefusals();
    TestAcceptStaysDormant();
    TestExactMatchChoice();
    TestTies();
    TestSameTimeEvents();
    TestPendingEarlyHits();
    TestAutoNotesAndMisses();
    TestLinkedPairs();
    TestHorizonAdmission();
    TestCommitJudge();
    TestOverFilterAndSlots();
    TestCadenceInvariance();
    TestShadowComparison();
    TestShadowFromAdapter();
    TestAdapterRefusesNativeSelection();
    return test::Finish("native_match");
}
