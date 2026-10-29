#pragma once

// Portable decoder for the native input-state staging buffer and for the
// per-frame bucket table it is compacted into (nostalgia.dll PAN-2025020501).
// It is the producer-side sibling of native_note_types.hpp: that module
// consumes single events, this one turns the published per-frame key state into
// that event stream and builds the bucket view the release gate (0x2F4060) and
// the glissando connector scan (0x1A60C0) read.
//
// Evidence (module-relative RVAs; analyze/impl-20261030/):
//   ABI_COMPLETION.md A1          bucket layout, rotation, compaction, slot fields
//   PROBE_ANALYSIS.md s1, s11     the build-B staging diff and the state machine
//   PRODUCER_FINDING.md           sub_180305B30 publishes 0x1813DEA10
//
//   producer      0x305B30    per-frame note-manager update; stores the pair
//                            `xmmword_1813DEA10 = {a2, a3}` at 0x180305C6C.
//                            a2 = the 28 x 28 staging buffer (obj+8 of the
//                            calling manager), a3 = a companion pointer
//                            (obj+0x31C) that nothing in this model reads.
//   rotate        0x307090    29 memmoves of 792 bytes from the top down, then
//                            bucket 0 is zeroed: bucket k receives k-1.
//   compact       0x307170    rebuilds bucket 0 from the staging buffer
//                            (phase 1) and, gated on byte_1813E4A8E == 1,
//                            recomputes the group id and repeat counter from
//                            buckets 2..5 (phase 2).
//   table         0x1813DEA20 30 buckets x 792 bytes; 28 slots of 28 bytes,
//                            count at +784, 4 filler bytes at +788.
//   scan count    0x1813DF048 bucket 1 + 784: where the glissando connector
//                            scan (0x1A60C0) starts walking.
//   selected      0x181366910 single writer (the initialiser), constant 0, so
//                            bucket 0 is the live record and bucket 1 the
//                            "one frame ago" record.
//
// Staging slot layout [bytes for +0/+4/+8, decompiler-only for +0x14/+0x18]:
//   +0x00 int   key, 1-based 1..28 (native uses key-1 clamped to 0..27)
//   +0x04 int   state: 1 = new this frame, 2 = held, 3 = release, 0 = free
//   +0x08 byte  value: the producer's averaged PANB nibble (observed 0..15)
//   +0x0C int   never read in this build - ignored, no meaning is invented
//   +0x10 int   never read in this build - ignored, no meaning is invented
//   +0x14 int   group id: always 0 in staging; assigned only by compaction
//   +0x18 int   repeat/merge counter (0..24, re-armed to 1 at 25)
//
// Frame semantics observed on the live staging buffer (PROBE_ANALYSIS.md s11):
//   press    a slot appears with state 1, value = the averaged nibble,
//            repeat 0 and group 0;
//   repeat   every later frame while held: state 2, repeat incremented
//            (cap 25, re-armed to 1 by compaction phase 2);
//   release  exactly one frame with state 3, then the slot is dropped (state 0);
//   chord    several slots share one state in the same frame; the native
//            iterates slots 0..27 (fixed bound 0x310 = 784, no count), so this
//            decoder emits events in slot order and never re-sorts them.
//   The staging buffer is not cleared between frames: a free slot keeps the
//   last key/value it held (the probe saw `slot=9 key=0 state=0 value=2` and
//   one idle slot with value 255), so a state-0 slot is ignored entirely.
//
// Compaction semantics (0x307170), implemented by BucketTableState:
//   phase 1  for every staging slot with state != 0: if bucket 1 holds the same
//            key with a state outside {1,2}, decay 2 -> 1 (re-press) and
//            3 -> 0 (drop); append the surviving slot to bucket 0 otherwise;
//   phase 2  for every new bucket-0 slot: same key in buckets 2..5 inherits
//            that group id and takes repeat = other + 1 (re-armed to 1 at 25);
//            else key+1, then key-1, inherits a group id that is not 1; the
//            result is forced to 1 when it is 0.
//
// Deliberately NOT modelled (fail closed):
//   * the identity of the code that fills the staging buffer through the
//     register base (PRODUCER_FINDING closes the publisher, not the filler);
//   * the second qword of the staging pair (obj+0x31C);
//   * +0x0C / +0x10 inside a slot: unused in this build;
//   * the *source* of the value byte. It matches the averaged PANB nibble
//     (0..15), but nothing proves it is the threshold-gated level, so it is
//     carried verbatim as `value` and never turned into a press/release edge.
//     panb_pressure.hpp owns the gated level and still defaults to
//     PanbThresholdSource::Unverified;
//   * phase-2 group/repeat derivation is opt-in (BucketTableConfig::
//     assign_groups). The coarse match order (same key, then key+1, then key-1,
//     buckets 2..5 ascending) is the pseudocode order, but the per-bucket match
//     priority and the treatment of non-held entries in those buckets are not
//     proven from bytes, so the default leaves group 0 and reports the pending
//     count instead of guessing. A group of 0 keeps the glissando connector scan
//     on its `note+272 == 0` wildcard path only;
//   * note+272's 1/2/3 derivation (0x1A7450, decompiler-only) stays caller
//     supplied, exactly as in native_note_types.hpp.
//
// Key domain: slot keys are 1-based 1..28 (A1: `dw0-1` clamped to 0..27 indexes
// the per-key arrays) and chart+88/chart+92 are 1-based as well, so the release
// gate and the connector scan compare raw values with no conversion. The lane
// convention of the rest of the pipeline is 0-based, hence lane = key - 1.
//
// No allocation, lock, logging, file or blocking call happens on any path here;
// every entry point is noexcept and works on fixed-size storage.

