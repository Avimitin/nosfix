#include "nosfix/bucket_input.hpp"

#include <cmath>

namespace nosfix {

using namespace bucket_input_layout;

namespace {

std::uint32_t ReadU32(std::span<const std::byte> data, std::size_t offset) noexcept {
    return static_cast<std::uint32_t>(std::to_integer<unsigned>(data[offset])) |
           (static_cast<std::uint32_t>(std::to_integer<unsigned>(data[offset + 1])) << 8) |
           (static_cast<std::uint32_t>(std::to_integer<unsigned>(data[offset + 2])) << 16) |
           (static_cast<std::uint32_t>(std::to_integer<unsigned>(data[offset + 3])) << 24);
}

std::int32_t ReadI32(std::span<const std::byte> data, std::size_t offset) noexcept {
    return static_cast<std::int32_t>(ReadU32(data, offset));
}

std::uint8_t ReadU8(std::span<const std::byte> data, std::size_t offset) noexcept {
    return std::to_integer<std::uint8_t>(data[offset]);
}

bool FiniteSongTime(double microseconds) noexcept {
    return std::isfinite(microseconds) && std::abs(microseconds) <= kNoteTypeMaximumTimeUs;
}

constexpr std::size_t LaneOf(std::int32_t key) noexcept {
    return static_cast<std::size_t>(key - kNativeInputKeyMin);
}

constexpr bool ValidState(std::int32_t state) noexcept {
    return state >= kStateFree && state <= kStateRelease;
}

} // namespace

const char* BucketInputStatusName(BucketInputStatus status) noexcept {
    switch (status) {
    case BucketInputStatus::Ok: return "ok";
    case BucketInputStatus::NotConfigured: return "not-configured";
    case BucketInputStatus::GenerationZero: return "generation-zero";
    case BucketInputStatus::StaleGeneration: return "stale-generation";
    case BucketInputStatus::NonFiniteTime: return "non-finite-time";
    case BucketInputStatus::BackwardTime: return "backward-time";
    case BucketInputStatus::MalformedBuffer: return "malformed-buffer";
    case BucketInputStatus::InvalidKey: return "invalid-key";
    case BucketInputStatus::InvalidState: return "invalid-state";
    case BucketInputStatus::InvalidRepeat: return "invalid-repeat";
    case BucketInputStatus::DuplicateKey: return "duplicate-key";
    case BucketInputStatus::OrphanEvent: return "orphan-event";
    case BucketInputStatus::InsufficientCapacity: return "insufficient-capacity";
    }
    return "invalid";
}

// ===========================================================================
// Slot decode (pure)
// ===========================================================================
BucketInputStatus DecodeStagingSlots(std::span<const std::byte> staging,
                                std::span<BucketSlot> slots,
                                BucketFrameCounts& counts) noexcept {
    if (staging.size() < kBucketStagingBytes) return BucketInputStatus::MalformedBuffer;
    if (slots.size() < kBucketSlotCount) return BucketInputStatus::InsufficientCapacity;

    std::array<BucketSlot, kBucketSlotCount> decoded{};
    BucketFrameCounts frame;
    frame.slots_seen = kBucketSlotCount;
    std::array<bool, static_cast<std::size_t>(kNativeInputKeyMax)> seen{};

    for (std::size_t index = 0; index < kBucketSlotCount; ++index) {
        const std::size_t base = index * kBucketSlotBytes;
        BucketSlot& slot = decoded[index];
        slot.key = ReadI32(staging, base + kSlotKey);
        const std::int32_t state = ReadI32(staging, base + kSlotState);
        slot.value = ReadU8(staging, base + kSlotValue);
        slot.group = ReadU32(staging, base + kSlotGroup);
        if (!ValidState(state)) return BucketInputStatus::InvalidState;
        slot.state = static_cast<std::uint8_t>(state);
        if (slot.state == kStateFree) {
            // The producer leaves the last key/value in a free slot (the probe
            // saw key=0/value=2 and one idle value 255), so nothing about a free
            // slot is validated or interpreted.
            ++frame.free_slots;
            if (slot.value != 0) ++frame.free_with_value;
            continue;
        }
        if (!IsNativeInputKey(slot.key)) return BucketInputStatus::InvalidKey;
        const std::uint32_t repeat = ReadU32(staging, base + kSlotRepeat);
        if (repeat > kRepeatLimit) return BucketInputStatus::InvalidRepeat;
        slot.repeat = repeat;
        const std::size_t lane = LaneOf(slot.key);
        if (seen[lane]) return BucketInputStatus::DuplicateKey;
        seen[lane] = true;
        ++frame.active;
        if (slot.value != 0) ++frame.values_nonzero;
        if (repeat > frame.max_repeat) frame.max_repeat = repeat;
        switch (slot.state) {
        case kStatePress: ++frame.presses; break;
        case kStateHeld: ++frame.repeats; break;
        default: ++frame.releases; break;
        }
    }

    frame.distinct_keys = frame.active; // duplicates are refused above
    for (std::size_t index = 0; index < kBucketSlotCount; ++index) {
        slots[index] = decoded[index];
    }
    counts = frame;
    return BucketInputStatus::Ok;
}

// ===========================================================================
// Frame decoder
// ===========================================================================
BucketInputStatus BucketInputState::Configure(const BucketInputConfig& config) noexcept {
    config_ = config;
    last_state_.fill(static_cast<std::uint8_t>(kStateFree));
    observed_press_.fill(false);
    generation_ = 0;
    last_song_us_ = 0.0;
    frames_ = 0;
    rejected_ = 0;
    configured_ = true;
    started_ = false;
    return BucketInputStatus::Ok;
}

void BucketInputState::Rebase() noexcept {
    last_state_.fill(static_cast<std::uint8_t>(kStateFree));
    observed_press_.fill(false);
}

void BucketInputState::Reset() noexcept {
    Rebase();
    generation_ = 0;
    last_song_us_ = 0.0;
    started_ = false;
}

BucketInputStatus BucketInputState::Apply(std::span<const std::byte> staging,
                                     const SongTime& frame_time,
                                     std::span<BucketInputEvent> events,
                                     BucketFrameCounts& counts) noexcept {
    counts = BucketFrameCounts{};
    if (!configured_) return BucketInputStatus::NotConfigured;
    if (frame_time.generation == 0) {
        ++rejected_;
        return BucketInputStatus::GenerationZero;
    }
    if (!FiniteSongTime(frame_time.microseconds)) {
        ++rejected_;
        return BucketInputStatus::NonFiniteTime;
    }
    if (started_ && frame_time.generation < generation_) {
        ++rejected_;
        return BucketInputStatus::StaleGeneration;
    }
    const bool new_generation = !started_ || frame_time.generation != generation_;
    if (!new_generation && frame_time.microseconds < last_song_us_) {
        ++rejected_;
        return BucketInputStatus::BackwardTime;
    }

    // Decode and classify into local storage first: a refusal must leave both the
    // published event span and the per-key tracking untouched.
    std::array<BucketSlot, kBucketSlotCount> slots{};
    BucketFrameCounts frame;
    const BucketInputStatus decoded = DecodeStagingSlots(staging, slots, frame);
    if (decoded != BucketInputStatus::Ok) {
        ++rejected_;
        return decoded;
    }
    if (events.size() < frame.active) {
        ++rejected_;
        return BucketInputStatus::InsufficientCapacity;
    }

    if (new_generation && !config_.keep_tracking_across_generations) Rebase();

    std::array<BucketInputEvent, kBucketMaxFrameEvents> local{};
    std::array<std::uint8_t, kBucketSlotCount> next_state = last_state_;
    std::array<bool, kBucketSlotCount> next_press = observed_press_;
    std::array<bool, kBucketSlotCount> present{};
    std::size_t written = 0;

    for (std::size_t index = 0; index < kBucketSlotCount; ++index) {
        const BucketSlot& slot = slots[index];
        if (slot.state == kStateFree) continue;
        const std::size_t lane = LaneOf(slot.key);
        present[lane] = true;
        BucketInputEvent& event = local[written];
        event = BucketInputEvent{};
        event.song_time = frame_time;
        event.slot = static_cast<std::uint8_t>(index);
        event.lane = static_cast<std::uint8_t>(lane);
        event.key = slot.key;
        event.value = slot.value;
        event.group = slot.group;
        event.repeat = slot.repeat;
        switch (slot.state) {
        case kStatePress:
            event.kind = NativeEventKind::Press;
            if (IsHeldInputState(next_state[lane])) ++frame.press_while_held;
            next_press[lane] = true;
            break;
        case kStateHeld:
            event.kind = NativeEventKind::Repeat;
            if (!next_press[lane]) {
                ++frame.orphan_repeats;
                event.orphan = true;
            }
            break;
        default:
            event.kind = NativeEventKind::Release;
            if (!next_press[lane]) {
                ++frame.orphan_releases;
                event.orphan = true;
            }
            next_press[lane] = false;
            break;
        }
        next_state[lane] = slot.state;
        ++written;
    }

    if (config_.refuse_orphans &&
        (frame.orphan_releases != 0 || frame.orphan_repeats != 0)) {
        counts = frame; // diagnostics survive the refusal; no event is published
        ++rejected_;
        return BucketInputStatus::OrphanEvent;
    }

    // Keys whose slot disappeared: a state-3 frame is followed by a dropped slot,
    // everything else vanishing is a producer anomaly worth counting.
    for (std::size_t lane = 0; lane < kBucketSlotCount; ++lane) {
        if (present[lane]) continue;
        if (next_state[lane] == kStateRelease) {
            ++frame.drops;
        } else if (IsHeldInputState(next_state[lane])) {
            ++frame.dropped_while_held;
        }
        next_state[lane] = static_cast<std::uint8_t>(kStateFree);
        next_press[lane] = false;
    }

    frame.rebased = new_generation;
    if (new_generation && started_) frame.generation_changes = 1;

    for (std::size_t index = 0; index < written; ++index) events[index] = local[index];
    last_state_ = next_state;
    observed_press_ = next_press;
    generation_ = frame_time.generation;
    last_song_us_ = frame_time.microseconds;
    started_ = true;
    ++frames_;
    counts = frame;
    return BucketInputStatus::Ok;
}

std::uint8_t BucketInputState::LastState(std::uint8_t lane) const noexcept {
    return lane < kBucketSlotCount ? last_state_[lane] : static_cast<std::uint8_t>(kStateFree);
}

bool BucketInputState::HasObservedPress(std::uint8_t lane) const noexcept {
    return lane < kBucketSlotCount && observed_press_[lane];
}

// ===========================================================================
// Bucket table
// ===========================================================================
BucketInputStatus BucketTableState::Configure(const BucketTableConfig& config) noexcept {
    config_ = config;
    Reset();
    configured_ = true;
    return BucketInputStatus::Ok;
}

void BucketTableState::Reset() noexcept {
    for (auto& bucket : buckets_) bucket.fill(BucketEntry{});
    counts_.fill(0);
    group_pending_ = 0;
    repeat_carried_ = 0;
    decayed_re_presses_ = 0;
    decayed_drops_ = 0;
}

BucketInputStatus BucketTableState::Rotate() noexcept {
    if (!configured_) return BucketInputStatus::NotConfigured;
    for (std::size_t bucket = kBucketTableCount - 1; bucket >= 1; --bucket) {
        buckets_[bucket] = buckets_[bucket - 1];
        counts_[bucket] = counts_[bucket - 1];
    }
    buckets_[0].fill(BucketEntry{});
    counts_[0] = 0;
    return BucketInputStatus::Ok;
}

const BucketEntry* BucketTableState::FindByKey(std::int32_t key) const noexcept {
    for (std::size_t bucket = kGroupScanFirstBucket; bucket <= kGroupScanLastBucket;
         ++bucket) {
        for (std::size_t index = 0; index < counts_[bucket]; ++index) {
            const BucketEntry& entry = buckets_[bucket][index];
            if (entry.key == key) return &entry;
        }
    }
    return nullptr;
}

const BucketEntry* BucketTableState::FindNeighbourGroup(std::int32_t key) const noexcept {
    // A1 lists the scan as "key == k, key == k+1, key == k-1"; the neighbour must
    // carry a group other than the default 1. Which of the two neighbours wins
    // when both appear is not proven from bytes, so the documented order above is
    // the implemented one and phase 2 stays opt-in.
    const std::int32_t candidates[2] = {key + 1, key - 1};
    for (const std::int32_t candidate : candidates) {
        for (std::size_t bucket = kGroupScanFirstBucket; bucket <= kGroupScanLastBucket;
             ++bucket) {
            for (std::size_t index = 0; index < counts_[bucket]; ++index) {
                const BucketEntry& entry = buckets_[bucket][index];
                if (entry.key != candidate) continue;
                if (entry.group == kDefaultGroup) continue;
                return &entry;
            }
        }
    }
    return nullptr;
}

BucketInputStatus BucketTableState::Compact(std::span<const std::byte> staging) noexcept {
    if (!configured_) return BucketInputStatus::NotConfigured;
    std::array<BucketSlot, kBucketSlotCount> slots{};
    BucketFrameCounts frame{};
    const BucketInputStatus decoded = DecodeStagingSlots(staging, slots, frame);
    if (decoded != BucketInputStatus::Ok) return decoded;

    std::array<BucketEntry, kBucketSlotCount> next{};
    std::size_t count = 0;
    std::size_t re_presses = decayed_re_presses_;
    std::size_t drops = decayed_drops_;

    // Phase 1: rebuild bucket 0 from the staging buffer with the bucket-1 decay.
    for (const BucketSlot& slot : slots) {
        if (slot.state == kStateFree) continue;
        std::uint8_t state = slot.state;
        for (std::size_t index = 0; index < counts_[kPreviousBucket]; ++index) {
            const BucketEntry& other = buckets_[kPreviousBucket][index];
            if (other.key != slot.key) continue;
            if (IsHeldInputState(other.state)) break; // held last frame: no decay
            if (state == kStateHeld) {
                state = static_cast<std::uint8_t>(kStatePress);
                ++re_presses;
            } else if (state == kStateRelease) {
                state = static_cast<std::uint8_t>(kStateFree);
            }
            break;
        }
        if (state == kStateFree) {
            ++drops;
            continue;
        }
        BucketEntry& entry = next[count];
        entry.key = slot.key;
        entry.state = state;
        entry.value = slot.value;
        entry.group = slot.group; // 0 in staging; phase 2 assigns 1..3
        entry.repeat = slot.repeat;
        ++count;
    }

    // Phase 2: group id and repeat counter from buckets 2..5 (opt-in).
    std::size_t pending = 0;
    std::size_t carried = 0;
    if (config_.assign_groups) {
        for (std::size_t index = 0; index < count; ++index) {
            BucketEntry& entry = next[index];
            if (const BucketEntry* same = FindByKey(entry.key)) {
                entry.group = same->group;
                std::uint32_t repeat = same->repeat + 1;
                if (repeat >= kRepeatLimit) repeat = 1; // re-armed to 1 at 25
                entry.repeat = repeat;
                ++carried;
            } else if (const BucketEntry* neighbour = FindNeighbourGroup(entry.key)) {
                entry.group = neighbour->group;
            }
            if (entry.group == 0) entry.group = kDefaultGroup;
        }
    } else {
        pending = count;
    }

    for (std::size_t index = 0; index < count; ++index) buckets_[0][index] = next[index];
    for (std::size_t index = count; index < kBucketSlotCount; ++index) {
        buckets_[0][index] = BucketEntry{};
    }
    counts_[0] = static_cast<std::uint32_t>(count);
    group_pending_ += pending;
    repeat_carried_ += carried;
    decayed_re_presses_ = re_presses;
    decayed_drops_ = drops;
    return BucketInputStatus::Ok;
}

std::size_t BucketTableState::Count(std::size_t bucket) const noexcept {
    return bucket < kBucketTableCount ? counts_[bucket] : 0;
}

BucketEntry BucketTableState::Entry(std::size_t bucket, std::size_t index) const noexcept {
    if (bucket >= kBucketTableCount || index >= counts_[bucket]) return BucketEntry{};
    return buckets_[bucket][index];
}

std::span<const BucketEntry> BucketTableState::Entries(std::size_t bucket) const noexcept {
    if (bucket >= kBucketTableCount) return {};
    return std::span<const BucketEntry>(buckets_[bucket].data(),
                                        static_cast<std::size_t>(counts_[bucket]));
}

std::size_t BucketTableState::ToActiveInputEntries(
    std::size_t bucket, std::span<ActiveInputEntry> out) const noexcept {
    if (bucket >= kBucketTableCount) return 0;
    const std::size_t count = counts_[bucket];
    const std::size_t limit = count < out.size() ? count : out.size();
    for (std::size_t index = 0; index < limit; ++index) {
        const BucketEntry& entry = buckets_[bucket][index];
        ActiveInputEntry& view = out[index];
        view = ActiveInputEntry{};
        view.key = entry.key;
        view.state = entry.state;
        view.group = entry.group;
        view.value = entry.value;
        view.repeat = entry.repeat;
    }
    return limit;
}

} // namespace nosfix
