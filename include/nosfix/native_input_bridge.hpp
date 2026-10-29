#pragma once

// Glue between the captured input pipeline and the non-normal note models.
//
// InputPipeline already exposes everything needed and is not modified here:
//   * PipelineInputEvent.input (LogicalInputEvent: lane, edge, velocity, stamp)
//     with PipelineInputEvent.song_time and has_song_time;
//   * AnalogLevels(std::span<AnalogLevelView>) for the latest per-lane level.
// This header only converts those values into the model's native event shape,
// so it stays free of Windows headers and of any new runtime seam. Nothing here
// is installed; the parent owns wiring.

#include "nosfix/native_note_types.hpp"
#include "nosfix/panb_pressure.hpp"
#include "nosfix/timing_events.hpp"

#include <cstdint>

namespace nosfix {

// PANB quantization of a normalized level, identical to PanbVelocity in
// input_decoder.hpp (level = min(uint8(normalized * 15.999), 14)). It is
// repeated here so the note-type and pressure models do not include HID or
// Windows headers; panb_pressure_tests cross-checks the two.
[[nodiscard]] constexpr std::uint8_t PanbLevelFromNormalized(float normalized) noexcept {
    if (!(normalized > 0.0f)) return 0;
    if (normalized >= 1.0f) return kPanbMaxLevel;
    const auto value = static_cast<std::uint8_t>(normalized * 15.999f);
    return value > kPanbMaxLevel ? kPanbMaxLevel : value;
}

// Maps one captured logical edge onto the native event shape. `event_id` and
// `group_id` are opaque host values (the native event producer is not decoded);
// `song_time` must be the clock snapshot taken at capture. Returns false for an
// out-of-range lane or a non-finite song time, so a caller never feeds a
// half-built event to a note state.
//
// Lane convention: the native lane/key numbers are 1-based (1..28) while
// InputPipeline lanes and this model are 0-based (0..27), so key k is lane k-1.
// The native PANB parser emits its 28 records in packet order; that this order
// equals the lane order is the documented adapter assumption (the analysis
// worker marks it as a hypothesis, not a proven byte fact).
[[nodiscard]] inline bool MakeNativeNoteEvent(const SongTime& song_time,
                                              const LogicalInputEvent& input,
                                              std::uint32_t event_id,
                                              std::uint32_t group_id,
                                              NativeNoteEvent& out) noexcept {
    if (input.lane >= kNativeTypeLaneCount) return false;
    if (!(song_time.microseconds == song_time.microseconds)) return false; // NaN
    out = NativeNoteEvent{};
    out.stamp = input.stamp;
    out.song_time = song_time;
    out.lane = input.lane;
    out.kind = input.edge == InputEdge::Press ? NativeEventKind::Press
                                              : NativeEventKind::Release;
    out.event_id = event_id;
    out.group_id = group_id;
    out.velocity = input.velocity;
    out.panb_level = PanbLevelFromNormalized(input.velocity);
    return true;
}

// Convenience overload for a host that kept the native TimedInputEvent.
[[nodiscard]] inline bool MakeNativeNoteEvent(const TimedInputEvent& timed,
                                              std::uint32_t event_id,
                                              std::uint32_t group_id,
                                              NativeNoteEvent& out) noexcept {
    return MakeNativeNoteEvent(timed.song_time, timed.input, event_id, group_id, out);
}

// Builds the pressure edge for one lane at a discontinuity, e.g. when a device
// reconnects or the pipeline reports a reset. `level` is the last observed
// normalized level; the model reports it as released.
[[nodiscard]] inline PanbEdge MakePanbReleaseEdge(std::uint8_t lane,
                                                  float last_level) noexcept {
    PanbEdge edge{};
    edge.lane = lane;
    edge.pressed = false;
    edge.level = 0;
    edge.average = PanbLevelFromNormalized(last_level);
    edge.held = 0;
    return edge;
}

} // namespace nosfix
