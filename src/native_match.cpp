#include "nosfix/native_match.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace nosfix {

namespace {

constexpr double kMaximumTimeUs = 1.0e12;

bool FiniteTime(double value) noexcept {
    return std::isfinite(value) && std::abs(value) <= kMaximumTimeUs;
}

std::int32_t ReadI32(const std::byte* bytes, std::size_t offset) noexcept {
    std::int32_t value = 0;
    std::memcpy(&value, bytes + offset, sizeof(value));
    return value;
}

// Integer song units -> microseconds with the caller's explicit rate. The native
// comparisons are integer song units; the seam keeps doubles everywhere and
// converts the small integer constants instead, which stays exact for any rate.
double UnitsToUs(std::int32_t units, double units_per_second) noexcept {
    return static_cast<double>(units) * 1'000'000.0 / units_per_second;
}

bool InSlotStateRange(std::int32_t state) noexcept {
    return state >= static_cast<std::int32_t>(NativeSlotState::Free) &&
           state <= static_cast<std::int32_t>(NativeSlotState::Release);
}

} // namespace

const char* WindowReadStatusName(WindowReadStatus status) noexcept {
    switch (status) {
    case WindowReadStatus::Ok: return "ok";
    case WindowReadStatus::Unavailable: return "unavailable";
    case WindowReadStatus::NotPostInitialization: return "not-post-initialization";
    case WindowReadStatus::ZeroStaticBlock: return "zero-static-block";
    case WindowReadStatus::InvalidRate: return "invalid-rate";
    case WindowReadStatus::InvalidUnits: return "invalid-units";
    case WindowReadStatus::InconsistentEffectBand: return "inconsistent-effect-band";
    }
    return "invalid";
}

const char* MatchStatusName(MatchStatus status) noexcept {
    switch (status) {
    case MatchStatus::Ok: return "ok";
    case MatchStatus::NotConfigured: return "not-configured";
    case MatchStatus::EvidenceIncomplete: return "evidence-incomplete";
    case MatchStatus::WindowsNotSampled: return "windows-not-sampled";
    case MatchStatus::InvalidWindows: return "invalid-windows";
    case MatchStatus::HorizonNotSampled: return "horizon-not-sampled";
    case MatchStatus::InvalidHorizon: return "invalid-horizon";
    case MatchStatus::LayoutNotSampled: return "layout-not-sampled";
    case MatchStatus::InvalidRate: return "invalid-rate";
    case MatchStatus::NonFiniteTime: return "non-finite-time";
    case MatchStatus::InvalidLane: return "invalid-lane";
    case MatchStatus::InvalidBucket: return "invalid-bucket";
    case MatchStatus::InvalidEntry: return "invalid-entry";
    case MatchStatus::InvalidSlotIndex: return "invalid-slot-index";
    case MatchStatus::SlotNotPress: return "slot-not-press";
    }
    return "invalid";
}

const char* MatchSlotName(MatchSlot slot) noexcept {
    switch (slot) {
    case MatchSlot::SameKey: return "same-key";
    case MatchSlot::LeftOfRange: return "left-of-range";
    case MatchSlot::RightOfRange: return "right-of-range";
    }
    return "invalid";
}

const char* ShadowFieldName(ShadowField field) noexcept {
    switch (field) {
    case ShadowField::None: return "none";
    case ShadowField::Kind: return "kind";
    case ShadowField::NoteIndex: return "note-index";
    case ShadowField::Lane: return "lane";
    case ShadowField::Rank: return "rank";
    case ShadowField::Score: return "score";
    case ShadowField::Effect: return "effect";
    case ShadowField::Time: return "time";
    case ShadowField::Order: return "order";
    }
    return "invalid";
}

// ===========================================================================
// Window table
// ===========================================================================

