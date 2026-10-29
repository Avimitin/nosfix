#pragma once

// Portable lifecycle models for the non-normal chart note types of
// nostalgia.dll PAN-2025020501: long/hold (0x2F4220), glissando (0x1A60C0) and
// trill (0x1A7AF0). This is a sibling of native_note_state.hpp (normal notes)
// and shares only normal_judgment.hpp's window kernel and timing_events.hpp's
// captured input representation. It never reads or writes game memory, owns no
// hook, and performs no allocation, locking, logging or I/O.
//
// Evidence base (module-relative RVAs; all pseudocode under analyze/evidence):
//   long judge        0x2F4220  verified_0x1802f4220.c
//   long update       0x2F4E80  verified_0x1802f4e80.c  (state pulse + stale arm)
//   gliss judge       0x1A60C0  verified_0x1801a60c0.c
//   gliss render      0x1A6820  verified_0x1801a6820.c  (connector records)
//   trill judge       0x1A7AF0  verified_0x1801a7af0.c
//   trill render      0x1A84F0  verified_0x1801a84f0.c
//   eligibility       0x2F4E20 / 0x1A8490
//   chart prepare     0x1A43E0  (native_chart_fields.json, native_chart_tail.json)
//
// Shared object layout (the three classes derive from the same note base as the
// normal note; offsets quoted from the judge pseudocode):
//   +0  vtable          +8  chart record pointer      +20 lane
//   +24 note time units (chart+20)                    +32 field copied to chart+80
//   +44 left key (chart+88)  +48 right key (chart+92) +52 mid key
//   +68 done byte       +76 pending-early byte        +80 pending rank
//   +84 event state     +108/+116 flags               +120 rank result
//   +124 mode           +128 render object
// Long adds:  +288 note object, +296 end time units (chart+24), +300 phase
//             (0 first press, 1 held, other finished), +304 head rank,
//             +308 effect index, +312 start-commit pulse, +313 stale hold,
//             +316 sound/event id, +320 recorded delta, +324 score scale.
// Trill adds: +304 note object, +312 end time units, +316 phase, +320 head rank,
//             +324 effect index, +328 start-commit pulse, +329 stale hold,
//             +332 recorded delta, +344 alternation counter, +352 scale.
// Gliss adds: +216 trail object (slot +24/+32 release), +272 group id,
//             +276 effect index; chart+64 / chart+72 connector records.
//
// Proven semantics (see the stream report for the exact pseudocode lines):
//   * rank codes are the normal ones: 0 TooEarly, 1 Missed, 2 Good, 3 Great,
//     4 Just; the judge compares note - song - judgment offset against the
//     inclusive 13668E8..13668FC windows (gliss inlines the union, long/trill
//     call 0x305A80 which returns the same code);
//   * long and trill defer a press with 0 < delta <= great_early and record
//     rank 3 + (rank == Just); the deferred press is resolved when song time
//     reaches note - judgment offset and is then judged with that recorded rank;
//   * long/trill assign a fresh keysound id (10000, incremented, reset to 10000
//     above 20000) when the start is committed by a non-null event; a commit
//     resolved by a null update gets no sound (-1);
//   * long finishes at its end time while held with the head rank and scale 100;
//     releasing more than 15 song units before the end yields rank Good(2) and
//     scale 50 and increments the native penalty counter; a "stale hold" (the
//     +313 arm set by the update when no start commit was observed) does the
//     same. A finish with rank < Great fades the sound; rank >= Great does not;
//   * trill finishes at its end time with the head rank and scale 100 when the
//     note+344 countdown is positive, otherwise rank Good(2) and scale 40. The
//     countdown is 99999 at the head commit and re-armed to 25 by every tail
//     press (0x1A7910), so a tail that has not alternated for 25 frames scores
//     Good/40;
//   * gliss produces exactly one judgement: a press inside a window defers to
//     the note time (recorded rank 4), a press at/after the note time is judged
//     at its own rank, a press past the outer late window finishes immediately
//     as Missed(1); auto notes finish as Just(4) at the note time. Its two
//     connector records (chart+64 / chart+72, pointers to other 104-byte chart
//     elements) do not participate in the judgement: render 0x1A6820 appends one
//     120-byte box record per connector into the primitive at note+216 between
//     its vtable slots +24 and +32, and the judge only tests the pair for
//     non-null and calls both slots again to close the bracket. With no press,
//     a post-note scan of the held-input table can still complete it at the
//     rank of the captured delta (see FindConnectorKey);
//   * a long release is gated by 0x2F4060: it proceeds only when no held input
//     entry lies in [left - tolerance, right + tolerance]; otherwise the release
//     is refused, each matching key is appended to the 0x1813E46F8 repeat list,
//     and note+84 is set to 1 when a matching key is strictly inside the span
//     (see EvaluateReleaseGate). The trill tail release is not gated;
//   * a trill tail press re-triggers the sound of the chart sub-element at
//     note+348 (advancing and wrapping that cursor), re-arms the 25-frame
//     countdown and never judges;
//
// Deliberately NOT modelled (fail closed, see the refused branches below):
//   * the held-input table itself (0x1813DEA20, 30 buckets x 792 bytes). Its
//     shape is proven (28 entries of {key at +0, state at +4, group at +20} and
//     a count at +784), but its writer is the undecoded input producer. The
//     released rules are implemented as pure functions over a caller-supplied
//     entry span, and both gates stay refused until the caller declares the
//     table available (release_gate_available / connector_scan_available).
//   * the shared 0x1813E46F8 repeat-id list beyond the keys the gate reports,
//     the still-unlocated input-event producer (event+0 key, +4 kind, +20
//     group; levels never reach a judge), note+96 auto slot objects, native
//     score points, effect/particle spawning, sound-table entries and vtable
//     dispatch. Outcome values describe what the native seam must forward. The
//     two other non-normal note classes (velocity 0x1AA340 and beats 0x1A58C0,
//     vtables 0x679D68/0x679960) are out of scope for this stream.
//   * the input event's +0x20 "value" (stored into note+84 and tested as
//     `id > 1 && rank > 1` by 0x303E80) is the one non-constant numeric input to
//     scoring. It belongs to the judgment stream's event producer and is not
//     modelled here; NativeNoteEvent deliberately carries no such field.
//
// Timing: all decisions use double microseconds and the captured SongTime of a
// press, never the render tick that consumes it. The end tolerance is expressed
// in legacy song units and converted with the caller's units_per_second, so a
// 60/90/120/144/165/240/360 Hz render cadence with jitter or stalls cannot
// change a rank, a scale, a sound id or the number of judgements.

