#pragma once

#include <cstdint>

namespace nosfix {

// Read-only runtime probes for live validation. The high-refresh design is
// recovered statically from the image; these probes only confirm that the
// static model matches the running build. Disabled by default and rate-limited.
// This is a diagnostic build aid, not a gameplay path.
void ConfigureDiagnosticProbe(bool enabled) noexcept;
[[nodiscard]] bool DiagnosticProbeEnabled() noexcept;

// Normal-note judge at 0x2F6800. `event` is the native event pointer passed in
// RDX; `mode` is the R8B argument. Logs the before/after note fields needed to
// decide whether note+84 is a chord slot and what event+0x20 carries.
void ProbeNormalJudgeBefore(const void* note, const void* event,
                            unsigned mode) noexcept;
void ProbeNormalJudgeAfter(const void* note, const void* event,
                           unsigned mode, std::int64_t result) noexcept;

// Called once per update by the motion hooks. `kind` is the caller's
// TimedNoteKind integer; `frame` is dword_18070A610. The probe also logs the
// actual vtable so an inverted gliss/velocity mapping is visible.
void ProbeMotionNote(const void* note, int kind, std::int32_t frame,
                     const void* module_base) noexcept;

// Diffs the 28x28 staging input-state buffer published at module+0x13DEA10
// once per simulation tick. This identifies the producer's final store without
// a debugger watchpoint.
void ProbeInputStateDiff(const void* module_base) noexcept;

} // namespace nosfix
