#pragma once

// Portable NORMAL-note lifecycle for the native timing migration.
//
// Evidence base (all RVAs module-relative to nostalgia.dll PAN-2025020501):
//   0x2F6800 normal judge, 0x2F7900 eligibility, 0x2F7C10 traversal,
//   0x2F6DF0 normal render, 0x1A43E0 chart preparation.
// Field offsets below are quoted from that pseudocode and are marked as
// requiring parent byte/disassembly verification. Nothing in this header reads
// or writes game memory; the live seam is a separate, disabled proposal.
//
// What is modelled (normal notes only):
//   * selection by the inclusive eligibility union with the separately supplied
//     judgment + visual offset sum (0x2F7900). WHICH note is chosen is the
//     explicit adapter policy AdapterEarliestPending, not the unverified
//     0x2F7C10 matching; the native policy is rejected by Configure;
//   * press ranking with the judgment offset and the inclusive Just/Great/Good
//     windows (0x2F6800), including immediate hits and the native pending-early
//     deferral (0 < delta <= great_early);
//   * pending-early resolution when song time reaches the note, recording
//     Just as Just and every other early rank as Great (0x2F6800 note+76/+80);
//   * auto-play completion at the note time with rank Just (0x2F6800 auto
//     branch, gated by the global/wrapper auto flag);
//   * null-event updates: they resolve pending notes, run auto notes and expire
//     misses; they never fabricate a press;
//   * miss expiry strictly after the inclusive Good-late deadline, gated by the
//     certified processed-input watermark;
//   * exactly one completion per note and one keysound start; releases cut the
//     active lane sound without scoring or re-judging.
//
// What is NOT modelled / not claimed:
//   * long 0x2F4220, gliss 0x1A60C0, trill 0x1A7AF0, 0x1A8490 and the object
//     with state at +300 (0x2F4E20). They use different layouts and are out of
//     scope; see REQUESTS.txt F.
//   * the note+96 "auto slot" branch at 0x1802F699E. Its gate (note[26], the
//     pointed-to +96 byte, chart+80) is not fully evidenced; no note is given
//     null-completion behavior. REQUESTS.txt B4.
//   * native score points, effects, sound labels, note-object allocation and
//     vtable calls. Outcomes describe the rank/grade/sound the native seam must
//     forward; they are not a reimplementation of the scoring tables.
//
// Invariants: decisions use double microseconds. A press is judged only at its
// captured time. Rendering cadence only decides when Advance() runs; it cannot
// change a rank, a sound count or a miss. All state is single-owner.

#include "nosfix/normal_judgment.hpp"
#include "nosfix/timing_events.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <vector>

