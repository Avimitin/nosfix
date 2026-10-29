#pragma once

#include "nosfix/song_clock.hpp"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace nosfix {

// Writer's claim about one snapshot. Monotonic means "this segment does not map
// any QPC to an earlier song time than the previous segment did for that same
// QPC". SongClock::Observe can apply a negative correction, which violates this
// claim; the clock owner must either bound corrections to non-negative values or
// publish the snapshot as PossiblyRegressed. The input pipeline treats a
// PossiblyRegressed snapshot, or any observed backwards mapping, as a broken
// song-time watermark for the rest of the playback generation.
enum class SongClockContinuity : std::uint8_t {
    Monotonic,
    PossiblyRegressed,
};

struct SongClockSnapshot final {
    SongClockSegment segment{};
    std::uint64_t version = 0; // strictly increasing for every completed Publish
    SongClockContinuity continuity = SongClockContinuity::Monotonic;
};

// ===========================================================================
// Single-writer / single-reader publication of one immutable clock snapshot
// ===========================================================================
//
// The audio/clock owner publishes a complete snapshot; the raw-input capture
// thread copies the newest snapshot *before* it timestamps a packet, and the
// packet retains that snapshot for later mapping. A queued old QPC is therefore
// never mapped with a clock segment published after the capture.
//
// The payload is a trivially copyable SongClockSnapshot. It is copied into
// fixed-width atomic words and a sequence counter brackets the stores. Every
// cell read or written is a std::atomic object, so a reader that races with
// Publish() is not a data race on a non-atomic object; the sequence check only
// decides whether the observed word set is one complete snapshot. The object's
// padding bytes are copied opaquely and are never compared: identity is the
// sequence and version, never a byte-wise equality of the payload.
//
// Required of the caller:
//   * Exactly one writer thread calls Publish(). Two concurrent writers are a
//     contract violation.
//   * Publish() must not be called from the installed input callback. It does
//     not allocate, lock, log or call the OS.
//   * The clock owner and the packet QPC producer should share a thread, or use
//     a serialization protocol around the whole snapshot+QPC acquisition. A
//     snapshot taken before QPC on another thread can straddle a publication;
//     the pipeline detects the resulting backwards mapping and fails the
//     song-time watermark closed, but it cannot reconstruct the right pair.
//
// TryRead() is bounded: it retries at most kReadAttempts times so the input
// callback cannot spin behind a pathological writer.
class SongClockPublication final {
public:
    SongClockPublication() noexcept;
    ~SongClockPublication() = default;
    SongClockPublication(const SongClockPublication&) = delete;
    SongClockPublication& operator=(const SongClockPublication&) = delete;

    // Writer: publishes a complete snapshot. Stores are release-ordered so a
    // reader that observes the new sequence also observes every payload word.
    void Publish(const SongClockSegment& segment,
                 SongClockContinuity continuity) noexcept;

    // Reader: copies the newest complete snapshot. Returns false when nothing
    // has been published yet or no consistent copy could be read within
    // kReadAttempts. On false, `snapshot` is left untouched.
    [[nodiscard]] bool TryRead(SongClockSnapshot& snapshot) const noexcept;

    [[nodiscard]] bool HasPublished() const noexcept;

    static constexpr unsigned kReadAttempts = 64;

private:
    static_assert(std::is_trivially_copyable_v<SongClockSnapshot>);
    static constexpr std::size_t kBytes = sizeof(SongClockSnapshot);
    static constexpr std::size_t kWordCount = (kBytes + sizeof(std::uint64_t) - 1) /
                                              sizeof(std::uint64_t);
    static_assert(std::atomic<std::uint64_t>::is_always_lock_free);

    alignas(64) std::atomic<std::uint64_t> sequence_{0};
    alignas(64) std::atomic<std::uint64_t> words_[kWordCount];
    // Writer-only.
    std::uint64_t next_version_ = 1;
};

} // namespace nosfix
