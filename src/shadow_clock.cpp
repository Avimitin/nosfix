#include "nosfix/shadow_clock.hpp"

#include <cmath>

namespace nosfix {

void ShadowClock::Configure(bool enabled) noexcept {
    if (enabled_.load(std::memory_order_acquire)) {
        if (!enabled) {
            (void)clock_.Stop();
            enabled_.store(false, std::memory_order_release);
        }
        return;
    }
    if (enabled) {
        enabled_.store(true, std::memory_order_release);
        // Invalidate any snapshot left from a previous session before the new
        // first observation. The segment is not running, so Map() returns null
        // and the input records are counted as unmapped, never stale-mapped.
        publication_.Publish(SongClockSegment{},
                             SongClockContinuity::PossiblyRegressed);
        has_published_ = false;
    }
}

void ShadowClock::Stop() noexcept {
    if (enabled_.load(std::memory_order_acquire)) {
        (void)clock_.Stop();
    }
    enabled_.store(false, std::memory_order_release);
    has_published_ = false;
}

ShadowClockStatus ShadowClock::Observe(double song_microseconds,
                                        std::int64_t qpc_ticks,
                                        std::int64_t qpc_frequency) noexcept {
    if (!enabled_.load(std::memory_order_acquire)) {
        return ShadowClockStatus::Disabled;
    }
    if (qpc_ticks <= 0 || qpc_frequency <= 0 ||
        !std::isfinite(song_microseconds)) {
        return ShadowClockStatus::Rejected;
    }

    SongClockContinuity continuity = SongClockContinuity::Monotonic;
    ShadowClockStatus status = ShadowClockStatus::Observed;

    if (!clock_.IsRunning() || clock_.Frequency() != qpc_frequency) {
        // Start advances one generation by itself. Only fall back to a full
        // Stop/Start when the caller moved QPC behind the old anchor.
        if (!clock_.Start(qpc_ticks, qpc_frequency, song_microseconds)) {
            (void)clock_.Stop();
            if (!clock_.Start(qpc_ticks, qpc_frequency, song_microseconds)) {
                return ShadowClockStatus::Rejected;
            }
        }
        status = ShadowClockStatus::Started;
    } else {
        const auto predicted = clock_.Snapshot().Map(qpc_ticks);
        if (!predicted) {
            if (!clock_.Seek(qpc_ticks, song_microseconds)) {
                return ShadowClockStatus::Rejected;
            }
            status = ShadowClockStatus::Seeked;
        } else {
            const double correction = song_microseconds - predicted->microseconds;
            if (!std::isfinite(correction) ||
                correction < -kMaxCorrectionUs || correction > kMaxCorrectionUs) {
                if (!clock_.Seek(qpc_ticks, song_microseconds)) {
                    return ShadowClockStatus::Rejected;
                }
                status = ShadowClockStatus::Seeked;
            } else {
                if (!clock_.Observe(qpc_ticks, qpc_frequency, song_microseconds,
                                    kMaxCorrectionUs)) {
                    if (!clock_.Seek(qpc_ticks, song_microseconds)) {
                        return ShadowClockStatus::Rejected;
                    }
                    status = ShadowClockStatus::Seeked;
                } else if (correction < 0.0) {
                    continuity = SongClockContinuity::PossiblyRegressed;
                }
            }
        }
    }

    publication_.Publish(clock_.Snapshot(), continuity);
    has_published_ = true;
    return status;
}

} // namespace nosfix