#include "nosfix/native_note_types.hpp"
#include "nosfix/song_clock.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace nosfix {

inline constexpr std::size_t kBucketSlotCount = 28;
inline constexpr std::size_t kBucketSlotBytes = 28;
inline constexpr std::size_t kBucketStagingBytes = kBucketSlotCount * kBucketSlotBytes; // 784
inline constexpr std::size_t kBucketTableCount = 30;
inline constexpr std::size_t kBucketEntryBytes = 792;
inline constexpr std::size_t kBucketEntryCountOffset = 784;
inline constexpr std::size_t kBucketMaxFrameEvents = kBucketSlotCount;

namespace bucket_input_layout {

inline constexpr std::uint32_t kProducerRva = 0x305B30;
inline constexpr std::uint32_t kRotateRva = 0x307090;
inline constexpr std::uint32_t kCompactRva = 0x307170;
inline constexpr std::uint32_t kStagingPairRva = 0x13DEA10;  // xmmword_1813DEA10
inline constexpr std::uint32_t kBucketTableRva = 0x13DEA20;  // unk_1813DEA20
inline constexpr std::uint32_t kScanCountRva = 0x13DF048;    // bucket 1 + 784
inline constexpr std::uint32_t kSelectedBucketRva = 0x1366910; // dword_181366910
inline constexpr std::uint32_t kGroupAssignmentToggleRva = 0x13E4A8E; // byte_1813E4A8E
inline constexpr std::uint32_t kSelectionCodeRva = 0x2F4060;  // release gate
inline constexpr std::uint32_t kConnectorScanRva = 0x1A60C0;  // glissando judge

// Slot offsets inside one 28-byte staging/bucket slot.
inline constexpr std::size_t kSlotKey = 0x00;
inline constexpr std::size_t kSlotState = 0x04;
inline constexpr std::size_t kSlotValue = 0x08;
inline constexpr std::size_t kSlotUnused0 = 0x0C; // never read in this build
inline constexpr std::size_t kSlotUnused1 = 0x10; // never read in this build
inline constexpr std::size_t kSlotGroup = 0x14;
inline constexpr std::size_t kSlotRepeat = 0x18;

// Slot states written by the producer / consumed by the note managers.
inline constexpr std::int32_t kStateFree = 0;
inline constexpr std::int32_t kStatePress = 1;
inline constexpr std::int32_t kStateHeld = 2;
inline constexpr std::int32_t kStateRelease = 3;

// Phase 2 re-arms the counter to 1 once it reaches 25; the staging value never
// exceeds 24 in the probe, so 25 is accepted as the last value before the wrap.
inline constexpr std::uint32_t kRepeatLimit = 25;

// bucket 0 is the live record, bucket 1 the previous frame (constant selection).
inline constexpr std::size_t kSelectedBucket = 0;
inline constexpr std::size_t kPreviousBucket = 1;
inline constexpr std::size_t kGroupScanFirstBucket = 2;
inline constexpr std::size_t kGroupScanLastBucket = 5;
inline constexpr std::uint32_t kDefaultGroup = 1; // phase 2 forces 0 -> 1

} // namespace bucket_input_layout

// ===========================================================================
// Status
// ===========================================================================
enum class BucketInputStatus : std::uint8_t {
    Ok = 0,
    NotConfigured,
    GenerationZero,      // a frame without a playback generation cannot be ordered
    StaleGeneration,     // frame from a generation older than the current one
    NonFiniteTime,       // NaN / inf / beyond kNoteTypeMaximumTimeUs
    BackwardTime,        // song time before the previous frame of this generation
    MalformedBuffer,     // staging shorter than 28 x 28 bytes
    InvalidKey,          // active slot whose key is outside 1..28
    InvalidState,        // slot state outside 0..3
    InvalidRepeat,       // repeat counter above the native re-arm point
    DuplicateKey,        // one key named by two active slots in one frame
    OrphanEvent,         // refuse_orphans: a release/held with no observed press
    InsufficientCapacity,// event span cannot hold every active slot
};