JudgmentWindowSample SampleJudgmentWindows(std::span<const std::byte> image,
                                           double units_per_second,
                                           bool post_initialization_sample) noexcept {
    using namespace native_layout;
    JudgmentWindowSample sample;
    sample.units_per_second = units_per_second;

    if (!post_initialization_sample) {
        sample.status = WindowReadStatus::NotPostInitialization;
        return sample;
    }
    if (!std::isfinite(units_per_second) || units_per_second <= 0.0 ||
        units_per_second > 1.0e6) {
        sample.status = WindowReadStatus::InvalidRate;
        return sample;
    }
    // 0x13668E0 (effect band) .. 0x1366904 (outer latest clamp).
    constexpr std::size_t kFirst = kEffectBandEarlyRva;
    constexpr std::size_t kLast = kEffectBandEarlyRva + 9 * sizeof(std::int32_t);
    if (image.size() < kLast) {
        sample.status = WindowReadStatus::Unavailable;
        return sample;
    }

    NativeWindowUnits units;
    units.effect_early = ReadI32(image.data(), kFirst);
    units.effect_late = ReadI32(image.data(), kFirst + 4);
    units.good_earliest = ReadI32(image.data(), kFirst + 8);
    units.good_latest = ReadI32(image.data(), kFirst + 12);
    units.great_earliest = ReadI32(image.data(), kFirst + 16);
    units.great_latest = ReadI32(image.data(), kFirst + 20);
    units.just_earliest = ReadI32(image.data(), kFirst + 24);
    units.just_latest = ReadI32(image.data(), kFirst + 28);
    sample.units = units;
    sample.outer_earliest_clamp = ReadI32(image.data(), kFirst + 32);
    sample.outer_latest_clamp = ReadI32(image.data(), kFirst + 36);

    // The eight judgment-window globals proper start at 0x13668E8. All eight
    // zero means the initializer has not run; the zero block is refused rather
    // than read as "every note is inside every window".
    const std::int32_t eight[8] = {
        units.good_earliest, units.good_latest,   units.great_earliest,
        units.great_latest,  units.just_earliest, units.just_latest,
        sample.outer_earliest_clamp, sample.outer_latest_clamp,
    };
    bool any_non_zero = false;
    for (std::int32_t value : eight) {
        if (value != 0) any_non_zero = true;
    }
    if (!any_non_zero) {
        sample.status = WindowReadStatus::ZeroStaticBlock;
        return sample;
    }
    if (sample.outer_earliest_clamp < units.good_earliest ||
        sample.outer_latest_clamp > units.good_latest) {
        sample.status = WindowReadStatus::InconsistentEffectBand;
        return sample;
    }

    const SampledNormalWindows converted = ValidateNormalWindowUnits(units, units_per_second);
    sample.matches_initial_seed = converted.matches_initial_seed;
    switch (converted.status) {
    case WindowSampleStatus::Ok:
        sample.windows = converted.windows;
        sample.status = WindowReadStatus::Ok;
        return sample;
    case WindowSampleStatus::Unavailable:
        sample.status = WindowReadStatus::Unavailable;
        return sample;
    case WindowSampleStatus::InvalidRate:
        sample.status = WindowReadStatus::InvalidRate;
        return sample;
    case WindowSampleStatus::InconsistentEffectBand:
        sample.status = WindowReadStatus::InconsistentEffectBand;
        return sample;
    case WindowSampleStatus::InvalidUnits:
        break;
    }
    sample.status = WindowReadStatus::InvalidUnits;
    return sample;
}

// ===========================================================================
// Bucket table (0x1813DEA20)
// ===========================================================================

const char* NativeSlotStateName(std::int32_t state) noexcept {
    switch (state) {
    case static_cast<std::int32_t>(NativeSlotState::Free): return "free";
    case static_cast<std::int32_t>(NativeSlotState::New): return "new";
    case static_cast<std::int32_t>(NativeSlotState::Held): return "held";
    case static_cast<std::int32_t>(NativeSlotState::Release): return "release";
    default: return "invalid";
    }
}

const char* BucketStatusName(BucketStatus status) noexcept {
    switch (status) {
    case BucketStatus::Ok: return "ok";
    case BucketStatus::InvalidSlot: return "invalid-slot";
    case BucketStatus::InvalidEntry: return "invalid-entry";
    case BucketStatus::InvalidStaging: return "invalid-staging";
    case BucketStatus::EntryNotEmpty: return "entry-not-empty";
    case BucketStatus::EntryFull: return "entry-full";
    }
    return "invalid";
}

bool NativeBucketSlot::valid() const noexcept {
    if (!InSlotStateRange(state)) return false;
    if (state == 0) {
        // A free slot is not an event: the staging buffer carries zeroed (or
        // stale) peers there, and the compaction never appends one.
        if (key < 0 || key > kNativeSlotKeyMax) return false;
    } else if (key < kNativeSlotKeyMin || key > kNativeSlotKeyMax) {
        return false;
    }
    if (group < 0 || group > kNativeSlotGroupMax) return false;
    if (repeat < 0 || repeat >= kNativeSlotRepeatLimit) return false;
    return true;
}

void NativeBucketEntry::Clear() noexcept {
    slots.fill(NativeBucketSlot{});
    count = 0;
}

const NativeBucketSlot* NativeBucketEntry::FindKey(std::int32_t key) const noexcept {
    const std::int32_t limit =
        std::min<std::int32_t>(count, static_cast<std::int32_t>(kNativeBucketSlotCount));
    for (std::int32_t i = 0; i < limit; ++i) {
        const NativeBucketSlot& slot = slots[static_cast<std::size_t>(i)];
        if (slot.key == key && slot.state != 0) return &slot;
    }
    return nullptr;
}

bool NativeBucketEntry::Append(const NativeBucketSlot& slot) noexcept {
    if (!slot.valid() || slot.state == 0) return false;
    if (count < 0 || count >= static_cast<std::int32_t>(kNativeBucketSlotCount)) return false;
    slots[static_cast<std::size_t>(count)] = slot;
    ++count;
    return true;
}

bool NativeBucketEntry::valid() const noexcept {
    if (count < 0 || count > static_cast<std::int32_t>(kNativeBucketSlotCount)) return false;
    for (std::int32_t i = 0; i < count; ++i) {
        const NativeBucketSlot& slot = slots[static_cast<std::size_t>(i)];
        // Inside a record every slot is a live event: the compaction only
        // appends state != 0, so a free slot inside the count is a producer bug.
        if (!slot.occupied() || !slot.valid()) return false;
    }
    return true;
}

