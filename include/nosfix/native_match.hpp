#pragma once

// Native normal-note MATCHING seam: the per-lane selection rules of the gameplay
// traversal at VA 0x1802F7C10 plus its horizon helpers 0x180303BA0 /
// 0x180303C10, the decoded input-state bucket table at 0x1813DEA20, the proven
// candidate comparator of 0x1800CAE90 / 0x1800CB0E0 / 0x1800CA230, the
// normal-note field layout of 0x1802F73B0 and a shadow comparator that can be
// validated without enabling any hook.
//
// Evidence base (analyze/evidence/ and analyze/impl-20261030/, all offsets
// module-relative RVAs of nostalgia.dll PAN-2025020501):
//   ABI_COMPLETION.md        A1 bucket table, A2 candidate order, A3 partner
//                           latch, A4 note/chart fields, A5 song-unit rate
//   PROBE_ANALYSIS.md        live slot layout, group=0 staging, note+84 == 1
//   PRODUCER_FINDING.md      sub_180305B30 publishes the 28x28 staging buffer
//   verified_0x1802f7c10.c   traversal, 2729 lines of pseudocode
//   verified_0x180303ba0.c   per-lane horizon admission
//   verified_0x180303c10.c   per-lane history write
//   verified_0x180303b50.c   lazily constructed debug/state singleton
//   verified_0x1802f6800.c   judge (called at 0x1802F954D)
//   verified_0x1800cb9a0.c   image initial values of every global used here
//   (no export for 0x1802F79C0; its auto-slot body is inlined in the traversal
//    at 0x1802F93BA..0x1802F93D1 and reproduced here)
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
//   * The bucket table, the comparator and the note-field decode below are
//     explicit types with validation; they never read game memory and refuse a
//     record that is outside its proven range instead of clamping it.
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
// Song-unit rate (ABI_COMPLETION A5, proven from bytes)
// ===========================================================================

// A5 closes the R1 blocker "the unit of dword_18070A610":
//   * 0x1802FC9D0 advances the counter by exactly 1 per timeline tick,
//   * the tick's rate global 0x18070B300 holds 60.0f,
//   * the 104-byte chart loader (0x1801A4945) converts chart milliseconds with
//     the literal doubles 60.0 (0x18070B2F8) and 1000.0 (0x180687218),
// so one song unit is exactly 1/60 s and
//   frame = (int)(milliseconds * 60.0 / 1000.0)        (truncation toward zero).
//
// What is NOT proven is the tick's CALL rate: the image cannot show that the
// timeline update runs once per rendered frame at 60 Hz. A port that decouples
// judgement logic from the render rate must advance the counter at exactly
// 60 Hz; that stays the caller's decision (MatchConfig::units_per_second).
inline constexpr double kProvenSongUnitsPerSecond = 60.0;
inline constexpr double kProvenSongUnitUs = 1'000'000.0 / kProvenSongUnitsPerSecond;

// Song units -> microseconds, and back, with an explicit rate.
[[nodiscard]] constexpr double SongUnitsToUs(
    double units, double units_per_second = kProvenSongUnitsPerSecond) noexcept {
    return units * 1'000'000.0 / units_per_second;
}

[[nodiscard]] constexpr double UsToSongUnits(
    double microseconds, double units_per_second = kProvenSongUnitsPerSecond) noexcept {
    return microseconds * units_per_second / 1'000'000.0;
}

// 0x1801A4935..0x1801A4945: `mulsd 60.0; divsd 1000.0; cvttsd2si`. The C cast is
// the same truncation toward zero for both signs, so a negative millisecond is
// not floored.
[[nodiscard]] constexpr std::int32_t ChartMsToSongUnits(std::int32_t milliseconds) noexcept {
    return static_cast<std::int32_t>(
        static_cast<double>(milliseconds) * kProvenSongUnitsPerSecond / 1000.0);
}

