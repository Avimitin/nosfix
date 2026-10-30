#pragma once

#include "nosfix/native_shadow_adapter.hpp"

#include <cstddef>
#include <cstdint>

namespace nosfix {

// Live shadow feed: reads the game-owned note objects and staging bucket, runs
// NativeShadowFeed beside the original judge, and logs outcomes. It never
// writes game memory, never installs a judge hook and never changes scoring.
struct ShadowLiveMetrics final {
    std::uint64_t frames = 0;
    std::uint64_t observed_notes = 0;
    std::uint64_t outcomes = 0;
    std::uint64_t effects = 0;
    std::uint64_t feed_errors = 0;
    ShadowFeedStatus last_status = ShadowFeedStatus::NotConfigured;
};

// `image` is the nostalgia module base with at least image_size bytes; the
// judgment-window sample and offset globals are read from it. `module_base`
// is where the 0x13DEA10 staging pointer and 0x70A610 frame counter live.
void ConfigureShadowLive(const std::byte* image, std::size_t image_size,
                         const void* module_base) noexcept;
void DisableShadowLive() noexcept;
[[nodiscard]] bool ShadowLiveEnabled() noexcept;
// True after ConfigureShadowLive was requested but the judgment-window block
// was still the pre-initialization zero block. The feed retries per tick.
[[nodiscard]] bool ShadowLivePending() noexcept;

// Called from the normal-note update detour after the original update.
void ShadowLiveObserveNormalNote(void* note) noexcept;

// Called once per simulation tick after the note managers have published the
// staging buffer. `now_us` is the mapped song time of that tick.
void ShadowLiveTick(double now_us, const void* module_base) noexcept;

[[nodiscard]] ShadowLiveMetrics TakeShadowLiveMetrics() noexcept;

} // namespace nosfix