void NativeBucketTable::Clear() noexcept {
    for (NativeBucketEntry& entry : entries_) entry.Clear();
}

void NativeBucketTable::RotateDown() noexcept {
    // 0x180307090: v1 = &entry[29]; 29 times { memmove(entry[k], entry[k-1], 792);
    // v1 -= 792; } then zero entry[0]. The direction matters: copying upward
    // would duplicate one record across the whole table.
    for (std::size_t index = kNativeBucketEntryCount - 1; index >= 1; --index) {
        entries_[index] = entries_[index - 1];
    }
    entries_[0].Clear();
}

BucketStatus NativeBucketTable::Validate() const noexcept {
    for (const NativeBucketEntry& entry : entries_) {
        if (entry.count < 0 ||
            entry.count > static_cast<std::int32_t>(kNativeBucketSlotCount)) {
            return BucketStatus::InvalidEntry;
        }
        for (std::int32_t i = 0; i < entry.count; ++i) {
            if (!entry.slots[static_cast<std::size_t>(i)].valid()) {
                return BucketStatus::InvalidSlot;
            }
        }
    }
    return BucketStatus::Ok;
}

const NativeBucketSlot* NativeBucketTable::Slot(std::size_t entry,
                                               std::size_t slot) const noexcept {
    if (entry >= kNativeBucketEntryCount) return nullptr;
    const NativeBucketEntry& record = entries_[entry];
    if (record.count < 0) return nullptr;
    if (slot >= static_cast<std::size_t>(record.count)) return nullptr;
    if (slot >= kNativeBucketSlotCount) return nullptr;
    return &record.slots[slot];
}

BucketStatus NativeBucketTable::CompactFromStaging(
    std::span<const NativeBucketSlot> staging, const NativeBucketCompactConfig& config,
    NativeBucketCompactReport& report) noexcept {
    report = NativeBucketCompactReport{};
    if (staging.size() != kNativeBucketSlotCount) return BucketStatus::InvalidStaging;
    // 0x180307170 iterates the fixed 28-slot bound and only appends slots whose
    // state survived the decay; a free slot is not an event. Validate the whole
    // staging buffer before touching entry 0 so a refusal changes nothing.
    for (const NativeBucketSlot& source : staging) {
        if (!source.valid()) return BucketStatus::InvalidSlot;
    }
    NativeBucketEntry& live = entries_[kNativeLiveBucketEntry];
    if (live.count != 0) return BucketStatus::EntryNotEmpty;
    const NativeBucketEntry& previous = entries_[kNativeLiveBucketEntry + 1];

    report.staging_slots = staging.size();
    for (const NativeBucketSlot& source : staging) {
        if (source.state == 0) continue; // dw1 != 0 is the only admission test
        std::int32_t state = source.state;
        // Decay against record 1: a key that a newer record already carries with
        // a state outside {1,2} demotes 2 -> 1 and 3 -> 0 (0x18030721F).
        if (const NativeBucketSlot* older = previous.FindKey(source.key);
            older != nullptr && older->state != static_cast<std::int32_t>(NativeSlotState::New) &&
            older->state != static_cast<std::int32_t>(NativeSlotState::Held)) {
            if (state == static_cast<std::int32_t>(NativeSlotState::Held)) {
                state = static_cast<std::int32_t>(NativeSlotState::New);
                ++report.decayed_to_new;
            } else if (state == static_cast<std::int32_t>(NativeSlotState::Release)) {
                state = static_cast<std::int32_t>(NativeSlotState::Free);
                ++report.decayed_to_free;
            }
        }
        if (state == 0) continue;
        NativeBucketSlot appended = source;
        appended.state = state;
        if (!live.Append(appended)) return BucketStatus::EntryFull;
        ++report.appended;
    }
    if (config.group_phase_enabled) ApplyGroupPhase(config, report);
    return BucketStatus::Ok;
}

void NativeBucketTable::ApplyGroupPhase(const NativeBucketCompactConfig& config,
                                        NativeBucketCompactReport& report) noexcept {
    // 0x180307170 phase 2, gated on byte_1813E4A8E == 1. Documented rules (B7):
    //   * same key            -> inherit the older slot's group, repeat = older+1,
    //                            and repeat >= 25 re-arms group 1 with repeat 0;
    //   * key - 1 or key + 1  -> inherit that slot's group unless it is 1;
    //   * a group that stays 0 becomes 1.
    // The window base is only decompiler-read (see NativeBucketCompactConfig), so
    // this runs only when the caller enables it deliberately.
    NativeBucketEntry& live = entries_[kNativeLiveBucketEntry];
    const std::size_t first = config.comparison_first;
    const std::size_t last = std::min(first + config.comparison_count, kNativeBucketEntryCount);
    for (std::int32_t i = 0; i < live.count; ++i) {
        NativeBucketSlot& slot = live.slots[static_cast<std::size_t>(i)];
        for (std::size_t record = first; record < last; ++record) {
            const NativeBucketEntry& older = entries_[record];
            const std::int32_t limit = std::min<std::int32_t>(
                older.count, static_cast<std::int32_t>(kNativeBucketSlotCount));
            for (std::int32_t j = 0; j < limit; ++j) {
                const NativeBucketSlot& other = older.slots[static_cast<std::size_t>(j)];
                if (other.state == 0) continue;
                if (other.key == slot.key) {
                    slot.group = other.group;
                    slot.repeat = other.repeat + 1;
                    if (slot.repeat >= kNativeSlotRepeatLimit) {
                        slot.repeat = 0;
                        slot.group = kNativeSlotGroupMin;
                    }
                    // A same-key match decides the slot for this record; the
                    // neighbour rule then does not overwrite it. The native scan
                    // order inside one record is not decoded, so this is the
                    // documented reading of B7 and the phase stays opt-in.
                    break;
                } else if ((other.key == slot.key - 1 || other.key == slot.key + 1) &&
                           other.group != kNativeSlotGroupMin) {
                    slot.group = other.group;
                }
            }
        }
        if (slot.group == 0) slot.group = kNativeSlotGroupMin;
    }
    report.group_phase = true;
}

