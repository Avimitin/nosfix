#pragma once

#include "nosfix/hooks.hpp"

#include <cstdint>

namespace nosfix {

// Added to the same verified, transactional hook set as the scheduler.
void AppendMotionHooks(std::vector<HookSpec>& specs);
void ConfigureMotion(HMODULE module, bool enabled) noexcept;
// Optional live-shadow observer for normal-note updates. Null disables it.
using ShadowNoteObserver = void (*)(void* note);
void SetShadowNoteObserver(ShadowNoteObserver observer) noexcept;
void BeginMotionPresentation(double phase) noexcept;
void EndMotionPresentation() noexcept;
void RetireMotionTick() noexcept;

struct MotionMetrics {
    std::uint64_t captures = 0;
    std::uint64_t predicted_draws = 0;
    std::uint64_t timed_draws = 0;
    std::uint64_t fallback_draws = 0;
    std::uint64_t timed_note_captures = 0;
    std::uint64_t timed_note_failures = 0;
    std::uint64_t unsupported_notes = 0;
    // Named counters for the kinds still on the original path. Each one means
    // "this note's geometry was simulated, not rebuilt"; the matching entry in
    // geometry_diagnostics names the exact missing evidence.
    std::uint64_t unsupported_glissando = 0;
    std::uint64_t unsupported_velocity_zones = 0;
    std::uint64_t unsupported_beats = 0;
    // Whole-object rebuilds (one draw call owning 0-2 quads). Stays zero until
    // the glissando connector anchors are proven.
    std::uint64_t object_part_builds = 0;
    // Indexed by GeometryDiagnostic; records why a rebuild fell back.
    std::uint64_t geometry_diagnostics[24] = {};
};
[[nodiscard]] MotionMetrics TakeMotionMetrics() noexcept;

} // namespace nosfix
