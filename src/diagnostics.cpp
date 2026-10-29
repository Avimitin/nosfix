#include "nosfix/diagnostics.hpp"

#include "nosfix/log.hpp"

#include <Windows.h>

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace nosfix {
namespace {

std::atomic<bool> g_enabled{false};
std::uint64_t g_judge_before = 0;
std::uint64_t g_judge_after = 0;
std::uint64_t g_motion_gliss = 0;
std::uint64_t g_motion_velocity = 0;
constexpr std::uint64_t kJudgeLimit = 500;
constexpr std::uint64_t kMotionLimit = 400;
constexpr std::uint64_t kStateDiffLimit = 400;
std::array<std::byte, 784> g_previous_state{};
bool g_has_previous_state = false;
std::uint64_t g_state_diff_count = 0;

bool Readable(const void* address, std::size_t bytes) noexcept {
    if (!address || bytes == 0) return false;
    MEMORY_BASIC_INFORMATION info{};
    if (VirtualQuery(address, &info, sizeof(info)) == 0) return false;
    if (info.State != MEM_COMMIT) return false;
    if ((info.Protect & (PAGE_NOACCESS | PAGE_GUARD)) != 0) return false;
    const auto start = reinterpret_cast<std::uintptr_t>(address);
    const auto end = start + bytes;
    const auto region_end = reinterpret_cast<std::uintptr_t>(info.BaseAddress) +
                            info.RegionSize;
    return end >= start && end <= region_end;
}

template <class T>
T Read(const void* base, std::size_t offset) noexcept {
    T value{};
    if (!base) return value;
    std::memcpy(&value, static_cast<const std::byte*>(base) + offset, sizeof(T));
    return value;
}

} // namespace

void ConfigureDiagnosticProbe(bool enabled) noexcept {
    g_enabled.store(enabled, std::memory_order_release);
    g_judge_before = 0;
    g_judge_after = 0;
    g_motion_gliss = 0;
    g_motion_velocity = 0;
}

bool DiagnosticProbeEnabled() noexcept {
    return g_enabled.load(std::memory_order_acquire);
}

void ProbeNormalJudgeBefore(const void* note, const void* event,
                            unsigned mode) noexcept {
    if (!DiagnosticProbeEnabled() || !Readable(note, 0x60) || !event) return;
    if (!Readable(event, 0x1C)) return;
    if (++g_judge_before > kJudgeLimit) return;

    const auto* chart = Read<const std::byte*>(note, 8);
    if (chart && !Readable(chart, 0x68)) chart = nullptr;
    const int event_id = Read<std::int32_t>(event, 0);
    const int event_kind = Read<std::int32_t>(event, 4);
    const int event_value = Read<std::uint8_t>(event, 8);
    const int event_group = Read<std::int32_t>(event, 0x14);
    const int event_repeat = Read<std::int32_t>(event, 0x18);
    char line[640]{};
    std::snprintf(line, sizeof(line),
        "PROBE judge before note=%p slot=%p mode=%u frame=%d note_frame=%d "
        "lane=%d n52=%d id=%d r19=%d r20=%d chart=%p chart_ms=%d chart_end=%d "
        "chart8=%d chart56=%d chart88=%d chart92=%d chart97=%u chart100=%d "
        "slot_key=%d slot_state=%d slot_value=%d slot_group=%d slot_repeat=%d",
        note, event, mode, Read<std::int32_t>(note, 0x18),
        Read<std::int32_t>(note, 24), Read<std::int32_t>(note, 20),
        Read<std::int32_t>(note, 52), Read<std::int32_t>(note, 84),
        Read<std::int32_t>(note, 76), Read<std::int32_t>(note, 80),
        static_cast<const void*>(chart),
        chart ? Read<std::int32_t>(chart, 0) : -1,
        chart ? Read<std::int32_t>(chart, 4) : -1,
        chart ? Read<std::int32_t>(chart, 8) : -1,
        chart ? Read<std::int32_t>(chart, 56) : -1,
        chart ? Read<std::int32_t>(chart, 88) : -1,
        chart ? Read<std::int32_t>(chart, 92) : -1,
        chart ? Read<std::uint8_t>(chart, 97) : 0,
        chart ? Read<std::int32_t>(chart, 100) : -1,
        event_id, event_kind, event_value, event_group, event_repeat);
    Log(LogLevel::Info, line);
}

void ProbeNormalJudgeAfter(const void* note, const void* event,
                           unsigned mode, std::int64_t result) noexcept {
    if (!DiagnosticProbeEnabled() || !Readable(note, 0x60) || !event) return;
    if (++g_judge_after > kJudgeLimit) return;

    char line[384]{};
    std::snprintf(line, sizeof(line),
        "PROBE judge after note=%p slot=%p mode=%u result=%lld id=%d r19=%d "
        "r20=%d frame=%d",
        note, event, mode, static_cast<long long>(result),
        Read<std::int32_t>(note, 84), Read<std::int32_t>(note, 76),
        Read<std::int32_t>(note, 80), Read<std::int32_t>(note, 0x18));
    Log(LogLevel::Info, line);
}

const char* VtableName(std::uintptr_t rva) noexcept {
    switch (rva) {
    case 0x684328: return "normal";
    case 0x6841D0: return "long";
    case 0x679960: return "beats";
    case 0x679AB8: return "gliss";
    case 0x679C10: return "trill";
    case 0x679D68: return "velocity";
    default: return "other";
    }
}

void ProbeMotionNote(const void* note, int kind, std::int32_t frame,
                     const void* module_base) noexcept {
    if (!DiagnosticProbeEnabled() || !Readable(note, 0x140)) return;
    if (kind != 3 && kind != 5) return; // caller's gliss/velocity labels
    std::uint64_t& counter = kind == 3 ? g_motion_gliss : g_motion_velocity;
    if (++counter > kMotionLimit) return;

    const auto* chart = Read<const std::byte*>(note, 8);
    if (chart && !Readable(chart, 0xA0)) chart = nullptr;
    const void* vtable = Read<const void*>(note, 0);
    const auto rva = reinterpret_cast<std::uintptr_t>(vtable) -
                     reinterpret_cast<std::uintptr_t>(module_base);
    char line[640]{};
    std::snprintf(line, sizeof(line),
        "PROBE motion kind=%d note=%p vtable=%p rva=0x%llX vt=%s "
        "frame=%d note_frame=%d lane=%d n52=%d chart=%p chart_ms=%d "
        "chart_end=%d chart8=%d chart56=%d chart88=%d chart92=%d "
        "chart97=%u chart100=%d note32=%d note296=%d note300=%d "
        "note316=%d chart50=%d",
        kind, note, vtable, static_cast<unsigned long long>(rva),
        VtableName(rva), frame, Read<std::int32_t>(note, 24),
        Read<std::int32_t>(note, 20), Read<std::int32_t>(note, 52),
        static_cast<const void*>(chart),
        chart ? Read<std::int32_t>(chart, 0) : -1,
        chart ? Read<std::int32_t>(chart, 4) : -1,
        chart ? Read<std::int32_t>(chart, 8) : -1,
        chart ? Read<std::int32_t>(chart, 56) : -1,
        chart ? Read<std::int32_t>(chart, 88) : -1,
        chart ? Read<std::int32_t>(chart, 92) : -1,
        chart ? Read<std::uint8_t>(chart, 97) : 0,
        chart ? Read<std::int32_t>(chart, 100) : -1,
        Read<std::int32_t>(note, 32), Read<std::int32_t>(note, 296),
        Read<std::int32_t>(note, 300), Read<std::int32_t>(note, 316),
        chart ? Read<std::int32_t>(chart, 0x50) : -1);
    Log(LogLevel::Info, line);
}

void ProbeInputStateDiff(const void* module_base) noexcept {
    if (!DiagnosticProbeEnabled() || !module_base) return;
    const auto* staging = Read<const std::byte*>(module_base, 0x13DEA10);
    if (!Readable(staging, 784)) return;

    std::array<std::byte, 784> current{};
    std::memcpy(current.data(), staging, current.size());
    if (!g_has_previous_state) {
        g_previous_state = current;
        g_has_previous_state = true;
        return;
    }
    if (std::memcmp(current.data(), g_previous_state.data(), current.size()) == 0)
        return;

    for (std::size_t slot = 0; slot < 28; ++slot) {
        const std::size_t offset = slot * 28;
        if (std::memcmp(current.data() + offset,
                        g_previous_state.data() + offset, 28) == 0) {
            continue;
        }
        if (++g_state_diff_count > kStateDiffLimit) break;
        char line[320]{};
        std::snprintf(line, sizeof(line),
            "PROBE input slot=%zu key=%d state=%d value=%u group=%d repeat=%d "
            "frame=%d",
            slot, Read<std::int32_t>(staging, offset + 0),
            Read<std::int32_t>(staging, offset + 4),
            Read<std::uint8_t>(staging, offset + 8),
            Read<std::int32_t>(staging, offset + 0x14),
            Read<std::int32_t>(staging, offset + 0x18),
            Read<std::int32_t>(module_base, 0x70A610));
        Log(LogLevel::Info, line);
    }
    g_previous_state = current;
}

} // namespace nosfix