// ===========================================================================
// Input-state bucket table (0x1813DEA20) - ABI_COMPLETION A1
// ===========================================================================
//
// 30 entries x 792 bytes at 0x1813DEA20 (`0x1813E43D8 - 29*792`), entry k at
// +792*k. Each entry is 28 slots of 28 bytes (7 dwords) at +0..+783, the live
// slot count as a dword at +784, and four filler bytes at +788..+791 that no
// function reads. sub_180307090 rotates the whole table down by one entry and
// zeroes entry 0; sub_180307170 then compacts the 28x28 staging buffer
// (published at 0x1813DEA10 by sub_180305B30) into entry 0. Entry 0 is this
// frame, entry 1 the previous frame; dword_181366910 selects entry 0 here.
//
// Slot fields, from bytes for +0/+4/+8 and from the decompiler for +0x14/+0x18:
//   +0x00 int   key / lane, 1..28 (index 0 is never a key)
//   +0x04 int   state: 1 new this frame, 2 held, 3 release, 0 free
//   +0x08 byte  value (consumed as the low byte of a dword)
//   +0x0C int   NOT READ by any function found - never given a meaning here
//   +0x10 int   NOT READ by any function found - never given a meaning here
//   +0x14 int   group id, 1..3 (written by the compaction phase 2)
//   +0x18 int   repeat/merge counter, 0..24 (>= 25 re-arms group 1)
inline constexpr std::size_t kNativeBucketEntryCount = 30;
inline constexpr std::size_t kNativeBucketSlotCount = 28;
inline constexpr std::size_t kNativeBucketSlotBytes = 28;
inline constexpr std::size_t kNativeBucketEntryBytes = 792;
inline constexpr std::size_t kNativeBucketCountOffset = 784;
inline constexpr std::size_t kNativeBucketFillerBytes = 4;

inline constexpr std::size_t kNativeSlotKeyOffset = 0x00;
inline constexpr std::size_t kNativeSlotStateOffset = 0x04;
inline constexpr std::size_t kNativeSlotValueOffset = 0x08;
inline constexpr std::size_t kNativeSlotReservedAOffset = 0x0C;
inline constexpr std::size_t kNativeSlotReservedBOffset = 0x10;
inline constexpr std::size_t kNativeSlotGroupOffset = 0x14;
inline constexpr std::size_t kNativeSlotRepeatOffset = 0x18;

inline constexpr std::int32_t kNativeSlotKeyMin = 1;
inline constexpr std::int32_t kNativeSlotKeyMax = 28;
inline constexpr std::int32_t kNativeSlotGroupMin = 1;
inline constexpr std::int32_t kNativeSlotGroupMax = 3;
inline constexpr std::int32_t kNativeSlotRepeatLimit = 25;

// The record index the live consumer reads (dword_181366910, written once by the
// initializer sub_1800CB9A0 and therefore a constant of this build).
inline constexpr std::size_t kNativeLiveBucketEntry = 0;

enum class NativeSlotState : std::uint8_t {
    Free = 0,    // idle: the consumer walks past it
    New = 1,     // a new event this frame: the only state that can judge
    Held = 2,    // still held
    Release = 3, // released this frame
};

[[nodiscard]] const char* NativeSlotStateName(std::int32_t state) noexcept;

struct NativeBucketSlot final {
    std::int32_t key = 0;    // +0x00, 1..28
    std::int32_t state = 0;  // +0x04, NativeSlotState
    std::uint8_t value = 0;  // +0x08 low byte
    std::int32_t group = 0;  // +0x14, 0 before the compaction phase 2, else 1..3
    std::int32_t repeat = 0; // +0x18, 0..24

    [[nodiscard]] bool occupied() const noexcept { return state != 0; }
    [[nodiscard]] bool valid() const noexcept;

    friend bool operator==(const NativeBucketSlot&, const NativeBucketSlot&) = default;
};

struct NativeBucketEntry final {
    std::array<NativeBucketSlot, kNativeBucketSlotCount> slots{};
    std::int32_t count = 0; // +784; slots at or beyond it are not read

    void Clear() noexcept;
    // First slot with the same key whose state is non-zero, or nullptr. Entry 1
    // is the previous frame; the compaction consults it for the 2->1 and 3->0
    // decay.
    [[nodiscard]] const NativeBucketSlot* FindKey(std::int32_t key) const noexcept;
    // Appends when there is room and the slot is valid. Returns false (and
    // changes nothing) when full or invalid.
    [[nodiscard]] bool Append(const NativeBucketSlot& slot) noexcept;
    [[nodiscard]] bool valid() const noexcept;

    friend bool operator==(const NativeBucketEntry&, const NativeBucketEntry&) = default;
};

enum class BucketStatus : std::uint8_t {
    Ok = 0,
    InvalidSlot = 1,     // key/state/group/repeat outside its proven range
    InvalidEntry = 2,    // count outside 0..28, or an invalid slot inside it
    InvalidStaging = 3,  // the staging span is not 28 slots
    EntryNotEmpty = 4,   // compaction needs a zeroed entry 0 (rotate first)
    EntryFull = 5,       // more than 28 slots survived the decay
};

[[nodiscard]] const char* BucketStatusName(BucketStatus status) noexcept;

