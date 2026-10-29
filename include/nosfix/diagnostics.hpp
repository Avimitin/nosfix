#pragma once

#include <cstdint>

namespace nosfix {

// Runtime diagnostic probes for the four remaining live-only questions.
// Disabled by default; every probe is read-only and rate-limited. This is a
// diagnostic build aid, not a gameplay path.
void ConfigureDiagnosticProbe(bool enabled) noexcept;
[[nodiscard]] bool DiagnosticProbeEnabled() noexcept;

// Normal-note judge at 0x2F6800. `event` is the native event pointer passed in
// RDX; `mode` is the R8B argument. Logs the before/after note fields needed to
// decide whether note+84 is a chord slot and what event+0x20 carries.
void ProbeNormalJudgeBefore(const void* note, const void* event,
                            unsigned mode) noexcept;
void ProbeNormalJudgeAfter(const void* note, const void* event,
                           unsigned mode, std::int64_t result) noexcept;

// Called once per update by the motion hooks. `kind` is the TimedNoteKind
// integer (3 glissando, 5 velocity zone); `frame` is dword_18070A610.
void ProbeMotionNote(const void* note, int kind, std::int32_t frame) noexcept;

} // namespace nosfix