[[nodiscard]] const char* BucketInputStatusName(BucketInputStatus status) noexcept;

// ===========================================================================
// Decoded frame
// ===========================================================================
// One slot of the staging buffer, decoded. A free slot (state 0) keeps whatever
// the producer last left there; the other fields are still reported verbatim so
// a caller can log the buffer, but only an active slot produces an event.
struct BucketSlot final {
    std::int32_t key = 0;    // +0x00, 1-based when the slot is active
    std::uint8_t state = 0;  // +0x04
    std::uint8_t value = 0;  // +0x08, raw byte (see the value note above)
    std::uint32_t group = 0; // +0x14, 0 from the staging buffer
    std::uint32_t repeat = 0;// +0x18
};

struct BucketFrameCounts final {
    std::size_t slots_seen = 0;      // 28 for every decoded frame
    std::size_t active = 0;          // state != 0
    std::size_t presses = 0;
    std::size_t repeats = 0;
    std::size_t releases = 0;
    std::size_t free_slots = 0;
    std::size_t free_with_value = 0; // free slot whose value byte is not 0
    std::size_t values_nonzero = 0;  // active slots with a nonzero value byte
    std::size_t max_repeat = 0;
    std::size_t distinct_keys = 0;   // equals active (duplicates are refused)
    std::size_t drops = 0;           // released last frame and absent now
    std::size_t dropped_while_held = 0; // held/pressed last frame and absent now
    std::size_t orphan_releases = 0; // release with no observed press
    std::size_t orphan_repeats = 0;  // held with no observed press
    std::size_t press_while_held = 0;// state 1 while the key was already down
    std::size_t generation_changes = 0;
    bool rebased = false;            // first frame of a new generation
};

// One ordered input event. `kind` uses the native judge's event kind so the
// value can be handed to NativeNoteEvent unchanged; `lane` is key - 1, the
// pipeline convention.
struct BucketInputEvent final {
    SongTime song_time{};
    std::uint8_t slot = 0;   // source slot 0..27, kept for diagnostics
    std::uint8_t lane = 0;   // key - 1 (0..27)
    std::int32_t key = 0;    // native key 1..28
    NativeEventKind kind = NativeEventKind::Press;
    std::uint8_t value = 0;  // slot +0x08, raw
    std::uint32_t group = 0; // slot +0x14, 0 from the staging buffer
    std::uint32_t repeat = 0;// slot +0x18
    bool orphan = false;     // release/held with no observed press
};

// Pure slot decode: reads all 28 slots of one staging buffer, validates them and
// reports the slot-level counts. `slots` must hold at least kBucketSlotCount
// entries. On any non-Ok status neither `slots` nor `counts` is modified.
[[nodiscard]] BucketInputStatus DecodeStagingSlots(std::span<const std::byte> staging,
                                              std::span<BucketSlot> slots,
                                              BucketFrameCounts& counts) noexcept;

// ===========================================================================
// Frame decoder (staging buffer -> ordered events)
// ===========================================================================
struct BucketInputConfig final {
    // A release or held state whose press was never observed is reported as
    // `orphan` (the native judges tolerate it). Set this to refuse the frame
    // instead, which a strict shadow comparator can use to detect a decoder
    // that started mid-hold.
    bool refuse_orphans = false;
    // Keep the per-key tracking across a generation change. The native has no
    // such notion (it only ever sees the producer's snapshot), so the default
    // re-baselines.
    bool keep_tracking_across_generations = false;
};

class BucketInputState final {
public:
    BucketInputState() = default;

    [[nodiscard]] BucketInputStatus Configure(const BucketInputConfig& config) noexcept;

    // Decodes one published staging frame. `frame_time` is the song-clock
    // snapshot for that frame; its generation orders the frames. Events are
    // written in slot order (the native iteration order) and `events` must hold
    // at least kBucketMaxFrameEvents entries. On any non-Ok status no state
    // changes and no event is written; the single exception is OrphanEvent,
    // which fills `counts` with the diagnostics that caused the refusal.
    [[nodiscard]] BucketInputStatus Apply(std::span<const std::byte> staging,
                                     const SongTime& frame_time,
                                     std::span<BucketInputEvent> events,
                                     BucketFrameCounts& counts) noexcept;

    // Seek / stop / reconnect: forget the per-key tracking. The configuration
    // survives; the next accepted frame is the first of its generation.
    void Reset() noexcept;