#include "nosfix/normal_judgment.hpp"
#include "nosfix/timing_events.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace nosfix {

inline constexpr std::uint8_t kNativeTypeLaneCount = 28;
inline constexpr std::size_t kNoteTypeMaxOutcomes = 4;
inline constexpr double kNoteTypeMaximumTimeUs = 1.0e12;

// ===========================================================================
// Verified layout and constants (evidence, not ABI claims)
// ===========================================================================
namespace nonnormal_layout {

inline constexpr std::uint32_t kLongJudgeRva = 0x2F4220;
inline constexpr std::uint32_t kLongUpdateRva = 0x2F4E80;
inline constexpr std::uint32_t kGlissJudgeRva = 0x1A60C0;
inline constexpr std::uint32_t kGlissRenderRva = 0x1A6820;
inline constexpr std::uint32_t kTrillJudgeRva = 0x1A7AF0;
inline constexpr std::uint32_t kTrillRenderRva = 0x1A84F0;
inline constexpr std::uint32_t kTrillEligibilityRva = 0x1A8490;
inline constexpr std::uint32_t kNonNormalEligibilityRva = 0x2F4E20;
inline constexpr std::uint32_t kChartPrepareRva = 0x1A43E0;

// 0x305A80: returns the normal rank code for (delta, chart auto byte); also
// receives the sound-table bounds, whose side effects this model does not
// reproduce. 0x3053E0 maps (delta, rank) to the effect index; that mapping is
// not decoded, so effect_grade stays 0 and is flagged.
inline constexpr std::uint32_t kRankEvaluateRva = 0x305A80;
inline constexpr std::uint32_t kRankEffectRva = 0x3053E0;

// Now decoded (answers/notes-input-001.md Q1/Q2/Q3/Q5). The rules are
// implemented; the runtime tables they read are the undecoded input producer's
// state, so callers supply them. Constants below are the proven defaults.
inline constexpr std::uint32_t kReleaseGateRva = 0x2F4060;
inline constexpr std::uint32_t kTrillTailHandlerRva = 0x1A7910;
// The held-input table is VA 0x1813DEA20 (RVA 0x13DEA20); the glissando scan
// starts at VA 0x1813DF048 = bucket 1's count field (RVA 0x13DF048).
inline constexpr std::uint32_t kGlissConnectorTableRva = 0x13DEA20; // bucket base
inline constexpr std::uint32_t kGlissConnectorCountRva = 0x13DF048; // bucket 1 +784
inline constexpr std::uint32_t kRepeatIdListRva = 0x13E46F8;
inline constexpr std::uint32_t kGlissBinderRva = 0x1A7450;     // note+272 writer
inline constexpr std::uint32_t kInputKeyTableRva = 0x13DEA20;
inline constexpr std::uint32_t kInputKeyBucketBytes = 792;      // 28*28 + count at +784
inline constexpr std::uint32_t kInputKeyEntryBytes = 28;
inline constexpr std::uint32_t kInputKeyEntryCount = 28;
inline constexpr std::uint32_t kInputKeyEntryGroupOffset = 20;

// 0x2F4060 widens the note's key span by dword_1813E4A84 before it treats a held
// input entry as "in range". The initialiser writes 3; chart/option code also
// writes it, so the caller should pass the live value.
inline constexpr std::int32_t kReleaseGateToleranceDefault = 3;
// Note+272 values written by 0x1A7450 (chart key vs midpoint). The connector
// scan treats 0 as "no group" and the event match below treats 1 as wildcard.
inline constexpr std::uint32_t kWildcardScanGroupId = 0;
inline constexpr std::uint32_t kWildcardEventGroupId = 1;
// Note+344 is a frame countdown, not a hit counter: 99999 at the trill head
// commit, re-armed to 25 by every tail press, decremented by the periodic tick.
inline constexpr std::int32_t kTrillCountdownAtCommit = 99999;
inline constexpr std::int32_t kTrillTailCountdownFrames = 25;
inline constexpr std::size_t kTrillMaxSubElements = 32;

// The release gate sets note+84 = 1. 0x303E80 forces rank 5 only when
// note+84 > 1, so this side effect cannot turn a release into a rank-5 hit.
inline constexpr std::int32_t kReleaseGateNote84Value = 1;

// 104-byte chart note record (chart preparation 1A43E0).
inline constexpr std::size_t kChartNoteBytes = 104;
inline constexpr std::size_t kChartStartMs = 0;
inline constexpr std::size_t kChartEndMs = 4;
inline constexpr std::size_t kChartType = 8;
inline constexpr std::size_t kChartFlags = 12;
inline constexpr std::size_t kChartLane = 16;       // int16
inline constexpr std::size_t kChartStartFrame = 20;
inline constexpr std::size_t kChartEndFrame = 24;
inline constexpr std::size_t kChartMidFrame = 28;
inline constexpr std::size_t kChartSubCount = 36;
inline constexpr std::size_t kChartSubPointer = 40;
inline constexpr std::size_t kChartState = 48;
inline constexpr std::size_t kChartMode = 52;
inline constexpr std::size_t kChartConnectorBefore = 64; // 104-byte chart record*
inline constexpr std::size_t kChartConnectorAfter = 72;  // 104-byte chart record*
inline constexpr std::size_t kChartLeftKey = 88;
inline constexpr std::size_t kChartRightKey = 92;
inline constexpr std::size_t kChartCommitFlag = 96;
inline constexpr std::size_t kChartAutoFlag = 97;

// Flags -> chart type, exactly as 1A4C01..1A4C2D derive it: bit 2 wins, then
// bit 6, then bit 1; no bit means a normal note (type 0).
inline constexpr std::int32_t kChartFlagLong = 0x2;
inline constexpr std::int32_t kChartFlagGliss = 0x4;
inline constexpr std::int32_t kChartFlagTrill = 0x40;

// 15 song units of end tolerance; the native compares the remaining hold time
// against +15 and 1 in legacy units.
inline constexpr std::int32_t kEndToleranceUnits = 15;
inline constexpr std::int32_t kEndReachedUnits = 1;

// Judge-virtual scores: vtable+104 is called with (rank, mode, scale).
inline constexpr std::uint16_t kScaleNormal = 100;
inline constexpr std::uint16_t kScaleLongPenalty = 50;
inline constexpr std::uint16_t kScaleTrillPenalty = 40;

// Keysound ids: dword_1813E56E0 starts at 10000, is pre-incremented per start
// and reset to 10000 once it would exceed 20000.
inline constexpr std::uint32_t kSoundSerialStart = 10000;
inline constexpr std::uint32_t kSoundSerialLimit = 20000;

// The glissando group id at +272 (1/2/3 from 0x1A7450) matches any event when
// it is 1; the connector scan treats 0 as "no group" instead.
inline constexpr std::uint32_t kWildcardGroupId = kWildcardEventGroupId;

// Trill countdown default written at head commit (note+344).
inline constexpr std::int32_t kTrillCounterInitial = kTrillCountdownAtCommit;

enum class ChartNoteType : std::uint8_t {
    Normal = 0,
    Long = 1,
    Gliss = 2,
    Trill = 4,
    Unknown = 0xFF,
};

// Derives the chart type from the record flags (chart+12), reproducing the
// precedence of 1A4C01..1A4C2D. A record with none of the three bits is normal.
[[nodiscard]] ChartNoteType ChartNoteTypeFromFlags(std::int32_t flags) noexcept;

constexpr bool IsChartNoteType(std::uint8_t value) noexcept {
    return value == 0 || value == 1 || value == 2 || value == 4;
}

} // namespace nonnormal_layout