namespace nosfix {

// ===========================================================================
// Explicit layout / guard data (evidence, not ABI claims)
// ===========================================================================
namespace native_layout {

// Verified PE identity (native_pe_layout.json).
inline constexpr std::uintptr_t kNostalgiaImageBase = 0x180000000ULL;
inline constexpr std::uint32_t kNostalgiaImageSize = 7'640'728;
inline constexpr std::uint32_t kNostalgiaSizeOfImage = 0x4B89000;
inline constexpr std::uint32_t kNormalVtableBytes = 256;
inline constexpr std::uint32_t kNormalObjectStride = 216;

// SHA256 of the analyzed nostalgia.dll, NATIVE_TIMING.txt.
inline constexpr char kNostalgiaSha256[] =
    "339247b90b6de067ab0daa4355c7410487bae9b6f84ad61a4050700fa2d5638b";

// libxact provides the initial audio observations. The cue-label query is
// engine/source progress, not DAC position; it is still behind the existing
// module guard and the exact 44-byte QPC fix.
inline constexpr char kLibxactSha256[] =
    "6ceee6952cfa636fd96deaaf11bc72e7ca6d7b7499560ef15b033b18cb9bfb9e";
inline constexpr std::uint32_t kLibxactGetSoundLabelLapseRva = 0x7230;

inline constexpr std::uint32_t kNormalNotesVtableRva = 0x684328;

// 32 entries (256 bytes) read from .rdata; the table is read-only, so a live
// seam must not patch it in place without an explicit, reversible protection
// step. Values are module-relative RVAs (evidence printed VAs).
inline constexpr std::array<std::uint32_t, 32> kNormalVtableRvas{{
    0x0CA4D0, 0x2F73B0, 0x2F73A0, 0x2F6DF0, 0x2F6800, 0x2F78E0, 0x2F78F0, 0x2F7900,
    0x2F7980, 0x2F7990, 0x2F79A0, 0x2F79B0, 0x2F79C0, 0x2F79E0, 0x2F7AA0, 0x0A2BB0,
    0x0A2BC0, 0x0A2BD0, 0x0A2BE0, 0x0A2BF0, 0x0A2C00, 0x0A2C10, 0x2F7380, 0x0A2C20,
    0x0A2C30, 0x0A2C40, 0x0A2C50, 0x0A2C60, 0x0A2C70, 0x0A2C80, 0x0A2C90, 0x0A2CA0,
}};

struct VtableSlot final {
    const char* name;
    std::uint32_t byte_offset;
    std::uint32_t rva;
    const char* evidence;
};

inline constexpr std::array<VtableSlot, 12> kNormalVtableSlots{{
    {"judge", 0x20, 0x2F6800, "2F7C10 indirect call [rax+20h]; dispatcher 2F9548"},
    {"eligibility", 0x38, 0x2F7900, "2F7C10/2F6DF0 window union"},
    {"set_auto_slot", 0x60, 0x2F79C0, "2F7C10 writes note+96/+104"},
    {"rank_score", 0x68, 0x2F79E0, "2F6800 calls (rank,event,100)"},
    {"effect_grade", 0x70, 0x2F7AA0, "2F6800 calls with a1[52]"},
    {"note_time", 0x90, 0x0A2BE0, "2F7C10 reads note+24"},
    {"done", 0xB8, 0x0A2C20, "returns note+68"},
    {"auto_flag", 0xC0, 0x0A2C30, "returns chart/linked+97"},
    {"chart_pointer", 0xD0, 0x0A2C50, "2F7C10 reads chart+56"},
    {"post_judge", 0xD8, 0x0A2C60, "2F7C10 checks ==0 after judge"},
    {"pending_early", 0xE0, 0x0A2C70, "returns note+76"},
    {"set_event_state", 0xE8, 0x0A2C80, "writes note+84 unless already 1"},
}};
inline constexpr std::uint32_t kChartPrepareRva = 0x1A43E0;
inline constexpr std::uint32_t kNormalJudgeRva = 0x2F6800;
inline constexpr std::uint32_t kNormalEligibilityRva = 0x2F7900;
inline constexpr std::uint32_t kTraversalRva = 0x2F7C10;
inline constexpr std::uint32_t kNormalRenderRva = 0x2F6DF0;
inline constexpr std::uint32_t kGameplayUpdateRva = 0x2FF090;
inline constexpr std::uint32_t kFrameAccumulatorRva = 0x2FC9D0;
inline constexpr std::uint32_t kChaseControllerRva = 0x2FDB30;
inline constexpr std::uint32_t kLongJudgeRva = 0x2F4220;
inline constexpr std::uint32_t kGlissJudgeRva = 0x1A60C0;
inline constexpr std::uint32_t kTrillJudgeRva = 0x1A7AF0;
inline constexpr std::uint32_t kTrillEligibilityRva = 0x1A8490;
inline constexpr std::uint32_t kNonNormalEligibilityRva = 0x2F4E20;

// Judgment/frame accumulator globals named by the evidence. Evidence prints
// some as VAs (0x1813xxxxx); these constants are the module-relative RVAs.
inline constexpr std::uint32_t kMusicFrameRva = 0x70A610;
inline constexpr std::uint32_t kJudgmentOffsetRva = 0x136690C;
inline constexpr std::uint32_t kVisualOffsetRva = 0x1366918;
inline constexpr std::uint32_t kNormalAutoFlagRva = 0x136691C;
inline constexpr std::uint32_t kLegacyRateDoubleRva = 0x70B2F8;
inline constexpr std::uint32_t kLegacyRateFloatRva = 0x70B300;
inline constexpr std::uint32_t kLegacyRateIntRva = 0x70B304;

// 104-byte chart note record. NATIVE_TIMING.txt fixes +0/+4 (original integer
// milliseconds after speed adjustment) and +20/+24 (rounded legacy frames).
// The remaining offsets come from 0x1801A48FC..0x1801A4C21 and are marked for
// verification in REQUESTS.txt D1.
inline constexpr std::size_t kChartNoteBytes = 104;
inline constexpr std::size_t kChartNoteStartMs = 0;
inline constexpr std::size_t kChartNoteEndMs = 4;
inline constexpr std::size_t kChartNoteType = 8;
inline constexpr std::size_t kChartNoteFlags = 12;
inline constexpr std::size_t kChartNoteLane = 16;
inline constexpr std::size_t kChartNoteStartFrame = 20;
inline constexpr std::size_t kChartNoteEndFrame = 24;
inline constexpr std::size_t kChartNoteMidFrame = 28;
inline constexpr std::size_t kChartNoteSubCount = 36;
inline constexpr std::size_t kChartNoteSubPointer = 40;
inline constexpr std::size_t kChartNoteField48 = 48;
inline constexpr std::size_t kChartNoteMode = 52;
inline constexpr std::size_t kChartNoteRawId = 56;
inline constexpr std::size_t kChartNoteLeftKey = 88;
inline constexpr std::size_t kChartNoteRightKey = 92;
inline constexpr std::size_t kChartNoteAutoByte = 97;    // chart/linked flag
inline constexpr std::size_t kChartNoteEffectKind = 100; // 2F74FF a2[25]
inline constexpr std::size_t kChartNoteField104 = 104;   // unverified

// One scalar field of the NormalNotes object. `evidence` names the routine and
// the operation that proves the offset; sizes are the access widths observed.
struct Field final {
    const char* name;
    std::uint32_t offset;
    std::uint8_t size;
    const char* evidence;
};

inline constexpr std::array<Field, 24> kNormalNoteFields{{
    {"vtable", 0, 8, "2F6800 compares *note to 684328; 2F6DF0 too"},
    {"chart_pointer", 8, 8, "2F73E2 copies chart record; 2F6800 reads +80/+84/+96/+97"},
    {"state_word", 16, 2, "2F73C3 initializes word to 1; meaning unverified"},
    {"lane", 20, 4, "2F73D5 copies chart+16; 2F6DF0 passes it to 305430"},
    {"note_time_units", 24, 4, "2F73E6 copies chart+20 legacy frame; judge a1[6]"},
    {"left_key", 44, 4, "2F740F copies chart+88; projection arg 9"},
    {"right_key", 48, 4, "2F7415 copies chart+92; projection arg 10"},
    {"mid_key", 52, 4, "2F7438 (left+right)/2; 2F6800 default when event null"},
    {"mid_key_float", 56, 4, "2F7451 float (left+right)*0.5"},
    {"key_span", 64, 4, "2F74FC chart+92 - chart+88"},
    {"done_byte", 68, 1, "2F6800 sets on completion; vtable+0xB8 returns it"},
    {"pending_early_byte", 76, 1, "2F6800 sets for 0<delta<=8F0; vtable+0xE0"},
    {"pending_rank", 80, 4, "2F6800 a1[20] = 3 + (rank==Just)"},
    {"event_state", 84, 4, "0A2C80 writes unless 1; called via vtable+0xE8"},
    {"effect_kind", 88, 4, "2F73F2 sets -1; 2F7502 copies chart+100"},
    {"auto_slot_pointer", 96, 8, "2F6800 v8==0 branch; vtable+0x60 sets it"},
    {"auto_slot_timer", 104, 4, "2F6800 a1[26]; written with the auto slot"},
    {"flag_108", 108, 1, "2F73FD clears; 2F6800 sets on completion"},
    {"default_id_112", 112, 4, "2F7401 initializes to -1"},
    {"flag_116", 116, 1, "2F6800 sets on auto completion"},
    {"rank_result", 120, 4, "2F6800 a1[30] = rank; rank/score call arg 2"},
    {"mode_byte", 124, 1, "2F6800 byte 124 = call mode"},
    {"render_state", 128, 0, "sub_1800D2A70(note+128); subobject, size unverified"},
    {"init_qword_204", 204, 8, "2F7604 initializes to 1"},
}};

inline constexpr std::array<Field, 14> kChartNoteFields{{
    {"start_ms", 0, 4, "1A4907 stores the speed-adjusted integer ms"},
    {"end_ms", 4, 4, "1A4912 stores the speed-adjusted integer ms"},
    {"type", 8, 4, "1A49B7 / 1A4C0E / 1A4C21 from flags"},
    {"flags", 12, 4, "1A4921 copies source flags"},
    {"lane", 16, 2, "1A4BF6 word r14[10h]; 2F73D5 copies chart+16 to note+20"},
    {"start_frame", 20, 4, "1A4945 rounds start_ms at 60"},
    {"end_frame", 24, 4, "1A496A rounds end_ms at 60"},
    {"mid_frame", 28, 4, "1A498E"},
    {"sub_count", 36, 4, "1A4A8C r14[28h]"},
    {"sub_pointer", 40, 8, "1A4AF5 allocates sub_count*24"},
    {"field_48", 48, 4, "1A499D r14[20h]; semantics unverified"},
    {"mode", 52, 4, "1A49AC r14[24h]; 1 clamps key fields"},
    {"left_key", 88, 4, "1A49E2 r14[18h]; 2F7415 to note+44"},
    {"right_key", 92, 4, "1A49D3 r14[14h]; 2F740F to note+48"},
}};

// Decodes only caller-owned 104-byte records through the evidenced offsets. It
// does not dereference record+40 or infer a note kind. `auto_flag` is the
// chart/linked byte 0x61 used by vtable+0xC0 (0A2C30); +97 is not the note.
struct ChartNoteView final {
    std::int32_t start_ms = 0;
    std::int32_t end_ms = 0;
    std::int32_t start_frame = 0;
    std::int32_t end_frame = 0;
    std::int32_t lane = 0;       // int16 at +16, widened
    std::int32_t left_key = 0;   // +88
    std::int32_t right_key = 0;  // +92
    std::int32_t mode = 0;       // +52
    std::int32_t effect_kind = 0; // +100
    bool auto_flag = false;      // +97
};

[[nodiscard]] bool DecodeChartNote(std::span<const std::byte> record,
                                   ChartNoteView& out) noexcept;

// Targets whose exact 32-byte prefixes are now exported (native_gameplay_guards
// .json, 2026-10-29). A live seam must compare all 32 bytes at the target RVA
// before changing anything. `kAbiAndMatchingVerified` stays false because the
// 2F7C10 selection/303BA0 path and completion side effects are not yet fully
// decoded, so readiness remains WaitingForEvidence.
struct Target final {
    const char* name;
    std::uint32_t rva;
    bool signature_known;
};

inline constexpr std::array<Target, 13> kTimingTargets{{
    {"chart-prepare", kChartPrepareRva, true},
    {"normal-judge", kNormalJudgeRva, true},
    {"normal-eligibility", kNormalEligibilityRva, true},
    {"traversal", kTraversalRva, true},
    {"normal-render", kNormalRenderRva, true},
    {"gameplay-update", kGameplayUpdateRva, true},
    {"frame-accumulator", kFrameAccumulatorRva, true},
    {"chase-controller", kChaseControllerRva, true},
    {"long-judge", kLongJudgeRva, true},
    {"gliss-judge", kGlissJudgeRva, true},
    {"trill-judge", kTrillJudgeRva, true},
    {"trill-eligibility", kTrillEligibilityRva, true},
    {"non-normal-eligibility", kNonNormalEligibilityRva, true},
}};

inline constexpr std::array<std::array<std::uint8_t, 32>, 13> kTargetPrefixes{{
    {{0x48,0x8B,0xC4,0x57,0x41,0x54,0x41,0x55,0x41,0x56,0x41,0x57,0x48,0x81,0xEC,0xF0,
      0x00,0x00,0x00,0x48,0xC7,0x44,0x24,0x78,0xFE,0xFF,0xFF,0xFF,0x48,0x89,0x58,0x18}},
    {{0x48,0x89,0x5C,0x24,0x08,0x48,0x89,0x6C,0x24,0x10,0x48,0x89,0x74,0x24,0x18,0x57,
      0x41,0x54,0x41,0x55,0x41,0x56,0x41,0x57,0x48,0x83,0xEC,0x60,0x8B,0x79,0x18,0x45}},
    {{0x8B,0x41,0x18,0x2B,0x05,0x07,0x2D,0x41,0x00,0x2B,0x05,0x09,0xF0,0x06,0x01,0x2B,
      0x05,0xF7,0xEF,0x06,0x01,0x39,0x05,0xDD,0xEF,0x06,0x01,0x7C,0x14,0x3B,0x05,0xD9}},
    {{0x48,0x8B,0xC4,0x55,0x41,0x54,0x41,0x55,0x41,0x56,0x41,0x57,0x48,0x8D,0xA8,0x08,
      0xFE,0xFF,0xFF,0x48,0x81,0xEC,0xD0,0x02,0x00,0x00,0x48,0xC7,0x45,0x20,0xFE,0xFF}},
    {{0x48,0x89,0x5C,0x24,0x20,0x55,0x56,0x41,0x54,0x41,0x56,0x41,0x57,0x48,0x8D,0x6C,
      0x24,0xE0,0x48,0x81,0xEC,0x20,0x01,0x00,0x00,0x48,0x8B,0x05,0x20,0x44,0x40,0x00}},
    {{0x40,0x56,0x57,0x48,0x83,0xEC,0x48,0x48,0x89,0x5C,0x24,0x68,0x48,0x8B,0xF9,0x0F,
      0x29,0x74,0x24,0x30,0xE8,0x77,0x83,0x00,0x00,0x48,0x8B,0xCF,0x0F,0xB7,0x70,0x18}},
    {{0x44,0x8B,0x19,0x4C,0x63,0x81,0x3C,0x01,0x00,0x00,0x41,0x8D,0x53,0x01,0x89,0x11,
      0x44,0x3B,0x81,0x38,0x01,0x00,0x00,0x7D,0x39,0x4C,0x8B,0x89,0x48,0x01,0x00,0x00}},
    {{0x48,0x8B,0xC4,0x48,0x89,0x48,0x08,0x55,0x41,0x57,0x48,0x81,0xEC,0xB8,0x00,0x00,
      0x00,0x48,0x89,0x58,0x10,0x48,0x8B,0xE9,0x48,0x89,0x70,0x18,0x48,0x89,0x78,0x20}},
    {{0x44,0x88,0x44,0x24,0x18,0x48,0x89,0x54,0x24,0x10,0x55,0x56,0x57,0x41,0x54,0x41,
      0x55,0x41,0x56,0x48,0x81,0xEC,0x88,0x00,0x00,0x00,0x48,0x8B,0x01,0x4C,0x8D,0x15}},
    {{0x44,0x88,0x44,0x24,0x18,0x48,0x89,0x54,0x24,0x10,0x55,0x56,0x57,0x41,0x56,0x41,
      0x57,0x48,0x81,0xEC,0x90,0x00,0x00,0x00,0x44,0x8B,0x71,0x18,0x45,0x0F,0xB6,0xF8}},
    {{0x44,0x88,0x44,0x24,0x18,0x55,0x56,0x41,0x54,0x41,0x55,0x41,0x56,0x41,0x57,0x48,
      0x83,0xEC,0x68,0x48,0x8B,0x01,0x4C,0x8D,0x25,0x03,0x21,0x4D,0x00,0x45,0x32,0xED}},
    {{0x83,0xB9,0x3C,0x01,0x00,0x00,0x01,0x75,0x03,0xB0,0x01,0xC3,0x8B,0x41,0x18,0x2B,
      0x05,0x6B,0x21,0x56,0x00,0x2B,0x05,0x61,0xE4,0x1B,0x01,0x39,0x05,0x47,0xE4,0x1B}},
    {{0x83,0xB9,0x2C,0x01,0x00,0x00,0x01,0x75,0x03,0xB0,0x01,0xC3,0x8B,0x41,0x18,0x2B,
      0x05,0xDB,0x57,0x41,0x00,0x2B,0x05,0xD1,0x1A,0x07,0x01,0x39,0x05,0xB7,0x1A,0x07}},
}};

// Explicitly false: the normal-note event matching (2F7C10 -> 303BA0 /
// 303C10 / 303B50), all completion side effects, and the note+96 auto slot are
// not fully decoded. No seam may install while this is false.
inline constexpr bool kAbiAndMatchingVerified = false;

static_assert(kNormalVtableRvas.size() * 8 == kNormalVtableBytes);
static_assert(kTargetPrefixes.size() == kTimingTargets.size());
static_assert(kChartNoteStartMs + 4 <= kChartNoteBytes);
static_assert(kChartNoteEndMs + 4 <= kChartNoteBytes);
static_assert(kChartNoteLane + 2 <= kChartNoteBytes);
static_assert(kChartNoteEffectKind + 4 <= kChartNoteBytes);
static_assert(kChartNoteAutoByte < kChartNoteBytes);

// Milliseconds -> microseconds for the original chart fields. The conversion is
// exact in double for integer milliseconds.
[[nodiscard]] constexpr double ChartMsToUs(std::int32_t milliseconds) noexcept {
    return static_cast<double>(milliseconds) * 1000.0;
}

// 0x13668E0..0x13668FC initial values proven from the image (0CB9A0 init at
// CC120..CC166): E0=9, E4=-11, E8=7, EC=-6, F0=4, F4=-4, F8=2, FC=-2.
// These are SEEDS, not live gameplay values. Read and validate the eight
// globals after initialization; never hardcode this table as permanent.
inline constexpr std::uint32_t kEffectBandEarlyRva = 0x13668E0;
inline constexpr std::uint32_t kEffectBandLateRva = 0x13668E4;

struct NativeWindowUnits final {
    std::int32_t effect_early = 0;  // 0x13668E0 (grade-1 outer threshold)
    std::int32_t effect_late = 0;   // 0x13668E4 (grade-2 outer threshold)
    std::int32_t good_earliest = 0; // 0x13668E8
    std::int32_t good_latest = 0;   // 0x13668EC
    std::int32_t great_earliest = 0; // 0x13668F0
    std::int32_t great_latest = 0;   // 0x13668F4
    std::int32_t just_earliest = 0;  // 0x13668F8
    std::int32_t just_latest = 0;    // 0x13668FC
};

inline constexpr NativeWindowUnits kInitialWindowSeeds{9, -11, 7, -6, 4, -4, 2, -2};

enum class WindowSampleStatus : std::uint8_t {
    Ok = 0,
    Unavailable,           // image/span too small
    InvalidRate,           // units_per_second not finite/positive
    InvalidUnits,          // sign/nesting/convert failure
    InconsistentEffectBand // E0 below good_earliest or E4 above good_latest
};

struct SampledNormalWindows final {
    WindowSampleStatus status = WindowSampleStatus::Unavailable;
    NativeWindowUnits units{};
    NormalWindows windows{};
    bool matches_initial_seed = false;
};

// Validates and converts one runtime sample of the eight globals. The caller
// supplies the active legacy units-per-second; 60 is never assumed here.
[[nodiscard]] SampledNormalWindows ValidateNormalWindowUnits(
    const NativeWindowUnits& units, double units_per_second) noexcept;

// Reads the eight int32 globals from a caller-owned module image span. No live
// pointer is dereferenced by this module; the integration passes the verified
// image. Returns Unavailable when the span does not cover 0x13668FC.
[[nodiscard]] SampledNormalWindows ReadNormalWindowUnits(
    std::span<const std::byte> image, double units_per_second) noexcept;

} // namespace native_layout

// ===========================================================================
// Note specification and lifecycle
// ===========================================================================

inline constexpr std::uint8_t kNativeNoteLaneCount = 28;
inline constexpr std::size_t kNativeMaxNotes = 65'536;
inline constexpr double kNativeMaximumTimeUs = 1.0e12;

enum class NoteKind : std::uint8_t {
    Normal = 0, // 0x2F6800; the only supported migration
    Long = 1,   // 0x2F4220, unsupported
    Gliss = 2,  // 0x1A60C0, unsupported
    Trill = 3,  // 0x1A7AF0, unsupported
};

[[nodiscard]] const char* NoteKindName(NoteKind kind) noexcept;

// Selection is an explicit adapter policy, not a claim about 0x2F7C10.
// The full traversal uses sub_180303BA0/303C10/303B50 and per-event state
// (v563/v558) that are not yet decoded; Native2F7C10 is rejected until they
// are. See REQUESTS.txt.
enum class NoteSelectionPolicy : std::uint8_t {
    AdapterEarliestPending = 0,
    Native2F7C10 = 1,
};

struct NativeNoteSpec final {
    double start_us = 0.0; // chart record +0 ms -> us; judgment time
    double end_us = 0.0;   // chart record +4 ms -> us; >= start_us
    std::uint32_t chart_index = 0; // stable chart order / tie-break, unique
    std::uint8_t lane = 0;
    NoteKind kind = NoteKind::Normal;
    bool auto_play = false; // native global auto flag / chart auto byte
    std::uint32_t sound_id = 0; // opaque host keysound id; 0 starts none
};

struct NativeNoteConfig final {
    // Convert the native 0x13668E8..0x13668FC integer song units with
    // ConvertLegacyNormalWindows(..., units_per_second); never assume 60 here.
    // The static image block is zero/uninitialized: sample the live table after
    // game initialization and set windows_sampled only after validation.
    NormalWindows windows{};
    double judgment_offset_us = 0.0;           // global RVA 0x136690C
    double eligibility_total_offset_us = 0.0;  // judgment + visual (0x1366918)
    double units_per_second = 60.0;            // explicit legacy unit rate
    bool windows_sampled = false;              // runtime values, not image zeros
    NoteSelectionPolicy selection = NoteSelectionPolicy::AdapterEarliestPending;
};

enum class NoteConfigStatus : std::uint8_t {
    Accepted = 0,
    TooManyNotes,
    NonFiniteTime,
    InvalidTimeRange,
    InvalidLane,
    UnsupportedKind,
    DuplicateChartIndex,
    InvalidWindows,
    WindowsNotSampled,    // runtime window values were not supplied
    DegenerateWindows,    // all six widths are zero; almost certainly unsampled
    InvalidOffset,
    InvalidRate,
    UnsupportedSelection // Native2F7C10 matching is not implemented/evidenced
};

[[nodiscard]] const char* NoteConfigStatusName(NoteConfigStatus status) noexcept;



enum class NativeNotePhase : std::uint8_t {
    Idle = 0,       // configured but no playback segment started
    Skipped = 1,    // note time was before the playback start; never scored
    Pending = 2,    // awaiting a press, auto update or miss
    PendingEarly = 3, // early press recorded, awaiting the note time
    Completed = 4,  // scored or auto-completed; never re-judged
    Missed = 5,     // expired after the inclusive Good-late deadline
};

enum class NoteOutcomeKind : std::uint8_t {
    Scored = 0,     // press captured inside a scoring window
    Auto = 1,       // auto-play completion at the note time, rank Just
    Missed = 2,     // expired after the deadline
    KeysoundCut = 3, // release matched an active lane sound; no judgment
};

// One externally observable effect. The native seam forwards `rank` to the
// judge virtual (vtable+104) with the event id and scale 100, and `effect_grade`
// to vtable+112; `sound_id` selects the keysound command. `at_us` is the song
// time of the decision (captured press, note crossing or miss deadline), never
// the render tick that happened to observe it.
struct NativeNoteOutcome final {
    std::uint32_t note_index = 0;
    std::uint8_t lane = 0;
    NoteOutcomeKind kind = NoteOutcomeKind::Scored;
    NormalRank rank = NormalRank::TooEarly;
    std::uint8_t effect_grade = 0; // 0 Just, 1 early non-Just, 2 late non-Just
    std::uint32_t sound_id = 0;
    double at_us = 0.0;
    double input_us = 0.0; // captured press time for Scored; 0 otherwise
};

struct NoteAdvance final {
    std::size_t written = 0;
    std::size_t scored = 0;
    std::size_t auto_completed = 0;
    std::size_t missed = 0;
    std::size_t cuts = 0;
    std::size_t ignored_presses = 0;
    std::size_t ignored_releases = 0;
    std::size_t early_deferred = 0;
    std::size_t early_overwritten = 0;
};

enum class NoteRunStatus : std::uint8_t {
    Ok = 0,
    NotConfigured,
    NotStarted,
    GenerationZero,
    GenerationNotNew,
    NonFiniteTime,
    BackwardTime,
    GenerationMismatch,
    WatermarkBehindCapture, // a press was applied before it was certified
    WatermarkAhead,         // decision time later than the update time
    InsufficientCapacity,
};

[[nodiscard]] const char* NoteRunStatusName(NoteRunStatus status) noexcept;

class NativeNoteState final {
public:
    NativeNoteState() = default;

