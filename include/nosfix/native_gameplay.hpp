#pragma once

// Native-gameplay timing adapter (NORMAL notes only, disabled by default).
//
// This is the integration shell around three reviewed/evidenced pieces:
//   * GameplayClock: explicit bounded audio observations, no render ticks;
//   * NativeNoteState: normal selection/rank/pending/auto/miss lifecycle;
//   * ChartEventCursor: bounded, once-only chart event delivery in song time.
// It adds the two things those kernels deliberately do not own:
//   1. an explicit input bridge: SubmitInput(TimedInputEvent) plus
//      SetInputWatermark(processed-through). No raw queue is read here.
//   2. a bounded scheduled-sound crossing table that mirrors the native
//      frame->sound dispatch in 0x2FDB30 (entry fired once, release once).
//
// Ordering contract for one Advance(now):
//   - only input captured at or before the certified watermark is applied;
//   - queued input is applied in capture order;
//   - before each press, note crossings up to its capture time are resolved;
//     misses strictly before the capture time are finalized first, so an
//     in-window press is never lost to a later frame;
//   - after input, note crossings/misses up to `now` are resolved;
//   - chart events and scheduled sounds are delivered as bounded prefixes.
// Misses use the certified watermark as the decision time, so a render stall or
// a slower consumer cannot miss a note whose input is still in flight.
//
// NOT included: the native 0x2F7C10 matching loop (selection is an explicit
// adapter policy), long/gliss/trill notes, HID decoding, the note+96 auto-slot
// branch, native score points/effects, live hook installation, and any write to
// the shared frame counter 0x70A610. See REQUESTS.txt and INTEGRATION.txt.

#include "nosfix/chart_event_cursor.hpp"
#include "nosfix/native_note_state.hpp"
#include "nosfix/timing_events.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace nosfix {

// One entry of the native 0x2FDB30 keysound/event table (24-byte stride, 128
// entries). The native entry has a trigger frame (+8), a release frame (+12),
// a started-sound id byte (+5), a velocity byte (+6), and a release target id
// (+0) used to find another entry to fade. This adapter keeps the same
// start-once / release-once semantics with explicit song times; the host maps
// sound ids to libxact labels/handles.
struct ScheduledSound final {
    bool has_start = true;
    double start_us = 0.0;
    std::uint32_t sound_id = 0;
    float velocity = 0.0f;
    bool has_release = false;
    double release_us = 0.0;
    std::uint32_t release_target_id = 0;
};

enum class SoundAction : std::uint8_t { Start = 0, Cut = 1 };

struct SoundCommand final {
    SoundAction action = SoundAction::Start;
    std::uint32_t sound_id = 0; // Start: this entry's id; Cut: release target
    std::uint32_t schedule_index = 0;
    float velocity = 0.0f;
    double at_us = 0.0; // scheduled crossing time, not the observing render
};

struct GameplayOutputs final {
    // Size this to at least MaxOutcomesPerAdvance() before Advance(). The bound
    // covers every note completion/miss plus one keysound cut per queued input;
    // a smaller span is rejected as a complete no-op, never partially written.
    std::span<NativeNoteOutcome> note_outcomes;
    std::span<ChartEvent> chart_events;
    std::span<SoundCommand> sounds;
};

enum class GameplayStatus : std::uint8_t {
    Ok = 0,
    NotConfigured,
    NotStarted,
    InvalidConfig,
    GenerationZero,
    GenerationNotNew,
    StaleGeneration,
    NonFiniteTime,
    BackwardTime,
    WatermarkAhead,       // certified input is ahead of current song time
    WatermarkRegressed,
    InputQueueFull,
    InputNotChronological,
    DuplicateInput,
    InvalidInput,
    InsufficientCapacity, // note outcome span smaller than the note count
};

[[nodiscard]] const char* GameplayStatusName(GameplayStatus status) noexcept;

