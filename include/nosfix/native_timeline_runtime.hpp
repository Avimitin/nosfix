#pragma once

#include <cstdint>

namespace nosfix {

// Enables the microsecond timeline tick. When disabled, the hook detour falls
// back to the original function.
void ConfigureNativeTimelineRuntime(bool enabled) noexcept;
[[nodiscard]] bool NativeTimelineRuntimeEnabled() noexcept;

// Absolute microsecond tick. Uses the published cue clock when it is valid and
// falls back to QPC elapsed time from the preroll reset. Writes the absolute
// position through AdvanceTimelineUs and returns the native beat index.
[[nodiscard]] std::int64_t NativeTimelineTick(void* timeline) noexcept;

} // namespace nosfix
