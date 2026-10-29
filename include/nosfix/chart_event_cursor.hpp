#pragma once

// Time-based chart event cursor for frame-cadence-independent dispatch.
//
// The cursor owns a validated copy of a chart event table and tracks how much
// of it has been delivered. Configure() is the only allocating call; Reset(),
// Drain() and the queries are noexcept and allocation-free. The cursor never
// reads a clock and holds no reference to the caller's table.
//
// Delivery model: Reset() opens a numbered playback segment at a start time;
// Drain() emits a capacity-limited, ordered prefix of that segment's
// undelivered events up to the caller's `now`. Every event is emitted at most
// once and an undelivered event is never passed over. A rejected call changes
// neither the cursor state nor the destination.
//
// Time base: double microseconds, matching SongTime. Only finiteness is
// required, so negative times (preroll) and large times are valid.
//
// Threading: single consumer; no synchronization is provided.

#include "nosfix/song_clock.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace nosfix {

// One chart event. The cursor copies these values when configured and never
// interprets `kind` or `payload`.
struct ChartEvent final {
    std::uint64_t id = 0;      // Unique within one configured table.
    double song_us = 0.0;      // Finite song time in microseconds.
    std::uint32_t kind = 0;    // Opaque to the cursor.
    std::uint32_t payload = 0; // Opaque payload/index, copied unchanged.
};

// Result of Configure(). Any value other than `accepted` leaves the previous
// configuration and playback state untouched.
enum class ChartTableStatus : std::uint8_t {
    accepted = 0,
    duplicate_id,    // Two or more events share an id.
    non_finite_time, // At least one song_us is NaN or infinite.
};

// Result of Reset() and Drain(). Rejections are complete no-ops.
enum class ChartCursorStatus : std::uint8_t {
    ok = 0,
    not_configured,     // No table has ever been accepted.
    not_started,        // A table is configured but Reset() has not succeeded.
    generation_zero,    // Reset() with generation 0 (0 is never a valid epoch).
    generation_not_new, // Reset() generation is not above the lifetime floor.
    non_finite_time,    // The supplied start/now time is NaN or infinite.
    stale_generation,   // Drain() generation differs from the Reset generation.
    empty_destination,  // Drain() destination had zero capacity; complete no-op.
    backward_time,      // Drain() now precedes the last accepted observation.
};

// Outcome of one Drain() call. `written` counts events copied and committed as
// delivered. `drained_through_now` means no undelivered event has a song time
// at or before the requested `now`; later events may still remain. On a
// non-ok status, written is 0, drained_through_now is false, and neither the
// cursor nor the destination changed.
struct ChartEventDrain final {
    std::size_t written = 0;
    bool drained_through_now = false;
    ChartCursorStatus status = ChartCursorStatus::ok;

    [[nodiscard]] bool ok() const noexcept {
        return status == ChartCursorStatus::ok;
    }
};

class ChartEventCursor final {
public:
    ChartEventCursor() = default;

    // Delivery state cannot be duplicated or transferred without risking
    // double delivery or an inconsistent cursor. Reuse one instance and
    // Configure() a new table instead.
    ChartEventCursor(const ChartEventCursor&) = delete;
    ChartEventCursor& operator=(const ChartEventCursor&) = delete;
    ChartEventCursor(ChartEventCursor&&) = delete;
    ChartEventCursor& operator=(ChartEventCursor&&) = delete;

    // Copies, validates and stable-sorts `events` by song_us, replacing the
    // configured table. Input need not be sorted; equal times keep their source
    // order, which is the delivery order. Duplicate ids or non-finite times are
    // rejected without changing anything. May allocate and throw
    // std::bad_alloc with the same no-change guarantee.
    //
    // Success invalidates playback: the cursor becomes unstarted and the
    // watermark and observed time are cleared. The generation floor is kept, so
    // a stale Reset() or Drain() from an earlier segment cannot become valid
    // again.
    [[nodiscard]] ChartTableStatus Configure(std::span<const ChartEvent> events);