BucketStatus NativeBucketTable::Advance(std::span<const NativeBucketSlot> staging,
                                        const NativeBucketCompactConfig& config,
                                        NativeBucketCompactReport& report) noexcept {
    RotateDown();
    return CompactFromStaging(staging, config, report);
}

// ===========================================================================
// Candidate ordering (0x1800CA230 / 0x1800CB0E0)
// ===========================================================================

bool NativeCandidatePrecedes(const NativeCandidateKey& a, const NativeCandidateKey& b,
                             std::int32_t frame_band) noexcept {
    // 1. frame band: only a difference wider than 0x13E496C orders by frame.
    if (std::abs(a.frame - b.frame) > frame_band) return a.frame < b.frame;
    // 2. the "candidate slot == 1" flag: the primary slot first.
    if (a.primary_slot != b.primary_slot) return a.primary_slot;
    // 3. frame.
    if (a.frame < b.frame) return true;
    if (a.frame > b.frame) return false;
    // 4. one-sided NULL-partner test, transcribed as written.
    if (!a.has_partner && b.has_partner) return true;
    // 5. smaller distance first. A2 records the test as "dA > dB -> A first" and
    //    glosses it "(smaller distance first)"; the two cannot both hold for a
    //    float |candidate time - event time|, so the gloss wins - it is the only
    //    reading consistent with key 6 (lane ascending) and with the candidate
    //    arming, which keeps the closest note. NaN falls through, as the native
    //    unordered float compare does.
    if (a.distance < b.distance) return true;
    if (b.distance < a.distance) return false;
    // 6. event lane ascending.
    if (a.event_lane != b.event_lane) return a.event_lane < b.event_lane;
    // 7. note+52 ascending.
    return a.centre_key < b.centre_key;
}

const char* CandidateSortStatusName(CandidateSortStatus status) noexcept {
    switch (status) {
    case CandidateSortStatus::Ok: return "ok";
    case CandidateSortStatus::InsufficientScratch: return "insufficient-scratch";
    }
    return "invalid";
}

CandidateSortStatus SortCandidateKeys(std::span<NativeCandidateKey> keys,
                                      std::span<NativeCandidateKey> scratch,
                                      std::int32_t frame_band) noexcept {
    const std::size_t count = keys.size();
    if (count < 2) return CandidateSortStatus::Ok;
    if (scratch.size() < count) return CandidateSortStatus::InsufficientScratch;
    // Bottom-up (insize doubling) stable merge, the shape of 0x1800CB0E0: the
    // right run is taken only when it precedes the left one, so equal records
    // keep their insertion order.
    for (std::size_t width = 1; width < count; width *= 2) {
        for (std::size_t start = 0; start < count; start += width * 2) {
            const std::size_t middle = std::min(start + width, count);
            const std::size_t end = std::min(start + width * 2, count);
            std::size_t left = start;
            std::size_t right = middle;
            std::size_t out = start;
            while (left < middle && right < end) {
                if (NativeCandidatePrecedes(keys[right], keys[left], frame_band)) {
                    scratch[out++] = keys[right++];
                } else {
                    scratch[out++] = keys[left++];
                }
            }
            while (left < middle) scratch[out++] = keys[left++];
            while (right < end) scratch[out++] = keys[right++];
            for (std::size_t i = start; i < end; ++i) keys[i] = scratch[i];
        }
    }
    return CandidateSortStatus::Ok;
}

// ===========================================================================
// Note field decode (0x1802F73B0)
// ===========================================================================

const char* NoteDecodeStatusName(NoteDecodeStatus status) noexcept {
    switch (status) {
    case NoteDecodeStatus::Ok: return "ok";
    case NoteDecodeStatus::InvalidKey: return "invalid-key";
    case NoteDecodeStatus::InvalidKeyRange: return "invalid-key-range";
    case NoteDecodeStatus::InvalidCandidateSlot: return "invalid-candidate-slot";
    case NoteDecodeStatus::InvalidSongUnits: return "invalid-song-units";
    case NoteDecodeStatus::InvalidGroup: return "invalid-group";
    }
    return "invalid";
}

std::uint8_t SpriteOffsetForChart(std::int32_t sprite_select, bool gate) noexcept {
    // chart+100: 1 -> 6, 3 -> 7, else 0, only when HIDWORD(qword_1813E4730) == 1.
    if (!gate) return 0;
    if (sprite_select == 1) return 6;
    if (sprite_select == 3) return 7;
    return 0;
}