struct GameplayAdvance final {
    GameplayStatus status = GameplayStatus::NotStarted;
    NoteAdvance note_counts{};
    std::size_t chart_written = 0;
    std::size_t sounds_written = 0;
    std::size_t inputs_applied = 0;
    std::size_t inputs_dropped = 0;
    bool chart_backlog = false;
    bool sound_backlog = false;

    [[nodiscard]] bool ok() const noexcept { return status == GameplayStatus::Ok; }
};

inline constexpr std::uint32_t kDefaultInputQueueCapacity = 1024;
inline constexpr std::uint32_t kMaximumInputQueueCapacity = 65'536;

// Diagnostic stage failure for Configure(). Production callers use None. It
// exists so tests can prove that a failure after any allocation/commit stage
// leaves the instance explicitly unconfigured, never partially running.
enum class ConfigureFailpoint : std::uint8_t {
    None = 0,
    BeforeNotes,
    AfterNotes,
    AfterChart,
    AfterSounds,
    AfterQueue,
};

class NativeGameplay final {
public:
    NativeGameplay() = default;

    // Copies and validates notes, chart events and scheduled sounds. Chart
    // events are stored with the effective native time
    //   record_us - judgment_offset - visual_offset
    // which is the 0x2FF090 dispatch condition. Their ids must be unique and
    // effective times finite.
    //
    // The instance is explicitly invalidated before any mutation: a rejected or
    // failing reconfigure never leaves a partially configured/running object.
    // Generation floors in the note/chart cursors and last_begin_generation_
    // are retained. `failpoint` is a diagnostics hook for stage-failure tests;
    // production callers pass None.
    [[nodiscard]] GameplayStatus Configure(std::span<const NativeNoteSpec> notes,
                                           std::span<const ChartEvent> chart_events,
                                           std::span<const ScheduledSound> sounds,
                                           const NativeNoteConfig& note_config,
                                           std::uint32_t input_capacity =
                                               kDefaultInputQueueCapacity,
                                           ConfigureFailpoint failpoint =
                                               ConfigureFailpoint::None);

    // Starts a playback generation. Past notes are skipped, not missed. Stale
    // queued input, watermark and chart/sound state are discarded. generation
    // must exceed every generation previously begun.
    [[nodiscard]] GameplayStatus Begin(std::uint64_t generation, double start_us);

    // Explicit input submission. Preserves capture order, rejects an older or
    // equal generation, an exact duplicate identity, non-monotonic capture time
    // and a full queue without consuming the record. The caller retries after
    // Advance() has drained capacity.
    [[nodiscard]] GameplayStatus SubmitInput(const TimedInputEvent& event) noexcept;

    // Certified same-generation processed-input-through watermark. Monotonic;
    // the producer must not certify a time whose input has not been submitted.
    [[nodiscard]] GameplayStatus SetInputWatermark(SongTime through) noexcept;

    // Applies queued input and advances note crossings, chart events and sounds
    // to `now`. Rejections are complete no-ops. Note outcome capacity must be
    // at least NoteCount() so every completion is observable; chart and sound
    // spans may be smaller and leave a resumable backlog.
    [[nodiscard]] GameplayAdvance Advance(SongTime now, const GameplayOutputs& outputs) noexcept;

    // ---- inspection ----
    [[nodiscard]] bool IsConfigured() const noexcept { return configured_; }
    [[nodiscard]] bool IsStarted() const noexcept { return started_; }
    [[nodiscard]] std::uint64_t Generation() const noexcept { return generation_; }
    [[nodiscard]] double Watermark() const noexcept { return watermark_us_; }
    [[nodiscard]] double LastNow() const noexcept { return last_now_us_; }
    [[nodiscard]] std::size_t QueueDepth() const noexcept { return queue_count_; }
    [[nodiscard]] std::size_t NoteCount() const noexcept { return notes_.NoteCount(); }
    // Safe outcome-span bound for one Advance: every note can complete/miss at
    // most once, and every queued input can emit at most one keysound cut.
    [[nodiscard]] std::size_t MaxOutcomesPerAdvance() const noexcept {
        return notes_.NoteCount() + queue_capacity_;
    }
    [[nodiscard]] std::size_t ChartPending() const noexcept { return chart_.PendingCount(); }
    [[nodiscard]] std::size_t SoundCount() const noexcept { return sounds_.size(); }

