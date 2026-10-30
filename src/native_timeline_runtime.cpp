#include "nosfix/native_timeline_runtime.hpp"

#include "nosfix/native_timeline.hpp"
#include "nosfix/song_clock_publication.hpp"
#include "nosfix/timed_rendering.hpp"

#include <Windows.h>

#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstring>

namespace nosfix {
namespace {

std::atomic<bool> g_enabled{false};
std::int64_t g_frequency = 0;
std::int64_t g_epoch_qpc = 0;
std::int64_t g_epoch_us = kTimelinePrerollUs;
std::int32_t g_last_counter = kTimelinePrerollUs;
std::uint64_t g_generation_at_reset = 0;
std::uint64_t g_last_generation = 0;
bool g_generation_armed = false;
bool g_started = false;

std::int64_t QpcNow() noexcept {
    LARGE_INTEGER now{};
    QueryPerformanceCounter(&now);
    return now.QuadPart;
}

std::int64_t QpcFrequency() noexcept {
    if (g_frequency > 0) return g_frequency;
    LARGE_INTEGER frequency{};
    if (QueryPerformanceFrequency(&frequency)) g_frequency = frequency.QuadPart;
    return g_frequency;
}

std::int64_t QpcElapsedUs(std::int64_t now) noexcept {
    const auto frequency = QpcFrequency();
    if (frequency <= 0) return g_epoch_us;
    const auto elapsed = now - g_epoch_qpc;
    return g_epoch_us + elapsed * 1'000'000 / frequency;
}

std::uint64_t PublishedGeneration() noexcept {
    const auto* publication = SongClockPublicationForInput();
    if (!publication) return 0;
    SongClockSnapshot snapshot{};
    if (!publication->TryRead(snapshot)) return 0;
    return snapshot.segment.Generation();
}

void ResetEpoch(std::int64_t now) noexcept {
    g_epoch_qpc = now;
    g_epoch_us = kTimelinePrerollUs;
    g_last_counter = kTimelinePrerollUs;
    g_generation_at_reset = PublishedGeneration();
    g_last_generation = 0;
    g_generation_armed = false;
    g_started = true;
}

} // namespace

void ConfigureNativeTimelineRuntime(bool enabled) noexcept {
    g_enabled.store(enabled, std::memory_order_release);
    g_frequency = 0;
    g_epoch_qpc = 0;
    g_epoch_us = kTimelinePrerollUs;
    g_last_counter = kTimelinePrerollUs;
    g_generation_at_reset = 0;
    g_last_generation = 0;
    g_generation_armed = false;
    g_started = false;
}

bool NativeTimelineRuntimeEnabled() noexcept {
    return g_enabled.load(std::memory_order_acquire);
}

std::int64_t NativeTimelineTick(void* timeline) noexcept {
    if (!timeline) return 0;

    std::int32_t before = 0;
    std::memcpy(&before, timeline, sizeof(before));
    const auto now = QpcNow();

    // A reset writes the preroll constant and a seek jumps backwards by more
    // than one second. Do not reset merely because the current value is still
    // the preroll constant: the first tick after a reset writes that same
    // constant, and the next tick must advance the QPC base clock from it.
    if (!g_started || before < g_last_counter - 1'000'000) {
        ResetEpoch(now);
    }

    auto us = QpcElapsedUs(now);
    if (us < kTimelinePrerollUs) us = kTimelinePrerollUs;

    const auto* publication = SongClockPublicationForInput();
    if (publication) {
        SongClockSnapshot snapshot{};
        if (publication->TryRead(snapshot) && snapshot.segment.IsRunning()) {
            const auto mapped = snapshot.segment.Map(now);
            if (mapped) {
                const auto generation = snapshot.segment.Generation();
                if (!g_generation_armed) {
                    // Ignore a publication left over from the previous song.
                    if (generation != 0 && generation != g_generation_at_reset) {
                        g_generation_armed = true;
                        g_last_generation = generation;
                    }
                } else if (generation != g_last_generation) {
                    g_last_generation = generation;
                }
                if (g_generation_armed && generation == g_last_generation) {
                    const auto audio_us = static_cast<std::int64_t>(
                        std::llround(mapped->microseconds));
                    auto difference = audio_us - us;
                    if (difference < 0) difference = -difference;
                    // Accept the cue clock only when it is close to the QPC
                    // base clock and does not move the timeline backwards.
                    if (audio_us >= before && difference <= 5'000'000) us = audio_us;
                }
            }
        }
    }

    if (us < before) us = before;
    if (us == before) {
        float beat = 0.0f;
        std::memcpy(&beat, static_cast<const std::byte*>(timeline) + kTimelineBeat,
                    sizeof(beat));
        return static_cast<std::int32_t>(beat);
    }
    const auto result = AdvanceTimelineUs(timeline, us);
    g_last_counter = static_cast<std::int32_t>(us);
    return result;
}

} // namespace nosfix