// ===========================================================================
// Captured input mapped to the native event shape
// ===========================================================================
// The native judge receives (event, mode) where event+0 is the pressed key,
// event+4 is the kind (1 press, 2 repeat/hold, 3 release) and event+20 is the
// group/segment tag (0x1A60C0 reads it as a dword; answers/notes-input-001.md Q6
// reports a byte at +5 instead, which conflicts with the judge pseudocode - the
// value stays opaque either way). The producer is not decoded, so nothing here
// invents an id scheme. `panb_level` is carried for consumers; it never
// participates in a rank, scale or judgement count.
enum class NativeEventKind : std::uint8_t {
    Press = 1,
    Repeat = 2,
    Release = 3,
};

[[nodiscard]] const char* NativeEventKindName(NativeEventKind kind) noexcept;

struct NativeNoteEvent final {
    InputStamp stamp{};
    SongTime song_time{};
    std::uint8_t lane = 0;
    NativeEventKind kind = NativeEventKind::Press;
    std::uint32_t event_id = 0; // native event +0: the key/id the judges append
    std::uint32_t group_id = 0; // native event +20 (see above)
    float velocity = 0.0f;
    std::uint8_t panb_level = 0;
};

// ===========================================================================
// Held-input state and the two rules that read it
// ===========================================================================
// Shape of one 28-byte entry in the per-key input table at 0x1813DEA20 (Q3):
// 30 buckets of { 28 entries, count at +784 }, entry +0 key, +4 state (1 or 2 =
// held), +20 group/segment tag. The writer is the undecoded input producer, so
// every function below takes the entries from the caller and is pure.
struct ActiveInputEntry final {
    std::int32_t key = 0;    // entry+0
    std::uint8_t state = 0;  // entry+4; 1 or 2 means held
    std::uint32_t group = 0; // entry+20
};