    [[nodiscard]] const NativeNoteState& Notes() const noexcept { return notes_; }
    [[nodiscard]] const ChartEventCursor& Chart() const noexcept { return chart_; }

private:
    // Starts and releases are flattened into one time-ordered crossing list so
    // a stalled Advance emits the same order as a fine cadence. Equal times are
    // ordered Start before Cut, then by schedule index.
    struct SoundCrossing final {
        double at_us = 0.0;
        std::uint32_t schedule_index = 0;
        SoundAction action = SoundAction::Start;
        std::uint32_t sound_id = 0;
        float velocity = 0.0f;
    };

    // A logical event identity is not just the capture stamp: one physical
    // control can map to several lanes, and a synthetic reset releases several
    // held lanes with the exact same stamp. Lane, edge, source and the synthetic
    // flag are part of identity. A 32-entry ring is enough for a full 28-lane
    // chord/reset and catches non-adjacent retries without allocating.
    struct InputIdentity final {
        std::uint64_t stream_epoch = 0;
        std::uint64_t sequence = 0;
        std::uint32_t ordinal = 0;
        std::uint64_t source_id = 0;
        std::uint64_t device_generation = 0;
        std::uint8_t lane = 0;
        std::uint8_t edge = 0;
        bool synthetic = false;
    };

    [[nodiscard]] bool SeenIdentity(const InputIdentity& identity) const noexcept;
    void RememberIdentity(const InputIdentity& identity) noexcept;

    [[nodiscard]] GameplayStatus ResetSounds(double start_us) noexcept;
    [[nodiscard]] GameplayStatus AdvanceSounds(double now_us,
                                               std::span<SoundCommand> out,
                                               std::size_t& written,
                                               bool& backlog) noexcept;

    NativeNoteState notes_{};
    ChartEventCursor chart_{};
    std::vector<ScheduledSound> sounds_{};
    std::vector<SoundCrossing> sound_crossings_{};
    std::size_t sound_cursor_ = 0;

    std::vector<TimedInputEvent> queue_{};
    std::size_t queue_head_ = 0;
    std::size_t queue_count_ = 0;
    std::size_t queue_capacity_ = 0;

    static constexpr std::size_t kRecentIdentityCount = 32;
    std::array<InputIdentity, kRecentIdentityCount> recent_{};
    std::size_t recent_head_ = 0;
    std::size_t recent_count_ = 0;
    bool has_last_capture_ = false;
    double last_capture_us_ = 0.0;

    double watermark_us_ = 0.0;
    double last_now_us_ = 0.0;
    std::uint64_t generation_ = 0;
    std::uint64_t last_begin_generation_ = 0;
    bool configured_ = false;
    bool started_ = false;
};

// ---------------------------------------------------------------------------
// Disabled live seam
// ---------------------------------------------------------------------------

enum class NativeSeamState : std::uint8_t {
    DisabledByDefault = 0,  // feature flag off
    WaitingForEvidence,     // flag on, but signatures/layout are unverified
    ReadyNotInstalled,      // all evidence supplied; installation is the parent's
};

struct NativeSeamReport final {
    NativeSeamState state = NativeSeamState::DisabledByDefault;
    const char* reason = "";
};

// Pure readiness decision. `feature_enabled` is the proposed nosfix.ini gate;
// `evidence_verified` may only be set by the integration after the parent
// verifies every required signature and layout from REQUESTS.txt.
[[nodiscard]] NativeSeamReport EvaluateNativeSeam(bool feature_enabled,
                                                  bool evidence_verified) noexcept;

// Exact targets the seam would need. The 32-byte prefixes are exported in
// native_layout::kTargetPrefixes, but readiness is still false because the
// matching/completion ABI is not decoded (kAbiAndMatchingVerified).
[[nodiscard]] std::span<const native_layout::Target> NativeSeamTargets() noexcept;

} // namespace nosfix
