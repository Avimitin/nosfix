#pragma once

#include "nosfix/song_clock.hpp"

#include <cstdint>

namespace nosfix {

// Host-observed dequeue time, not a hardware switch timestamp. Reports in
// one RAWINPUT packet share QPC and sequence; ordinal preserves report/edge
// order. stream_epoch changes on loss/disconnect, independently of playback.
struct InputStamp final {
    std::int64_t qpc = 0;
    std::uint64_t sequence = 0;
    std::uint64_t stream_epoch = 0;
    std::uint32_t ordinal = 0;
};

enum class InputEdge : std::uint8_t { Press, Release };

// Device identity must include a reconnect generation assigned by a decoder.
// A reused Win32 HANDLE is insufficient. This representation is independent
// of keyboard scan codes and HID report-descriptor details.
struct PhysicalControlState final {
    std::uint64_t source_id = 0;
    std::uint64_t device_generation = 0;
    std::uint32_t control_id = 0;
    bool down = false;
    float velocity = 0.0f;
};

struct LogicalInputEvent final {
    InputStamp stamp;
    std::uint64_t source_id = 0;
    std::uint64_t device_generation = 0;
    std::uint8_t lane = 0; // Zero-based, 0..27.
    InputEdge edge = InputEdge::Release;
    float velocity = 0.0f; // Normalized 0..1; zero for releases.
    bool synthetic = false; // Reset releases are never physical judgments.
};

// Map through the capture-time immutable clock snapshot once. Rendering,
// queue-drain time and later audio corrections must not retime this event.
struct TimedInputEvent final {
    LogicalInputEvent input;
    SongTime song_time;
};

} // namespace nosfix
