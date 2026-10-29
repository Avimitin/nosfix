#pragma once

// Native PANB pressure path of nostalgia.dll PAN-2025020501.
//
// Evidence (all module-relative RVAs, pseudocode under analyze/evidence and
// disassembly re-checked against analyze/nostalgia.dll SHA-256 339247b9...):
//
//   0x365DA0 case 6  ac_io_panb_get_control_status_buffer(driver + 2224)
//   0x352560         sub_1803520D0(driver + 16, status buffer) then
//                    sub_180353600(driver, driver + 16)
//   0x3520D0         the per-key average, threshold and hold counter
//   0x353600         forwards the copied 788-byte device block to listeners
//   0x3526C0         CPianoDevice constructor zeroes the 28 key records
//
// Buffer shape (verified byte by byte in 0x3520D0 and 0x353600):
//   byte 0        sample-set count N
//   bytes 1..2    unused
//   record r      bytes 3 + 16r .. 16 + 16r, of which the first 14 carry the
//                 28 keys as nibbles: byte j holds key 2j+1 in its high nibble
//                 and key 2j+2 in its low nibble (key k -> lane k-1). Bytes 14
//                 and 15 of each record are unused.
//
// Per-key rule (verified from the instruction bytes at 0x1803522C6..0x180352312
// and its seven duplicated blocks):
//   sum      = unsigned sum of the N nibbles for that key
//   average  = sum / N                      (unsigned 32-bit division)
//   level    = average < byte_1812D3E77 ? 0 : average   (unsigned cmp/jae)
//   held     = level != 0 ? held + 1 : 0
//   record   = { int32 average, int32 level, int32 held } at device+16, stride 12
//
// byte_1812D3E77 is a configuration byte, not a constant and not a nibble
// width. Proven writer (answers/notes-input-001.md Q4): sub_1803525D0 loads
// `config+0x2C` and stores it with the x64 absolute-address form
//   movzx ecx, byte ptr [rax+2Ch] ; mov cs:byte_1812D3E77, cl
// at 0x180352687, which is why a RIP-relative reference sweep finds nothing.
// Sub_1803525D0 returns before the store when *(config+0x25) == 0. The byte is
// not a nibble width and is not the "1/15.999" value: the rule is
// "pressed iff the mean 4-bit level over the packet is >= threshold", unsigned,
// in raw 0..15 units.
//
// Initial value: the analysis database reports 0xFF at that address and 0xFF
// around it, while the range is not file-backed (.data virtual size 0x4455AAC
// against a 0x1A200 raw size), so the loader's zero fill is what a cold boot
// holds. The two readings disagree in effect (255 gates every level to 0, 0
// gates nothing); this model therefore still refuses to choose and takes the
// value from the caller. PanbThresholdSource::Unverified is the default and
// produces no level and no edge at all; a caller that has read config+0x2C (or
// sampled the live byte) opts in with RuntimeSampled and its own value.
//
// What judgement consumes: nothing from this path. The judge chain
// (0x2F7C10 -> note vtable slot 4 -> 0x2F79E0 -> 0x303E80) never calls
// 0x3520D0, never reads a per-key record and never reads byte_1812D3E77; rank
// comes from the time windows alone. 0x3520D0 is reached from 0x3524A0 /
// 0x352560 and both call 0x353600, which is the LED / key-flash update. The
// only non-constant numeric input to scoring anywhere is the input event's
// +0x20 value (stored into note+84 and tested as `id > 1 && rank > 1` by
// 0x303E80); that field belongs to the judgment stream's event producer and is
// not modelled here. This module therefore derives press/release edges from
// level != 0 for hosts (lights, visuals, any later velocity rule), carries the
// exact level on the press edge, and keeps the native hold counter. It makes no
// claim that a level selects a rank.
//
// Lane mapping: the native parser emits 28 records in packet order and the
// native lane arrays are 1-based (lane 1..28). This model keeps the existing
// pipeline convention lane = key - 1 (0..27) and the adapter states that
// packet order equals lane order, which the analysis worker marks as a
// hypothesis rather than a proven byte fact.
//
// No allocation, lock, logging, file or blocking call happens on any path here.

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace nosfix {

inline constexpr std::size_t kPanbKeyCount = 28;
inline constexpr std::size_t kPanbHeaderBytes = 3;
inline constexpr std::size_t kPanbRecordBytes = 16;
inline constexpr std::size_t kPanbNibbleBytes = 14;
inline constexpr std::uint8_t kPanbMaxLevel = 14;       // Spice producer quantization
inline constexpr std::uint8_t kPanbMaxNativeLevel = 15; // native 4-bit record field

inline constexpr std::uint32_t kPanbThresholdRva = 0x12D3E77; // byte_1812D3E77
inline constexpr std::uint32_t kPanbThresholdWriterRva = 0x3525D0;
inline constexpr std::uint32_t kPanbThresholdStoreRva = 0x352687;
inline constexpr std::uint32_t kPanbThresholdConfigOffset = 0x2C; // config+44
inline constexpr std::uint32_t kPanbThresholdConfigEnableOffset = 0x25;
inline constexpr std::uint8_t kPanbThresholdImageFiller = 0xFF; // database filler
inline constexpr std::uint8_t kPanbThresholdPreConfig = 0;      // loader zero fill
inline constexpr std::uint32_t kPanbAverageRva = 0x3520D0;
inline constexpr std::uint32_t kPanbDeviceUpdateRva = 0x352560;
inline constexpr std::uint32_t kPanbDispatchRva = 0x353600;
inline constexpr std::uint32_t kPanbStatusBufferOffset = 2224; // driver + 2224
inline constexpr std::uint32_t kPanbKeyRecordOffset = 16;      // driver + 16
inline constexpr std::uint32_t kPanbKeyRecordStride = 12;      // {avg, level, held}

// Where the threshold value comes from. Only RuntimeSampled enables the rule.
enum class PanbThresholdSource : std::uint8_t {
    Unverified = 0,     // fail closed: no level, no edge, no state change
    RuntimeSampled = 1, // the caller read config+0x2C or the live module byte
};

struct PanbConfig final {
    PanbThresholdSource threshold_source = PanbThresholdSource::Unverified;
    std::uint8_t threshold = 0; // compared unsigned: level = average >= threshold
    // The native parser keeps a 4-bit level up to 15 unclamped; only the Spice
    // producer stops at 14. Set this for a producer known to stop at 14, to turn
    // a 15 into a visible layout error; default is the native behaviour.
    bool reject_level_15 = false;
};

enum class PanbStatus : std::uint8_t {
    Ok = 0,
    NotConfigured,
    UnverifiedRule,      // threshold source is Unverified
    MalformedBuffer,     // too short for the declared sample count
    EmptyFrame,          // sample count 0: the native loop stores nothing
    LevelOutOfRange,     // a nibble above 14 with reject_level_15 set
    InsufficientCapacity // output spans cannot hold 28 keys / 28 edges
};

[[nodiscard]] const char* PanbStatusName(PanbStatus status) noexcept;

// One per key, in lane order (lane 0 = PANB Key 1).
struct PanbKeySample final {
    std::uint8_t average = 0; // ungated integer average of the nibbles
    std::uint8_t level = 0;   // 0 when average < threshold
    std::uint32_t held = 0;   // consecutive applied frames with level != 0
    bool pressed = false;     // level != 0
    bool valid = false;       // set once the key appeared in an applied frame
};

struct PanbEdge final {
    std::uint8_t lane = 0;
    bool pressed = false;
    std::uint8_t level = 0;
    std::uint8_t average = 0;
    std::uint32_t held = 0; // counter after this frame
};

struct PanbChangeCounts final {
    std::uint8_t samples = 0;
    std::size_t keys_nonzero = 0; // keys with a nonzero raw average
    std::size_t keys_gated = 0;   // keys whose average was below the threshold
    std::size_t edges = 0;
    std::size_t presses = 0;
    std::size_t releases = 0;
};

class PanbPressureState final {
public:
    PanbPressureState() = default;

    [[nodiscard]] PanbStatus Configure(const PanbConfig& config) noexcept;

    // Applies one status buffer. `samples` and `edges` must each hold
    // kPanbKeyCount entries; otherwise the call is a no-op with
    // InsufficientCapacity. On any non-Ok status nothing in the state changes.
    [[nodiscard]] PanbStatus Apply(std::span<const std::byte> buffer,
                                   std::span<PanbKeySample> samples,
                                   std::span<PanbEdge> edges,
                                   PanbChangeCounts& counts) noexcept;

    // Connection loss / reconnect / chart reset: every pressed lane produces one
    // synthetic release edge so a held key cannot stick. Returns the count.
    std::size_t Reset(std::span<PanbEdge> edges) noexcept;

    [[nodiscard]] bool Verified() const noexcept {
        return configured_ &&
               config_.threshold_source == PanbThresholdSource::RuntimeSampled;
    }
    [[nodiscard]] PanbThresholdSource ThresholdSource() const noexcept {
        return config_.threshold_source;
    }
    [[nodiscard]] std::uint8_t Threshold() const noexcept { return config_.threshold; }
    [[nodiscard]] std::size_t PressedCount() const noexcept;
    [[nodiscard]] bool Pressed(std::uint8_t lane) const noexcept;
    [[nodiscard]] PanbKeySample Key(std::uint8_t lane) const noexcept;

private:
    PanbConfig config_{};
    std::array<PanbKeySample, kPanbKeyCount> keys_{};
    bool configured_ = false;
};

// Nibble geometry helpers (pure, no state). Lane 0 is the high nibble of the
// first data byte of each record.
[[nodiscard]] constexpr std::size_t PanbDataByteForLane(std::uint8_t lane) noexcept {
    return static_cast<std::size_t>(lane) / 2;
}
[[nodiscard]] constexpr bool PanbLaneIsHighNibble(std::uint8_t lane) noexcept {
    return (lane % 2) == 0; // even lane (Key 1, 3, ...) is the high nibble
}

} // namespace nosfix
