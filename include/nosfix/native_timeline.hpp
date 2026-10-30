#pragma once

// Microsecond music-timeline core.
//
// This is the replacement for the body of sub_1802FC9D0. The native function
// currently does:
//   counter += 1;
//   if (counter == tempo_event_frame) { ... }
//   beat = (counter - last_tempo_frame) / ((rate*rate)/BPM) + beat_base;
//
// At high refresh that increments by one per call, uses exact equality for
// tempo events, and needs the native rate globals. The replacement below writes
// an absolute microsecond position, drains every tempo event at or before that
// position, and computes the beat with the fixed 60,000,000 us/minute constant.
//
// The native object layout is intentionally addressed by byte offset; this
// module does not invent a C++ object or a compatibility frame view.
//
// One intentional difference from the frame model: sub_1802FC9D0 applies a
// tempo event one 60 Hz frame late (it copies the previous tick's beat into the
// event base). This replacement uses the exact event time. That is a musical
// correctness fix, not a clock assumption; it affects beat/effect phase at a
// tempo boundary, not the note comparison time.

#include <cstddef>
#include <cstdint>

namespace nosfix {

inline constexpr std::int32_t kTimelinePrerollUs = -3'333'333;
inline constexpr std::int32_t kTimelineUsPerMinute = 60'000'000;
inline constexpr std::int32_t kTimelineUsPerSecond = 1'000'000;

// Native music timeline offsets (sub_1802FC9D0 / sub_1802FC810).
inline constexpr std::size_t kTimelineCounter = 0x000;
inline constexpr std::size_t kTimelineBpm = 0x114;
inline constexpr std::size_t kTimelinePhase = 0x118;
inline constexpr std::size_t kTimelinePreviousPhase = 0x11C;
inline constexpr std::size_t kTimelineBeatFlag = 0x120;
inline constexpr std::size_t kTimelineBeat = 0x124;
inline constexpr std::size_t kTimelinePreviousBeat = 0x128;
inline constexpr std::size_t kTimelineBeatBase = 0x12C;
inline constexpr std::size_t kTimelineLastTempoUs = 0x134;
inline constexpr std::size_t kTimelineTempoCount = 0x138;
inline constexpr std::size_t kTimelineTempoIndex = 0x13C;
inline constexpr std::size_t kTimelineTempoArray = 0x148;

// Tempo array: a pointer at +0x148, an 8-byte header, then 8-byte entries.
// Entry layout: +0 BPM (float), +4 event time (int32, microseconds after the
// conversion). The native entry time field is at array + 8*index + 12.
inline constexpr std::size_t kTempoEntrySize = 8;
inline constexpr std::size_t kTempoEntryBpm = 0;
inline constexpr std::size_t kTempoEntryTime = 4;
inline constexpr std::size_t kTempoArrayHeader = 8;

// Rounded load-time conversions. These are used only while reading chart or
// setting data; no runtime frame view is created.
[[nodiscard]] constexpr std::int32_t ChartMsToTimelineUs(std::int32_t ms) noexcept {
    return static_cast<std::int32_t>(static_cast<std::int64_t>(ms) * 1000);
}

[[nodiscard]] constexpr std::int32_t FrameBoundaryToTimelineUs(std::int32_t frame) noexcept {
    // Round to nearest microsecond; 1/60 s is not an integer number of us.
    const auto scaled = static_cast<std::int64_t>(frame) * kTimelineUsPerSecond;
    return static_cast<std::int32_t>(
        scaled >= 0 ? (scaled + 30) / 60 : -((-scaled + 30) / 60));
}

// Advances the native timeline to an absolute microsecond position and returns
// the integer beat index, matching the original return value. The caller must
// guarantee `timeline` points to a live native timeline object and that all
// tempo entries and note/chart times have already been converted to us.
[[nodiscard]] std::int32_t AdvanceTimelineUs(void* timeline,
                                             std::int64_t now_us) noexcept;

} // namespace nosfix