NoteDecodeStatus DecodeNormalNote(const NativeChartFields& chart,
                                 std::uint32_t candidate_slot,
                                 std::int32_t keysound_index,
                                 bool keysound_index_known, bool sprite_gate,
                                 NativeNoteFields& out) noexcept {
    if (chart.left_key < kNativeSlotKeyMin || chart.left_key > kNativeSlotKeyMax ||
        chart.right_key < kNativeSlotKeyMin || chart.right_key > kNativeSlotKeyMax) {
        return NoteDecodeStatus::InvalidKey;
    }
    if (chart.right_key < chart.left_key) return NoteDecodeStatus::InvalidKeyRange;
    if (candidate_slot < kNativeNoteSlotMin || candidate_slot > kNativeNoteSlotMax) {
        return NoteDecodeStatus::InvalidCandidateSlot;
    }
    const std::int32_t sum = chart.left_key + chart.right_key; // <= 56: no overflow
    NativeNoteFields decoded;
    decoded.left_key = chart.left_key;
    decoded.right_key = chart.right_key;
    // note+52: (left + right) / 2 with C truncation. Both keys are positive, so
    // the truncation is the plain integer division.
    decoded.centre_key = sum / 2;
    // note+56: (float)(left + right) * 0.5f.
    decoded.mid_key = static_cast<float>(sum) * 0.5f;
    decoded.key_span = chart.right_key - chart.left_key; // note+64
    decoded.candidate_slot = candidate_slot;             // note+84
    decoded.unset_link = kNativeNoteUnsetLink;           // note+112
    decoded.keysound_index = keysound_index_known ? keysound_index : 0;
    decoded.keysound_index_known = keysound_index_known; // note+168 / +172
    decoded.sprite_offset = SpriteOffsetForChart(chart.sprite_select, sprite_gate);
    decoded.flags = kNativeNoteDefaultFlags; // note+204
    out = decoded;
    return NoteDecodeStatus::Ok;
}

// ===========================================================================
// NativeMatchPolicy
// ===========================================================================

MatchStatus NativeMatchPolicy::Configure(const MatchConfig& config,
                                         const NativeMatchEvidence& evidence) noexcept {
    // Fail closed on anything unproven. kNativeMatchModelVerified is the
    // compile-time half of the gate; the caller's evidence record is the runtime
    // half, and NativeNoteState still refuses the Native2F7C10 selection.
    if (!evidence.Complete()) return MatchStatus::EvidenceIncomplete;
    if (!config.horizon.sampled) return MatchStatus::HorizonNotSampled;
    if (!config.layout_sampled) return MatchStatus::LayoutNotSampled;
    if (config.horizon.history_stamp < 0 || config.horizon.history_window < 0 ||
        config.horizon.lane_spread < 0 ||
        config.horizon.lane_spread >= static_cast<std::int32_t>(kNativeLaneHistorySlots) ||
        config.horizon.lane_cooldown < 0) {
        return MatchStatus::InvalidHorizon;
    }
    if (!std::isfinite(config.units_per_second) || config.units_per_second <= 0.0 ||
        config.units_per_second > 1.0e6) {
        return MatchStatus::InvalidRate;
    }
    if (!std::isfinite(config.key_spread) || config.key_spread < 0.0f ||
        config.key_spread > 1024.0f) {
        return MatchStatus::InvalidHorizon;
    }
    if (config.lane_over_filter < 0 || config.lane_over_filter > 64) {
        return MatchStatus::InvalidHorizon;
    }
    if (ValidateNormalWindows(config.windows) != NormalWindowStatus::Ok) {
        return MatchStatus::InvalidWindows;
    }
    const NormalWindows& windows = config.windows;
    if (windows.just_early_us == 0.0 && windows.just_late_us == 0.0 &&
        windows.great_early_us == 0.0 && windows.great_late_us == 0.0 &&
        windows.good_early_us == 0.0 && windows.good_late_us == 0.0) {
        return MatchStatus::InvalidWindows; // a zero table is not a valid sample
    }
    if (!FiniteTime(config.judgment_offset_us) || !FiniteTime(config.visual_offset_us)) {
        return MatchStatus::NonFiniteTime;
    }
    if (!FiniteTime(UnitsToUs(config.effect_early_units, config.units_per_second))) {
        return MatchStatus::InvalidRate;
    }

    config_ = config;
    evidence_ = evidence;
    history_stamp_.fill(0);
    history_value_.fill(0.0);
    ResetGroupMasks();
    now_us_ = 0.0;
    groups_ = 0;
    update_open_ = false;
    configured_ = true;
    return MatchStatus::Ok;
}

void NativeMatchPolicy::ResetGroupMasks() noexcept {
    masked_.fill(false);
    resolved_.fill(false);
}

void NativeMatchPolicy::BeginUpdate(double now_us) noexcept {
    now_us_ = now_us;
    update_open_ = configured_ && FiniteTime(now_us);
}

void NativeMatchPolicy::BeginGroup() noexcept {
    ResetGroupMasks();
    if (configured_) ++groups_;
}