[[nodiscard]] constexpr bool IsHeldInputState(std::uint8_t state) noexcept {
    return state == 1 || state == 2;
}

// Result of 0x2F4060. `proceed` is true only when the gate returns 0, i.e. no
// held entry lies inside the widened span; a long release may then run.
struct ReleaseGateDecision final {
    bool evaluated = false;        // the caller ran the rule over its entries
    bool proceed = false;          // 0x2F4060 returned 0
    bool note84_set = false;       // some matching key was strictly inside
    std::int32_t held_key = 0;     // first matching key, 0 when none
    std::size_t appended_keys = 0; // keys appended to the 0x1813E46F8 list
};

// 0x2F4060: walks the held entries and returns 0 (proceed) only when none is in
// [left - tolerance, right + tolerance]. Every match is appended to the repeat
// list and, when it is strictly inside [left, right], sets note+84 = 1. The
// comparison is inclusive on both ends and uses the native key domain for both
// sides (answers/notes-input-001.md reports entry keys as 0..27 and chart+88/+92
// as 1-based; the caller must pass values in one domain - the ±1 question is
// unresolved, so this function never converts).
[[nodiscard]] ReleaseGateDecision EvaluateReleaseGate(
    std::int32_t left_key, std::int32_t right_key, std::int32_t tolerance_keys,
    std::span<const ActiveInputEntry> held_entries) noexcept;

// Glissando post-note connector scan (0x1A60C0 at 0x1A63E6). Returns the key of
// the first entry whose key is inside [left, right] (inclusive) and whose +20
// matches `note_group_id`, or -1. `note_group_id == 0` disables the group test.
// Unlike the release gate this scan does not test the entry state.
[[nodiscard]] std::int32_t FindConnectorKey(std::span<const ActiveInputEntry> entries,
                                            std::int32_t left_key, std::int32_t right_key,
                                            std::uint32_t note_group_id) noexcept;

// ===========================================================================
// Configuration, results and status
// ===========================================================================
struct NativeTypeConfig final {
    NormalWindows windows{};
    bool windows_sampled = false;
    double judgment_offset_us = 0.0; // global RVA 0x136690C
    double units_per_second = 60.0;  // explicit legacy unit rate
    std::uint32_t sound_serial_start = nonnormal_layout::kSoundSerialStart;
    std::uint32_t sound_serial_limit = nonnormal_layout::kSoundSerialLimit;

    // The two runtime tables these rules read are the undecoded input
    // producer's state. Both default to unavailable, which keeps every gated
    // branch refused; a caller that has the table opts in explicitly.
    bool release_gate_available = false;    // 0x2F4060 + 0x1813DEA20
    std::int32_t release_gate_tolerance_keys =
        nonnormal_layout::kReleaseGateToleranceDefault;
    bool connector_scan_available = false;  // 0x1813DF048 / 0x1813DEA20
    bool trill_tail_handler_proven = false; // 0x1A7910 sub-element effects
    bool gliss_repeat_as_press = false;     // 0x1813E46F8 repeat list

    // Adapter policy, not a claim about a judge routine: an unpressed head is
    // dropped by the native dispatcher (2F7C10 / vtable+232), which this model
    // does not implement. When true, an unpressed head past its inclusive
    // Good-late deadline is reported once as Missed with judged == false; both
    // the update time and the certified decision watermark must be past it.
    bool expire_unpressed_heads = true;
};

