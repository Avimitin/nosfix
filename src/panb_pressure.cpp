#include "nosfix/panb_pressure.hpp"

namespace nosfix {

namespace {

std::uint8_t HighNibble(std::byte value) noexcept {
    return static_cast<std::uint8_t>(std::to_integer<std::uint8_t>(value) >> 4);
}

std::uint8_t LowNibble(std::byte value) noexcept {
    return static_cast<std::uint8_t>(std::to_integer<std::uint8_t>(value) & 0x0Fu);
}

void ClearKey(PanbKeySample& key) noexcept {
    key.average = 0;
    key.level = 0;
    key.held = 0;
    key.pressed = false;
    key.valid = false;
}

} // namespace

const char* PanbStatusName(PanbStatus status) noexcept {
    switch (status) {
    case PanbStatus::Ok: return "ok";
    case PanbStatus::NotConfigured: return "not-configured";
    case PanbStatus::UnverifiedRule: return "unverified-rule";
    case PanbStatus::MalformedBuffer: return "malformed-buffer";
    case PanbStatus::EmptyFrame: return "empty-frame";
    case PanbStatus::LevelOutOfRange: return "level-out-of-range";
    case PanbStatus::InsufficientCapacity: return "insufficient-capacity";
    }
    return "invalid";
}

PanbStatus PanbPressureState::Configure(const PanbConfig& config) noexcept {
    config_ = config;
    for (PanbKeySample& key : keys_) ClearKey(key);
    configured_ = true;
    return PanbStatus::Ok;
}

PanbStatus PanbPressureState::Apply(std::span<const std::byte> buffer,
                                    std::span<PanbKeySample> samples,
                                    std::span<PanbEdge> edges,
                                    PanbChangeCounts& counts) noexcept {
    counts = PanbChangeCounts{};
    if (!configured_) return PanbStatus::NotConfigured;
    if (config_.threshold_source != PanbThresholdSource::RuntimeSampled) {
        // Without the live threshold the level, the counter and the edges are
        // all unprovable; refuse rather than assume a zero threshold.
        return PanbStatus::UnverifiedRule;
    }
    if (buffer.size() < kPanbHeaderBytes) return PanbStatus::MalformedBuffer;
    const auto declared = std::to_integer<std::uint8_t>(buffer[0]);
    if (declared == 0) return PanbStatus::EmptyFrame;
    const std::size_t samples_declared = declared;
    const std::size_t required = kPanbHeaderBytes + kPanbRecordBytes * (samples_declared - 1) +
                                 kPanbNibbleBytes;
    if (buffer.size() < required) return PanbStatus::MalformedBuffer;
    if (samples.size() < kPanbKeyCount || edges.size() < kPanbKeyCount) {
        return PanbStatus::InsufficientCapacity;
    }

    std::array<std::uint32_t, kPanbKeyCount> sums{};
    for (std::size_t record = 0; record < samples_declared; ++record) {
        const std::size_t base = kPanbHeaderBytes + kPanbRecordBytes * record;
        for (std::size_t byte = 0; byte < kPanbNibbleBytes; ++byte) {
            const std::byte value = buffer[base + byte];
            // Key 2j+1 (even lane) is the high nibble, key 2j+2 the low one.
            sums[byte * 2] += HighNibble(value);
            sums[byte * 2 + 1] += LowNibble(value);
        }
    }

    std::array<PanbKeySample, kPanbKeyCount> next{};
    for (std::size_t lane = 0; lane < kPanbKeyCount; ++lane) {
        const std::uint32_t average = sums[lane] / static_cast<std::uint32_t>(samples_declared);
        // The native stores a byte-wide average; a count above 14 samples can
        // only arise from a corrupt buffer and is refused below.
        if (average > 0xFFu) return PanbStatus::LevelOutOfRange;
        PanbKeySample& key = next[lane];
        key.average = static_cast<std::uint8_t>(average);
        // Unsigned compare exactly as 0x1803522F7 (cmp/jae).
        key.level = key.average < config_.threshold ? 0 : key.average;
        if (config_.reject_level_15 && key.level > kPanbMaxLevel) {
            // The producer excludes 15; anything above 14 is a different layout.
            return PanbStatus::LevelOutOfRange;
        }
        key.pressed = key.level != 0;
        key.held = key.pressed ? keys_[lane].held + 1 : 0;
        key.valid = true;
    }

    for (std::size_t lane = 0; lane < kPanbKeyCount; ++lane) {
        const PanbKeySample& current = next[lane];
        const PanbKeySample& previous = keys_[lane];
        if (current.pressed != previous.pressed) {
            PanbEdge& edge = edges[counts.edges++];
            edge.lane = static_cast<std::uint8_t>(lane);
            edge.pressed = current.pressed;
            edge.level = current.level;
            edge.average = current.average;
            edge.held = current.held;
            if (current.pressed) {
                ++counts.presses;
            } else {
                ++counts.releases;
            }
        }
        if (current.average != 0) ++counts.keys_nonzero;
        if (current.average != 0 && current.level == 0) ++counts.keys_gated;
        samples[lane] = current;
    }
    counts.samples = declared;
    keys_ = next;
    return PanbStatus::Ok;
}

std::size_t PanbPressureState::Reset(std::span<PanbEdge> edges) noexcept {
    std::size_t written = 0;
    for (std::size_t lane = 0; lane < kPanbKeyCount; ++lane) {
        PanbKeySample& key = keys_[lane];
        if (key.pressed) {
            if (written < edges.size()) {
                PanbEdge& edge = edges[written];
                edge.lane = static_cast<std::uint8_t>(lane);
                edge.pressed = false;
                edge.level = 0;
                edge.average = key.average;
                edge.held = key.held;
            }
            ++written;
        }
        key.pressed = false;
        key.level = 0;
        key.held = 0;
        key.average = 0;
    }
    return written;
}

std::size_t PanbPressureState::PressedCount() const noexcept {
    std::size_t count = 0;
    for (const PanbKeySample& key : keys_) {
        if (key.pressed) ++count;
    }
    return count;
}

bool PanbPressureState::Pressed(std::uint8_t lane) const noexcept {
    return lane < kPanbKeyCount && keys_[lane].pressed;
}

PanbKeySample PanbPressureState::Key(std::uint8_t lane) const noexcept {
    return lane < kPanbKeyCount ? keys_[lane] : PanbKeySample{};
}

} // namespace nosfix