double NativeMatchPolicy::Type4PushUs() const noexcept {
    return JudgmentNowUs() + UnitsToUs(config_.effect_early_units, config_.units_per_second);
}

std::uint8_t NativeMatchPolicy::LanesTouchedFirst(std::uint8_t lane) const noexcept {
    const std::int32_t spread = config_.horizon.lane_spread;
    const std::int32_t first = static_cast<std::int32_t>(lane) - spread;
    return static_cast<std::uint8_t>(first < 0 ? 0 : first);
}

std::uint8_t NativeMatchPolicy::LanesTouchedLast(std::uint8_t lane) const noexcept {
    const std::int32_t spread = config_.horizon.lane_spread;
    const std::int32_t last = static_cast<std::int32_t>(kNativeLaneHistorySlots) - 1;
    const std::int32_t value = static_cast<std::int32_t>(lane) + spread;
    return static_cast<std::uint8_t>(value > last ? last : value);
}

bool NativeMatchPolicy::HorizonAllows(double candidate_us, std::uint8_t lane) const noexcept {
    if (!configured_) return false;
    if (lane == 0 || lane >= kNativeLaneHistorySlots) return false; // 1..28 only
    if (!FiniteTime(candidate_us)) return false;
    // 0x180303BB4: `if (dword_1813E4968 >= 0)`. A negative stamp global disables
    // the admission filter entirely, exactly as the native code does.
    if (config_.horizon.history_stamp < 0) return true;
    const std::int32_t stamp = history_stamp_[lane];
    if (stamp <= 0) return true; // v4 > 0 is required before the reject can fire
    const double value = history_value_[lane];
    if (value <= 0.0) return true; // `v4 <= 0 ? 0 : value`, then value > 0 required
    const double limit =
        UnitsToUs(config_.horizon.history_stamp + config_.horizon.history_window,
                  config_.units_per_second) +
        value;
    return !(candidate_us > limit);
}

void NativeMatchPolicy::PushHorizon(double value_us, std::uint8_t lane) noexcept {
    if (!configured_ || lane >= kNativeLaneHistorySlots || !FiniteTime(value_us)) return;
    const std::int32_t spread = config_.horizon.lane_spread;
    const std::int32_t last = static_cast<std::int32_t>(kNativeLaneHistorySlots) - 1;
    const std::int32_t centre = static_cast<std::int32_t>(lane);
    // 0x180303C10 writes entry[lane], walks up while the address stays at or
    // below 0x1813E4A5C, then walks down from lane-1 while index >= lane-spread.
    for (std::int32_t i = centre; i <= std::min(last, centre + spread); ++i) {
        history_stamp_[static_cast<std::size_t>(i)] = config_.horizon.history_stamp;
        history_value_[static_cast<std::size_t>(i)] = value_us;
    }
    for (std::int32_t i = centre - 1; i >= 0 && i >= centre - spread; --i) {
        history_stamp_[static_cast<std::size_t>(i)] = config_.horizon.history_stamp;
        history_value_[static_cast<std::size_t>(i)] = value_us;
    }
}

MatchDecision NativeMatchPolicy::Select(std::uint8_t lane,
                                        std::span<const MatchCandidate> candidates) noexcept {
    return SelectCore(lane, nullptr, candidates);
}

MatchDecision NativeMatchPolicy::SelectSlot(const NativeBucketTable& table,
                                            std::uint32_t entry,
                                            std::uint32_t slot_index,
                                            std::span<const MatchCandidate> candidates) noexcept {
    MatchDecision decision;
    if (!configured_ || !update_open_) {
        decision.status = MatchStatus::NotConfigured;
        return decision;
    }
    if (entry >= kNativeBucketEntryCount) {
        decision.status = MatchStatus::InvalidEntry;
        return decision;
    }
    if (table.Validate() != BucketStatus::Ok) {
        decision.status = MatchStatus::InvalidBucket;
        return decision;
    }
    const NativeBucketEntry& record = table.Entry(entry);
    if (record.count < 0 ||
        slot_index >= static_cast<std::uint32_t>(record.count)) {
        decision.status = MatchStatus::InvalidSlotIndex;
        return decision;
    }
    const NativeBucketSlot& slot = record.slots[slot_index];
    if (slot.key < kNativeSlotKeyMin || slot.key > kNativeSlotKeyMax) {
        decision.status = MatchStatus::InvalidBucket;
        return decision;
    }
    decision.lane = static_cast<std::uint8_t>(slot.key);
    if (slot.state != static_cast<std::int32_t>(NativeSlotState::New)) {
        // State 2 is a repeat and state 3 a release: neither reaches the judge.
        decision.status = MatchStatus::SlotNotPress;
        return decision;
    }
    return SelectCore(static_cast<std::uint8_t>(slot.key), &slot, candidates);
}