enum class NoteTypeStatus : std::uint8_t {
    Ok = 0,
    NotConfigured,
    NotStarted,
    GenerationZero,
    GenerationNotNew,
    GenerationMismatch,
    NonFiniteTime,
    BackwardTime,
    WatermarkBehindCapture,
    WatermarkAhead,
    InvalidSpec,
    WindowsNotSampled,
    InvalidWindows,
    InvalidOffset,
    InvalidRate,
};

[[nodiscard]] const char* NoteTypeStatusName(NoteTypeStatus status) noexcept;

enum class LongPhase : std::uint8_t {
    Idle = 0,     // configured, no playback segment yet
    Skipped = 1,  // note time before the playback start
    Waiting = 2,  // first press outstanding
    PendingEarly = 3,
    Held = 4,
    Finished = 5,
    Missed = 6,
};

enum class GlissPhase : std::uint8_t {
    Idle = 0,
    Skipped = 1,
    Waiting = 2,
    PendingEarly = 3,
    Finished = 4,
    Missed = 5,
};

enum class TrillPhase : std::uint8_t {
    Idle = 0,
    Skipped = 1,
    Head = 2,      // first press outstanding
    PendingEarly = 3,
    Tail = 4,      // head committed, tail stage active
    Finished = 5,
    Missed = 6,
};

enum class NoteTypeOutcomeKind : std::uint8_t {
    HeadCommitted = 0,  // start accepted; no judgement
    PendingEarly = 1,   // press deferred to the note time
    Scored = 2,         // judgement: rank/scale forwarded to vtable+104
    Auto = 3,           // auto completion (rank Just, scale 100)
    Missed = 4,         // judgement rank Missed(1)
    KeysoundCut = 5,    // host must fade the named sound; no judgement
    RepeatIgnored = 6,  // kind-2 event recorded by the native, never judged
    ConnectorReleased = 7, // gliss trail object begin/end pair
    Unsupported = 8,    // refused branch; reason is a static string
    ReleaseRefused = 9, // 0x2F4060 found a held key in the span; release stays
    TrillTailHit = 10,  // 0x1A7910 effects: sub-element sound, cursor, countdown
};

[[nodiscard]] const char* NoteTypeOutcomeKindName(NoteTypeOutcomeKind kind) noexcept;

struct NoteTypeOutcome final {
    NoteTypeOutcomeKind kind = NoteTypeOutcomeKind::HeadCommitted;
    NormalRank rank = NormalRank::TooEarly; // Scored/Missed/Auto
    std::uint8_t effect_grade = 0;          // vtable+112 argument (see flags)
    std::uint16_t scale = nonnormal_layout::kScaleNormal;
    std::uint32_t sound_id = 0;
    std::uint32_t event_id = 0;
    std::uint8_t lane = 0;
    std::uint8_t connectors = 0; // ConnectorReleased: 1 or 2
    std::int32_t connector_key = 0; // gliss scan hit key; 0 when none
    std::int32_t gate_key = 0;      // ReleaseRefused: the held key that blocked
    std::uint32_t gate_appended = 0; // ReleaseRefused: keys added to 0x1813E46F8
    std::uint32_t sub_element = 0;   // TrillTailHit: cursor (note+348) used
    std::uint64_t cue_reference = 0; // TrillTailHit: sub-element +16 sample ref
    std::uint32_t length_frames = 0; // TrillTailHit: elem end - start (min 1)
    std::int32_t countdown = 0;      // TrillTailHit: note+344 after re-arm
    bool gate_note84 = false;        // ReleaseRefused: note+84 was set to 1
    bool judged = false;         // a vtable+104 call happens
    bool sound_started = false;
    bool sound_cut = false;
    bool effect_unverified = true; // effect_grade is this model's stand-in
    double at_us = 0.0;
    double delta_us = 0.0;     // note delta for presses; 0 for null updates
    double remaining_us = 0.0; // native v11 for hold/tail decisions; 0 otherwise
    const char* reason = nullptr; // static literal for Unsupported
};

struct NoteTypeResult final {
    NoteTypeStatus status = NoteTypeStatus::NotConfigured;
    std::size_t count = 0;
    std::array<NoteTypeOutcome, kNoteTypeMaxOutcomes> outcomes{};
    std::size_t ignored = 0;     // accepted events with no observable effect
    std::size_t unsupported = 0; // refused branches
    std::size_t repeats = 0;     // kind-2 events observed
    std::size_t penalties = 0;   // native penalty-counter increments
    std::size_t cuts = 0;        // keysound cuts
    std::size_t refused_releases = 0; // 0x2F4060 refused a release
};

// ===========================================================================
// Shared helpers
// ===========================================================================

