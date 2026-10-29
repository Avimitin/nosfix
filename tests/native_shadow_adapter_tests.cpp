// Round-2 judgement/scoring tests: the decoded bucket table (0x1813DEA20), the
// proven candidate comparator (0x1800CAE90 / 0x1800CB0E0 / 0x1800CA230), the
// normal-note field decode (0x1802F73B0), the 1/60 s song unit (A5) and the
// shadow feed seam. Every refusal path and every dormancy gate is exercised.

#include "nosfix/native_shadow_adapter.hpp"

#include "test_support.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <numeric>
#include <span>
#include <vector>

namespace {

using nosfix::BucketStatus;
using nosfix::CandidateSortStatus;
using nosfix::ChartMsToSongUnits;
using nosfix::DecideSlotOverride;
using nosfix::DecodeNormalNote;
using nosfix::MakeMatchCandidate;
using nosfix::MatchAction;
using nosfix::MatchCandidate;
using nosfix::MatchConfig;
using nosfix::MatchDecision;
using nosfix::MatchSlot;
using nosfix::MatchStatus;
using nosfix::NativeBucketCompactConfig;
using nosfix::NativeBucketCompactReport;
using nosfix::NativeBucketSlot;
using nosfix::NativeBucketTable;
using nosfix::NativeCandidateKey;
using nosfix::NativeCandidatePrecedes;
using nosfix::NativeChartFields;
using nosfix::NativeMatchEvidence;
using nosfix::NativeNoteFields;
using nosfix::NativeNoteOutcome;
using nosfix::NativeScoringConfig;
using nosfix::NativeShadowComparison;
using nosfix::NativeShadowEvidence;
using nosfix::NativeShadowFeed;
using nosfix::NativeSlotState;
using nosfix::NormalRank;
using nosfix::NormalWindows;
using nosfix::NoteDecodeStatus;
using nosfix::NoteOutcomeKind;
using nosfix::ShadowFeedConfig;
using nosfix::ShadowFeedRecord;
using nosfix::ShadowFeedStatus;
using nosfix::ShadowOutcome;
using nosfix::ScoreEffect;
using nosfix::ScoreGrade;
using nosfix::SlotOverrideDecision;
using nosfix::SortCandidateKeys;
using nosfix::SpriteOffsetForChart;

constexpr std::size_t kSlots = nosfix::kNativeBucketSlotCount;

double Us(std::int32_t units) {
    return static_cast<double>(units) * 1'000'000.0 / 60.0;
}

NormalWindows UsWindows() {
    NormalWindows windows;
    windows.just_early_us = Us(2);
    windows.just_late_us = Us(2);
    windows.great_early_us = Us(4);
    windows.great_late_us = Us(4);
    windows.good_early_us = Us(7);
    windows.good_late_us = Us(6);
    return windows;
}

NativeMatchEvidence CompleteMatchEvidence() {
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

NativeShadowEvidence CompleteShadowEvidence() {
    NativeShadowEvidence evidence;
    evidence.bucket_layout_known = true;
    evidence.bucket_rotation_known = true;
    evidence.bucket_compaction_known = true;
    evidence.candidate_ordering_known = true;
    evidence.note_field_layout_known = true;
    evidence.song_unit_rate_known = true;
    return evidence;
}

MatchConfig GoodMatchConfig() {
    MatchConfig config;
    config.windows = UsWindows();
    config.horizon.sampled = true;
    config.layout_sampled = true;
    config.units_per_second = nosfix::kProvenSongUnitsPerSecond;
    config.lane_over_filter = 3;
    config.key_spread = 0.4f;
    return config;
}

ShadowFeedConfig GoodFeedConfig() {
    ShadowFeedConfig config;
    config.match = GoodMatchConfig();
    config.scoring = NativeScoringConfig{};
    config.compact = NativeBucketCompactConfig{};
    config.match_evidence = CompleteMatchEvidence();
    config.evidence = CompleteShadowEvidence();
    return config;
}

NativeBucketSlot Slot(std::int32_t key, std::int32_t state, std::uint8_t value = 0,
                     std::int32_t group = 0, std::int32_t repeat = 0) {
    NativeBucketSlot slot;
    slot.key = key;
    slot.state = state;
    slot.value = value;
    slot.group = group;
    slot.repeat = repeat;
    return slot;
}

std::array<NativeBucketSlot, kSlots> EmptyStaging() {
    std::array<NativeBucketSlot, kSlots> staging{};
    return staging;
}

// One normal note in the caller's view, as MakeMatchCandidate would build it.
MatchCandidate Note(std::uint32_t index, std::int32_t left_key, std::int32_t right_key,
                    double note_us, std::uint8_t group = 0, std::int32_t keysound = 7) {
    MatchCandidate candidate;
    candidate.note = index;
    candidate.note_us = note_us;
    candidate.left_key = left_key;
    candidate.right_key = right_key;
    candidate.mid_key = static_cast<float>(left_key + right_key) * 0.5f;
    candidate.chart_type = group;
    candidate.keysound_id = keysound;
    candidate.auto_slot_empty = true;
    return candidate;
}

// ===========================================================================
// A5: the song unit is 1/60 s
// ===========================================================================

void TestSongUnits() {
    CHECK(nosfix::kProvenSongUnitsPerSecond == 60.0);
    CHECK(std::abs(nosfix::kProvenSongUnitUs - 1'000'000.0 / 60.0) < 1e-9);
    CHECK(std::abs(nosfix::SongUnitsToUs(1.0) - 1'000'000.0 / 60.0) < 1e-9);
    CHECK(nosfix::SongUnitsToUs(60.0) == 1'000'000.0);
    CHECK(nosfix::SongUnitsToUs(120.0) == 2'000'000.0);
    CHECK(std::abs(nosfix::UsToSongUnits(1'000'000.0) - 60.0) < 1e-9);
    // The explicit rate still wins: 120 units/s halves every conversion.
    CHECK(nosfix::SongUnitsToUs(120.0, 120.0) == 1'000'000.0);
    CHECK(nosfix::UsToSongUnits(1'000'000.0, 120.0) == 120.0);

    // 0x1801A4935: frame = (int)(ms * 60.0 / 1000.0), truncation toward zero.
    CHECK(ChartMsToSongUnits(0) == 0);
    CHECK(ChartMsToSongUnits(1000) == 60);
    CHECK(ChartMsToSongUnits(2000) == 120);
    CHECK(ChartMsToSongUnits(16) == 0);   // 0.96
    CHECK(ChartMsToSongUnits(17) == 1);   // 1.02
    CHECK(ChartMsToSongUnits(500) == 30); // 30.0
    CHECK(ChartMsToSongUnits(-17) == -1); // -1.02 truncates to -1, it is not floored
    CHECK(ChartMsToSongUnits(-16) == 0);  // -0.96
}

// ===========================================================================
// A1: bucket table layout, validation, rotation and compaction
// ===========================================================================

void TestBucketLayout() {
    CHECK(nosfix::kNativeBucketEntryCount == 30);
    CHECK(kSlots == 28);
    CHECK(nosfix::kNativeBucketSlotBytes == 28);
    CHECK(nosfix::kNativeBucketEntryBytes == 792);
    CHECK(nosfix::kNativeBucketCountOffset == 784);
    CHECK(nosfix::kNativeBucketFillerBytes == 4);
    // 28 slots of 28 bytes + a dword count + four filler bytes = 792 (0x318).
    CHECK(nosfix::kNativeBucketSlotBytes * kSlots == 784);
    CHECK(nosfix::kNativeBucketSlotBytes * kSlots + 4 + nosfix::kNativeBucketFillerBytes ==
          nosfix::kNativeBucketEntryBytes);
    // Seven dwords per slot, in the proven order.
    CHECK(nosfix::kNativeSlotKeyOffset == 0x00);
    CHECK(nosfix::kNativeSlotStateOffset == 0x04);
    CHECK(nosfix::kNativeSlotValueOffset == 0x08);
    CHECK(nosfix::kNativeSlotReservedAOffset == 0x0C);
    CHECK(nosfix::kNativeSlotReservedBOffset == 0x10);
    CHECK(nosfix::kNativeSlotGroupOffset == 0x14);
    CHECK(nosfix::kNativeSlotRepeatOffset == 0x18);
    CHECK(nosfix::kNativeSlotRepeatOffset + 4 == nosfix::kNativeBucketSlotBytes);
    CHECK(nosfix::kNativeLiveBucketEntry == 0);

    // A default table is empty and valid.
    NativeBucketTable table;
    CHECK(table.Validate() == BucketStatus::Ok);
    CHECK(table.Live().count == 0);

    // Slot ranges: 1..28 keys, states 0..3, groups 0..3, repeats 0..24.
    CHECK(Slot(1, 1).valid());
    CHECK(Slot(28, 3).valid());
    CHECK(!Slot(0, 1).valid());
    CHECK(!Slot(29, 1).valid());
    CHECK(!Slot(5, 4).valid());
    CHECK(!Slot(5, -1).valid());
    CHECK(!Slot(5, 1, 0, 4).valid());
    CHECK(!Slot(5, 1, 0, 0, 25).valid());
    CHECK(Slot(5, 1, 0, 0, 24).valid());
    CHECK(!Slot(5, 1, 0, -1).valid());

    // Append refuses an invalid or free slot and stops at 28.
    nosfix::NativeBucketEntry entry;
    CHECK(!entry.Append(Slot(5, 0)));
    CHECK(!entry.Append(Slot(0, 1)));
    for (std::int32_t key = 1; key <= 28; ++key) {
        CHECK(entry.Append(Slot(key, 1)));
    }
    CHECK(entry.count == 28);
    CHECK(!entry.Append(Slot(3, 1)));
    CHECK(entry.FindKey(28) != nullptr);
    CHECK(entry.FindKey(29) == nullptr);
    entry.Clear();
    CHECK(entry.count == 0);
    CHECK(entry.FindKey(1) == nullptr);
    // A free slot is not found even when its key matches.
    entry.slots[0] = Slot(7, 0);
    entry.count = 1;
    CHECK(entry.FindKey(7) == nullptr);
    // ... but a free slot inside the count is still a validation failure: the
    // compaction never appends one.
    CHECK(!entry.valid());
    entry.slots[0] = Slot(7, 1);
    CHECK(entry.valid());
    entry.count = 29;
    CHECK(!entry.valid());
    CHECK(table.Entry(0).count == 0);
}

void TestBucketRotation() {
    // sub_180307090: entry[k] = entry[k-1] for k = 29..1, then entry[0] = 0.
    // Only 28 keys exist for 30 records, so the entry ordinal travels in `value`.
    NativeBucketTable table;
    for (std::size_t index = 0; index < nosfix::kNativeBucketEntryCount; ++index) {
        const std::int32_t key = static_cast<std::int32_t>(index % kSlots) + 1;
        CHECK(table.Entry(index).Append(
            Slot(key, 1, static_cast<std::uint8_t>(index))));
    }
    CHECK(table.Validate() == BucketStatus::Ok);
    table.RotateDown();
    CHECK(table.Live().count == 0);          // entry 0 was zeroed
    CHECK(table.Entry(1).count == 1);        // entry 1 is the previous entry 0
    CHECK(table.Entry(1).slots[0].value == 0);   // ordinal of the old entry 0
    CHECK(table.Entry(2).slots[0].value == 1);   // ordinal of the old entry 1
    CHECK(table.Entry(29).slots[0].value == 28); // ordinal of the old entry 28
    // The old entry 29 fell off the end, exactly as the 29-step memmove drops it.
    table.RotateDown();
    CHECK(table.Entry(1).count == 0);
    CHECK(table.Entry(2).slots[0].value == 0);
    CHECK(table.Entry(29).slots[0].value == 27);

    // Rotation is idempotent on an all-empty table.
    NativeBucketTable empty;
    empty.RotateDown();
    CHECK(empty.Validate() == BucketStatus::Ok);
    CHECK(empty.Live().count == 0);
}

void TestBucketCompaction() {
    NativeBucketTable table;
    NativeBucketCompactConfig config;
    NativeBucketCompactReport report;

    // Refusals: the staging span must be the fixed 28-slot bound, and entry 0
    // must be empty (rotate first) so a compact never mixes two frames.
    std::array<NativeBucketSlot, 3> shortStaging{};
    CHECK(table.CompactFromStaging(shortStaging, config, report) ==
          BucketStatus::InvalidStaging);
    CHECK(table.Live().count == 0);

    auto staging = EmptyStaging();
    CHECK(table.Entry(0).Append(Slot(4, 1)));
    CHECK(table.CompactFromStaging(staging, config, report) == BucketStatus::EntryNotEmpty);
    CHECK(table.Live().count == 1); // unchanged
    table.Clear();

    // An invalid staging slot refuses the whole compaction and writes nothing.
    staging[0] = Slot(0, 1);
    CHECK(table.CompactFromStaging(staging, config, report) == BucketStatus::InvalidSlot);
    CHECK(table.Live().count == 0);
    CHECK(report.appended == 0);
    staging[0] = Slot(5, 1, 3);

    // A new slot is appended; free slots are not events.
    staging[1] = Slot(9, 0);
    CHECK(table.CompactFromStaging(staging, config, report) == BucketStatus::Ok);
    CHECK(table.Live().count == 1);
    CHECK(table.Live().slots[0].key == 5);
    CHECK(table.Live().slots[0].state == 1);
    CHECK(table.Live().slots[0].value == 3);
    CHECK(report.staging_slots == 28);
    CHECK(report.appended == 1);
    CHECK(!report.group_phase);

    // Decay against record 1: a key that the previous frame already released
    // demotes held -> new and release -> free (0x18030721F).
    table.RotateDown(); // entry 1 now holds {key 5, state 1}
    CHECK(table.Entry(1).count == 1);
    CHECK(table.Entry(1).slots[0].key == 5);
    // Shape record 1 as a release for key 5 and a held for key 6, then compact
    // WITHOUT rotating so record 1 stays the decay source.
    table.Entry(1).Clear();
    CHECK(table.Entry(1).Append(Slot(5, 3)));
    CHECK(table.Entry(1).Append(Slot(6, 2)));
    auto staged = EmptyStaging();
    staged[0] = Slot(5, 2); // held -> new
    staged[1] = Slot(6, 2); // held stays held (record 1 says 2, not outside {1,2})
    staged[2] = Slot(5, 3); // release -> free, dropped
    CHECK(table.CompactFromStaging(staged, config, report) == BucketStatus::Ok);
    CHECK(report.appended == 2);
    CHECK(report.decayed_to_new == 1);
    CHECK(report.decayed_to_free == 1);
    CHECK(table.Live().count == 2);
    CHECK(table.Live().slots[0].key == 5);
    CHECK(table.Live().slots[0].state == 1);
    CHECK(table.Live().slots[1].key == 6);
    CHECK(table.Live().slots[1].state == 2);

    // Phase 2 (0x307170) is off by default and only runs when the caller turns
    // it on; with it on, the documented inheritance rules apply.
    NativeBucketTable phased;
    NativeBucketCompactConfig phasedConfig;
    phasedConfig.group_phase_enabled = true;
    // Record 2 holds key 5 with group 2 / repeat 3, key 6 with group 3, key 1
    // with group 1.
    CHECK(phased.Entry(2).Append(Slot(5, 2, 0, 2, 3)));
    CHECK(phased.Entry(2).Append(Slot(6, 1, 0, 3)));
    CHECK(phased.Entry(2).Append(Slot(1, 2, 0, 1)));
    std::array<NativeBucketSlot, kSlots> phasedStaged = EmptyStaging();
    phasedStaged[0] = Slot(5, 1);  // same key: inherit group 2, repeat 3 + 1
    phasedStaged[1] = Slot(7, 1);  // key 6 is a neighbour of 7: group 3
    phasedStaged[2] = Slot(2, 1);  // key 1 is a neighbour but its group is 1
    phasedStaged[3] = Slot(20, 1); // nothing matches: group 1
    CHECK(phased.CompactFromStaging(phasedStaged, phasedConfig, report) == BucketStatus::Ok);
    CHECK(report.group_phase);
    CHECK(phased.Live().count == 4);
    CHECK(phased.Live().slots[0].group == 2);
    CHECK(phased.Live().slots[0].repeat == 4);
    CHECK(phased.Live().slots[1].group == 3);
    CHECK(phased.Live().slots[2].group == 1);
    CHECK(phased.Live().slots[3].group == 1);

    // repeat >= 25 re-arms group 1 with repeat 0.
    NativeBucketTable capped;
    CHECK(capped.Entry(2).Append(Slot(9, 2, 0, 3, 24)));
    auto cappedStaged = EmptyStaging();
    cappedStaged[0] = Slot(9, 1);
    CHECK(capped.CompactFromStaging(cappedStaged, phasedConfig, report) == BucketStatus::Ok);
    CHECK(capped.Live().slots[0].repeat == 0);
    CHECK(capped.Live().slots[0].group == 1);
    CHECK(capped.Validate() == BucketStatus::Ok);

    // Validate catches a count outside 0..28 and a slot outside its range.
    NativeBucketTable broken;
    broken.Entry(3).count = 29;
    CHECK(broken.Validate() == BucketStatus::InvalidEntry);
    broken.Entry(3).count = 1;
    broken.Entry(3).slots[0] = Slot(0, 1);
    CHECK(broken.Validate() == BucketStatus::InvalidSlot);
    CHECK(broken.Slot(3, 0) != nullptr);
    CHECK(broken.Slot(3, 1) == nullptr);  // past the count
    CHECK(broken.Slot(30, 0) == nullptr); // past the table
    CHECK(broken.Slot(0, 0) == nullptr);  // entry 0 is empty
}

// ===========================================================================
// A2: the seven-key comparator and its stable sort
// ===========================================================================

void TestCandidateComparator() {
    NativeCandidateKey a;
    a.frame = 100;
    a.primary_slot = true;
    a.event_lane = 3;
    a.centre_key = 10;
    NativeCandidateKey b = a;
    CHECK(!NativeCandidatePrecedes(a, b));
    CHECK(!NativeCandidatePrecedes(b, a));

    // 1. Frame band: a difference wider than 0x13E496C orders by frame alone,
    // even when the later record is the primary slot.
    b.frame = 103;
    b.primary_slot = true;
    a.primary_slot = false;
    CHECK(NativeCandidatePrecedes(a, b));  // 100 < 103
    CHECK(!NativeCandidatePrecedes(b, a));
    // Inside the band (|dt| <= 1) the frame does NOT decide: the slot-1 flag does.
    b.frame = 101;
    CHECK(NativeCandidatePrecedes(b, a)); // b is primary, a is not
    CHECK(!NativeCandidatePrecedes(a, b));

    // 2/3. Same primary flag: the smaller frame wins.
    a.primary_slot = true;
    b.primary_slot = true;
    CHECK(NativeCandidatePrecedes(a, b));
    CHECK(!NativeCandidatePrecedes(b, a));

    // 4. A NULL partner precedes a non-NULL one.
    NativeCandidateKey nullPartner = a;
    NativeCandidateKey partnered = a;
    partnered.has_partner = true;
    CHECK(NativeCandidatePrecedes(nullPartner, partnered));
    // The image tests this one-sided, so the reverse does not order: it falls
    // through to key 5, and here the two distances are equal, so neither precedes.
    CHECK(!NativeCandidatePrecedes(partnered, nullPartner));

    // 5. Smaller distance first.
    NativeCandidateKey near = a;
    NativeCandidateKey far = a;
    near.distance = 1.0f;
    far.distance = 2.0f;
    CHECK(NativeCandidatePrecedes(near, far));
    CHECK(!NativeCandidatePrecedes(far, near));
    // The reverse one-sided partner case still falls through to the distance.
    NativeCandidateKey partneredFar = partnered;
    partneredFar.distance = 0.5f;
    NativeCandidateKey nullFar = nullPartner;
    nullFar.distance = 1.0f;
    CHECK(NativeCandidatePrecedes(partneredFar, nullFar)); // 0.5 < 1.0
    // A NaN distance compares unordered and falls through to the event lane.
    NativeCandidateKey nan = a;
    nan.distance = std::numeric_limits<float>::quiet_NaN();
    NativeCandidateKey lane4 = a;
    lane4.event_lane = 4;
    CHECK(NativeCandidatePrecedes(nan, lane4)); // lane 3 < 4 decides it

    // 6. Event lane ascending.
    NativeCandidateKey laneLow = a;
    NativeCandidateKey laneHigh = a;
    laneLow.event_lane = 2;
    laneHigh.event_lane = 9;
    CHECK(NativeCandidatePrecedes(laneLow, laneHigh));
    CHECK(!NativeCandidatePrecedes(laneHigh, laneLow));

    // 7. note+52 ascending.
    NativeCandidateKey centreLow = a;
    NativeCandidateKey centreHigh = a;
    centreLow.centre_key = 4;
    centreHigh.centre_key = 12;
    CHECK(NativeCandidatePrecedes(centreLow, centreHigh));
    CHECK(!NativeCandidatePrecedes(centreHigh, centreLow));

    // A wider band is a caller decision, not a constant. Inside the band the
    // frame still decides at key 3 - the band only decides whether the frame
    // outranks the slot-1 flag.
    NativeCandidateKey frameA = a;
    NativeCandidateKey frameB = a;
    frameA.frame = 10;
    frameB.frame = 11;
    CHECK(NativeCandidatePrecedes(frameA, frameB, 1));
    CHECK(!NativeCandidatePrecedes(frameB, frameA, 1));
}

void TestCandidateSort() {
    // Equal keys keep their insertion order (the sort is stable, and the native
    // merge only takes the right run when it precedes the left one).
    std::array<NativeCandidateKey, 5> equal{};
    for (std::size_t i = 0; i < equal.size(); ++i) {
        equal[i].frame = 100;
        equal[i].primary_slot = true;
        equal[i].event_lane = static_cast<std::uint8_t>(i + 1);
        equal[i].centre_key = 3;
    }
    std::array<NativeCandidateKey, 5> scratch{};
    // Distinct lanes make the expected order explicit; a stable sort by lane.
    CHECK(SortCandidateKeys(equal, scratch) == CandidateSortStatus::Ok);
    for (std::size_t i = 0; i < equal.size(); ++i) {
        CHECK(equal[i].event_lane == static_cast<std::uint8_t>(i + 1));
    }
    // Truly identical records: order preserved.
    std::array<NativeCandidateKey, 4> identical{};
    for (auto& key : identical) {
        key.frame = 50;
        key.primary_slot = true;
        key.event_lane = 7;
        key.centre_key = 7;
        key.distance = 0.0f;
    }
    std::array<NativeCandidateKey, 4> identicalScratch{};
    CHECK(SortCandidateKeys(identical, identicalScratch) == CandidateSortStatus::Ok);

    // A scrambled frame order sorts ascending when the band is wide enough for
    // every pair (the comparator then reaches key 3).
    std::array<NativeCandidateKey, 6> keys{};
    const std::int32_t frames[6] = {40, 10, 30, 20, 60, 50};
    for (std::size_t i = 0; i < keys.size(); ++i) {
        keys[i].frame = frames[i];
        keys[i].primary_slot = true;
        keys[i].event_lane = 1;
        keys[i].centre_key = 1;
    }
    std::array<NativeCandidateKey, 6> sortScratch{};
    CHECK(SortCandidateKeys(keys, sortScratch, 1000) == CandidateSortStatus::Ok);
    for (std::size_t i = 1; i < keys.size(); ++i) {
        CHECK(keys[i - 1].frame < keys[i].frame);
    }

    // Primary slots come first inside one band, then the frame decides.
    std::array<NativeCandidateKey, 4> mixed{};
    mixed[0].frame = 10;
    mixed[0].primary_slot = false;
    mixed[0].distance = 0.0f;
    mixed[1].frame = 11;
    mixed[1].primary_slot = true;
    mixed[1].distance = 1.0f;
    mixed[2].frame = 10;
    mixed[2].primary_slot = true;
    mixed[2].distance = 2.0f;
    mixed[3].frame = 10;
    mixed[3].primary_slot = true;
    mixed[3].distance = 0.5f;
    std::array<NativeCandidateKey, 4> mixedScratch{};
    CHECK(SortCandidateKeys(mixed, mixedScratch, 1) == CandidateSortStatus::Ok);
    // Primary records first and, inside the frame band, the smaller distance wins
    // before the exact frame does: D(10, 0.5), C(10, 2.0), B(11, 1.0), A.
    CHECK(mixed[0].primary_slot);
    CHECK(mixed[0].distance == 0.5f);
    CHECK(mixed[1].primary_slot);
    CHECK(mixed[1].distance == 2.0f);
    CHECK(mixed[2].primary_slot);
    CHECK(mixed[2].frame == 11);
    CHECK(!mixed[3].primary_slot);

    // A short scratch buffer refuses and changes nothing.
    std::array<NativeCandidateKey, 4> guarded{};
    for (std::size_t i = 0; i < guarded.size(); ++i) guarded[i].frame = frames[i];
    std::array<NativeCandidateKey, 1> tiny{};
    const std::array<NativeCandidateKey, 4> before = guarded;
    CHECK(SortCandidateKeys(guarded, tiny) == CandidateSortStatus::InsufficientScratch);
    for (std::size_t i = 0; i < guarded.size(); ++i) {
        CHECK(guarded[i].frame == before[i].frame);
    }
    // A zero- or one-element span needs no scratch at all.
    std::array<NativeCandidateKey, 0> none{};
    CHECK(SortCandidateKeys(none, tiny) == CandidateSortStatus::Ok);
}

// ===========================================================================
// A4: normal-note and chart field decode
// ===========================================================================

void TestNoteDecode() {
    NativeChartFields chart;
    chart.group = 1;
    chart.lane = 52;      // a channel id, never a 1..28 key
    chart.frame = 120;
    chart.keysound_id = 9;
    chart.chord_key = 33;
    chart.left_key = 10;
    chart.right_key = 12;
    chart.auto_play = false;
    chart.sprite_select = 1;

    NativeNoteFields note;
    CHECK(DecodeNormalNote(chart, 2, 0, false, false, note) == NoteDecodeStatus::Ok);
    CHECK(note.left_key == 10);
    CHECK(note.right_key == 12);
    CHECK(note.centre_key == 11);   // note+52 = (10 + 12) / 2
    CHECK(note.mid_key == 11.0f);   // note+56 = (float)(10 + 12) * 0.5f
    CHECK(note.key_span == 2);      // note+64 = right - left
    CHECK(note.candidate_slot == 2);        // note+84
    CHECK(note.unset_link == -1);           // note+112
    CHECK(note.flags == 1);                 // note+204
    CHECK(note.keysound_index == 0);        // note+168 is not decoded yet
    CHECK(!note.keysound_index_known);
    CHECK(note.sprite_offset == 0);         // chart+100 gate is closed

    // chart+100 mapping, only with HIDWORD(qword_1813E4730) == 1.
    CHECK(SpriteOffsetForChart(1, false) == 0);
    CHECK(SpriteOffsetForChart(1, true) == 6);
    CHECK(SpriteOffsetForChart(3, true) == 7);
    CHECK(SpriteOffsetForChart(0, true) == 0);
    CHECK(SpriteOffsetForChart(2, true) == 0);
    CHECK(DecodeNormalNote(chart, 1, 0, false, true, note) == NoteDecodeStatus::Ok);
    CHECK(note.sprite_offset == 6);

    // A resolved keysound index is carried through; note+168 stays an index.
    CHECK(DecodeNormalNote(chart, 3, 41, true, false, note) == NoteDecodeStatus::Ok);
    CHECK(note.keysound_index == 41);
    CHECK(note.keysound_index_known);

    // Refusals: keys outside 1..28, an inverted range, a bad candidate slot.
    NativeChartFields bad = chart;
    bad.left_key = 0;
    CHECK(DecodeNormalNote(bad, 1, 0, false, false, note) == NoteDecodeStatus::InvalidKey);
    bad = chart;
    bad.right_key = 29;
    CHECK(DecodeNormalNote(bad, 1, 0, false, false, note) == NoteDecodeStatus::InvalidKey);
    bad = chart;
    bad.left_key = 12;
    bad.right_key = 10;
    CHECK(DecodeNormalNote(bad, 1, 0, false, false, note) ==
          NoteDecodeStatus::InvalidKeyRange);
    CHECK(DecodeNormalNote(chart, 0, 0, false, false, note) ==
          NoteDecodeStatus::InvalidCandidateSlot);
    CHECK(DecodeNormalNote(chart, 4, 0, false, false, note) ==
          NoteDecodeStatus::InvalidCandidateSlot);
    // A refusal leaves the output untouched.
    NativeNoteFields untouched;
    untouched.left_key = 44;
    CHECK(DecodeNormalNote(bad, 1, 0, false, false, untouched) ==
          NoteDecodeStatus::InvalidKeyRange);
    CHECK(untouched.left_key == 44);

    // MakeMatchCandidate: chart+0x14 is converted at the caller's rate.
    MatchCandidate candidate;
    CHECK(MakeMatchCandidate(chart, note, 77, 60.0, candidate) == NoteDecodeStatus::Ok);
    CHECK(candidate.note == 77);
    CHECK(candidate.note_us == 2'000'000.0); // 120 units at 60 units/s
    CHECK(candidate.left_key == 10);
    CHECK(candidate.right_key == 12);
    CHECK(candidate.mid_key == 11.0f);
    CHECK(candidate.chart_type == 1);
    CHECK(candidate.keysound_id == 9);
    CHECK(candidate.event_lane == 0); // SelectSlot supplies it from the bucket slot
    CHECK(!candidate.auto_note);
    CHECK(MakeMatchCandidate(chart, note, 1, 120.0, candidate) == NoteDecodeStatus::Ok);
    CHECK(candidate.note_us == 1'000'000.0);
    CHECK(MakeMatchCandidate(chart, note, 1, 0.0, candidate) ==
          NoteDecodeStatus::InvalidSongUnits);
    NativeChartFields group5 = chart;
    group5.group = 5;
    CHECK(MakeMatchCandidate(group5, note, 1, 60.0, candidate) ==
          NoteDecodeStatus::InvalidGroup);
    group5.group = -1;
    CHECK(MakeMatchCandidate(group5, note, 1, 60.0, candidate) ==
          NoteDecodeStatus::InvalidGroup);

    // Odd sums truncate toward zero: 10 + 11 -> 10 (native (lo + hi) / 2).
    chart.right_key = 11;
    CHECK(DecodeNormalNote(chart, 1, 0, false, false, note) == NoteDecodeStatus::Ok);
    CHECK(note.centre_key == 10);
    CHECK(note.mid_key == 10.5f);
    CHECK(MakeMatchCandidate(chart, note, 2, 60.0, candidate) == NoteDecodeStatus::Ok);
    CHECK(candidate.right_key == 11);
    CHECK(candidate.mid_key == 10.5f);
}

// ===========================================================================
// SelectSlot: the bucket slot IS the event descriptor
// ===========================================================================

MatchDecision SelectOne(NativeShadowFeed& feed, const NativeBucketTable& table,
                        std::uint32_t entry, std::uint32_t slot,
                        std::span<const MatchCandidate> notes) {
    return feed.Policy().SelectSlot(table, entry, slot, notes);
}

void TestSelectSlot() {
    NativeShadowFeed feed;
    CHECK(feed.Configure(GoodFeedConfig()) == ShadowFeedStatus::Ok);
    CHECK(feed.BeginFrame(Us(120)) == ShadowFeedStatus::Ok);

    NativeBucketTable table;
    CHECK(table.Entry(0).Append(Slot(5, 1, 4)));   // a new press on key 5
    CHECK(table.Entry(0).Append(Slot(9, 2)));      // held
    CHECK(table.Entry(0).Append(Slot(11, 3)));     // release

    const std::array<MatchCandidate, 1> notes{Note(3, 5, 5, Us(120))};
    MatchDecision decision = SelectOne(feed, table, 0, 0, notes);
    CHECK(decision.status == MatchStatus::Ok);
    CHECK(decision.action == MatchAction::Judge);
    CHECK(decision.note == 3);
    CHECK(decision.lane == 5);
    CHECK(decision.slot == MatchSlot::SameKey);
    CHECK(decision.event_state == 1);
    CHECK(decision.left_key == 5);
    CHECK(decision.right_key == 5);
    CHECK(decision.chart_group == 0);
    CHECK(decision.keysound_id == 7);
    CHECK(decision.candidate_index == 0);

    // Only state 1 may judge; a repeat and a release are refused.
    CHECK(SelectOne(feed, table, 0, 1, notes).status == MatchStatus::SlotNotPress);
    CHECK(SelectOne(feed, table, 0, 2, notes).status == MatchStatus::SlotNotPress);
    // Out-of-range entry, slot and a broken table.
    CHECK(SelectOne(feed, table, 30, 0, notes).status == MatchStatus::InvalidEntry);
    CHECK(SelectOne(feed, table, 0, 3, notes).status == MatchStatus::InvalidSlotIndex);
    NativeBucketTable broken;
    broken.Entry(0).count = 1;
    broken.Entry(0).slots[0] = Slot(0, 1);
    CHECK(SelectOne(feed, broken, 0, 0, notes).status == MatchStatus::InvalidBucket);

    // An unconfigured policy answers NotConfigured before anything else.
    NativeShadowFeed cold;
    CHECK(SelectOne(cold, table, 0, 0, notes).status == MatchStatus::NotConfigured);

    // The slot lane replaces every candidate's own event lane.
    const std::array<MatchCandidate, 1> laneZero{Note(4, 5, 5, Us(120))};
    CHECK(SelectOne(feed, table, 0, 0, laneZero).status == MatchStatus::Ok);
}

// ===========================================================================
// Scoring: the note+84 override stays fail-closed
// ===========================================================================

void TestSlotOverrideGate() {
    CHECK(!nosfix::kSlotOverrideVerified);
    CHECK(DecideSlotOverride(1, 4) == SlotOverrideDecision::NotApplicable);
    CHECK(DecideSlotOverride(0, 4) == SlotOverrideDecision::NotApplicable);
    CHECK(DecideSlotOverride(3, 1) == SlotOverrideDecision::NotApplicable);
    CHECK(DecideSlotOverride(3, 0) == SlotOverrideDecision::NotApplicable);
    CHECK(DecideSlotOverride(2, 4) == SlotOverrideDecision::Unverified);
    CHECK(DecideSlotOverride(3, 3) == SlotOverrideDecision::Unverified);
    CHECK(DecideSlotOverride(2, 4, true) == SlotOverrideDecision::Applies);
    CHECK(DecideSlotOverride(2, 1, true) == SlotOverrideDecision::NotApplicable);
    // The R1 scoring arithmetic is untouched: Apply still reproduces 0x303EDC.
    nosfix::NativeScoringState state;
    CHECK(state.Configure(NativeScoringConfig{}) == nosfix::ScoreStatus::Ok);
    nosfix::ScoreInput input;
    input.grade = 4;
    input.event_state = 1;
    ScoreEffect effect;
    CHECK(state.Apply(input, effect) == nosfix::ScoreStatus::Ok);
    CHECK(!effect.grade_overridden);
    CHECK(effect.grade == 4);
}

// ===========================================================================
// The shadow feed
// ===========================================================================

void TestFeedConfigureRefusals() {
    NativeShadowFeed feed;
    CHECK(!feed.IsConfigured());
    CHECK(!feed.ModelVerified()); // kNativeMatchModelVerified is still false

    ShadowFeedConfig config = GoodFeedConfig();
    config.evidence = NativeShadowEvidence{};
    CHECK(feed.Configure(config) == ShadowFeedStatus::EvidenceIncomplete);
    // Every single flag is required.
    const int flag_count = 6;
    for (int index = 0; index < flag_count; ++index) {
        NativeShadowEvidence evidence = CompleteShadowEvidence();
        switch (index) {
        case 0: evidence.bucket_layout_known = false; break;
        case 1: evidence.bucket_rotation_known = false; break;
        case 2: evidence.bucket_compaction_known = false; break;
        case 3: evidence.candidate_ordering_known = false; break;
        case 4: evidence.note_field_layout_known = false; break;
        default: evidence.song_unit_rate_known = false; break;
        }
        CHECK(!evidence.Complete());
        config = GoodFeedConfig();
        config.evidence = evidence;
        CHECK(feed.Configure(config) == ShadowFeedStatus::EvidenceIncomplete);
    }
    // The two unproven rules are feature gates, not Configure gates: they keep
    // Complete() true.
    NativeShadowEvidence gates = CompleteShadowEvidence();
    gates.slot_override_known = false;
    gates.group_phase_known = false;
    CHECK(gates.Complete());

    // The compaction phase 2 needs its evidence flag before it can be switched on.
    NativeShadowFeed phasedFeed;
    config = GoodFeedConfig();
    config.compact.group_phase_enabled = true;
    CHECK(phasedFeed.Configure(config) == ShadowFeedStatus::GroupPhaseNotEvidenced);
    config.evidence.group_phase_known = true;
    CHECK(phasedFeed.Configure(config) == ShadowFeedStatus::Ok);

    // The R1 gates still apply through the feed.
    NativeShadowFeed r1;
    config = GoodFeedConfig();
    config.match_evidence = NativeMatchEvidence{};
    CHECK(r1.Configure(config) == ShadowFeedStatus::InvalidMatchConfig);
    config = GoodFeedConfig();
    config.match.windows = NormalWindows{};
    CHECK(r1.Configure(config) == ShadowFeedStatus::InvalidMatchConfig);
    config = GoodFeedConfig();
    config.match.layout_sampled = false;
    CHECK(r1.Configure(config) == ShadowFeedStatus::InvalidMatchConfig);
    config = GoodFeedConfig();
    config.scoring.combo_limit = 0;
    CHECK(r1.Configure(config) == ShadowFeedStatus::InvalidScoringConfig);
    CHECK(!r1.IsConfigured());

    // Nothing above configured anything.
    CHECK(!feed.IsConfigured());
    std::array<NativeNoteOutcome, kSlots> outcomes{};
    std::array<ScoreEffect, kSlots> effects{};
    std::size_t outcome_count = 0;
    std::size_t effect_count = 0;
    const std::array<MatchCandidate, 0> notes{};
    CHECK(feed.FeedFrame(EmptyStaging(), notes, outcomes, outcome_count, effects,
                         effect_count) == ShadowFeedStatus::NotConfigured);
    CHECK(outcome_count == 0);
    CHECK(effect_count == 0);
}

void TestFeedFrameLifecycle() {
    NativeShadowFeed feed;
    CHECK(feed.Configure(GoodFeedConfig()) == ShadowFeedStatus::Ok);
    std::array<NativeNoteOutcome, kSlots> outcomes{};
    std::array<ScoreEffect, kSlots> effects{};
    std::size_t outcome_count = 0;
    std::size_t effect_count = 0;
    const std::array<MatchCandidate, 0> notes{};

    // A frame must be opened first.
    CHECK(feed.FeedFrame(EmptyStaging(), notes, outcomes, outcome_count, effects,
                         effect_count) == ShadowFeedStatus::FrameNotOpen);
    CHECK(feed.BeginFrame(Us(100)) == ShadowFeedStatus::Ok);
    CHECK(feed.Counts().frames == 1);
    // Non-finite and backward times are refused and close the frame.
    CHECK(feed.BeginFrame(std::numeric_limits<double>::quiet_NaN()) ==
          ShadowFeedStatus::NonFiniteTime);
    CHECK(feed.BeginFrame(Us(99)) == ShadowFeedStatus::TimeBackwards);
    CHECK(feed.FeedFrame(EmptyStaging(), notes, outcomes, outcome_count, effects,
                         effect_count) == ShadowFeedStatus::FrameNotOpen);
    CHECK(feed.BeginFrame(Us(100)) == ShadowFeedStatus::Ok); // equal time is allowed
    // A staging buffer of the wrong length is refused before anything rotates.
    std::array<NativeBucketSlot, 4> shortStaging{};
    CHECK(feed.FeedFrame(shortStaging, notes, outcomes, outcome_count, effects,
                         effect_count) == ShadowFeedStatus::InvalidStaging);
    CHECK(feed.Counts().rotations == 0);
    // Insufficient capacity is refused before the bucket table mutates.
    auto staging = EmptyStaging();
    staging[0] = Slot(5, 1);
    std::array<NativeNoteOutcome, 1> tinyOutcomes{};
    std::array<ScoreEffect, 1> tinyEffects{};
    CHECK(feed.FeedFrame(staging, notes, tinyOutcomes, outcome_count, tinyEffects,
                         effect_count) == ShadowFeedStatus::InsufficientCapacity);
    CHECK(feed.Bucket().Live().count == 0);
    CHECK(feed.Bucket().Validate() == BucketStatus::Ok);
    CHECK(feed.Counts().rotations == 0);
}

void TestFeedScoresAPress() {
    NativeShadowFeed feed;
    ShadowFeedConfig config = GoodFeedConfig();
    config.scoring.history_enabled = true;
    CHECK(feed.Configure(config) == ShadowFeedStatus::Ok);
    CHECK(feed.BeginFrame(Us(120)) == ShadowFeedStatus::Ok);

    auto staging = EmptyStaging();
    staging[0] = Slot(5, 1, 6);  // key 5, new, value 6
    staging[1] = Slot(9, 0);     // free: not an event
    const std::array<MatchCandidate, 1> notes{Note(11, 5, 5, Us(120))};
    std::array<NativeNoteOutcome, kSlots> outcomes{};
    std::array<ScoreEffect, kSlots> effects{};
    std::size_t outcome_count = 0;
    std::size_t effect_count = 0;
    CHECK(feed.FeedFrame(staging, notes, outcomes, outcome_count, effects, effect_count) ==
          ShadowFeedStatus::Ok);
    CHECK(outcome_count == 1);
    CHECK(effect_count == 1);
    CHECK(outcomes[0].note_index == 11);
    CHECK(outcomes[0].lane == 5);
    CHECK(outcomes[0].kind == NoteOutcomeKind::Scored);
    CHECK(outcomes[0].rank == NormalRank::Just);
    CHECK(outcomes[0].effect_grade == 0);
    CHECK(outcomes[0].at_us == Us(120));
    CHECK(outcomes[0].input_us == Us(120));
    CHECK(outcomes[0].sound_id == 0); // the keysound handle is not claimed
    CHECK(effects[0].grade == static_cast<std::uint8_t>(ScoreGrade::Just));
    CHECK(!effects[0].grade_overridden);
    CHECK(effects[0].combo_after == 1);
    CHECK(effects[0].score == 100);
    CHECK(effects[0].history_appended);
    CHECK(feed.Counts().frames == 1);
    CHECK(feed.Counts().rotations == 1);
    CHECK(feed.Counts().staging_slots == 28);
    CHECK(feed.Counts().compacted_slots == 1);
    CHECK(feed.Counts().decisions == 1);
    CHECK(feed.Counts().judged == 1);
    CHECK(feed.Counts().scored == 1);
    CHECK(feed.Scoring().Combo() == 1);
    CHECK(feed.Bucket().Live().count == 1);
    CHECK(feed.Bucket().Entry(1).count == 0); // first frame: nothing to rotate in

    // The record ring keeps the evaluated slot for inspection.
    std::array<ShadowFeedRecord, 4> records{};
    CHECK(feed.RecentRecords(records) == 1);
    CHECK(records[0].note_index == 11);
    CHECK(records[0].lane == 5);
    CHECK(records[0].rank == static_cast<std::uint8_t>(NormalRank::Just));
    CHECK(records[0].event_state == 1);
    CHECK(!records[0].slot_override_unproven);
    // A ring that is too small reports nothing rather than truncating.
    std::array<ShadowFeedRecord, 0> none{};
    CHECK(feed.RecentRecords(none) == 0);

    // Next frame: the previous record moved to entry 1.
    CHECK(feed.BeginFrame(Us(130)) == ShadowFeedStatus::Ok);
    auto empty = EmptyStaging();
    CHECK(feed.FeedFrame(empty, notes, outcomes, outcome_count, effects, effect_count) ==
          ShadowFeedStatus::Ok);
    CHECK(outcome_count == 0);
    CHECK(effect_count == 0);
    CHECK(feed.Bucket().Live().count == 0);
    CHECK(feed.Bucket().Entry(1).slots[0].key == 5);
    CHECK(feed.Counts().rotations == 2);
}

void TestFeedRanksAndGates() {
    NativeShadowFeed feed;
    CHECK(feed.Configure(GoodFeedConfig()) == ShadowFeedStatus::Ok);

    std::array<NativeNoteOutcome, kSlots> outcomes{};
    std::array<ScoreEffect, kSlots> effects{};
    std::size_t outcome_count = 0;
    std::size_t effect_count = 0;
    // Every judgement pushes the lane horizon over +-4 lanes, so each case uses a
    // lane outside the earlier windows.
    auto LaneStaging = [](std::int32_t key) {
        auto staging = EmptyStaging();
        staging[0] = Slot(key, 1);
        return staging;
    };

    // A late Great press: 3 units past the note, inside Great (4) but outside
    // Just (2). effect_grade 2 marks a late non-Just grade.
    CHECK(feed.BeginFrame(Us(123)) == ShadowFeedStatus::Ok);
    const std::array<MatchCandidate, 1> notes{Note(21, 5, 5, Us(120))};
    auto staging = LaneStaging(5);
    CHECK(feed.FeedFrame(staging, notes, outcomes, outcome_count, effects, effect_count) ==
          ShadowFeedStatus::Ok);
    CHECK(outcome_count == 1);
    CHECK(outcomes[0].rank == NormalRank::Great);
    CHECK(outcomes[0].effect_grade == 2);
    CHECK(effects[0].grade == static_cast<std::uint8_t>(ScoreGrade::Great));
    CHECK(effects[0].combo_after == 1);

    // An early Good press: 6 units before the note, inside Good (7) but outside
    // Great (4). effect_grade 1 marks an early non-Just grade.
    CHECK(feed.BeginFrame(Us(200)) == ShadowFeedStatus::Ok);
    const std::array<MatchCandidate, 1> early{Note(22, 10, 10, Us(206))};
    staging = LaneStaging(10);
    CHECK(feed.FeedFrame(staging, early, outcomes, outcome_count, effects, effect_count) ==
          ShadowFeedStatus::Ok);
    CHECK(outcome_count == 1);
    CHECK(outcomes[0].rank == NormalRank::Good);
    CHECK(outcomes[0].effect_grade == 1);
    CHECK(effects[0].grade == static_cast<std::uint8_t>(ScoreGrade::Good));

    // TooEarly consumes nothing: no outcome, no effect, no combo change.
    CHECK(feed.BeginFrame(Us(300)) == ShadowFeedStatus::Ok);
    const std::array<MatchCandidate, 1> far{Note(23, 15, 15, Us(400))};
    staging = LaneStaging(15);
    const std::int32_t combo_before = feed.Scoring().Combo();
    CHECK(feed.FeedFrame(staging, far, outcomes, outcome_count, effects, effect_count) ==
          ShadowFeedStatus::Ok);
    CHECK(outcome_count == 0);
    CHECK(effect_count == 0);
    CHECK(feed.Counts().early_ignored == 1);
    CHECK(feed.Scoring().Combo() == combo_before);

    // A press past the late window is a Missed rank: it consumes and resets.
    CHECK(feed.BeginFrame(Us(500)) == ShadowFeedStatus::Ok);
    const std::array<MatchCandidate, 1> gone{Note(24, 20, 20, Us(400))};
    staging = LaneStaging(20);
    CHECK(feed.FeedFrame(staging, gone, outcomes, outcome_count, effects, effect_count) ==
          ShadowFeedStatus::Ok);
    CHECK(outcome_count == 1);
    CHECK(outcomes[0].rank == NormalRank::Missed);
    CHECK(outcomes[0].effect_grade == 2);
    CHECK(effects[0].grade == static_cast<std::uint8_t>(ScoreGrade::Miss));
    CHECK(effects[0].combo_reset);
    CHECK(feed.Counts().late_presses == 1);
    CHECK(feed.Scoring().Combo() == 0);

    // chart+48 == 2 (keysound id 2) disables the score but not the judgement.
    NativeShadowFeed gated;
    CHECK(gated.Configure(GoodFeedConfig()) == ShadowFeedStatus::Ok);
    CHECK(gated.BeginFrame(Us(120)) == ShadowFeedStatus::Ok);
    const std::array<MatchCandidate, 1> silent{Note(25, 5, 5, Us(120), 0, 2)};
    staging = LaneStaging(5);
    CHECK(gated.FeedFrame(staging, silent, outcomes, outcome_count, effects,
                          effect_count) == ShadowFeedStatus::Ok);
    CHECK(outcome_count == 1);
    CHECK(effect_count == 0);
    CHECK(gated.Counts().not_scored == 1);
    CHECK(gated.Scoring().Combo() == 0);
}

void TestFeedHeldReleaseAndMisses() {
    NativeShadowFeed feed;
    CHECK(feed.Configure(GoodFeedConfig()) == ShadowFeedStatus::Ok);
    CHECK(feed.BeginFrame(Us(120)) == ShadowFeedStatus::Ok);
    auto staging = EmptyStaging();
    staging[0] = Slot(5, 2);  // held
    staging[1] = Slot(7, 3);  // release
    staging[2] = Slot(9, 0);  // free
    const std::array<MatchCandidate, 1> notes{Note(31, 5, 5, Us(120))};
    std::array<NativeNoteOutcome, kSlots> outcomes{};
    std::array<ScoreEffect, kSlots> effects{};
    std::size_t outcome_count = 0;
    std::size_t effect_count = 0;
    CHECK(feed.FeedFrame(staging, notes, outcomes, outcome_count, effects, effect_count) ==
          ShadowFeedStatus::Ok);
    // Only the release produces an outcome; the held slot is not a judgement.
    CHECK(outcome_count == 1);
    CHECK(outcomes[0].kind == NoteOutcomeKind::KeysoundCut);
    CHECK(outcomes[0].lane == 7);
    CHECK(outcomes[0].note_index == 0);
    CHECK(effect_count == 0);
    CHECK(feed.Counts().releases == 1);
    CHECK(feed.Counts().judged == 0);
    CHECK(feed.Counts().decisions == 0);

    // A new slot with no candidate: counted, nothing produced.
    CHECK(feed.BeginFrame(Us(130)) == ShadowFeedStatus::Ok);
    auto second = EmptyStaging();
    second[0] = Slot(5, 1);
    const std::array<MatchCandidate, 0> none{};
    CHECK(feed.FeedFrame(second, none, outcomes, outcome_count, effects, effect_count) ==
          ShadowFeedStatus::Ok);
    CHECK(outcome_count == 0);
    CHECK(effect_count == 0);
    CHECK(feed.Counts().no_candidate == 1);
    CHECK(feed.Counts().decisions == 1);
}

void TestFeedSlotOverrideStaysFailClosed() {
    // A note whose key range is 10..12 answered from lane 14 lands in the right
    // slot, so the record's event state is 3 and note+84 would become 3.
    const std::array<MatchCandidate, 1> notes{Note(41, 10, 12, Us(120))};
    std::array<NativeNoteOutcome, kSlots> outcomes{};
    std::array<ScoreEffect, kSlots> effects{};
    std::size_t outcome_count = 0;
    std::size_t effect_count = 0;
    auto staging = EmptyStaging();
    staging[0] = Slot(14, 1);

    NativeShadowFeed unproven;
    CHECK(unproven.Configure(GoodFeedConfig()) == ShadowFeedStatus::Ok);
    CHECK(unproven.BeginFrame(Us(123)) == ShadowFeedStatus::Ok); // a late Great
    CHECK(unproven.FeedFrame(staging, notes, outcomes, outcome_count, effects,
                             effect_count) == ShadowFeedStatus::Ok);
    CHECK(outcome_count == 1);
    CHECK(effect_count == 1);
    CHECK(outcomes[0].rank == NormalRank::Great);
    // The rank-5 override is NOT applied while note+84 > 1 is unproven.
    CHECK(!effects[0].grade_overridden);
    CHECK(effects[0].grade == static_cast<std::uint8_t>(ScoreGrade::Great));
    CHECK(effects[0].score == 100);
    CHECK(unproven.Counts().slot_override_unproven == 1);
    CHECK(unproven.ModelVerified() == false);
    std::array<ShadowFeedRecord, 4> records{};
    CHECK(unproven.RecentRecords(records) == 1);
    CHECK(records[0].event_state == 3);
    CHECK(records[0].slot_override_unproven);

    // With the trigger declared proven the branch applies (and only then).
    NativeShadowFeed proven;
    ShadowFeedConfig config = GoodFeedConfig();
    config.evidence.slot_override_known = true;
    CHECK(proven.Configure(config) == ShadowFeedStatus::Ok);
    CHECK(proven.BeginFrame(Us(123)) == ShadowFeedStatus::Ok);
    CHECK(proven.FeedFrame(staging, notes, outcomes, outcome_count, effects, effect_count) ==
          ShadowFeedStatus::Ok);
    CHECK(effect_count == 1);
    CHECK(effects[0].grade_overridden);
    CHECK(effects[0].grade == static_cast<std::uint8_t>(ScoreGrade::Special));
    CHECK(effects[0].score == 100);
    CHECK(proven.Counts().slot_override_unproven == 0);
}

void TestShadowDivergenceThroughTheFeed() {
    NativeShadowFeed feed;
    CHECK(feed.Configure(GoodFeedConfig()) == ShadowFeedStatus::Ok);
    CHECK(feed.BeginFrame(Us(120)) == ShadowFeedStatus::Ok);
    auto staging = EmptyStaging();
    staging[0] = Slot(5, 1);
    const std::array<MatchCandidate, 1> notes{Note(51, 5, 5, Us(120))};
    std::array<NativeNoteOutcome, kSlots> outcomes{};
    std::array<ScoreEffect, kSlots> effects{};
    std::size_t outcome_count = 0;
    std::size_t effect_count = 0;
    CHECK(feed.FeedFrame(staging, notes, outcomes, outcome_count, effects, effect_count) ==
          ShadowFeedStatus::Ok);
    CHECK(outcome_count == 1);

    // The parent compares the adapter's stream against this one.
    NativeShadowComparison comparison;
    comparison.Configure(0.0);
    const ShadowOutcome native =
        NativeShadowComparison::FromAdapter(outcomes[0], effects[0].score);
    CHECK(!comparison.Observe(native, native).diverged);
    CHECK(comparison.Compared() == 1);
    ShadowOutcome adapter = native;
    adapter.rank = static_cast<std::uint8_t>(NormalRank::Great);
    const nosfix::ShadowDivergence divergence = comparison.Observe(adapter, native);
    CHECK(divergence.diverged);
    CHECK(divergence.field == nosfix::ShadowField::Rank);
    CHECK(divergence.note_index == 51);
    CHECK(divergence.at_us == Us(120));
    comparison.Reset();
    CHECK(!comparison.Diverged());

    // A score divergence is caught too.
    adapter = native;
    adapter.score = native.score - 100;
    CHECK(comparison.Observe(adapter, native).field == nosfix::ShadowField::Score);
}

// A captured bucket table can be replayed without compaction (LoadBucket), which
// is how the parent will feed a decode of 0x1813DEA20 straight in.
void TestFeedLoadBucket() {
    NativeShadowFeed feed;
    CHECK(feed.Configure(GoodFeedConfig()) == ShadowFeedStatus::Ok);
    CHECK(feed.BeginFrame(Us(120)) == ShadowFeedStatus::Ok);

    NativeBucketTable captured;
    CHECK(captured.Entry(0).Append(Slot(6, 1, 2, 1, 0)));
    CHECK(feed.LoadBucket(captured) == ShadowFeedStatus::Ok);
    CHECK(feed.Bucket().Live().count == 1);

    const std::array<MatchCandidate, 1> notes{Note(61, 6, 6, Us(120))};
    std::array<NativeNoteOutcome, kSlots> outcomes{};
    std::array<ScoreEffect, kSlots> effects{};
    std::size_t outcome_count = 0;
    std::size_t effect_count = 0;
    CHECK(feed.FeedLiveRecord(notes, outcomes, outcome_count, effects, effect_count) ==
          ShadowFeedStatus::Ok);
    CHECK(outcome_count == 1);
    CHECK(effect_count == 1);
    CHECK(outcomes[0].lane == 6);
    CHECK(feed.Scoring().Combo() == 1);

    // A sample that does not validate is refused and the current table stays.
    NativeBucketTable broken;
    broken.Entry(0).count = 1;
    broken.Entry(0).slots[0] = Slot(99, 1);
    CHECK(feed.LoadBucket(broken) == ShadowFeedStatus::InvalidBucket);
    CHECK(feed.Bucket().Live().count == 1);
    CHECK(feed.Bucket().Live().slots[0].key == 6);

    // Without a frame open the record cannot be evaluated.
    NativeShadowFeed cold;
    CHECK(cold.LoadBucket(captured) == ShadowFeedStatus::NotConfigured);
    CHECK(cold.Configure(GoodFeedConfig()) == ShadowFeedStatus::Ok);
    CHECK(cold.LoadBucket(captured) == ShadowFeedStatus::Ok);
    CHECK(cold.FeedLiveRecord(notes, outcomes, outcome_count, effects, effect_count) ==
          ShadowFeedStatus::FrameNotOpen);
}

// The adapter is still refused by NativeNoteState: no configuration can reach any
// of the round-2 types.
void TestAdapterRefusesNativeSelection() {
    nosfix::NativeNoteState state;
    std::array<nosfix::NativeNoteSpec, 1> notes{};
    notes[0].chart_index = 1;
    nosfix::NativeNoteConfig config;
    config.windows = UsWindows();
    config.windows_sampled = true;
    config.selection = nosfix::NoteSelectionPolicy::Native2F7C10;
    CHECK(state.Configure(notes, config) == nosfix::NoteConfigStatus::UnsupportedSelection);
}

} // namespace

int main() {
    TestSongUnits();
    TestBucketLayout();
    TestBucketRotation();
    TestBucketCompaction();
    TestCandidateComparator();
    TestCandidateSort();
    TestNoteDecode();
    TestSelectSlot();
    TestSlotOverrideGate();
    TestFeedConfigureRefusals();
    TestFeedFrameLifecycle();
    TestFeedScoresAPress();
    TestFeedRanksAndGates();
    TestFeedHeldReleaseAndMisses();
    TestFeedSlotOverrideStaysFailClosed();
    TestShadowDivergenceThroughTheFeed();
    TestFeedLoadBucket();
    TestAdapterRefusesNativeSelection();
    return test::Finish("native_shadow_adapter");
}
