#include "nosfix/song_clock_publication.hpp"

#include <cstring>

namespace nosfix {

SongClockPublication::SongClockPublication() noexcept {
    for (std::size_t i = 0; i < kWordCount; ++i) {
        words_[i].store(0, std::memory_order_relaxed);
    }
}

void SongClockPublication::Publish(const SongClockSegment& segment,
                                   SongClockContinuity continuity) noexcept {
    // Zero first so the object's padding bytes are deterministic. They are
    // copied opaquely and never compared, but a stable representation keeps the
    // publication free of indeterminate bytes.
    SongClockSnapshot snapshot;
    std::memset(&snapshot, 0, sizeof(snapshot));
    snapshot.segment = segment;
    snapshot.version = next_version_++;
    snapshot.continuity = continuity;

    std::uint64_t payload[kWordCount] = {};
    std::memcpy(payload, &snapshot, kBytes);

    const std::uint64_t sequence = sequence_.load(std::memory_order_relaxed);
    sequence_.store(sequence + 1, std::memory_order_relaxed);
    std::atomic_thread_fence(std::memory_order_release);
    for (std::size_t i = 0; i < kWordCount; ++i) {
        words_[i].store(payload[i], std::memory_order_relaxed);
    }
    sequence_.store(sequence + 2, std::memory_order_release);
}

bool SongClockPublication::TryRead(SongClockSnapshot& snapshot) const noexcept {
    for (unsigned attempt = 0; attempt < kReadAttempts; ++attempt) {
        const std::uint64_t before = sequence_.load(std::memory_order_acquire);
        if (before == 0) return false;
        if ((before & 1U) != 0) continue;

        std::atomic_thread_fence(std::memory_order_acquire);
        std::uint64_t payload[kWordCount] = {};
        for (std::size_t i = 0; i < kWordCount; ++i) {
            payload[i] = words_[i].load(std::memory_order_relaxed);
        }
        std::atomic_thread_fence(std::memory_order_acquire);
        const std::uint64_t after = sequence_.load(std::memory_order_acquire);
        if (before == after) {
            std::memcpy(&snapshot, payload, kBytes);
            return true;
        }
    }
    return false;
}

bool SongClockPublication::HasPublished() const noexcept {
    return sequence_.load(std::memory_order_acquire) != 0;
}

} // namespace nosfix