MatchDecision NativeMatchPolicy::SelectCore(std::uint8_t lane, const NativeBucketSlot* slot,
                                            std::span<const MatchCandidate> candidates) noexcept {
    MatchDecision decision;
    decision.lane = lane;
    decision.at_us = now_us_;
    if (!configured_ || !update_open_) {
        decision.status = MatchStatus::NotConfigured;
        return decision;
    }
    // judgment-001 item 1: 28 keys numbered 1..28, index 0 is never a lane.
    if (lane == 0 || lane >= kNativeLaneHistorySlots) {
        decision.status = MatchStatus::InvalidLane;
        return decision;
    }
    decision.status = MatchStatus::Ok;

    // ---- phase 1: arm the three candidate slots of this entry --------------
    // 0x1802F8C47..0x1802F8ED7. The closest candidate wins; ties go to the
    // smaller note+56 key float.
    constexpr std::size_t kSlotCount = 3;
    struct Armed final {
        int index = -1;
        double delta = 0.0;
        float key = 0.0f;
    };
    std::array<Armed, kSlotCount> slots{};
    const float spread = config_.key_spread;
    const std::int32_t entry = static_cast<std::int32_t>(lane);
    for (std::size_t i = 0; i < candidates.size(); ++i) {
        const MatchCandidate& candidate = candidates[i];
        if (candidate.auto_note) continue; // 0x1802F8B93: chart+97 != 0
        if (candidate.chart_type == 3 || candidate.chart_type == 5) continue;
        if (!FiniteTime(candidate.note_us)) continue;
        // 0x1802F8D4A..0x1802F8D84: the arming loop skips a finished note and one
        // whose chart+84 is already non-zero, before the slot comparison.
        if (candidate.done) {
            decision.skipped_done = true;
            continue;
        }
        if (candidate.consumed) {
            decision.skipped_consumed = true;
            continue;
        }
        // With a bucket slot the event lane is the slot key for every candidate.
        const std::uint8_t event_lane = slot != nullptr ? lane : candidate.event_lane;
        if (event_lane == 0 || event_lane >= kNativeLaneHistorySlots) {
            decision.skipped_invalid_lane = true;
            continue;
        }

        std::int32_t lo = candidate.left_key;
        std::int32_t hi = candidate.right_key;
        const std::int32_t span = candidate.right_key - candidate.left_key;
        if (span >= 4) lo += static_cast<std::int32_t>(spread * -0.5f);
        if (span >= 3) hi += static_cast<std::int32_t>(spread * 0.5f - 0.9f);
        // 0x1802F8CA6: a non-type-2 note widens the acceptance band by 0x13E4A84.
        const std::int32_t outer_lo =
            candidate.chart_type == 2 ? lo : lo - config_.lane_over_filter;
        const std::int32_t outer_hi =
            candidate.chart_type == 2 ? hi : hi + config_.lane_over_filter;
        if (entry < outer_lo || entry > outer_hi) continue; // 0x1802F8ED3

        std::size_t which = 0; // inside [lo, hi]
        if (entry < lo) {
            which = 1; // left of the widened range
        } else if (entry > hi) {
            which = 2; // right of the widened range
        }
        const double delta = candidate.note_us - now_us_;
        Armed& armed = slots[which];
        if (armed.index < 0 || delta < armed.delta ||
            (delta == armed.delta && armed.key > candidate.mid_key)) {
            armed.index = static_cast<int>(i);
            armed.delta = delta;
            armed.key = candidate.mid_key;
        }
    }

    // ---- phase 2: gate the armed slots in native slot order ----------------
    for (std::size_t which = 0; which < kSlotCount; ++which) {
        if (slots[which].index < 0) continue;
        const std::size_t candidate_index = static_cast<std::size_t>(slots[which].index);
        const MatchCandidate& candidate = candidates[candidate_index];
        // 0x1802F8F5A..0x1802F8FDE: a candidate whose note+56 float equals an
        // earlier armed slot's is registered as a partner instead of judged
        // (the native test is key == earlier_key && distance == earlier_distance).
        bool duplicate = false;
        for (std::size_t earlier = 0; earlier < which; ++earlier) {
            if (slots[earlier].index >= 0 &&
                candidates[static_cast<std::size_t>(slots[earlier].index)].mid_key ==
                    candidate.mid_key) {
                duplicate = true;
            }
        }
        if (duplicate) continue;

        const std::uint8_t event_lane = slot != nullptr ? lane : candidate.event_lane;
        if (event_lane == 0 || event_lane >= kNativeLaneHistorySlots) {
            decision.skipped_invalid_lane = true;
            continue;
        }
        // (A) 0x1802F92E7: done note, or no horizon clearance and not linked-tail.
        if (candidate.done) {
            decision.skipped_done = true;
            continue;
        }
        const bool horizon_ok = HorizonAllows(candidate.note_us, event_lane);
        if (!horizon_ok && !candidate.linked_open) {
            decision.horizon_ok = false;
            continue;
        }
        // (B) 0x1802F9301: one judgment attempt per event lane per group.
        if (masked_[event_lane]) {
            decision.lane_masked = true;
            continue;
        }
        // (C) 0x1802F9336: a pending early hit blocks unless the record's slot
        // index is 1, i.e. unless the primary candidate slot answered.
        const std::uint32_t event_state = static_cast<std::uint32_t>(which) + 1u;
        if (candidate.pending_early && event_state != 1u) {
            decision.skipped_pending_early = true;
            continue;
        }
        // (D) 0x1802F942C: a lane already resolved by a linked pair is skipped.
        if (resolved_[event_lane]) {
            decision.lane_resolved = true;
            continue;
        }
        if (candidate.consumed) { // chart+84 > 0: the judge already took it
            decision.skipped_consumed = true;
            continue;
        }

        decision.note = candidate.note;
        decision.slot = static_cast<MatchSlot>(which);
        decision.event_state = event_state;
        decision.horizon_ok = horizon_ok;
        decision.at_us = candidate.note_us;
        decision.left_key = candidate.left_key;
        decision.right_key = candidate.right_key;
        decision.chart_group = static_cast<std::int32_t>(candidate.chart_type);
        decision.keysound_id = candidate.keysound_id;
        decision.candidate_index = static_cast<std::int32_t>(candidate_index);

        // (E) 0x1802F933F..0x1802F9413: the linked branch records the event
        // state, arms the note+96 auto slot and pushes the lane horizon. It never
        // reaches the judge virtual.
        if (candidate.linked) {
            decision.action = MatchAction::LinkedState;
            decision.auto_slot_armed = candidate.auto_slot_empty;
            resolved_[event_lane] = true;
            PushHorizon(candidate.note_us, event_lane);
            decision.horizon_pushed = true;
            decision.horizon_push_us = candidate.note_us;
            decision.horizon_push_first_lane = LanesTouchedFirst(event_lane);
            decision.horizon_push_last_lane = LanesTouchedLast(event_lane);
            return decision;
        }

        // (F) 0x1802F942C..0x1802F9461: arm the judge. The lane mask is taken
        // here, before the judge runs, exactly as v558[e] = 1 is stored before
        // the virtual call.
        masked_[event_lane] = true;
        decision.action = MatchAction::Judge;
        decision.horizon_push_us = candidate.chart_type == 4 ? Type4PushUs()
                                                             : candidate.note_us;
        decision.horizon_push_first_lane = LanesTouchedFirst(event_lane);
        decision.horizon_push_last_lane = LanesTouchedLast(event_lane);
        return decision;
    }
    return decision;
}