// 0x2F4E20: the note is eligible while its phase field (+300) is 1, otherwise
// the inclusive union of the three windows decides. `delta_us` is
// note - captured - judgment offset.
[[nodiscard]] bool LongEligible(bool held, double delta_us,
                                const NormalWindows& windows) noexcept;

// 0x1A8490: identical structure with the trill tail field (+316).
[[nodiscard]] bool TrillEligible(bool tail, double delta_us,
                                 const NormalWindows& windows) noexcept;

// ===========================================================================
// Long / hold
// ===========================================================================
struct NativeLongSpec final {
    double start_us = 0.0; // chart+0 ms -> us
    double end_us = 0.0;   // chart+4 ms -> us, >= start_us
    std::uint8_t lane = 0;
    std::uint32_t chart_index = 0;
    // Native keys used by the 0x2F4060 span test (chart+88 / chart+92). Same
    // domain as ActiveInputEntry::key; the model never converts them.
    std::int32_t left_key = 0;
    std::int32_t right_key = 0;
    bool auto_play = false; // global auto flag or chart+97
};

class NativeLongState final {
public:
    NativeLongState() = default;

    [[nodiscard]] NoteTypeStatus Configure(const NativeLongSpec& spec,
                                           const NativeTypeConfig& config) noexcept;

    // Starts a playback segment. Notes whose start precedes `position_us` are
    // Skipped and can never score. generation must be non-zero and new.
    [[nodiscard]] NoteTypeStatus Begin(std::uint64_t generation, double position_us) noexcept;

    // `held_entries` is the caller's view of the per-key input table. It is only
    // read when config.release_gate_available is set, and only to answer the
    // 0x2F4060 span test for a release; an empty span then means "no held key".
    [[nodiscard]] NoteTypeResult Press(const NativeNoteEvent& event, double decision_us,
                                       std::span<const ActiveInputEntry> held_entries = {}) noexcept;
    [[nodiscard]] NoteTypeResult Release(const NativeNoteEvent& event, double decision_us,
                                         std::span<const ActiveInputEntry> held_entries = {}) noexcept;
    // Null-event update. `hold_commit_missed` is the native +313 arm produced by
    // the update when a held note never observed its start-commit pulse.
    [[nodiscard]] NoteTypeResult Advance(double up_to_us, double decision_us,
                                         bool hold_commit_missed = false) noexcept;

    [[nodiscard]] LongPhase Phase() const noexcept { return phase_; }
    [[nodiscard]] NormalRank FinalRank() const noexcept { return final_rank_; }
    [[nodiscard]] bool Done() const noexcept { return done_; }
    [[nodiscard]] bool PendingEarly() const noexcept { return pending_; }
    [[nodiscard]] std::uint32_t ActiveSound() const noexcept { return sound_id_; }
    [[nodiscard]] double RecordedDeltaUs() const noexcept { return recorded_delta_us_; }
    [[nodiscard]] std::uint32_t NextSoundSerial() const noexcept { return sound_serial_; }
    [[nodiscard]] std::size_t PenaltyCount() const noexcept { return penalties_; }
    [[nodiscard]] std::uint64_t Generation() const noexcept { return generation_; }

private:
    [[nodiscard]] bool Ready(NoteTypeResult& result) const noexcept;
    [[nodiscard]] NoteTypeStatus CheckEvent(const NativeNoteEvent& event) const noexcept;
    [[nodiscard]] double DeltaUs(const NativeNoteEvent& event) const noexcept;
    [[nodiscard]] double RemainingUs(double captured_us) const noexcept;
    [[nodiscard]] double UnitUs() const noexcept { return unit_us_; }
    [[nodiscard]] std::uint32_t NextSerial() noexcept;
    void CommitStart(NormalRank rank, bool have_event, double captured_us, double delta_us,
                     NoteTypeResult& result) noexcept;
    void Finish(NormalRank rank, std::uint16_t scale, NoteTypeOutcomeKind kind,
                double captured_us, double delta_us, NoteTypeResult& result) noexcept;
    [[nodiscard]] NoteTypeResult JudgeHead(const NativeNoteEvent* event, double captured_us,
                                           bool have_event) noexcept;
    [[nodiscard]] NoteTypeResult HeldStep(bool is_release, bool commit_missed,
                                          double captured_us, double delta_us) noexcept;
    [[nodiscard]] NoteTypeResult Expire(double up_to_us, double decision_us) noexcept;

