// Bucket input: staging-buffer decode, ordered press/repeat/release events,
// generation and time handling, bucket rotation/compaction, the PANB threshold
// provenance, the velocity layout facts and the bridge mapping.
//
// Evidence: analyze/impl-20261030/ABI_COMPLETION.md A1/B6/B7, PROBE_ANALYSIS.md
// s1/s11, PRODUCER_FINDING.md.

#include "test_support.hpp"

#include "nosfix/bucket_input.hpp"
#include "nosfix/native_input_bridge.hpp"
#include "nosfix/native_note_types.hpp"
#include "nosfix/panb_pressure.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <string_view>

namespace {

using namespace nosfix;

using Staging = std::array<std::byte, kBucketStagingBytes>;

void WriteU32(Staging& staging, std::size_t offset, std::uint32_t value) {
    for (std::size_t index = 0; index < 4; ++index) {
        staging[offset + index] = static_cast<std::byte>((value >> (8 * index)) & 0xFFu);
    }
}

void WriteI32(Staging& staging, std::size_t offset, std::int32_t value) {
    WriteU32(staging, offset, static_cast<std::uint32_t>(value));
}

void WriteU8(Staging& staging, std::size_t offset, std::uint8_t value) {
    staging[offset] = static_cast<std::byte>(value);
}

void SetSlot(Staging& staging, std::size_t slot, std::int32_t key, std::int32_t state,
             std::uint8_t value = 0, std::uint32_t group = 0, std::uint32_t repeat = 0) {
    const std::size_t base = slot * kBucketSlotBytes;
    WriteI32(staging, base + bucket_input_layout::kSlotKey, key);
    WriteI32(staging, base + bucket_input_layout::kSlotState, state);
    WriteU8(staging, base + bucket_input_layout::kSlotValue, value);
    WriteU32(staging, base + bucket_input_layout::kSlotGroup, group);
    WriteU32(staging, base + bucket_input_layout::kSlotRepeat, repeat);
}

SongTime FrameTime(std::uint64_t generation, double microseconds) {
    SongTime time;
    time.generation = generation;
    time.microseconds = microseconds;
    return time;
}

// ===========================================================================
// Layout constants and the two pure helpers
// ===========================================================================
void TestGeometry() {
    static_assert(kBucketSlotCount == 28);
    static_assert(kBucketSlotBytes == 28);
    static_assert(kBucketStagingBytes == 784);
    static_assert(kBucketTableCount == 30);
    static_assert(kBucketEntryBytes == 792);
    static_assert(kBucketEntryCountOffset == 784);
    static_assert(bucket_input_layout::kSlotKey == 0x00);
    static_assert(bucket_input_layout::kSlotState == 0x04);
    static_assert(bucket_input_layout::kSlotValue == 0x08);
    static_assert(bucket_input_layout::kSlotGroup == 0x14);
    static_assert(bucket_input_layout::kSlotRepeat == 0x18);
    static_assert(bucket_input_layout::kSelectedBucket == 0);
    static_assert(bucket_input_layout::kPreviousBucket == 1);
    static_assert(kNativeInputKeyMin == 1);
    static_assert(kNativeInputKeyMax == 28);
    static_assert(NativeLaneFromKey(1) == 0);
    static_assert(NativeLaneFromKey(28) == 27);
    static_assert(IsNativeInputKey(1) && IsNativeInputKey(28));
    static_assert(!IsNativeInputKey(0) && !IsNativeInputKey(29));
    static_assert(velocity_layout::kFlagOffset == 296);
    static_assert(velocity_layout::kStateOffset == 300);
    static_assert(velocity_layout::kSpanOffset == 316);
    static_assert(velocity_layout::kActiveOffset == 308);
    static_assert(velocity_layout::kDistanceNumerator == 180);
    static_assert(!velocity_layout::kFlagWriterLocated);
    static_assert(!velocity_layout::kStateIsTwoStateFlag);

    CHECK(bucket_input_layout::kProducerRva == 0x305B30);
    CHECK(bucket_input_layout::kRotateRva == 0x307090);
    CHECK(bucket_input_layout::kCompactRva == 0x307170);
    CHECK(bucket_input_layout::kStagingPairRva == 0x13DEA10);
    CHECK(bucket_input_layout::kBucketTableRva == 0x13DEA20);
    CHECK(bucket_input_layout::kScanCountRva == 0x13DF048);
    CHECK(bucket_input_layout::kSelectedBucketRva == 0x1366910);
    CHECK(bucket_input_layout::kGroupAssignmentToggleRva == 0x13E4A8E);
    CHECK(bucket_input_layout::kRepeatLimit == 25);
    CHECK(velocity_layout::kVelocityInputRva == 0x1AA340);
    CHECK(velocity_layout::kVelocityRenderRva == 0x1AA4D0);
    CHECK(velocity_layout::kVelocityDistanceRva == 0x1AA64F);

    CHECK(std::string_view(BucketInputStatusName(BucketInputStatus::DuplicateKey)) == "duplicate-key");
    CHECK(std::string_view(BucketInputStatusName(BucketInputStatus::Ok)) == "ok");
    CHECK(std::string_view(BucketInputStatusName(static_cast<BucketInputStatus>(200))) == "invalid");
}

void TestVelocityDistance() {
    double units = -1.0;
    CHECK(velocity_layout::VelocitySpanDistanceUnits(0, 60.0, units));
    CHECK(units == 0.0);
    CHECK(velocity_layout::VelocitySpanDistanceUnits(562, 60.0, units));
    CHECK(std::abs(units - (180.0 * 562.0 / 3600.0)) < 1.0e-9);
    CHECK(velocity_layout::VelocitySpanDistanceUnits(820, 120.0, units));
    CHECK(std::abs(units - (180.0 * 820.0 / 14400.0)) < 1.0e-9);
    // The native computes +316 = +296 - +316, so a negative span is a legal
    // intermediate and must not be refused.
    CHECK(velocity_layout::VelocitySpanDistanceUnits(-100, 60.0, units));
    CHECK(std::abs(units + 5.0) < 1.0e-9);

    units = 42.0;
    CHECK(!velocity_layout::VelocitySpanDistanceUnits(10, 0.0, units));
    CHECK(units == 42.0);
    CHECK(!velocity_layout::VelocitySpanDistanceUnits(10, -60.0, units));
    CHECK(!velocity_layout::VelocitySpanDistanceUnits(
        10, std::numeric_limits<double>::quiet_NaN(), units));
    CHECK(!velocity_layout::VelocitySpanDistanceUnits(
        10, std::numeric_limits<double>::infinity(), units));
    CHECK(!velocity_layout::VelocitySpanDistanceUnits(10, 1.0e7, units));
    CHECK(units == 42.0);
}

// ===========================================================================
// Slot decode
// ===========================================================================
void TestSlotDecode() {
    Staging staging{};
    SetSlot(staging, 0, 11, 1, 6);
    SetSlot(staging, 1, 12, 1, 9);
    SetSlot(staging, 9, 0, 0, 2);          // the probe's idle slot
    SetSlot(staging, 20, 255, 0, 255, 0);  // junk that must stay uninterpreted
    SetSlot(staging, 27, 28, 3, 0, 0, 4);

    std::array<BucketSlot, kBucketSlotCount> slots{};
    BucketFrameCounts counts{};
    CHECK(DecodeStagingSlots(staging, slots, counts) == BucketInputStatus::Ok);
    CHECK(counts.slots_seen == 28);
    CHECK(counts.active == 3);
    CHECK(counts.presses == 2);
    CHECK(counts.releases == 1);
    CHECK(counts.repeats == 0);
    CHECK(counts.free_slots == 25);
    CHECK(counts.free_with_value == 2);
    CHECK(counts.values_nonzero == 2);
    CHECK(counts.max_repeat == 4);
    CHECK(counts.distinct_keys == 3);
    CHECK(counts.drops == 0);
    CHECK(counts.generation_changes == 0);
    CHECK(!counts.rebased);

    CHECK(slots[0].key == 11);
    CHECK(slots[0].state == 1);
    CHECK(slots[0].value == 6);
    CHECK(slots[0].group == 0);
    CHECK(slots[1].key == 12);
    CHECK(slots[1].value == 9);
    CHECK(slots[9].state == 0);
    CHECK(slots[9].key == 0);
    CHECK(slots[9].value == 2);
    CHECK(slots[20].state == 0);
    CHECK(slots[27].key == 28);
    CHECK(slots[27].state == 3);
    CHECK(slots[27].repeat == 4);

    // Every 28-byte slot is read on its own: two slots that differ only in the
    // unused dwords still decode identically to the proven fields.
    Staging other = staging;
    WriteU32(other, bucket_input_layout::kSlotUnused0, 0xDEADBEEF);
    WriteU32(other, bucket_input_layout::kSlotUnused1, 0xFEEDFACE);
    std::array<BucketSlot, kBucketSlotCount> again{};
    BucketFrameCounts other_counts{};
    CHECK(DecodeStagingSlots(other, again, other_counts) == BucketInputStatus::Ok);
    CHECK(again[0].key == slots[0].key);
    CHECK(again[0].value == slots[0].value);
    CHECK(other_counts.active == counts.active);
}

void TestSlotRefusals() {
    std::array<BucketSlot, kBucketSlotCount> slots{};
    BucketFrameCounts counts{};

    Staging staging{};
    CHECK(DecodeStagingSlots(std::span<const std::byte>(staging.data(), 783), slots, counts) ==
          BucketInputStatus::MalformedBuffer);
    CHECK(DecodeStagingSlots(
              staging, std::span<BucketSlot>(slots.data(), kBucketSlotCount - 1), counts) ==
          BucketInputStatus::InsufficientCapacity);

    SetSlot(staging, 0, 5, 4);
    CHECK(DecodeStagingSlots(staging, slots, counts) == BucketInputStatus::InvalidState);
    SetSlot(staging, 0, 5, -1);
    CHECK(DecodeStagingSlots(staging, slots, counts) == BucketInputStatus::InvalidState);
    SetSlot(staging, 0, 0, 1);
    CHECK(DecodeStagingSlots(staging, slots, counts) == BucketInputStatus::InvalidKey);
    SetSlot(staging, 0, 29, 2);
    CHECK(DecodeStagingSlots(staging, slots, counts) == BucketInputStatus::InvalidKey);
    SetSlot(staging, 0, 5, 2, 0, 0, 26);
    CHECK(DecodeStagingSlots(staging, slots, counts) == BucketInputStatus::InvalidRepeat);
    SetSlot(staging, 0, 5, 2, 0, 0, 25);
    CHECK(DecodeStagingSlots(staging, slots, counts) == BucketInputStatus::Ok);

    SetSlot(staging, 0, 7, 1);
    SetSlot(staging, 3, 7, 2);
    CHECK(DecodeStagingSlots(staging, slots, counts) == BucketInputStatus::DuplicateKey);

    // A free slot is never validated: its key, value, group and repeat are junk.
    Staging junk{};
    SetSlot(junk, 4, -3, 0, 200, 99, 99);
    CHECK(DecodeStagingSlots(junk, slots, counts) == BucketInputStatus::Ok);
    CHECK(counts.active == 0);
    CHECK(counts.free_slots == 28);
    CHECK(counts.free_with_value == 1);
}

// ===========================================================================
// Ordered events
// ===========================================================================
void TestOrderedEvents() {
    BucketInputState state;
    BucketFrameCounts not_configured{};
    CHECK(state.Apply(Staging{}, FrameTime(1, 0.0), {}, not_configured) ==
          BucketInputStatus::NotConfigured);
    CHECK(not_configured.active == 0);

    CHECK(state.Configure(BucketInputConfig{}) == BucketInputStatus::Ok);
    std::array<BucketInputEvent, kBucketMaxFrameEvents> events{};
    BucketFrameCounts counts{};

    // Frame 1: the probe's two-key chord, slots in rank order.
    Staging staging{};
    SetSlot(staging, 0, 11, 1, 6);
    SetSlot(staging, 1, 12, 1, 9);
    CHECK(state.Apply(staging, FrameTime(1, 1000.0), events, counts) == BucketInputStatus::Ok);
    CHECK(counts.active == 2);
    CHECK(counts.presses == 2);
    CHECK(events[0].key == 11);
    CHECK(events[0].lane == 10);
    CHECK(events[0].kind == NativeEventKind::Press);
    CHECK(events[0].value == 6);
    CHECK(events[0].group == 0);
    CHECK(events[0].repeat == 0);
    CHECK(events[0].slot == 0);
    CHECK(events[0].song_time.generation == 1);
    CHECK(events[0].song_time.microseconds == 1000.0);
    CHECK(events[1].key == 12);
    CHECK(events[1].lane == 11);
    CHECK(events[1].slot == 1);
    CHECK(!events[1].orphan);
    CHECK(state.FrameCount() == 1);
    CHECK(state.Generation() == 1);
    CHECK(state.LastSongTimeUs() == 1000.0);
    CHECK(state.LastState(10) == 1);
    CHECK(state.HasObservedPress(10));

    // Frame 2: both held. The native reports state 2 every frame with the repeat
    // counter the producer maintained.
    Staging held{};
    SetSlot(held, 0, 11, 2, 6, 0, 1);
    SetSlot(held, 1, 12, 2, 9, 0, 1);
    CHECK(state.Apply(held, FrameTime(1, 1016.7), events, counts) == BucketInputStatus::Ok);
    CHECK(counts.repeats == 2);
    CHECK(counts.orphan_repeats == 0);
    CHECK(events[0].kind == NativeEventKind::Repeat);
    CHECK(events[0].repeat == 1);
    CHECK(events[1].kind == NativeEventKind::Repeat);

    // Frame 3: release both. A release is a single frame.
    Staging released{};
    SetSlot(released, 0, 11, 3, 6, 0, 2);
    SetSlot(released, 1, 12, 3, 9, 0, 2);
    CHECK(state.Apply(released, FrameTime(1, 1033.4), events, counts) == BucketInputStatus::Ok);
    CHECK(counts.releases == 2);
    CHECK(events[0].kind == NativeEventKind::Release);
    CHECK(!events[0].orphan);
    CHECK(!state.HasObservedPress(10));

    // Frame 4: the slots are dropped.
    CHECK(state.Apply(Staging{}, FrameTime(1, 1050.1), events, counts) == BucketInputStatus::Ok);
    CHECK(counts.active == 0);
    CHECK(counts.drops == 2);
    CHECK(counts.dropped_while_held == 0);
    CHECK(state.LastState(10) == 0);

    // Frame 5: a new press of the same key is not "while held".
    CHECK(state.Apply(staging, FrameTime(1, 1066.8), events, counts) == BucketInputStatus::Ok);
    CHECK(counts.presses == 2);
    CHECK(counts.press_while_held == 0);

    // A producer that re-sends state 1 while the key is down is counted, not
    // hidden: the event is still a press, exactly as the native consumes it.
    CHECK(state.Apply(staging, FrameTime(1, 1083.5), events, counts) == BucketInputStatus::Ok);
    CHECK(counts.press_while_held == 2);
    CHECK(counts.presses == 2);

    CHECK(state.RejectedFrames() == 0); // NotConfigured is not a frame rejection
}

void TestOrphans() {
    BucketInputState state;
    CHECK(state.Configure(BucketInputConfig{}) == BucketInputStatus::Ok);
    std::array<BucketInputEvent, kBucketMaxFrameEvents> events{};
    BucketFrameCounts counts{};

    // A decoder that starts mid-hold sees state 2 with no press.
    Staging held{};
    SetSlot(held, 0, 20, 2, 3);
    CHECK(state.Apply(held, FrameTime(4, 500.0), events, counts) == BucketInputStatus::Ok);
    CHECK(counts.orphan_repeats == 1);
    CHECK(events[0].orphan);
    CHECK(events[0].kind == NativeEventKind::Repeat);

    // A release without a tracked press in this generation.
    Staging released{};
    SetSlot(released, 0, 20, 3, 3);
    CHECK(state.Apply(released, FrameTime(4, 516.0), events, counts) == BucketInputStatus::Ok);
    CHECK(counts.orphan_releases == 1);
    CHECK(events[0].orphan);

    // A held key whose slot vanishes is a producer anomaly, not a drop.
    Staging pressed{};
    SetSlot(pressed, 0, 21, 1, 1);
    CHECK(state.Apply(pressed, FrameTime(4, 532.0), events, counts) == BucketInputStatus::Ok);
    CHECK(state.Apply(Staging{}, FrameTime(4, 548.0), events, counts) == BucketInputStatus::Ok);
    CHECK(counts.dropped_while_held == 1);
    CHECK(counts.drops == 0);

    // refuse_orphans turns the same frame into a refusal that publishes nothing
    // and changes no tracking.
    BucketInputState strict;
    BucketInputConfig config;
    config.refuse_orphans = true;
    CHECK(strict.Configure(config) == BucketInputStatus::Ok);
    std::array<BucketInputEvent, kBucketMaxFrameEvents> untouched{};
    untouched[0].key = 999;
    BucketFrameCounts refused{};
    CHECK(strict.Apply(held, FrameTime(1, 0.0), untouched, refused) ==
          BucketInputStatus::OrphanEvent);
    CHECK(untouched[0].key == 999);
    CHECK(refused.orphan_repeats == 1);
    CHECK(strict.LastState(19) == 0);
    CHECK(strict.FrameCount() == 0);
    CHECK(strict.RejectedFrames() == 1);

    // The same state is accepted once the press has been observed.
    Staging held21{};
    SetSlot(held21, 0, 21, 2, 1, 0, 1);
    CHECK(strict.Apply(pressed, FrameTime(1, 16.0), untouched, refused) == BucketInputStatus::Ok);
    CHECK(strict.Apply(held21, FrameTime(1, 32.0), untouched, refused) == BucketInputStatus::Ok);
    CHECK(refused.orphan_repeats == 0);
}

// ===========================================================================
// Generation and time handling
// ===========================================================================
void TestGenerations() {
    BucketInputState state;
    CHECK(state.Configure(BucketInputConfig{}) == BucketInputStatus::Ok);
    std::array<BucketInputEvent, kBucketMaxFrameEvents> events{};
    BucketFrameCounts counts{};

    Staging pressed{};
    SetSlot(pressed, 0, 9, 1, 4);
    Staging held{};
    SetSlot(held, 0, 9, 2, 4, 0, 3);

    CHECK(state.Apply(pressed, FrameTime(0, 0.0), events, counts) ==
          BucketInputStatus::GenerationZero);
    CHECK(state.RejectedFrames() == 1);

    const double nan = std::numeric_limits<double>::quiet_NaN();
    CHECK(state.Apply(pressed, FrameTime(1, nan), events, counts) ==
          BucketInputStatus::NonFiniteTime);
    CHECK(state.Apply(pressed, FrameTime(1, std::numeric_limits<double>::infinity()), events,
                      counts) == BucketInputStatus::NonFiniteTime);
    CHECK(state.Apply(pressed, FrameTime(1, 1.0e13), events, counts) ==
          BucketInputStatus::NonFiniteTime);
    CHECK(state.RejectedFrames() == 4);

    CHECK(state.Apply(pressed, FrameTime(1, 100.0), events, counts) == BucketInputStatus::Ok);
    CHECK(counts.rebased);
    CHECK(counts.generation_changes == 0);
    CHECK(state.Apply(pressed, FrameTime(1, 150.0), events, counts) == BucketInputStatus::Ok);
    CHECK(counts.press_while_held == 1);

    // Within one generation the song time may not go backwards.
    CHECK(state.Apply(pressed, FrameTime(1, 120.0), events, counts) ==
          BucketInputStatus::BackwardTime);
    // Equal timestamps are accepted: a host may publish the same frame twice.
    CHECK(state.Apply(pressed, FrameTime(1, 150.0), events, counts) == BucketInputStatus::Ok);

    // An older generation is stale and must never be applied.
    Staging newer{};
    SetSlot(newer, 0, 9, 2, 4, 0, 1);
    CHECK(state.Apply(newer, FrameTime(2, 10.0), events, counts) == BucketInputStatus::Ok);
    CHECK(counts.rebased);
    CHECK(counts.generation_changes == 1);
    CHECK(counts.orphan_repeats == 1); // tracking was re-based
    CHECK(state.Generation() == 2);
    CHECK(state.Apply(pressed, FrameTime(1, 9000.0), events, counts) ==
          BucketInputStatus::StaleGeneration);
    // A new generation may legitimately restart the song clock.
    CHECK(state.Apply(newer, FrameTime(3, 0.0), events, counts) == BucketInputStatus::Ok);

    // keep_tracking_across_generations keeps the press flag alive.
    BucketInputState sticky;
    BucketInputConfig config;
    config.keep_tracking_across_generations = true;
    CHECK(sticky.Configure(config) == BucketInputStatus::Ok);
    CHECK(sticky.Apply(pressed, FrameTime(1, 0.0), events, counts) == BucketInputStatus::Ok);
    CHECK(sticky.Apply(held, FrameTime(2, 0.0), events, counts) == BucketInputStatus::Ok);
    CHECK(counts.orphan_repeats == 0);
    CHECK(counts.generation_changes == 1);
    CHECK(sticky.HasObservedPress(8));

    // Reset forgets the tracking but keeps the configuration and the counters.
    sticky.Reset();
    CHECK(sticky.Configured());
    CHECK(sticky.Generation() == 0);
    CHECK(sticky.FrameCount() == 2);
    CHECK(!sticky.HasObservedPress(8));
    CHECK(sticky.Apply(held, FrameTime(2, 0.0), events, counts) == BucketInputStatus::Ok);
    CHECK(counts.orphan_repeats == 1);
}

void TestCapacityAndRefusalIsolation() {
    BucketInputState state;
    CHECK(state.Configure(BucketInputConfig{}) == BucketInputStatus::Ok);
    std::array<BucketInputEvent, 1> one{};
    BucketFrameCounts counts{};
    Staging chord{};
    SetSlot(chord, 0, 3, 1, 1);
    SetSlot(chord, 1, 4, 1, 1);
    CHECK(state.Apply(chord, FrameTime(1, 0.0), one, counts) ==
          BucketInputStatus::InsufficientCapacity);
    CHECK(one[0].key == 0);
    CHECK(state.FrameCount() == 0);
    CHECK(state.LastState(2) == 0);

    std::array<BucketInputEvent, kBucketMaxFrameEvents> events{};
    CHECK(state.Apply(chord, FrameTime(1, 0.0), events, counts) == BucketInputStatus::Ok);
    CHECK(state.FrameCount() == 1);

    // A corrupt frame after a good one changes neither tracking nor counters
    // beyond the rejection counter.
    Staging corrupt = chord;
    SetSlot(corrupt, 2, 5, 9);
    const std::uint8_t before = state.LastState(2);
    CHECK(state.Apply(corrupt, FrameTime(1, 16.0), events, counts) ==
          BucketInputStatus::InvalidState);
    CHECK(state.LastState(2) == before);
    CHECK(state.FrameCount() == 1);
    CHECK(state.RejectedFrames() == 2);
    CHECK(counts.active == 0);
}

void TestRepeatedFrames() {
    BucketInputState state;
    CHECK(state.Configure(BucketInputConfig{}) == BucketInputStatus::Ok);
    std::array<BucketInputEvent, kBucketMaxFrameEvents> events{};
    BucketFrameCounts counts{};
    Staging held{};
    SetSlot(held, 0, 14, 2, 5, 0, 7);
    for (int frame = 0; frame < 4; ++frame) {
        CHECK(state.Apply(held, FrameTime(1, 16.0 * frame), events, counts) ==
              BucketInputStatus::Ok);
        CHECK(counts.repeats == 1);
        CHECK(events[0].repeat == 7);
        CHECK(events[0].orphan);
    }
    CHECK(state.FrameCount() == 4);
}

// ===========================================================================
// PANB threshold provenance and the value byte
// ===========================================================================
std::span<const std::byte> OneSampleBuffer(std::array<std::byte, 19>& buffer,
                                           const std::array<std::uint8_t, 14>& nibbles) {
    buffer.fill(std::byte{0});
    buffer[0] = std::byte{1};
    for (std::size_t index = 0; index < nibbles.size(); ++index) {
        buffer[kPanbHeaderBytes + index] = static_cast<std::byte>(nibbles[index]);
    }
    return {buffer.data(), buffer.size()};
}

void TestPanbThreshold() {
    // The default is still the fail-closed state: no level, no edge.
    PanbConfig default_config;
    CHECK(default_config.threshold_source == PanbThresholdSource::Unverified);
    PanbPressureState unverified;
    CHECK(unverified.Configure(default_config) == PanbStatus::Ok);
    CHECK(!unverified.Verified());
    std::array<PanbKeySample, kPanbKeyCount> samples{};
    std::array<PanbEdge, kPanbKeyCount> edges{};
    PanbChangeCounts counts{};
    std::array<std::byte, 19> bytes{};
    const std::array<std::uint8_t, 14> nibbles{0x87, 0, 0, 0, 0, 0, 0,
                                               0,    0, 0, 0, 0, 0, 0};
    CHECK(unverified.Apply(OneSampleBuffer(bytes, nibbles), samples, edges, counts) ==
          PanbStatus::UnverifiedRule);
    CHECK(counts.edges == 0);
    CHECK(!unverified.Pressed(0));

    // The explicit config+0x2C value enables the rule: lane 0 averages 8, lane 1
    // averages 7.
    PanbPressureState pressure;
    CHECK(pressure.Configure(PanbConfigFromConfigByte(8)) == PanbStatus::Ok);
    CHECK(pressure.Verified());
    CHECK(pressure.Threshold() == 8);
    CHECK(pressure.Apply(OneSampleBuffer(bytes, nibbles), samples, edges, counts) ==
          PanbStatus::Ok);
    CHECK(samples[0].average == 8);
    CHECK(samples[0].level == 8);
    CHECK(samples[1].average == 7);
    CHECK(samples[1].level == 0);
    CHECK(counts.presses == 1);
    CHECK(pressure.PressedCount() == 1);

    // A threshold of 0 gates nothing (the pre-config loader value).
    PanbPressureState ungated;
    CHECK(ungated.Configure(PanbConfigFromConfigByte(0)) == PanbStatus::Ok);
    CHECK(ungated.Apply(OneSampleBuffer(bytes, nibbles), samples, edges, counts) ==
          PanbStatus::Ok);
    CHECK(samples[0].level == 8);
    CHECK(samples[1].level == 7);
    CHECK(counts.presses == 2);

    // A threshold at the image filler value gates every key.
    PanbPressureState gated;
    CHECK(gated.Configure(PanbConfigFromConfigByte(kPanbThresholdImageFiller)) ==
          PanbStatus::Ok);
    CHECK(gated.Apply(OneSampleBuffer(bytes, nibbles), samples, edges, counts) ==
          PanbStatus::Ok);
    CHECK(samples[0].level == 0);
    CHECK(samples[1].level == 0);
    CHECK(counts.presses == 0);

    // The staging value byte is carried verbatim: it is the averaged nibble, not
    // the gated level, so the decoder never zeroes a value the threshold would
    // have gated.
    Staging staging{};
    SetSlot(staging, 0, 2, 1, 7);
    std::array<BucketSlot, kBucketSlotCount> slots{};
    BucketFrameCounts frame{};
    CHECK(DecodeStagingSlots(staging, slots, frame) == BucketInputStatus::Ok);
    CHECK(slots[0].value == 7);
    BucketInputState state;
    CHECK(state.Configure(BucketInputConfig{}) == BucketInputStatus::Ok);
    std::array<BucketInputEvent, kBucketMaxFrameEvents> events{};
    CHECK(state.Apply(staging, FrameTime(1, 0.0), events, frame) == BucketInputStatus::Ok);
    CHECK(events[0].value == 7);
    // The bridge view of the same byte (TestBridge covers the full mapping).
}

// ===========================================================================
// Bucket table: rotation and compaction
// ===========================================================================
void TestRotation() {
    BucketTableState table;
    CHECK(table.Rotate() == BucketInputStatus::NotConfigured);
    CHECK(table.Compact(Staging{}) == BucketInputStatus::NotConfigured);
    CHECK(table.Configure(BucketTableConfig{}) == BucketInputStatus::Ok);
    CHECK(!table.GroupsAssigned());
    CHECK(table.Count(0) == 0);

    Staging pressed{};
    SetSlot(pressed, 0, 5, 1, 3);
    CHECK(table.Compact(pressed) == BucketInputStatus::Ok);
    CHECK(table.Count(0) == 1);
    CHECK(table.Entry(0, 0).key == 5);
    CHECK(table.Entry(0, 0).state == 1);
    CHECK(table.Entry(0, 0).value == 3);
    CHECK(table.Entry(0, 0).group == 0);
    CHECK(table.GroupPending() == 1);
    CHECK(table.Entry(0, 1).key == 0); // out of range
    CHECK(table.Entry(31, 0).key == 0);

    CHECK(table.Rotate() == BucketInputStatus::Ok);
    CHECK(table.Count(0) == 0);
    CHECK(table.Count(1) == 1);
    CHECK(table.Entry(1, 0).key == 5);
    CHECK(table.Rotate() == BucketInputStatus::Ok);
    CHECK(table.Count(2) == 1);
    CHECK(table.Count(1) == 0);
    CHECK(table.Count(30) == 0);
    CHECK(table.Entries(2).size() == 1);
    CHECK(table.Entries(30).empty());

    // A short buffer is refused and leaves the table untouched.
    CHECK(table.Compact(std::span<const std::byte>(pressed.data(), 100)) ==
          BucketInputStatus::MalformedBuffer);
    CHECK(table.Count(2) == 1);

    table.Reset();
    CHECK(table.Count(2) == 0);
    CHECK(table.GroupPending() == 0);
    CHECK(table.DecayedDrops() == 0);
}

void TestCompactionStateMachine() {
    BucketTableState table;
    CHECK(table.Configure(BucketTableConfig{}) == BucketInputStatus::Ok);

    Staging press{};
    SetSlot(press, 0, 12, 1, 9);
    CHECK(table.Compact(press) == BucketInputStatus::Ok);
    CHECK(table.Count(0) == 1);

    Staging held{};
    SetSlot(held, 0, 12, 2, 9, 0, 1);
    CHECK(table.Rotate() == BucketInputStatus::Ok);
    CHECK(table.Compact(held) == BucketInputStatus::Ok);
    CHECK(table.Count(0) == 1);
    CHECK(table.Entry(0, 0).state == 2); // held survives: bucket 1 held state 1

    Staging release{};
    SetSlot(release, 0, 12, 3, 9, 0, 2);
    CHECK(table.Rotate() == BucketInputStatus::Ok);
    CHECK(table.Compact(release) == BucketInputStatus::Ok);
    CHECK(table.Entry(0, 0).state == 3); // release survives one frame

    // Bucket 1 now holds the state-3 release: the next staging release decays to
    // 0 and is dropped instead of being appended.
    CHECK(table.Rotate() == BucketInputStatus::Ok);
    CHECK(table.Compact(release) == BucketInputStatus::Ok);
    CHECK(table.Count(0) == 0);
    CHECK(table.DecayedDrops() == 1);

    // Held after a release in bucket 1 is a re-press: 2 -> 1.
    CHECK(table.Rotate() == BucketInputStatus::Ok); // bucket 1 = empty (decayed)
    CHECK(table.Compact(held) == BucketInputStatus::Ok);
    CHECK(table.Count(0) == 1);
    CHECK(table.Entry(0, 0).state == 2);
    CHECK(table.Rotate() == BucketInputStatus::Ok); // bucket 1 = held
    Staging pressed_again{};
    SetSlot(pressed_again, 0, 12, 2, 9, 0, 3);
    CHECK(table.Compact(pressed_again) == BucketInputStatus::Ok);
    CHECK(table.Entry(0, 0).state == 2);

    // Force the decay 2 -> 1 with a release in bucket 1.
    BucketTableState second;
    CHECK(second.Configure(BucketTableConfig{}) == BucketInputStatus::Ok);
    CHECK(second.Compact(release) == BucketInputStatus::Ok);
    CHECK(second.Rotate() == BucketInputStatus::Ok);
    CHECK(second.Compact(held) == BucketInputStatus::Ok);
    CHECK(second.Entry(0, 0).state == 1);
    CHECK(second.DecayedRePresses() == 1);

    // A chord keeps slot order and the count.
    BucketTableState chords;
    CHECK(chords.Configure(BucketTableConfig{}) == BucketInputStatus::Ok);
    Staging chord{};
    SetSlot(chord, 2, 7, 1, 1);
    SetSlot(chord, 5, 8, 1, 2);
    SetSlot(chord, 7, 9, 1, 3);
    CHECK(chords.Compact(chord) == BucketInputStatus::Ok);
    CHECK(chords.Count(0) == 3);
    CHECK(chords.Entry(0, 0).key == 7);
    CHECK(chords.Entry(0, 1).key == 8);
    CHECK(chords.Entry(0, 2).key == 9);
}

void TestGroupAssignment() {
    BucketTableState table;
    BucketTableConfig config;
    config.assign_groups = true;
    CHECK(table.Configure(config) == BucketInputStatus::Ok);
    CHECK(table.GroupsAssigned());

    // Nothing older: the group is forced to 1.
    Staging press{};
    SetSlot(press, 0, 5, 1, 1);
    CHECK(table.Compact(press) == BucketInputStatus::Ok);
    CHECK(table.Entry(0, 0).group == 1);
    CHECK(table.Entry(0, 0).repeat == 0);
    CHECK(table.GroupPending() == 0);

    // Same key two buckets back: inherit the group and increment the counter.
    CHECK(table.Rotate() == BucketInputStatus::Ok);
    CHECK(table.Rotate() == BucketInputStatus::Ok);
    CHECK(table.Count(2) == 1);
    CHECK(table.Compact(press) == BucketInputStatus::Ok);
    CHECK(table.Entry(0, 0).group == 1);
    CHECK(table.Entry(0, 0).repeat == 1);
    CHECK(table.RepeatCarried() == 1);

    // The counter re-arms to 1 at 25.
    BucketTableState rearm;
    CHECK(rearm.Configure(config) == BucketInputStatus::Ok);
    Staging high{};
    SetSlot(high, 0, 6, 1, 1, 0, 24);
    CHECK(rearm.Compact(high) == BucketInputStatus::Ok);
    CHECK(rearm.Entry(0, 0).repeat == 24);
    CHECK(rearm.Rotate() == BucketInputStatus::Ok);
    CHECK(rearm.Rotate() == BucketInputStatus::Ok);
    CHECK(rearm.Compact(high) == BucketInputStatus::Ok);
    CHECK(rearm.Entry(0, 0).repeat == 1);

    // Neighbour inheritance: key+1 wins over key-1, and only a group other than
    // 1 is inherited. A staging group is preserved verbatim by phase 1, which is
    // how a non-default group can exist in the first place.
    BucketTableState neighbours;
    CHECK(neighbours.Configure(config) == BucketInputStatus::Ok);
    Staging group3{};
    SetSlot(group3, 0, 10, 1, 1, 3);
    CHECK(neighbours.Compact(group3) == BucketInputStatus::Ok);
    CHECK(neighbours.Entry(0, 0).group == 3);
    CHECK(neighbours.Rotate() == BucketInputStatus::Ok);
    CHECK(neighbours.Rotate() == BucketInputStatus::Ok);
    Staging group2{};
    SetSlot(group2, 0, 8, 1, 1, 2);
    CHECK(neighbours.Compact(group2) == BucketInputStatus::Ok);
    CHECK(neighbours.Entry(0, 0).group == 2);

    // key 9: same key absent, key+1 = 10 carries group 3, key-1 = 8 carries 2.
    Staging middle{};
    SetSlot(middle, 0, 9, 1, 1);
    CHECK(neighbours.Compact(middle) == BucketInputStatus::Ok);
    CHECK(neighbours.Entry(0, 0).group == 3);

    // key 11: only key-1 = 10 matches.
    Staging below{};
    SetSlot(below, 0, 11, 1, 1);
    CHECK(neighbours.Compact(below) == BucketInputStatus::Ok);
    CHECK(neighbours.Entry(0, 0).group == 3);

    // Group 1 neighbours are not inherited, so an isolated key keeps the default.
    BucketTableState plain;
    CHECK(plain.Configure(config) == BucketInputStatus::Ok);
    Staging group1{};
    SetSlot(group1, 0, 20, 1, 1);
    CHECK(plain.Compact(group1) == BucketInputStatus::Ok);
    CHECK(plain.Entry(0, 0).group == 1);
    CHECK(plain.Rotate() == BucketInputStatus::Ok);
    CHECK(plain.Rotate() == BucketInputStatus::Ok);
    Staging lonely{};
    SetSlot(lonely, 0, 21, 1, 1);
    CHECK(plain.Compact(lonely) == BucketInputStatus::Ok);
    CHECK(plain.Entry(0, 0).group == 1);
    CHECK(plain.RepeatCarried() == 0);
}

// ===========================================================================
// Bucket view feeding the existing pure rules
// ===========================================================================
void TestBucketViewAndGates() {
    BucketTableState table;
    BucketTableConfig config;
    config.assign_groups = true;
    CHECK(table.Configure(config) == BucketInputStatus::Ok);

    // The note spans chart+88..chart+92 = 11..13; the held key 12 is inside it.
    Staging press{};
    SetSlot(press, 0, 12, 1, 1);
    CHECK(table.Compact(press) == BucketInputStatus::Ok);

    std::array<ActiveInputEntry, kBucketSlotCount> view{};
    const std::size_t count = table.ToActiveInputEntries(0, view);
    CHECK(count == 1);
    CHECK(view[0].key == 12);
    CHECK(view[0].state == 1);
    CHECK(view[0].group == 1);
    CHECK(view[0].value == 1);
    CHECK(view[0].repeat == 0);
    CHECK(table.ToActiveInputEntries(30, view) == 0);
    CHECK(table.ToActiveInputEntries(0, std::span<ActiveInputEntry>(view.data(), 0)) == 0);

    const ReleaseGateDecision gated = EvaluateReleaseGate(
        11, 13, 0, std::span<const ActiveInputEntry>(view.data(), count));
    CHECK(gated.evaluated);
    CHECK(!gated.proceed);
    CHECK(gated.held_key == 12);
    CHECK(gated.note84_set);
    CHECK(gated.appended_keys == 1);

    const ReleaseGateDecision outside = EvaluateReleaseGate(
        13, 15, 0, std::span<const ActiveInputEntry>(view.data(), count));
    CHECK(outside.proceed);

    // The glissando connector scan reads bucket 1 and matches note+272.
    CHECK(table.Rotate() == BucketInputStatus::Ok);
    std::array<ActiveInputEntry, kBucketSlotCount> previous{};
    const std::size_t previous_count = table.ToActiveInputEntries(1, previous);
    CHECK(previous_count == 1);
    const std::span<const ActiveInputEntry> entries(previous.data(), previous_count);
    CHECK(FindConnectorKey(entries, 11, 13, 1) == 12);
    CHECK(FindConnectorKey(entries, 11, 13, 0) == 12); // wildcard
    CHECK(FindConnectorKey(entries, 11, 13, 2) == -1); // group mismatch
    CHECK(FindConnectorKey(entries, 13, 15, 1) == -1);
    // Key domain: a 0-based caller value (11 for chart key 12) never matches, so
    // no conversion is hidden in the rule.
    CHECK(FindConnectorKey(entries, 0, 11, 1) == -1);
    CHECK(FindConnectorKey(entries, 12, 12, 1) == 12);
}

// ===========================================================================
// Bridge
// ===========================================================================
void TestBridge() {
    BucketInputState state;
    CHECK(state.Configure(BucketInputConfig{}) == BucketInputStatus::Ok);
    std::array<BucketInputEvent, kBucketMaxFrameEvents> events{};
    BucketFrameCounts counts{};
    Staging staging{};
    SetSlot(staging, 3, 26, 1, 12);
    CHECK(state.Apply(staging, FrameTime(7, 4242.0), events, counts) == BucketInputStatus::Ok);

    NativeNoteEvent native;
    CHECK(MakeNativeNoteEvent(events[0], native));
    CHECK(native.lane == 25);
    CHECK(native.kind == NativeEventKind::Press);
    CHECK(native.event_id == 26);
    CHECK(native.group_id == 0);
    CHECK(native.panb_level == 12);
    CHECK(native.song_time.generation == 7);
    CHECK(native.song_time.microseconds == 4242.0);

    BucketInputEvent invalid;
    invalid.lane = 28;
    CHECK(!MakeNativeNoteEvent(invalid, native));
}

void TestNoAllocation() {
    BucketInputState state;
    BucketTableState table;
    std::array<BucketInputEvent, kBucketMaxFrameEvents> events{};
    BucketFrameCounts counts{};
    Staging staging{};
    SetSlot(staging, 0, 1, 1, 15);
    SetSlot(staging, 1, 2, 2, 15, 0, 5);
    SetSlot(staging, 2, 3, 3, 15, 0, 6);

    test::BeginAllocationProbe();
    CHECK(state.Configure(BucketInputConfig{}) == BucketInputStatus::Ok);
    CHECK(table.Configure(BucketTableConfig{}) == BucketInputStatus::Ok);
    CHECK(state.Apply(staging, FrameTime(1, 0.0), events, counts) == BucketInputStatus::Ok);
    CHECK(table.Compact(staging) == BucketInputStatus::Ok);
    CHECK(table.Rotate() == BucketInputStatus::Ok);
    std::array<ActiveInputEntry, kBucketSlotCount> view{};
    const std::size_t view_count = table.ToActiveInputEntries(1, view);
    const long long allocations = test::EndAllocationProbe();
    CHECK(allocations == 0);
    CHECK(view_count == 3);
    CHECK(counts.active == 3);
}

} // namespace

int main() {
    TestGeometry();
    TestVelocityDistance();
    TestSlotDecode();
    TestSlotRefusals();
    TestOrderedEvents();
    TestOrphans();
    TestGenerations();
    TestCapacityAndRefusalIsolation();
    TestRepeatedFrames();
    TestPanbThreshold();
    TestRotation();
    TestCompactionStateMachine();
    TestGroupAssignment();
    TestBucketViewAndGates();
    TestBridge();
    TestNoAllocation();
    return test::Finish("bucket_input");
}
