#pragma once

#include "nosfix/timed_note_geometry.hpp"
#include "nosfix/timed_normal.hpp"
#include <Windows.h>
#include <cstdint>
#include <optional>
#include <string>

namespace nosfix {

class SongClockPublication;

enum class TimedNoteKind : std::uint8_t {
    Normal = 0,
    Long = 1,
    Beats = 2,
    Glissando = 3,
    Trill = 4,
    VelocityZone = 5,
};

// Capture-time outcome. Names are logged so a fallback is never silent.
enum class TimedNoteCapture : std::uint8_t {
    Ok = 0,
    Disabled,
    WrongVtable,
    Judged,
    InvalidChart,
    LayoutUnsupported,
    MissingPart,
    InvalidPart,
};

[[nodiscard]] const char* TimedNoteCaptureName(TimedNoteCapture value) noexcept;

// How a note kind's ribbon half width is derived from the chart. Proven from
// each kind's own update function:
//   HeadAndTail  long/trill  shrink * wide / 2 - 10 + note+60 / 2
//                (0x2F4E80 and 0x1A84F0; wide = dword 0x13E476C)
//   VelocityZone velocity     note+60 / 2 - 10, no shrink or wide term
//                (0x1801AAF31 inside 0x1AA4D0)
//   None         the kind has no box ribbon at all (normal notes)
enum class RibbonWidthRule : std::uint8_t {
    HeadAndTail = 0,
    VelocityZone = 1,
    None = 2,
};

[[nodiscard]] RibbonWidthRule RibbonWidthRuleFor(TimedNoteKind kind) noexcept;

// Applies `rule` to the raw chart words. Returns false for the None rule or a
// value outside the representable range; the caller then keeps its own value.
[[nodiscard]] bool RibbonHalfWidth(RibbonWidthRule rule, int note_width,
                                   int shrink, int wide_offset,
                                   float& half_width) noexcept;

// True for the layouts whose chart-time rebuild is evidenced end to end:
// normal, long/hold, trill and glissando.
[[nodiscard]] bool TimedNoteSupported(TimedNoteKind kind) noexcept;

// The exact reason a kind is not rebuilt from chart time; None when it is
// supported. Used for the per-kind fallback counters.
[[nodiscard]] GeometryDiagnostic TimedNoteRefusal(TimedNoteKind kind) noexcept;

[[nodiscard]] bool PrepareTimedRendering(HMODULE game, std::string& reason);
void ConfigureTimedRendering(bool enabled) noexcept;
[[nodiscard]] bool TimedRenderingEnabled() noexcept;
// Shadow clock publication: the render thread observes the same corrected cue
// time used by clock_motion, but this path never feeds geometry or judgement.
// Enabling it requires a prior PrepareTimedRendering and capture_input.
void ConfigureSongClock(bool enabled) noexcept;
[[nodiscard]] bool SongClockEnabled() noexcept;
[[nodiscard]] const SongClockPublication* SongClockPublicationForInput() noexcept;
// Query once per presentation, independently of legacy simulation phase. The
// QPC pair stamps the observation; it is not a render-derived song time.
void BeginTimedRendering(std::int64_t qpc_ticks,
                         std::int64_t qpc_frequency) noexcept;
void EndTimedRendering() noexcept;
[[nodiscard]] std::optional<double> RenderSongMilliseconds() noexcept;
// Test seam: injects the presentation song time so integration tests can drive
// the draw hook without the live audio cue. The runtime never calls this.
void SetTimedSongMillisecondsForTest(double ms) noexcept;
[[nodiscard]] bool CaptureTimedNormal(void* note, std::span<const std::byte> raw,
                                      TimedNormal& target) noexcept;
// Captures every evidenced sub-object of one note as private copies.
[[nodiscard]] TimedNoteCapture CaptureTimedNote(void* note, TimedNoteKind kind,
                                                TimedNoteGeometry& target) noexcept;
[[nodiscard]] bool ProjectGameNote(void*, int lane, float distance,
                                  int left, int right, NoteProjection& result) noexcept;
} // namespace nosfix