    [[nodiscard]] bool Configured() const noexcept { return configured_; }
    [[nodiscard]] std::uint64_t Generation() const noexcept { return generation_; }
    [[nodiscard]] double LastSongTimeUs() const noexcept { return last_song_us_; }
    [[nodiscard]] std::size_t FrameCount() const noexcept { return frames_; }
    // Rejected frame count; a call on an unconfigured instance is not counted.
    [[nodiscard]] std::size_t RejectedFrames() const noexcept { return rejected_; }
    // Last observed state for a lane (0 free, 1 press, 2 held, 3 release).
    [[nodiscard]] std::uint8_t LastState(std::uint8_t lane) const noexcept;
    [[nodiscard]] bool HasObservedPress(std::uint8_t lane) const noexcept;

private:
    void Rebase() noexcept;

    BucketInputConfig config_{};
    std::array<std::uint8_t, kBucketSlotCount> last_state_{};
    std::array<bool, kBucketSlotCount> observed_press_{};
    std::uint64_t generation_ = 0;
    double last_song_us_ = 0.0;
    std::size_t frames_ = 0;
    std::size_t rejected_ = 0;
    bool configured_ = false;
    bool started_ = false;
};

// ===========================================================================
// Bucket table (rotation + compaction)
// ===========================================================================
// One entry of a bucket record. The entry is the model's view; the 792-byte
// record also holds 28 slots, a count at +784 and 4 filler bytes.
struct BucketEntry final {
    std::int32_t key = 0;    // +0x00
    std::uint8_t state = 0;  // +0x04
    std::uint8_t value = 0;  // +0x08
    std::uint32_t group = 0; // +0x14
    std::uint32_t repeat = 0;// +0x18
};

struct BucketTableConfig final {
    // Phase 2 of 0x307170, gated in the native build on byte_1813E4A8E == 1.
    // Off by default: the group id and the repeat counter then keep the
    // staging values and every new entry is reported as group-pending.
    bool assign_groups = false;
};

class BucketTableState final {
public:
    BucketTableState() = default;

    [[nodiscard]] BucketInputStatus Configure(const BucketTableConfig& config) noexcept;
    void Reset() noexcept;

    // 0x307090: bucket k <- bucket k-1 for k = 29..1, then bucket 0 is zeroed.
    [[nodiscard]] BucketInputStatus Rotate() noexcept;

    // 0x307170: phase 1 rebuilds bucket 0 from the staging buffer (see the
    // module comment for the decay rule), phase 2 assigns groups when enabled.
    // Rotation is the caller's step, exactly as in the native update, where
    // 0x307090 runs immediately before 0x307170.
    [[nodiscard]] BucketInputStatus Compact(std::span<const std::byte> staging) noexcept;

    [[nodiscard]] std::size_t Count(std::size_t bucket) const noexcept;
    [[nodiscard]] BucketEntry Entry(std::size_t bucket, std::size_t index) const noexcept;
    [[nodiscard]] std::span<const BucketEntry> Entries(std::size_t bucket) const noexcept;
    [[nodiscard]] bool Configured() const noexcept { return configured_; }
    [[nodiscard]] bool GroupsAssigned() const noexcept { return config_.assign_groups; }
    [[nodiscard]] std::size_t GroupPending() const noexcept { return group_pending_; }
    [[nodiscard]] std::size_t RepeatCarried() const noexcept { return repeat_carried_; }
    [[nodiscard]] std::size_t DecayedRePresses() const noexcept { return decayed_re_presses_; }
    [[nodiscard]] std::size_t DecayedDrops() const noexcept { return decayed_drops_; }

    // Copies a bucket into the caller's view of the held-input table so the
    // existing pure rules can run on it: EvaluateReleaseGate and
    // FindConnectorKey (native_note_types.hpp). Returns the entry count that
    // fits; never more than kBucketSlotCount.
    std::size_t ToActiveInputEntries(std::size_t bucket,
                                     std::span<ActiveInputEntry> out) const noexcept;

private:
    [[nodiscard]] const BucketEntry* FindByKey(std::int32_t key) const noexcept;
    [[nodiscard]] const BucketEntry* FindNeighbourGroup(std::int32_t key) const noexcept;

    BucketTableConfig config_{};
    std::array<std::array<BucketEntry, kBucketSlotCount>, kBucketTableCount> buckets_{};
    std::array<std::uint32_t, kBucketTableCount> counts_{};
    std::size_t group_pending_ = 0;
    std::size_t repeat_carried_ = 0;
    std::size_t decayed_re_presses_ = 0;
    std::size_t decayed_drops_ = 0;
    bool configured_ = false;
};

} // namespace nosfix
