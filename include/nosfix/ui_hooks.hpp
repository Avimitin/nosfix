#pragma once

#include "nosfix\call_patch.hpp"

#include <Windows.h>

#include <cstdint>
#include <string>
#include <vector>

namespace nosfix {

// Verify the AFP helper and its callsite, then append the verified redirect to
// the caller's existing call-patch transaction. The parent performs the AFP
// SHA-256 guard before calling this function.
[[nodiscard]] bool PrepareUiMotion(HMODULE afp_module, bool enabled,
                                   std::vector<CallPatchSpec>& patches,
                                   std::string& reason);

// Arm only after all hooks and the presentation proxy commit. Disable resets
// histories and makes the callsite forward unchanged.
void ConfigureUiMotion(bool enabled) noexcept;

// Bracket each presentation render. tick_serial is implicit and advances once
// per completed legacy simulation tick via RetireUiTick().
void BeginUiPresentation(double phase) noexcept;
void EndUiPresentation() noexcept;
void RetireUiTick() noexcept;

struct UiMotionMetrics {
    std::uint64_t captures = 0;
    std::uint64_t predicted_draws = 0;
    std::uint64_t fallback_draws = 0;
};
[[nodiscard]] UiMotionMetrics TakeUiMetrics() noexcept;

} // namespace nosfix