    NativeLongSpec spec_{};
    NativeTypeConfig config_{};
    LongPhase phase_ = LongPhase::Idle;
    NormalRank head_rank_ = NormalRank::TooEarly;
    NormalRank pending_rank_ = NormalRank::TooEarly;
    NormalRank final_rank_ = NormalRank::TooEarly;
    double pending_resolve_us_ = 0.0; // note - judgment offset
    double deadline_us_ = 0.0;        // end + one unit (inclusive update bound)
    double recorded_delta_us_ = 0.0;
    double unit_us_ = 0.0;
    std::uint32_t sound_id_ = 0;
    std::uint32_t sound_serial_ = nonnormal_layout::kSoundSerialStart;
    std::uint64_t generation_ = 0;
    std::uint64_t last_begin_generation_ = 0;
    double advanced_us_ = 0.0;
    std::size_t penalties_ = 0;
    bool pending_ = false;
    bool done_ = false;
    bool configured_ = false;
    bool started_ = false;
};

// ===========================================================================
// Glissando
// ===========================================================================
struct NativeGlissSpec final {
    double start_us = 0.0; // chart+0 ms -> us; the only judged time
    std::uint8_t lane = 0;
    std::uint32_t chart_index = 0;
    bool auto_play = false;
    // note+272. 1 = wildcard for the event-group match; 0 = "no group" for the
    // connector scan. The writer 0x1A7450 derives 1/2/3 from the chart keys and
    // their midpoint; that derivation is decompiler-only, so the caller supplies
    // the value (the 0x1A7450 default is 1).
    std::uint32_t group_id = nonnormal_layout::kWildcardGroupId;
    std::int32_t left_key = 0;  // chart+88, for the connector scan span
    std::int32_t right_key = 0; // chart+92
    std::uint8_t connector_count = 0; // chart+64 / chart+72 non-null: 0..2
};

class NativeGlissState final {
public:
    NativeGlissState() = default;

    [[nodiscard]] NoteTypeStatus Configure(const NativeGlissSpec& spec,
                                           const NativeTypeConfig& config) noexcept;
    [[nodiscard]] NoteTypeStatus Begin(std::uint64_t generation, double position_us) noexcept;

    // `entries` is the caller's view of the held-input table (0x1813DEA20). It
    // is only read when config.connector_scan_available is set, and only for the
    // post-note connector scan; otherwise Advance reports the refused branch.
    [[nodiscard]] NoteTypeResult Press(const NativeNoteEvent& event, double decision_us) noexcept;
    [[nodiscard]] NoteTypeResult Release(const NativeNoteEvent& event, double decision_us) noexcept;
    [[nodiscard]] NoteTypeResult Advance(
        double up_to_us, double decision_us,
        std::span<const ActiveInputEntry> entries = {}) noexcept;

    [[nodiscard]] GlissPhase Phase() const noexcept { return phase_; }
    [[nodiscard]] NormalRank FinalRank() const noexcept { return final_rank_; }
    [[nodiscard]] bool Done() const noexcept { return done_; }
    [[nodiscard]] bool PendingEarly() const noexcept { return pending_; }
    [[nodiscard]] std::uint32_t ActiveSound() const noexcept { return sound_id_; }
    [[nodiscard]] std::uint32_t GroupId() const noexcept { return spec_.group_id; }
    [[nodiscard]] std::uint64_t Generation() const noexcept { return generation_; }

private:
    [[nodiscard]] bool Ready(NoteTypeResult& result) const noexcept;
    [[nodiscard]] NoteTypeStatus CheckEvent(const NativeNoteEvent& event) const noexcept;
    [[nodiscard]] double DeltaUs(const NativeNoteEvent& event) const noexcept;
    [[nodiscard]] NoteTypeResult Judge(double captured_us, double delta_us, NormalRank rank,
                                       std::uint32_t event_id, NoteTypeOutcomeKind kind) noexcept;
    [[nodiscard]] NoteTypeResult HandleJudgeEvent(const NativeNoteEvent& event, double captured_us,
                                                  double delta_us, NormalRank rank) noexcept;
    [[nodiscard]] NoteTypeResult Expire(double up_to_us, double decision_us) noexcept;
    void EmitConnectors(double captured_us, NoteTypeResult& result) noexcept;

    NativeGlissSpec spec_{};
    NativeTypeConfig config_{};
    GlissPhase phase_ = GlissPhase::Idle;
    NormalRank pending_rank_ = NormalRank::Just;
    NormalRank final_rank_ = NormalRank::TooEarly;
    double resolve_us_ = 0.0;
    double unit_us_ = 0.0;
    std::uint32_t sound_id_ = 0;
    std::uint64_t generation_ = 0;
    std::uint64_t last_begin_generation_ = 0;
    double advanced_us_ = 0.0;
    bool pending_ = false;
    bool done_ = false;
    bool scan_reported_ = false;
    bool configured_ = false;
    bool started_ = false;
};

// ===========================================================================
// Trill
// ===========================================================================
// One 24-byte chart sub-element (chart+40 array, count chart+36) as 0x1A7910
// reads it: +0 byte field, +4 byte field, +8 start frame, +12 end frame, +16
// qword sample/cue reference. The array drives the trill tail sound.
struct TrillSubElement final {
    std::int32_t start_frame = 0;    // elem+8
    std::int32_t end_frame = 0;      // elem+12
    std::uint8_t field0 = 0;         // elem+0
    std::uint8_t field4 = 0;         // elem+4
    std::uint64_t cue_reference = 0; // elem+16
};