// 0x180307170 phase 2 rebuilds the group id and the repeat counter by comparing
// every entry-0 slot against the older records. It is gated on
// byte_1813E4A8E == 1 (a build constant) but its input window is only
// decompiler-read: A1 names "records 2..5 (0x1813DF360, +792 x 4)", while
// 0x1813DF360 is entry 2's count field (entry 2 starts at 0x1813DF050 and entry
// 3 at 0x1813DF368), so the exact window base is unresolved. The phase is
// therefore OFF by default and only runs when the caller says it is proven.
struct NativeBucketCompactConfig final {
    bool group_phase_enabled = false; // byte_1813E4A8E == 1 AND a resolved window
    std::size_t comparison_first = 2; // records 2..5 per A1
    std::size_t comparison_count = 4;

    friend bool operator==(const NativeBucketCompactConfig&,
                           const NativeBucketCompactConfig&) = default;
};

struct NativeBucketCompactReport final {
    std::size_t staging_slots = 0;
    std::size_t appended = 0;
    std::size_t decayed_to_new = 0;  // 2 -> 1: a newer record already moved that key
    std::size_t decayed_to_free = 0; // 3 -> 0: idem, for a release
    bool group_phase = false;
};

class NativeBucketTable final {
public:
    NativeBucketTable() = default;

    void Clear() noexcept;

    // sub_180307090: `for k = 29 down to 1: entry[k] = entry[k-1]`, then zero
    // entry 0 (memmove of 792 bytes per step, then a 792-byte zero at the base).
    void RotateDown() noexcept;

    // Validates every entry's count and every slot inside it. Slots at or
    // beyond an entry's count belong to no record and are not inspected.
    [[nodiscard]] BucketStatus Validate() const noexcept;

    // sub_180307170: appends every non-free staging slot to entry 0, applying the
    // record-1 decay first (2 -> 1 and 3 -> 0 when entry 1 holds the same key
    // with a state outside {1,2}). Refuses unless entry 0 is empty, so the
    // caller rotates first; nothing is written when the call refuses.
    [[nodiscard]] BucketStatus CompactFromStaging(std::span<const NativeBucketSlot> staging,
                                                  const NativeBucketCompactConfig& config,
                                                  NativeBucketCompactReport& report) noexcept;

    // RotateDown() followed by CompactFromStaging() - one frame of the native
    // update order (0x18019761B calls 0x180307090 then 0x180307170).
    [[nodiscard]] BucketStatus Advance(std::span<const NativeBucketSlot> staging,
                                       const NativeBucketCompactConfig& config,
                                       NativeBucketCompactReport& report) noexcept;

    [[nodiscard]] const NativeBucketEntry& Entry(std::size_t index) const noexcept {
        return entries_[index < kNativeBucketEntryCount ? index : kNativeLiveBucketEntry];
    }
    [[nodiscard]] NativeBucketEntry& Entry(std::size_t index) noexcept {
        return entries_[index < kNativeBucketEntryCount ? index : kNativeLiveBucketEntry];
    }
    [[nodiscard]] const NativeBucketEntry& Live() const noexcept {
        return entries_[kNativeLiveBucketEntry];
    }
    [[nodiscard]] const NativeBucketSlot* Slot(std::size_t entry, std::size_t slot) const noexcept;

private:
    void ApplyGroupPhase(const NativeBucketCompactConfig& config,
                         NativeBucketCompactReport& report) noexcept;
    std::array<NativeBucketEntry, kNativeBucketEntryCount> entries_{};
};

// ===========================================================================
// Candidate ordering (0x1800CAE90 / 0x1800CB0E0 / 0x1800CA230) - A2
// ===========================================================================
//
// 0x1800CAE90 is a thin wrapper around a RECURSIVE, STABLE merge sort of the
// intrusive candidate list; it merges the right run first only when
// 0x1800CA230(right, left) says "right precedes left". Its `mode` argument is
// passed down but never read, so it is not part of this model.
//
// 0x1800CA230 evaluates seven keys in this order, re-reading both records at
// every step:
//   1. |frameA - frameB| > 0x13E496C  ->  frameA < frameB   (note+24)
//   2. the "candidate slot == 1" flag  ->  slot 1 first      (payload+0x14)
//   3. frameA < frameB, then frameA > frameB                 (note+24)
//   4. otherA == NULL && otherB != NULL -> A first           (payload+0x20)
//   5. distanceA > distanceB -> A first (smaller distance)   (payload+0x18 float)
//   6. event lane ascending                                  (*(int*)payload+0x08)
//   7. note+52 (the integer centre key) ascending            (vtable slot 10)
// Key 4 is a one-sided test in the image: a non-NULL A against a NULL B falls
// through to key 5 instead of ordering B first. That asymmetry is reproduced
// literally rather than repaired.
//
// 0x13E496C is the same global the horizon uses as its window; it is 1 in this
// build (sub_1800CB9A0), and it stays an explicit parameter because the R1 seam
// refuses to hardcode a global the initializer owns.
struct NativeCandidateKey final {
    std::int32_t frame = 0;      // note+24
    bool primary_slot = false;   // payload+0x14: the candidate slot == 1
    bool has_partner = false;    // payload+0x20 != NULL
    float distance = 0.0f;       // payload+0x18: |candidate time - event time|
    std::uint8_t event_lane = 0; // *(int*)(payload+0x08)
    std::int32_t centre_key = 0; // note+52

