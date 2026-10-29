#include "nosfix/chart_event_cursor.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace nosfix {

ChartTableStatus ChartEventCursor::Configure(std::span<const ChartEvent> events) {
    // Validate before touching any member so a rejected table is a no-op.
    for (const ChartEvent& event : events) {
        if (!std::isfinite(event.song_us)) {
            return ChartTableStatus::non_finite_time;
        }
    }

    std::vector<ChartEvent> table(events.begin(), events.end());
    std::stable_sort(table.begin(), table.end(),
                     [](const ChartEvent& lhs, const ChartEvent& rhs) noexcept {
                         return lhs.song_us < rhs.song_us;
                     });

    std::vector<std::uint64_t> ids;
    ids.reserve(table.size());
    for (const ChartEvent& event : table) {
        ids.push_back(event.id);
    }
    std::sort(ids.begin(), ids.end());
    if (std::adjacent_find(ids.begin(), ids.end()) != ids.end()) {
        return ChartTableStatus::duplicate_id;
    }

    // Commit point: vector move assignment does not throw with std::allocator.
    // last_reset_generation_ is deliberately untouched: the lifetime floor
    // must outlive chart reloads, otherwise a stale epoch becomes usable again.
    events_ = std::move(table);
    configured_ = true;
    started_ = false;
    active_generation_ = 0;
    cursor_ = 0;
    last_observed_us_ = 0.0;
    has_watermark_ = false;
    watermark_us_ = 0.0;
    return ChartTableStatus::accepted;
}

std::size_t ChartEventCursor::LowerBound(double song_us) const noexcept {
    std::size_t low = 0;
    std::size_t high = events_.size();
    while (low < high) {
        const std::size_t mid = low + (high - low) / 2;
        if (events_[mid].song_us < song_us) {
            low = mid + 1;
        } else {
            high = mid;
        }
    }
    return low;
}

ChartCursorStatus ChartEventCursor::Reset(std::uint64_t playback_generation,
                                          double start_song_us) noexcept {
    if (!configured_) return ChartCursorStatus::not_configured;
    if (playback_generation == 0) return ChartCursorStatus::generation_zero;
    if (!std::isfinite(start_song_us)) return ChartCursorStatus::non_finite_time;
    // The floor is the last successful Reset() generation, not the active one.
    // It stays valid while unstarted, across Configure(), and after rejection.
    if (last_reset_generation_ != 0 &&
        playback_generation <= last_reset_generation_) {
        return ChartCursorStatus::generation_not_new;
    }

    active_generation_ = playback_generation;
    last_reset_generation_ = playback_generation;
    cursor_ = LowerBound(start_song_us);
    last_observed_us_ = start_song_us;
    has_watermark_ = false;
    watermark_us_ = 0.0;
    started_ = true;
    return ChartCursorStatus::ok;
}

ChartEventDrain ChartEventCursor::Drain(SongTime now,
                                        std::span<ChartEvent> destination) noexcept {
    ChartEventDrain result;

    if (!configured_) {
        result.status = ChartCursorStatus::not_configured;
        return result;
    }
    if (!started_) {
        result.status = ChartCursorStatus::not_started;
        return result;
    }
    if (!std::isfinite(now.microseconds)) {
        result.status = ChartCursorStatus::non_finite_time;
        return result;
    }
    if (now.generation != active_generation_) {
        result.status = ChartCursorStatus::stale_generation;
        return result;
    }
    if (destination.empty()) {
        // A zero-capacity consumer cannot accept delivery. Report a complete
        // no-op rather than claiming progress that was never made.
        result.status = ChartCursorStatus::empty_destination;
        return result;
    }
    if (now.microseconds < last_observed_us_) {
        // A seek or restart must be signalled through Reset() with a new
        // generation; Drain() never rewinds on its own.
        result.status = ChartCursorStatus::backward_time;
        return result;
    }

    last_observed_us_ = now.microseconds;

    const std::size_t count = events_.size();
    std::size_t written = 0;
    while (written < destination.size() && cursor_ < count &&
           events_[cursor_].song_us <= now.microseconds) {
        destination[written] = events_[cursor_];
        ++cursor_;
        ++written;
    }

    const bool complete =
        cursor_ >= count || events_[cursor_].song_us > now.microseconds;
    if (complete && (!has_watermark_ || now.microseconds > watermark_us_)) {
        watermark_us_ = now.microseconds;
        has_watermark_ = true;
    }

    result.written = written;
    result.drained_through_now = complete;
    result.status = ChartCursorStatus::ok;
    return result;
}

} // namespace nosfix