    // Copies and validates the complete note table. Windows/offsets/rate are
    // validated once. A rejected table leaves the previous configuration and
    // playback state untouched. The generation floor is retained across chart
    // reload, so an old playback generation can never become valid again.
    [[nodiscard]] NoteConfigStatus Configure(std::span<const NativeNoteSpec> notes,
                                             const NativeNoteConfig& config);

    // Starts a playback segment. generation must be non-zero and above the
    // lifetime floor. Notes whose start is before start_us are Skipped and can
    // never score. Rejected calls change nothing.
    [[nodiscard]] NoteRunStatus Begin(std::uint64_t generation, double start_us) noexcept;

    // Null-event / time update: resolves pending-early notes whose note time has
    // been reached, auto-completes auto notes, and finalizes misses whose
    // inclusive deadline is strictly before both up_to_us and decision_us.
    // decision_us is the certified processed-input-through song time. up_to_us
    // must not be earlier than the previous accepted update.
    //
    // out.size() must be at least MaxOutcomesPerAdvance() (the note count);
    // otherwise the call is a complete no-op with InsufficientCapacity. This
    // keeps every effect transactional and lets the caller preallocate once.
    [[nodiscard]] NoteRunStatus Advance(double up_to_us, double decision_us,
                                        std::span<NativeNoteOutcome> out,
                                        NoteAdvance& counts) noexcept;

