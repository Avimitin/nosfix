#pragma once

#include "nosfix/timed_note_geometry.hpp"
#include "nosfix/timed_normal.hpp"
#include <Windows.h>
#include <optional>
#include <string>

namespace nosfix {

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

// True for the layouts whose chart-time rebuild is evidenced end to end:
// normal, long/hold and trill.
[[nodiscard]] bool TimedNoteSupported(TimedNoteKind kind) noexcept;

[[nodiscard]] bool PrepareTimedRendering(HMODULE game, std::string& reason);
void ConfigureTimedRendering(bool enabled) noexcept;
[[nodiscard]] bool TimedRenderingEnabled() noexcept;
// Query once per presentation, independently of legacy simulation phase.
void BeginTimedRendering() noexcept;
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