    // Starts a playback segment or seeks the active one. Requires a configured
    // table, a finite start, and -- unless this is the first successful Reset()
    // -- a generation above LastResetGeneration(); equal or older generations
    // are generation_not_new even while unstarted, and generation 0 is always
    // generation_zero. On success the cursor sits at the first event with
    // song_us >= start_song_us (an event exactly at the start is included);
    // earlier events are skipped by this explicit policy only. The watermark is
    // cleared and start_song_us becomes the backward-time baseline. A rejected
    // call changes nothing, including the floor.
    //
    // The floor survives Configure() and rejection. At UINT64_MAX it is
    // exhausted: no further Reset() can succeed, and a new cursor instance is
    // the only way to start another segment.
    [[nodiscard]] ChartCursorStatus Reset(std::uint64_t playback_generation,
                                          double start_song_us) noexcept;

    // Emits at most destination.size() pending events with song_us <= now, in
    // delivery order. Delivery is prefix-only, so a capacity-limited call can
    // resume at the same `now`; a later `now` still delivers the older backlog
    // first.
    //
    // Rejection precedence, all complete no-ops: not_configured, not_started,
    // non_finite_time, stale_generation, empty_destination, backward_time.
    // now.generation must equal the generation accepted by Reset(). An empty
    // destination consumes nothing and reports drained_through_now = false.
    //
    // On success now.microseconds becomes LastObserved() even when a backlog
    // remains; the watermark advances only when nothing at or before `now`
    // stays undelivered.
    [[nodiscard]] ChartEventDrain Drain(SongTime now,
                                        std::span<ChartEvent> destination) noexcept;

    [[nodiscard]] bool IsConfigured() const noexcept { return configured_; }
    [[nodiscard]] bool IsStarted() const noexcept { return started_; }

    // Number of events in the configured table (0 before configuration).
    [[nodiscard]] std::size_t EventCount() const noexcept { return events_.size(); }

    // Undelivered events in the active segment; 0 when unstarted.
    [[nodiscard]] std::size_t PendingCount() const noexcept {
        return started_ ? events_.size() - cursor_ : 0;
    }

    // Generation of the active playback segment; 0 when unstarted. Compare
    // LastResetGeneration() for the lifetime floor.
    [[nodiscard]] std::uint64_t ActiveGeneration() const noexcept {
        return started_ ? active_generation_ : 0;
    }

    // Generation of the last successful Reset(), retained across Configure();
    // 0 before the first successful Reset. This is the floor Reset() must
    // exceed.
    [[nodiscard]] std::uint64_t LastResetGeneration() const noexcept {
        return last_reset_generation_;
    }

    // First undelivered event, or nullptr when unstarted or fully drained.
    // Stable until the next successful Configure().
    [[nodiscard]] const ChartEvent* NextEvent() const noexcept {
        if (!started_ || cursor_ >= events_.size()) return nullptr;
        return &events_[cursor_];
    }

    // Watermark of the active segment: a time T through which every event in
    // the segment (events before the Reset start are outside it) has been
    // delivered. Absent until a Drain() completes through its `now`; cleared by
    // Reset() and Configure(). Never advanced by a partial or empty drain, and
    // always below the song time of the first retained event, if any. This is
    // delivery state, not the latest observed `now`.
    [[nodiscard]] std::optional<SongTime> CompletedThrough() const noexcept {
        if (!started_ || !has_watermark_) return std::nullopt;
        return SongTime{watermark_us_, active_generation_};
    }

    // Latest accepted Drain() time, for diagnostics. May exceed
    // CompletedThrough() while a backlog is retained; it is not delivery
    // evidence.
    [[nodiscard]] std::optional<SongTime> LastObserved() const noexcept {
        if (!started_) return std::nullopt;
        return SongTime{last_observed_us_, active_generation_};
    }

    // The configured table in delivery order. Invalidated by Configure().
    [[nodiscard]] std::span<const ChartEvent> Events() const noexcept {
        return events_;
    }

private:
    [[nodiscard]] std::size_t LowerBound(double song_us) const noexcept;

    std::vector<ChartEvent> events_{};
    std::size_t cursor_ = 0;
    std::uint64_t active_generation_ = 0;
    std::uint64_t last_reset_generation_ = 0;
    double last_observed_us_ = 0.0;
    double watermark_us_ = 0.0;
    bool configured_ = false;
    bool started_ = false;
    bool has_watermark_ = false;
};

} // namespace nosfix