    // Applies one captured logical edge. Press: advances note crossings to the
    // captured time first (so an earlier pending note is resolved in time
    // order), selects the earliest pending note in the lane whose eligibility
    // is true at the captured time, then ranks it. Release: cuts the active lane
    // keysound, never scores. A press from another generation is rejected.
    [[nodiscard]] NoteRunStatus Press(const TimedInputEvent& event, double decision_us,
                                      std::span<NativeNoteOutcome> out,
                                      NoteAdvance& counts) noexcept;
    [[nodiscard]] NoteRunStatus Release(const TimedInputEvent& event,
                                        std::span<NativeNoteOutcome> out,
                                        NoteAdvance& counts) noexcept;

    // ---- inspection ----
    [[nodiscard]] bool IsConfigured() const noexcept { return configured_; }
    [[nodiscard]] bool IsStarted() const noexcept { return started_; }
    [[nodiscard]] std::size_t NoteCount() const noexcept { return specs_.size(); }
    [[nodiscard]] std::size_t MaxOutcomesPerAdvance() const noexcept { return specs_.size(); }
    [[nodiscard]] std::uint64_t Generation() const noexcept { return generation_; }
    [[nodiscard]] std::uint64_t LastBeginGeneration() const noexcept {
        return last_begin_generation_;
    }
    [[nodiscard]] NativeNotePhase Phase(std::size_t index) const noexcept;
    [[nodiscard]] NormalRank FinalRank(std::size_t index) const noexcept;
    [[nodiscard]] std::size_t PendingCount() const noexcept;
    [[nodiscard]] std::size_t CompletedCount() const noexcept;
    [[nodiscard]] std::size_t MissedCount() const noexcept;
    [[nodiscard]] std::size_t SkippedCount() const noexcept;
    [[nodiscard]] std::uint32_t ActiveSound(std::uint8_t lane) const noexcept;

private:
    struct Due final {
        double at_us;
        std::uint32_t chart_index;
        std::uint32_t note;
        NoteOutcomeKind kind;
        NormalRank rank;
        std::uint8_t grade;
        double input_us;
    };