struct NativeTrillSpec final {
    double start_us = 0.0; // chart+0 ms -> us
    double end_us = 0.0;   // chart+4 ms -> us
    std::uint8_t lane = 0;
    std::uint32_t chart_index = 0;
    std::uint16_t chart_flags = 0; // chart+12; bit 3 disables the event-id drive
    bool auto_play = false;
    std::array<TrillSubElement, nonnormal_layout::kTrillMaxSubElements> sub_elements{};
    std::uint8_t sub_element_count = 0; // chart+36, clamped to the array bound
};

class NativeTrillState final {
public:
    NativeTrillState() = default;

    [[nodiscard]] NoteTypeStatus Configure(const NativeTrillSpec& spec,
                                           const NativeTypeConfig& config) noexcept;
    [[nodiscard]] NoteTypeStatus Begin(std::uint64_t generation, double position_us) noexcept;

    [[nodiscard]] NoteTypeResult Press(const NativeNoteEvent& event, double decision_us) noexcept;
    [[nodiscard]] NoteTypeResult Release(const NativeNoteEvent& event, double decision_us) noexcept;
    [[nodiscard]] NoteTypeResult Advance(double up_to_us, double decision_us,
                                         bool hold_commit_missed = false) noexcept;

    // note+344 is a frame countdown, not a hit counter: 99999 at the head
    // commit, re-armed to 25 by every tail press, decremented by the periodic
    // tick. The caller owns that tick, so it publishes the value here. A
    // non-positive countdown makes the tail finish as Good(2) with scale 40.
    void SetTailCountdown(std::int32_t countdown) noexcept { countdown_ = countdown; }

    [[nodiscard]] TrillPhase Phase() const noexcept { return phase_; }
    [[nodiscard]] NormalRank FinalRank() const noexcept { return final_rank_; }
    [[nodiscard]] bool Done() const noexcept { return done_; }
    [[nodiscard]] bool PendingEarly() const noexcept { return pending_; }
    [[nodiscard]] std::uint32_t ActiveSound() const noexcept { return sound_id_; }
    [[nodiscard]] std::int32_t TailCountdown() const noexcept { return countdown_; }
    [[nodiscard]] std::uint32_t SubElementCursor() const noexcept { return sub_cursor_; }
    [[nodiscard]] double RecordedDeltaUs() const noexcept { return recorded_delta_us_; }
    [[nodiscard]] std::uint64_t Generation() const noexcept { return generation_; }

private:
    [[nodiscard]] bool Ready(NoteTypeResult& result) const noexcept;
    [[nodiscard]] NoteTypeStatus CheckEvent(const NativeNoteEvent& event) const noexcept;
    [[nodiscard]] double DeltaUs(const NativeNoteEvent& event) const noexcept;
    [[nodiscard]] double RemainingUs(double captured_us) const noexcept;
    [[nodiscard]] double UnitUs() const noexcept { return unit_us_; }
    void CommitStart(NormalRank rank, bool have_event, double captured_us, double delta_us,
                     NoteTypeResult& result) noexcept;
    void Finish(NormalRank rank, std::uint16_t scale, NoteTypeOutcomeKind kind,
                double captured_us, double delta_us, NoteTypeResult& result) noexcept;
    [[nodiscard]] NoteTypeResult TailStep(bool is_release, bool commit_missed,
                                          double captured_us, double delta_us) noexcept;
    [[nodiscard]] NoteTypeResult TailHit(const NativeNoteEvent& event, double captured_us) noexcept;
    [[nodiscard]] NoteTypeResult Expire(double up_to_us, double decision_us) noexcept;

    NativeTrillSpec spec_{};
    NativeTypeConfig config_{};
    TrillPhase phase_ = TrillPhase::Idle;
    NormalRank head_rank_ = NormalRank::TooEarly;
    NormalRank pending_rank_ = NormalRank::TooEarly;
    NormalRank final_rank_ = NormalRank::TooEarly;
    double pending_resolve_us_ = 0.0;
    double recorded_delta_us_ = 0.0;
    double unit_us_ = 0.0;
    std::uint32_t sound_id_ = 0;
    std::uint32_t sound_serial_ = nonnormal_layout::kSoundSerialStart;
    std::int32_t countdown_ = nonnormal_layout::kTrillCounterInitial;
    std::uint32_t sub_cursor_ = 0; // note+348
    std::uint64_t generation_ = 0;
    std::uint64_t last_begin_generation_ = 0;
    double advanced_us_ = 0.0;
    bool pending_ = false;
    bool done_ = false;
    bool configured_ = false;
    bool started_ = false;
};

} // namespace nosfix