void NativeMatchPolicy::CommitJudge(const MatchCandidate& candidate, bool scored,
                                    bool post_judged) noexcept {
    if (!configured_) return;
    // 0x1802F954F / 0x1802F95DE: only a press whose judge consumed the note (or
    // whose post_judge() reported one) pushes the horizon.
    if (!scored && !post_judged) return;
    if (candidate.event_kind != 1) return; // the push is inside `event+4 == 1`
    PushHorizon(candidate.chart_type == 4 ? Type4PushUs() : candidate.note_us,
                candidate.event_lane);
}

// ===========================================================================
// Shadow comparison
// ===========================================================================

void NativeShadowComparison::Configure(double time_tolerance_us) noexcept {
    time_tolerance_us_ =
        std::isfinite(time_tolerance_us) && time_tolerance_us > 0.0 ? time_tolerance_us : 0.0;
}

void NativeShadowComparison::Reset() noexcept {
    divergence_ = ShadowDivergence{};
    compared_ = 0;
}

ShadowOutcome NativeShadowComparison::FromAdapter(const NativeNoteOutcome& outcome,
                                                  std::int32_t running_score) noexcept {
    ShadowOutcome shadow;
    shadow.note_index = outcome.note_index;
    shadow.lane = outcome.lane;
    shadow.kind = static_cast<std::uint8_t>(outcome.kind);
    shadow.rank = static_cast<std::uint8_t>(outcome.rank);
    shadow.effect_grade = outcome.effect_grade;
    shadow.score = running_score;
    shadow.at_us = outcome.at_us;
    return shadow;
}

ShadowDivergence NativeShadowComparison::Observe(const ShadowOutcome& adapter,
                                                 const ShadowOutcome& native) noexcept {
    if (divergence_.diverged) return divergence_;

    ShadowField field = ShadowField::None;
    if (adapter.kind != native.kind) {
        field = ShadowField::Kind;
    } else if (adapter.note_index != native.note_index) {
        field = ShadowField::NoteIndex;
    } else if (adapter.lane != native.lane) {
        field = ShadowField::Lane;
    } else if (adapter.rank != native.rank) {
        field = ShadowField::Rank;
    } else if (adapter.effect_grade != native.effect_grade) {
        field = ShadowField::Effect;
    } else if (adapter.score != native.score) {
        field = ShadowField::Score;
    } else {
        const double difference = std::abs(adapter.at_us - native.at_us);
        if (!(difference <= time_tolerance_us_)) field = ShadowField::Time;
    }

    if (field != ShadowField::None) {
        divergence_.diverged = true;
        divergence_.index = compared_;
        divergence_.field = field;
        divergence_.at_us = adapter.at_us;
        divergence_.note_index = adapter.note_index;
        divergence_.adapter_kind = adapter.kind;
        divergence_.native_kind = native.kind;
        divergence_.adapter_rank = adapter.rank;
        divergence_.native_rank = native.rank;
        divergence_.adapter_score = adapter.score;
        divergence_.native_score = native.score;
        divergence_.adapter_effect = adapter.effect_grade;
        divergence_.native_effect = native.effect_grade;
        return divergence_;
    }
    ++compared_;
    return divergence_;
}

} // namespace nosfix