    struct Runtime final {
        NativeNotePhase phase = NativeNotePhase::Idle;
        NormalRank rank = NormalRank::TooEarly;
        NormalRank pending_rank = NormalRank::TooEarly;
        std::uint8_t pending_grade = 0;
        double pending_input_us = 0.0;
        double deadline_us = 0.0;   // note - judgment + good_late
        double resolve_us = 0.0;    // note - judgment
        double auto_us = 0.0;       // note - judgment - one legacy unit
    };

    [[nodiscard]] NoteRunStatus AdvanceLocked(double up_to_us, double decision_us,
                                              std::span<NativeNoteOutcome> out,
                                              NoteAdvance& counts) noexcept;
    [[nodiscard]] int SelectPending(std::uint8_t lane, const SongTime& captured) const noexcept;
    void Emit(const Due& due, std::span<NativeNoteOutcome> out, NoteAdvance& counts) noexcept;
    void Complete(std::size_t index, NormalRank rank, NoteOutcomeKind kind,
                  NoteAdvance& counts) noexcept;
    void Miss(std::size_t index, NoteAdvance& counts) noexcept;
    void ResetRuntime() noexcept;
    [[nodiscard]] bool CapacityOk(std::span<NativeNoteOutcome> out) const noexcept {
        return out.size() >= specs_.size();
    }

    std::vector<NativeNoteSpec> specs_{};
    std::vector<Runtime> runtime_{};
    std::vector<std::uint32_t> order_{}; // by (start_us, chart_index)
    std::vector<Due> due_{};             // preallocated scratch
    NativeNoteConfig config_{};
    std::array<std::uint32_t, kNativeNoteLaneCount> active_sound_{};
    std::array<std::uint32_t, kNativeNoteLaneCount> active_sound_note_{};
    std::uint64_t generation_ = 0;
    std::uint64_t last_begin_generation_ = 0;
    double advanced_us_ = 0.0;
    bool configured_ = false;
    bool started_ = false;
};

} // namespace nosfix
