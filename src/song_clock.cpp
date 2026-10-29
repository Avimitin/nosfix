#include "nosfix/song_clock.hpp"

#include <cmath>
#include <limits>

namespace nosfix {
namespace {

bool SafeNonnegativeDelta(std::int64_t later, std::int64_t earlier,
                          std::int64_t& delta) noexcept {
    if (later < earlier) return false;
    if (earlier < 0 && later > std::numeric_limits<std::int64_t>::max() + earlier) {
        return false;
    }
    delta = later - earlier;
    return true;
}

} // namespace

std::optional<SongTime> SongClockSegment::Map(std::int64_t qpc_ticks) const noexcept {
    if (!IsRunning() || qpc_frequency_ <= 0 ||
        !std::isfinite(rate_) || rate_ <= 0.0) {
        return std::nullopt;
    }

    std::int64_t delta_ticks = 0;
    if (!SafeNonnegativeDelta(qpc_ticks, qpc_anchor_, delta_ticks)) {
        return std::nullopt;
    }

    // Subtract the signed QPC values before converting to floating point.
    const double elapsed_seconds = static_cast<double>(delta_ticks) /
                                   static_cast<double>(qpc_frequency_);
    const double song_us = song_anchor_us_ + elapsed_seconds * 1'000'000.0 * rate_;
    if (!std::isfinite(song_us)) return std::nullopt;
    return SongTime{song_us, generation_};
}

bool SongClock::AdvanceGeneration() noexcept {
    if (generation_ == std::numeric_limits<std::uint64_t>::max()) {
        return false;
    }
    ++generation_;
    return true;
}

bool SongClock::Start(std::int64_t qpc_ticks, std::int64_t qpc_frequency,
                      double song_microseconds) noexcept {
    if (qpc_frequency <= 0 || !std::isfinite(song_microseconds)) return false;
    if (current_.IsValid() && qpc_ticks < current_.qpc_anchor_) return false;
    if (!AdvanceGeneration()) return false;

    current_ = {};
    current_.qpc_anchor_ = qpc_ticks;
    current_.qpc_frequency_ = qpc_frequency;
    current_.song_anchor_us_ = song_microseconds;
    current_.rate_ = 1.0;
    current_.generation_ = generation_;
    current_.valid_ = true;
    current_.running_ = true;
    return true;
}

bool SongClock::ChangeRate(std::int64_t qpc_ticks, double rate) noexcept {
    constexpr double kMaximumRate = 4.0;
    if (!current_.IsRunning() || !std::isfinite(rate) ||
        rate <= 0.0 || rate > kMaximumRate) {
        return false;
    }
    const auto position = current_.Map(qpc_ticks);
    if (!position) return false;
    // Continuous join: the mapped value at qpc_ticks is unchanged; only the
    // slope after this anchor changes. A later anchor is always forward, so
    // Map() cannot move backwards.
    current_.qpc_anchor_ = qpc_ticks;
    current_.song_anchor_us_ = position->microseconds;
    current_.rate_ = rate;
    return true;
}

bool SongClock::Observe(std::int64_t qpc_ticks, std::int64_t qpc_frequency,
                        double song_microseconds,
                        double max_correction_microseconds) noexcept {
    if (!current_.IsRunning() || qpc_frequency != current_.qpc_frequency_ ||
        !std::isfinite(song_microseconds) ||
        !std::isfinite(max_correction_microseconds) ||
        max_correction_microseconds < 0.0 || qpc_ticks < current_.qpc_anchor_) {
        return false;
    }

    if (qpc_ticks == current_.qpc_anchor_) {
        return song_microseconds == current_.song_anchor_us_;
    }

    const auto prediction = current_.Map(qpc_ticks);
    if (!prediction) return false;
    if (song_microseconds < current_.song_anchor_us_) return false;
    // Legacy Observe may accept a bounded sub-prediction correction at rate 1
    // (the reviewed kernel tests rely on this). On a rate-adjusted segment a
    // value below the prediction would move mapped time backwards, so require
    // the prediction there; use ChangeRate for ongoing synchronization.
    if (current_.rate_ != 1.0 &&
        song_microseconds < prediction->microseconds) {
        return false;
    }
    const double correction = song_microseconds - prediction->microseconds;
    if (!std::isfinite(correction) ||
        std::abs(correction) > max_correction_microseconds) {
        return false;
    }

    current_.qpc_anchor_ = qpc_ticks;
    current_.song_anchor_us_ = song_microseconds;
    return true;
}

bool SongClock::Seek(std::int64_t qpc_ticks, double song_microseconds) noexcept {
    if (!current_.IsValid() || current_.qpc_frequency_ <= 0 ||
        qpc_ticks < current_.qpc_anchor_) {
        return false;
    }
    return Start(qpc_ticks, current_.qpc_frequency_, song_microseconds);
}

bool SongClock::Pause(std::int64_t qpc_ticks) noexcept {
    const auto position = current_.Map(qpc_ticks);
    if (!position) return false;

    current_.qpc_anchor_ = qpc_ticks;
    current_.song_anchor_us_ = position->microseconds;
    current_.running_ = false;
    return true;
}

bool SongClock::Resume(std::int64_t qpc_ticks) noexcept {
    if (!current_.IsValid() || current_.running_ || qpc_ticks < current_.qpc_anchor_) {
        return false;
    }

    current_.qpc_anchor_ = qpc_ticks;
    current_.running_ = true;
    return true;
}

bool SongClock::Stop() noexcept {
    if (!AdvanceGeneration()) return false;
    current_ = {};
    return true;
}

} // namespace nosfix
