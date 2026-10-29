#include "nosfix/diagnostics.hpp"

#include "nosfix/log.hpp"

#include <Windows.h>

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
constexpr std::uint64_t kJudgeLimit = 400;
constexpr std::uint64_t kMotionLimit = 400;

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
    if (!DiagnosticProbeEnabled() || !Readable(note, 0x60)) return;
    if (++g_judge_before > kJudgeLimit) return;

    const auto* chart = Read<const std::byte*>(note, 8);
    if (chart && !Readable(chart, 0x68)) chart = nullptr;
    if (event && !Readable(event, 0x24)) event = nullptr;
    const int event_id = event ? Read<std::int32_t>(event, 0) : -1;
    const int event_kind = event ? Read<std::int32_t>(event, 4) : -1;
    const int event_group = event ? Read<std::int32_t>(event, 0x14) : -1;
    const int event_value = event ? Read<std::int32_t>(event, 0x20) : -1;
    char line[512]{};
    std::snprintf(line, sizeof(line),
        "PROBE judge before note=%p event=%p mode=%u frame=%d note_frame=%d "
        "lane=%d id=%d r19=%d r20=%d chart=%p chart_ms=%d chart_end=%d "
        "event_id=%d event_kind=%d event_group=%d event_value=%d",
        note, event, mode, Read<std::int32_t>(note, 0x18),
        Read<std::int32_t>(note, 24), Read<std::int32_t>(note, 20),
        Read<std::int32_t>(note, 84), Read<std::int32_t>(note, 76),
        Read<std::int32_t>(note, 80), static_cast<const void*>(chart),
        chart ? Read<std::int32_t>(chart, 0) : -1,
        chart ? Read<std::int32_t>(chart, 4) : -1,
        event_id, event_kind, event_group, event_value);
    Log(LogLevel::Info, line);
}

void ProbeNormalJudgeAfter(const void* note, const void* event,
                           unsigned mode, std::int64_t result) noexcept {
    if (!DiagnosticProbeEnabled() || !Readable(note, 0x60)) return;
    if (++g_judge_after > kJudgeLimit) return;

    char line[384]{};
    std::snprintf(line, sizeof(line),
        "PROBE judge after note=%p event=%p mode=%u result=%lld id=%d r19=%d "
        "r20=%d frame=%d",
        note, event, mode, static_cast<long long>(result),
        Read<std::int32_t>(note, 84), Read<std::int32_t>(note, 76),
        Read<std::int32_t>(note, 80), Read<std::int32_t>(note, 0x18));
    Log(LogLevel::Info, line);
}

void ProbeMotionNote(const void* note, int kind, std::int32_t frame) noexcept {
    if (!DiagnosticProbeEnabled() || !Readable(note, 0x140)) return;
    if (kind != 3 && kind != 5) return; // glissando, velocity zone
    std::uint64_t& counter = kind == 3 ? g_motion_gliss : g_motion_velocity;
    if (++counter > kMotionLimit) return;

    const auto* chart = Read<const std::byte*>(note, 8);
    if (chart && !Readable(chart, 0xA0)) chart = nullptr;
    const int event_kind = kind == 3 ? 3 : 5;
    char line[512]{};
    std::snprintf(line, sizeof(line),
        "PROBE motion kind=%d(%s) note=%p frame=%d note_frame=%d lane=%d "
        "chart=%p chart_ms=%d chart_end=%d chart8=%d chart56=%d chart97=%d "
        "note32=%d note296=%d note300=%d note316=%d chart50=%d",
        event_kind, kind == 3 ? "gliss" : "velocity", note, frame,
        Read<std::int32_t>(note, 24), Read<std::int32_t>(note, 20),
        static_cast<const void*>(chart),
        chart ? Read<std::int32_t>(chart, 0) : -1,
        chart ? Read<std::int32_t>(chart, 4) : -1,
        chart ? Read<std::int32_t>(chart, 8) : -1,
        chart ? Read<std::int32_t>(chart, 56) : -1,
        chart ? Read<std::int32_t>(chart, 97) : -1,
        Read<std::int32_t>(note, 32), Read<std::int32_t>(note, 296),
        Read<std::int32_t>(note, 300), Read<std::int32_t>(note, 316),
        chart ? Read<std::int32_t>(chart, 0x50) : -1);
    Log(LogLevel::Info, line);
}

} // namespace nosfix
