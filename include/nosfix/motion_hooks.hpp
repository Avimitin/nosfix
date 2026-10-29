#pragma once

#include "nosfix/hooks.hpp"

#include <cstdint>

namespace nosfix {

// Added to the same verified, transactional hook set as the scheduler.
void AppendMotionHooks(std::vector<HookSpec>& specs);
void ConfigureMotion(HMODULE module, bool enabled) noexcept;
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
    // Indexed by GeometryDiagnostic; records why a rebuild fell back.
    std::uint64_t geometry_diagnostics[16] = {};
};
[[nodiscard]] MotionMetrics TakeMotionMetrics() noexcept;

} // namespace nosfix