    friend bool operator==(const NativeCandidateKey&, const NativeCandidateKey&) = default;
};

// True when `a` sorts before `b`. A NaN distance compares unordered, exactly as
// the native float comparison does, so such a record falls through to key 6.
[[nodiscard]] bool NativeCandidatePrecedes(const NativeCandidateKey& a,
                                          const NativeCandidateKey& b,
                                          std::int32_t frame_band = kInitialHistoryWindow) noexcept;

enum class CandidateSortStatus : std::uint8_t {
    Ok = 0,
    InsufficientScratch = 1, // scratch.size() < keys.size()
};

[[nodiscard]] const char* CandidateSortStatusName(CandidateSortStatus status) noexcept;

// Stable bottom-up merge sort with the native merge rule: take the right run
// only when NativeCandidatePrecedes(right, left). `scratch` must hold at least
// keys.size() entries; nothing is allocated and the call is noexcept. On a
// refusal the input is untouched, so a caller can retry with a larger buffer.
[[nodiscard]] CandidateSortStatus SortCandidateKeys(
    std::span<NativeCandidateKey> keys, std::span<NativeCandidateKey> scratch,
    std::int32_t frame_band = kInitialHistoryWindow) noexcept;

// ===========================================================================
// Normal-note field layout (ABI_COMPLETION A4, from sub_1802F73B0)
// ===========================================================================
//
// The 104-byte chart record (+0 .. +0x67) and the note it binds:
//   chart+0x10 -> note+20 lane (a CHANNEL id, observed 31..72: never a 1..28 key)
//   chart+0x14 -> note+24 frame (song units, 1/60 s)
//   chart+0x30 (=chart+48) keysound/sprite selector id
//   chart+0x38 (=chart+56) duplicate/chord key; read only by the optional record
//                           ring, which is disabled in a default boot
//   chart+0x58/+0x5C -> note+44/+48 left and right key (1..28)
//   chart+0x61 (=chart+97) auto byte
//   chart+0x64 (=chart+100) sprite offset selector: 1 -> 6, 3 -> 7, else 0, and
//                           only when HIDWORD(qword_1813E4730) == 1
//   note+52  = (left + right) / 2, signed truncation (the centre key)
//   note+56  = (float)(left + right) * 0.5f
//   note+64  = right - left
//   note+84  = the candidate slot that answered (1..3); written only by vtable
//              slot 29 from the input event's +0x20 dword, never by the binder
//   note+112 = -1 (unset link)
//   note+168 = index into the shared sprite/keysound ledger qword_184AEF6D0,
//              produced by sub_180301CB0(chart+48, chart+0x30) "plus up to 9 by
//              width". sub_180301CB0 is NOT decoded, so the index is a caller
//              input here and the field stays 0 with keysound_index_known false.
//              The note owns only the index plus a copy at +172: nothing may be
//              released through it (the ledger object owns the handle).
//   note+204 = 1
inline constexpr std::int32_t kNativeNoteUnsetLink = -1;   // note+112
inline constexpr std::int32_t kNativeNoteDefaultFlags = 1; // note+204
inline constexpr std::uint32_t kNativeNoteSlotMin = 1;     // note+84
inline constexpr std::uint32_t kNativeNoteSlotMax = 3;

struct NativeChartFields final {
    std::int32_t group = 0;         // +0x08, the score-table row (clamped 0..4)
    std::int32_t lane = 0;          // +0x10, channel id 31..72, not a key
    std::int32_t frame = 0;         // +0x14, song units
    std::int32_t keysound_id = 0;   // +0x30, chart+48
    std::int32_t chord_key = 0;     // +0x38, chart+56
    std::int32_t left_key = 0;      // +0x58, chart+88
    std::int32_t right_key = 0;     // +0x5C, chart+92
    bool auto_play = false;         // +0x61, chart+97
    std::int32_t sprite_select = 0; // +0x64, chart+100

    friend bool operator==(const NativeChartFields&, const NativeChartFields&) = default;
};

