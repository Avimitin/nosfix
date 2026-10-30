#include "nosfix/native_timeline.hpp"

#include <algorithm>
#include <cstring>
#include <limits>

namespace nosfix {
namespace {

template <class T>
T Load(const void* base, std::size_t offset) noexcept {
    T value{};
    std::memcpy(&value, static_cast<const std::byte*>(base) + offset, sizeof(T));
    return value;
}

template <class T>
void Store(void* base, std::size_t offset, T value) noexcept {
    std::memcpy(static_cast<std::byte*>(base) + offset, &value, sizeof(T));
}

std::int32_t ClampToCounter(std::int64_t value) noexcept {
    return static_cast<std::int32_t>(std::clamp<std::int64_t>(
        value, std::numeric_limits<std::int32_t>::min(),
        std::numeric_limits<std::int32_t>::max()));
}

float BeatFraction(float beat) noexcept {
    // The native code uses a C cast to int, i.e. truncation toward zero.
    return beat - static_cast<float>(static_cast<std::int32_t>(beat));
}

} // namespace

std::int32_t AdvanceTimelineUs(void* timeline, std::int64_t now_us) noexcept {
    if (!timeline) return 0;

    const std::int32_t now = ClampToCounter(now_us);
    const std::int32_t previous_counter = Load<std::int32_t>(timeline, kTimelineCounter);
    Store<std::int32_t>(timeline, kTimelineCounter, now);

    std::int32_t tempo_index = Load<std::int32_t>(timeline, kTimelineTempoIndex);
    const std::int32_t tempo_count = Load<std::int32_t>(timeline, kTimelineTempoCount);
    const auto tempo_array = Load<std::uintptr_t>(timeline, kTimelineTempoArray);

    float bpm = Load<float>(timeline, kTimelineBpm);
    float beat_base = Load<float>(timeline, kTimelineBeatBase);
    std::int32_t last_tempo_us = Load<std::int32_t>(timeline, kTimelineLastTempoUs);
    const float previous_beat = Load<float>(timeline, kTimelineBeat);
    float current_beat = previous_beat;

    // Drain every event at or before the new absolute position. A high-refresh
    // or late update can cross several events; exact equality would skip them.
    std::int32_t guard = 0;
    while (tempo_array != 0 && tempo_index >= 0 && tempo_index < tempo_count &&
           guard++ <= tempo_count) {
        const auto entry = tempo_array + kTempoArrayHeader +
                           static_cast<std::uintptr_t>(tempo_index) * kTempoEntrySize;
        const float event_bpm = Load<float>(
            reinterpret_cast<const void*>(entry), kTempoEntryBpm);
        const std::int32_t event_us = Load<std::int32_t>(
            reinterpret_cast<const void*>(entry), kTempoEntryTime);
        if (now < event_us) break;

        const std::int32_t delta_us = event_us - last_tempo_us;
        beat_base += static_cast<float>(delta_us) * bpm /
                     static_cast<float>(kTimelineUsPerMinute);
        current_beat = beat_base;
        bpm = event_bpm;
        last_tempo_us = event_us;
        ++tempo_index;

        Store<float>(timeline, kTimelineBpm, bpm);
        Store<float>(timeline, kTimelineBeatBase, beat_base);
        Store<std::int32_t>(timeline, kTimelineLastTempoUs, last_tempo_us);
        Store<std::int32_t>(timeline, kTimelineTempoIndex, tempo_index);
    }

    const std::int32_t final_delta_us = now - last_tempo_us;
    current_beat = beat_base + static_cast<float>(final_delta_us) * bpm /
                                  static_cast<float>(kTimelineUsPerMinute);

    Store<float>(timeline, kTimelinePreviousBeat, previous_beat);
    Store<float>(timeline, kTimelineBeat, current_beat);

    const bool was_unset = Load<std::uint8_t>(timeline, kTimelineBeatFlag) == 0;
    if (was_unset) {
        if (now >= 0 &&
            (BeatFraction(previous_beat) > BeatFraction(current_beat) || now == 0)) {
            Store<std::uint8_t>(timeline, kTimelineBeatFlag, 1);
        }
    } else {
        Store<std::uint8_t>(timeline, kTimelineBeatFlag, 0);
    }

    const std::uint32_t previous_phase = Load<std::uint32_t>(timeline, kTimelinePhase);
    Store<std::uint32_t>(timeline, kTimelinePreviousPhase, previous_phase);
    if (previous_counter >= 0) {
        Store<float>(timeline, kTimelinePhase,
                     1.0f - BeatFraction(current_beat));
        return static_cast<std::int32_t>(current_beat);
    }

    Store<float>(timeline, kTimelinePhase, 1.0f);
    return static_cast<std::int32_t>(previous_phase);
}

} // namespace nosfix