struct NativeNoteFields final {
    std::int32_t left_key = 0;        // note+44
    std::int32_t right_key = 0;       // note+48
    std::int32_t centre_key = 0;      // note+52 = (left + right) / 2
    float mid_key = 0.0f;             // note+56
    std::int32_t key_span = 0;        // note+64 = right - left
    std::uint32_t candidate_slot = 1; // note+84, 1..3
    std::int32_t unset_link = kNativeNoteUnsetLink; // note+112
    std::int32_t keysound_index = 0;  // note+168, 0 while the selector is undecoded
    bool keysound_index_known = false;
    std::uint8_t sprite_offset = 0;   // chart+100 mapping
    std::int32_t flags = kNativeNoteDefaultFlags; // note+204

    friend bool operator==(const NativeNoteFields&, const NativeNoteFields&) = default;
};

enum class NoteDecodeStatus : std::uint8_t {
    Ok = 0,
    InvalidKey = 1,           // left/right outside 1..28
    InvalidKeyRange = 2,      // right < left
    InvalidCandidateSlot = 3, // note+84 outside 1..3
    InvalidSongUnits = 4,     // frame outside the exact-double range, or a bad rate
    InvalidGroup = 5,         // chart+8 outside 0..4 (the five score rows)
};

[[nodiscard]] const char* NoteDecodeStatusName(NoteDecodeStatus status) noexcept;

// chart+100 -> sprite offset. `gate` is HIDWORD(qword_1813E4730) == 1, which is 0
// in a default boot; with the gate closed the offset is always 0.
[[nodiscard]] std::uint8_t SpriteOffsetForChart(std::int32_t sprite_select,
                                                bool gate) noexcept;

// Decodes the note fields of one normal note from its chart record and the
// candidate slot that answered. `out` is written only on Ok.
[[nodiscard]] NoteDecodeStatus DecodeNormalNote(const NativeChartFields& chart,
                                                std::uint32_t candidate_slot,
                                                std::int32_t keysound_index,
                                                bool keysound_index_known,
                                                bool sprite_gate,
                                                NativeNoteFields& out) noexcept;

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
    InvalidBucket = 11,     // NativeBucketTable::Validate() refused the table
    InvalidEntry = 12,      // bucket entry index outside 0..29
    InvalidSlotIndex = 13,  // slot index outside the entry's count
    SlotNotPress = 14,      // the slot's state is not New (1): nothing may be judged
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
    std::int32_t keysound_id = 0;   // chart+48: the keysound/sprite selector id
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

    // Echoed from the chosen candidate so a shadow consumer can drive 0x180303E80
    // (chart+8 row, chart+48 gate and keysound id, chart+88/+92 popup geometry)
    // without re-scanning the candidate span.
    std::int32_t left_key = 0;     // note+44 of the chosen candidate
    std::int32_t right_key = 0;    // note+48
    std::int32_t chart_group = 0;  // chart+8
    std::int32_t keysound_id = 0;  // chart+48
    std::int32_t candidate_index = -1; // index into the span handed to Select*

    [[nodiscard]] bool ok() const noexcept { return status == MatchStatus::Ok; }
};

struct MatchConfig final {
    NativeHorizonConfig horizon{};
    NormalWindows windows{};             // sampled 0x13668E8..0x1366904
    double judgment_offset_us = 0.0;     // 0x136690C
    double visual_offset_us = 0.0;       // 0x1366918 (only for the type-4 push)
    double units_per_second = kProvenSongUnitsPerSecond; // A5; caller's tick rate
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

    // Evaluates one slot of a decoded bucket table instead of a caller-supplied
    // lane. The slot IS the native event descriptor (PROBE_ANALYSIS s1): its
    // `key` is the event lane and its `state` must be New (1), because state 2
    // is a repeat and state 3 is a release that only gates a keysound. Refusals:
    // InvalidBucket (Validate() != Ok), InvalidEntry (index outside 0..29),
    // InvalidSlotIndex (index outside the entry's count), SlotNotPress
    // (state != 1). The table is validated on every call - 30 records x 28 slots
    // of range checks, no allocation, no recursion.
    [[nodiscard]] MatchDecision SelectSlot(const NativeBucketTable& table,
                                          std::uint32_t entry,
                                          std::uint32_t slot_index,
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
    // `slot` != nullptr: the lane and the event kind come from the bucket slot
    // for every candidate. nullptr: each candidate carries its own event lane.
    [[nodiscard]] MatchDecision SelectCore(std::uint8_t lane, const NativeBucketSlot* slot,
                                           std::span<const MatchCandidate> candidates) noexcept;
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