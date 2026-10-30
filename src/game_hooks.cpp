#include "nosfix/game_hooks.hpp"
#include "nosfix/audio_clock_patch.hpp"

#include "nosfix/asio_route.hpp"
#include "nosfix/config.hpp"
#include "nosfix/call_patch.hpp"
#include "nosfix/diagnostics.hpp"
#include "nosfix/hooks.hpp"
#include "nosfix/log.hpp"
#include "nosfix/module_guard.hpp"
#include "nosfix/motion_hooks.hpp"
#include "nosfix/monitor_check.hpp"
#include "nosfix/native_timeline_patch.hpp"
#include "nosfix/native_timeline_runtime.hpp"
#include "nosfix/presentation.hpp"
#include "nosfix/render_tail.hpp"
#include "nosfix/raw_input_capture.hpp"
#include "nosfix/scheduler.hpp"
#include "nosfix/shadow_live.hpp"
#include "nosfix/song_clock_publication.hpp"
#include "nosfix/ui_hooks.hpp"
#include "nosfix/timed_rendering.hpp"

#include <Windows.h>
#include <mmsystem.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <string>

namespace nosfix {
namespace {

using RenderFunction = std::intptr_t(__fastcall*)(void* app);
using HasEntryFunction = char(__fastcall*)(void* list);
using ProcessDispatchFunction = void* (__fastcall*)(void* process);
using TransitionFunction = char(__fastcall*)(void* list, int state);
using NormalJudgeFunction = std::int64_t(__fastcall*)(int*, std::int64_t,
                                                       unsigned char,
                                                       std::int64_t);
using TimelineTickFunction = std::int64_t(__fastcall*)(void*);
using SeedJudgeOffsetFunction = std::int64_t(__fastcall*)(void*, std::int64_t,
                                                           std::int64_t, char*,
                                                           char, char);
using UiJudgeOffsetFunction = std::int64_t(__fastcall*)(void*, int);

constexpr DWORD kRenderRva = 0x1C31C0;
constexpr DWORD kNormalJudgeRva = 0x2F6800;
constexpr DWORD kTimelineTickRva = 0x2FC9D0;
constexpr DWORD kSeedJudgeOffsetRva = 0x305B30;
constexpr DWORD kUiJudgeOffsetRva = 0x277000;
constexpr DWORD kHasEntryRva = 0x355CD0;
constexpr DWORD kProcessDispatchRva = 0x355170;
constexpr DWORD kTransitionRva = 0x356010;
constexpr DWORD kFrameDeadlineRva = 0x13E94E0;
constexpr std::int64_t kTicksPer100ns = 10'000'000;

constexpr std::array<unsigned char, 24> kRenderBytes{
    0x48,0x8B,0xC4,0x55,0x41,0x54,0x41,0x56,0x48,0x8B,0xEC,0x48,
    0x83,0xEC,0x60,0x48,0x89,0x58,0x10,0x48,0x89,0x70,0x18,0x48};
constexpr std::array<unsigned char, 24> kHasEntryBytes{
    0x48,0x89,0x5C,0x24,0x08,0x48,0x89,0x74,0x24,0x10,0x57,0x48,
    0x83,0xEC,0x20,0x44,0x0F,0xB6,0x41,0x54,0x48,0xBE,0xAB,0xAA};
constexpr std::array<unsigned char, 24> kDispatchBytes{
    0x48,0x89,0x5C,0x24,0x18,0x55,0x56,0x41,0x55,0x41,0x56,0x41,
    0x57,0x48,0x83,0xEC,0x70,0x48,0x8B,0x41,0x20,0x4C,0x8D,0x3D};
constexpr std::array<unsigned char, 24> kTransitionBytes{
    0x48,0x89,0x5C,0x24,0x08,0x57,0x48,0x83,0xEC,0x40,0x8B,0xFA,
    0xC7,0x44,0x24,0x20,0xFF,0xFF,0xFF,0x7F,0x48,0x8B,0xD9,0xC6};
constexpr std::array<unsigned char, 24> kNormalJudgeBytes{
    0x48,0x89,0x5C,0x24,0x08,0x48,0x89,0x6C,0x24,0x10,0x48,0x89,
    0x74,0x24,0x18,0x57,0x41,0x54,0x41,0x55,0x41,0x56,0x41,0x57};
constexpr std::array<unsigned char, 24> kTimelineTickBytes{
    0x44,0x8B,0x19,0x4C,0x63,0x81,0x3C,0x01,0x00,0x00,0x41,0x8D,
    0x53,0x01,0x89,0x11,0x44,0x3B,0x81,0x38,0x01,0x00,0x00,0x7D};
constexpr std::array<unsigned char, 24> kSeedJudgeOffsetBytes{
    0x40,0x53,0x55,0x56,0x57,0x41,0x54,0x41,0x55,0x41,0x56,0x41,
    0x57,0x48,0x81,0xEC,0x48,0x03,0x00,0x00,0x48,0xC7,0x84,0x24};
constexpr std::array<unsigned char, 24> kUiJudgeOffsetBytes{
    0x48,0x8B,0xC4,0x55,0x41,0x54,0x41,0x55,0x41,0x56,0x41,0x57,
    0x48,0x8B,0xEC,0x48,0x83,0xEC,0x70,0x48,0xC7,0x45,0xD0,0xFE};
constexpr std::array<unsigned char, 5> kHasEntryCall{
    0xE8,0x48,0x24,0x19,0x00};
constexpr std::array<unsigned char, 5> kDispatchCall{
    0xE8,0x5F,0x18,0x19,0x00};

std::atomic<bool> g_runtime_active{false};
std::atomic<bool> g_input_observation{false};
std::atomic<bool> g_input_pending{false};
unsigned char* g_module_base = nullptr;
RenderFunction g_render = nullptr;
HasEntryFunction g_has_entry = nullptr;
ProcessDispatchFunction g_process_dispatch = nullptr;
TransitionFunction g_transition = nullptr;
NormalJudgeFunction g_normal_judge = nullptr;
TimelineTickFunction g_timeline_tick = nullptr;
SeedJudgeOffsetFunction g_seed_judge_offset = nullptr;
UiJudgeOffsetFunction g_ui_judge_offset = nullptr;
thread_local bool g_in_render = false;

struct ThreadState {
    FrameScheduler scheduler;
    bool scheduled = false;
    bool long_stall_reported = false;
    std::int64_t qpc_frequency = 0;
    std::int64_t metrics_start = 0;
    std::uint64_t tick_count = 0;
    std::uint64_t render_count = 0;
    std::int64_t render_ticks_sum = 0;
    std::int64_t render_ticks_max = 0;
    std::uint64_t input_keyboard_packets = 0;
    std::uint64_t input_hid_reports = 0;
    std::uint64_t input_resets = 0;
    std::int64_t input_consume_age_max = 0;
    std::uint64_t shadow_packets = 0;
    std::uint64_t shadow_mapped = 0;
    std::uint64_t shadow_unmapped = 0;
    std::uint64_t shadow_backwards = 0;
    std::uint64_t shadow_regressed = 0;
    std::uint64_t shadow_fences = 0;
    std::uint64_t shadow_generation = 0;
    double shadow_last_us = 0.0;
    bool shadow_has_last = false;
};
thread_local ThreadState g_thread;

std::int64_t QpcNow() noexcept {
    LARGE_INTEGER now{};
    QueryPerformanceCounter(&now);
    return now.QuadPart;
}

HANDLE HighResolutionTimer() noexcept {
    thread_local HANDLE timer = []() noexcept {
        constexpr DWORD high_resolution = 0x00000002;
        HANDLE result = CreateWaitableTimerExW(nullptr, nullptr, high_resolution,
                                                TIMER_ALL_ACCESS);
        if (!result) result = CreateWaitableTimerW(nullptr, TRUE, nullptr);
        return result;
    }();
    return timer;
}

std::int64_t To100ns(std::int64_t ticks, std::int64_t frequency) noexcept {
    const std::int64_t whole = ticks / frequency;
    const std::int64_t remainder = ticks % frequency;
    return whole * kTicksPer100ns +
        (remainder * kTicksPer100ns + frequency - 1) / frequency;
}

void WaitUntilQpc(std::int64_t deadline) noexcept {
    for (;;) {
        const std::int64_t now = QpcNow();
        const std::int64_t remaining = deadline - now;
        if (remaining <= 0) return;

        if (remaining > g_thread.qpc_frequency / 1000) {
            if (HANDLE timer = HighResolutionTimer()) {
                LARGE_INTEGER due{};
                due.QuadPart = -std::max<std::int64_t>(
                    1, To100ns(remaining, g_thread.qpc_frequency));
                if (SetWaitableTimer(timer, &due, 0, nullptr, nullptr, FALSE)) {
                    WaitForSingleObject(timer, INFINITE);
                    continue;
                }
            }
            Sleep(1);
        } else {
            SwitchToThread();
        }
    }
}

void RecordRender(std::int64_t elapsed_ticks) noexcept {
    ++g_thread.render_count;
    g_thread.render_ticks_sum += elapsed_ticks;
    g_thread.render_ticks_max = std::max(g_thread.render_ticks_max, elapsed_ticks);
}

void DrainRawInputDiagnostics(std::int64_t now) noexcept {
    if (!g_input_observation.load(std::memory_order_acquire)) return;
    // Observation only: raw payloads are never logged or sent to judgments.
    // The future mapping layer will replace this diagnostic consumer.
    RawInputPacket packet;
    for (std::size_t i = 0; i <= kRawInputCaptureCapacity &&
         TryPopRawInputPacket(packet); ++i) {
        if (SongClockEnabled()) {
            ++g_thread.shadow_packets;
            if (packet.kind == RawInputPacketKind::Fence) ++g_thread.shadow_fences;
            if (packet.kind == RawInputPacketKind::Input) {
                if (packet.clock.valid && packet.qpc_exact) {
                    if (packet.clock.continuity != SongClockContinuity::Monotonic)
                        ++g_thread.shadow_regressed;
                    const auto mapped = packet.clock.segment.Map(packet.qpc);
                    if (mapped) {
                        ++g_thread.shadow_mapped;
                        if (g_thread.shadow_generation != mapped->generation) {
                            g_thread.shadow_generation = mapped->generation;
                            g_thread.shadow_has_last = false;
                        }
                        if (g_thread.shadow_has_last &&
                            mapped->microseconds < g_thread.shadow_last_us) {
                            ++g_thread.shadow_backwards;
                        }
                        g_thread.shadow_last_us = mapped->microseconds;
                        g_thread.shadow_has_last = true;
                    } else {
                        ++g_thread.shadow_unmapped;
                    }
                } else {
                    ++g_thread.shadow_unmapped;
                }
            }
        }
        if (packet.kind == RawInputPacketKind::StreamReset) {
            ++g_thread.input_resets;
            g_thread.shadow_has_last = false;
            continue;
        }
        if (packet.raw_type == RIM_TYPEKEYBOARD) {
            ++g_thread.input_keyboard_packets;
        } else if (packet.raw_type == RIM_TYPEHID) {
            constexpr auto count_offset = offsetof(RAWINPUT, data) + offsetof(RAWHID, dwCount);
            if (packet.payload_size >= count_offset + sizeof(DWORD)) {
                DWORD reports = 0;
                std::memcpy(&reports, packet.payload.data() + count_offset, sizeof(reports));
                g_thread.input_hid_reports += reports;
            }
        }
        if (packet.qpc > 0 && packet.qpc <= now)
            g_thread.input_consume_age_max = std::max(
                g_thread.input_consume_age_max, now - packet.qpc);
    }
}

void LogWindowMetrics(std::int64_t now) {
    if (g_thread.metrics_start == 0) {
        g_thread.metrics_start = now;
        return;
    }
    const auto elapsed = now - g_thread.metrics_start;
    if (elapsed < g_thread.qpc_frequency * 5) return;

    const double seconds = static_cast<double>(elapsed) / g_thread.qpc_frequency;
    const double tick_hz = static_cast<double>(g_thread.tick_count) / seconds;
    const double render_hz = static_cast<double>(g_thread.render_count) / seconds;
    const double mean_render_ms = g_thread.render_count
        ? (static_cast<double>(g_thread.render_ticks_sum) * 1000.0 /
           g_thread.qpc_frequency / g_thread.render_count) : 0.0;
    const double max_render_ms = static_cast<double>(g_thread.render_ticks_max) *
        1000.0 / g_thread.qpc_frequency;
    char line[512]{};
    sprintf_s(line, "5s timing: native ticks=%.2f Hz, renders=%.2f Hz, "
                    "render+Present wall mean=%.3f ms max=%.3f ms",
              tick_hz, render_hz, mean_render_ms, max_render_ms);
    Log(LogLevel::Info, line);

    if (g_input_observation.load(std::memory_order_acquire)) {
        const auto input = RawInputCaptureStatsNow();
        sprintf_s(line, "5s raw input observation: keyboard packets=%llu, HID reports=%llu, "
            "stream resets=%llu, max consume age=%.3f ms; cumulative overflow=%llu, "
            "oversize=%llu, read errors=%llu, malformed=%llu",
            static_cast<unsigned long long>(g_thread.input_keyboard_packets),
            static_cast<unsigned long long>(g_thread.input_hid_reports),
            static_cast<unsigned long long>(g_thread.input_resets),
            static_cast<double>(g_thread.input_consume_age_max) * 1000.0 / g_thread.qpc_frequency,
            static_cast<unsigned long long>(input.dropped_packets),
            static_cast<unsigned long long>(input.oversize_packets),
            static_cast<unsigned long long>(input.read_failures),
            static_cast<unsigned long long>(input.malformed_packets));
        Log(LogLevel::Info, line);
    }

    if (SongClockEnabled()) {
        sprintf_s(line, "5s shadow clock: packets=%llu, mapped=%llu, unmapped=%llu, "
            "backwards=%llu, regressed=%llu, fences=%llu, generation=%llu",
            static_cast<unsigned long long>(g_thread.shadow_packets),
            static_cast<unsigned long long>(g_thread.shadow_mapped),
            static_cast<unsigned long long>(g_thread.shadow_unmapped),
            static_cast<unsigned long long>(g_thread.shadow_backwards),
            static_cast<unsigned long long>(g_thread.shadow_regressed),
            static_cast<unsigned long long>(g_thread.shadow_fences),
            static_cast<unsigned long long>(g_thread.shadow_generation));
        Log(LogLevel::Info, line);
    }

    if (ShadowLiveEnabled()) {
        const auto shadow = TakeShadowLiveMetrics();
        sprintf_s(line, "5s SHADOW live: frames=%llu, notes=%llu, outcomes=%llu, "
            "effects=%llu, errors=%llu, last=%s",
            static_cast<unsigned long long>(shadow.frames),
            static_cast<unsigned long long>(shadow.observed_notes),
            static_cast<unsigned long long>(shadow.outcomes),
            static_cast<unsigned long long>(shadow.effects),
            static_cast<unsigned long long>(shadow.feed_errors),
            ShadowFeedStatusName(shadow.last_status));
        Log(LogLevel::Info, line);
    }

    const auto motion = TakeMotionMetrics();
    sprintf_s(line, "5s note motion: captures=%llu, predicted draws=%llu, song-time draws=%llu, guarded fallbacks=%llu",
        static_cast<unsigned long long>(motion.captures),
        static_cast<unsigned long long>(motion.predicted_draws),
        static_cast<unsigned long long>(motion.timed_draws),
        static_cast<unsigned long long>(motion.fallback_draws));
    Log(LogLevel::Info, line);

    const auto ui = TakeUiMetrics();
    sprintf_s(line, "5s UI motion: captures=%llu, predicted draws=%llu, guarded fallbacks=%llu",
        static_cast<unsigned long long>(ui.captures),
        static_cast<unsigned long long>(ui.predicted_draws),
        static_cast<unsigned long long>(ui.fallback_draws));
    Log(LogLevel::Info, line);

    g_thread.metrics_start = now;
    g_thread.tick_count = 0;
    g_thread.render_count = 0;
    g_thread.render_ticks_sum = 0;
    g_thread.render_ticks_max = 0;
    g_thread.input_keyboard_packets = 0;
    g_thread.input_hid_reports = 0;
    g_thread.input_resets = 0;
    g_thread.input_consume_age_max = 0;
    g_thread.shadow_packets = 0;
    g_thread.shadow_mapped = 0;
    g_thread.shadow_unmapped = 0;
    g_thread.shadow_backwards = 0;
    g_thread.shadow_regressed = 0;
    g_thread.shadow_fences = 0;
    g_thread.shadow_has_last = false;
}

char TailHasEntry(void*, void* list) {
    return g_has_entry ? g_has_entry(list) : 0;
}

char TailTransition(void*, void* list, int state) {
    return g_transition ? g_transition(list, state) : 0;
}

void* TailProcessDispatch(void*, void* process) {
    return g_process_dispatch ? g_process_dispatch(process) : nullptr;
}

struct IterationContext { void* app; };

struct RenderScope {
    bool previous = g_in_render;
    RenderScope() noexcept { g_in_render = true; }
    ~RenderScope() {
        EndTimedRendering();
        EndUiPresentation();
        EndMotionPresentation();
        g_in_render = previous;
    }
};

std::int64_t IterationNow(void*) { return QpcNow(); }

void IterationWait(void*, std::int64_t deadline) { WaitUntilQpc(deadline); }

std::intptr_t IterationRender(void* opaque) {
    auto* context = static_cast<IterationContext*>(opaque);
    *reinterpret_cast<DWORD*>(g_module_base + kFrameDeadlineRva) = timeGetTime();
    const auto begin = QpcNow();
    std::intptr_t result = 0;
    {
        RenderScope scope;
        std::int64_t qpc_frequency = g_thread.qpc_frequency;
        if (qpc_frequency <= 0) {
            LARGE_INTEGER frequency{};
            if (QueryPerformanceFrequency(&frequency))
                qpc_frequency = frequency.QuadPart;
        }
        BeginTimedRendering(begin, qpc_frequency);
        const double phase = PresentationRate() > 60
            ? g_thread.scheduler.SimulationPhase(begin) : 0.0;
        BeginMotionPresentation(phase);
        BeginUiPresentation(phase);
        result = g_render(context->app);
    }
    RecordRender(QpcNow() - begin);
    return result;
}

void IterationDispatch(void* opaque, std::int64_t late_by_qpc) {
    auto* context = static_cast<IterationContext*>(opaque);
    DrainRawInputDiagnostics(QpcNow());
    if (DiagnosticProbeEnabled()) ProbeInputStateDiff(g_module_base);
    if (ShadowLiveEnabled() || ShadowLivePending()) {
        const auto now = QpcNow();
        const auto* publication = SongClockPublicationForInput();
        SongClockSnapshot snapshot{};
        if (publication && publication->TryRead(snapshot)) {
            const auto mapped = snapshot.segment.Map(now);
            if (mapped) ShadowLiveTick(mapped->microseconds, g_module_base);
        }
    }
    ++g_thread.tick_count;
    if (late_by_qpc >= g_thread.qpc_frequency / 4 &&
        !g_thread.long_stall_reported) {
        const double late_ms = static_cast<double>(late_by_qpc) * 1000.0 /
                               g_thread.qpc_frequency;
        char line[160]{};
        sprintf_s(line, "long stall: fixed simulation boundary late by %.1f ms; "
                        "render debt dropped while one tick is retired per loop", late_ms);
        Log(LogLevel::Warning, line);
        g_thread.long_stall_reported = true;
    }
    if (late_by_qpc < g_thread.qpc_frequency / 60) g_thread.long_stall_reported = false;

    const RenderTailCalls tail_calls{
        nullptr, TailHasEntry, TailTransition, TailProcessDispatch};
    DispatchRenderTail(context->app, tail_calls);
    RetireMotionTick();
    RetireUiTick();
    LogWindowMetrics(QpcNow());
}

std::intptr_t __fastcall RenderDetour(void* app) {
    if (!g_runtime_active.load(std::memory_order_acquire) || !g_render)
        return g_render ? g_render(app) : 0;

    if (!g_thread.scheduled) {
        LARGE_INTEGER frequency{};
        const unsigned presentation_hz = PresentationRate();
        if (!QueryPerformanceFrequency(&frequency) ||
            !g_thread.scheduler.Start(frequency.QuadPart, presentation_hz,
                                      QpcNow())) {
            Log(LogLevel::Error, "cannot start the display-rate QPC scheduler");
            return g_render(app);
        }
        g_thread.qpc_frequency = frequency.QuadPart;
        g_thread.scheduled = true;
    }

    // The SDK entry point precedes several seconds of game initialization.
    // Begin observation only once the game loop can consume the bounded queue.
    if (g_input_pending.exchange(false, std::memory_order_acq_rel)) {
        std::string input_reason;
        const SongClockPublication* clock =
            SongClockEnabled() ? SongClockPublicationForInput() : nullptr;
        if (StartRawInputCapture(input_reason, clock)) {
            g_input_observation.store(true, std::memory_order_release);
            Log(LogLevel::Info, clock
                ? "raw input observation active with shadow song-clock snapshots; judgments unchanged"
                : "raw input observation active on Spice input thread; host QPC dequeue times only, judgments unchanged");
        } else {
            Log(LogLevel::Warning, "raw input observation inactive: " + input_reason);
        }
    }

    IterationContext context{app};
    const IterationCallbacks callbacks{
        &context,
        IterationNow,
        IterationWait,
        IterationRender,
        IterationDispatch};
    return RunScheduledIteration(g_thread.scheduler, callbacks);
}

char __fastcall HasEntryCallsite(void* list) {
    if (g_runtime_active.load(std::memory_order_acquire) && g_in_render) return 0;
    return g_has_entry ? g_has_entry(list) : 0;
}

void* __fastcall ProcessDispatchCallsite(void* process) {
    if (g_runtime_active.load(std::memory_order_acquire) && g_in_render) return nullptr;
    return g_process_dispatch ? g_process_dispatch(process) : nullptr;
}

std::int64_t __fastcall NormalJudgeProbe(int* note, std::int64_t event,
                                          unsigned char mode,
                                          std::int64_t a4) {
    const auto* event_pointer = reinterpret_cast<const void*>(event);
    ProbeNormalJudgeBefore(note, event_pointer, mode);
    const std::int64_t result = g_normal_judge
        ? g_normal_judge(note, event, mode, a4) : 0;
    ProbeNormalJudgeAfter(note, event_pointer, mode, result);
    return result;
}

std::int64_t __fastcall TimelineTickRuntime(void* timeline) {
    return NativeTimelineRuntimeEnabled() ? NativeTimelineTick(timeline) : 0;
}

void ScaleJudgeOffsetToMicroseconds() noexcept {
    if (!g_module_base) return;
    std::int32_t value = 0;
    std::memcpy(&value, g_module_base + 0x1366918, sizeof(value));
    // The native setting and UI are frame-domain and clamped to [-5, +5].
    // Ignore values already in microseconds so a repeated init does not scale
    // the same offset twice.
    if (value < -5 || value > 5) return;
    value *= 16'667;
    std::memcpy(g_module_base + 0x1366918, &value, sizeof(value));
}

std::int64_t __fastcall SeedJudgeOffsetDetour(void* a1, std::int64_t a2,
                                               std::int64_t a3, char* a4,
                                               char a5, char a6) {
    const auto result = g_seed_judge_offset
        ? g_seed_judge_offset(a1, a2, a3, a4, a5, a6) : 0;
    ScaleJudgeOffsetToMicroseconds();
    return result;
}

std::int64_t __fastcall UiJudgeOffsetDetour(void* a1, int a2) {
    const auto result = g_ui_judge_offset ? g_ui_judge_offset(a1, a2) : 0;
    ScaleJudgeOffsetToMicroseconds();
    return result;
}

void SetRuntimeActive(bool active) noexcept {
    g_runtime_active.store(active, std::memory_order_release);
}

} // namespace

bool InstallGameHooks(HMODULE module, const Config& config, std::string& reason) {
    if (!module) { reason = "nostalgia module handle is null"; return false; }
    g_module_base = reinterpret_cast<unsigned char*>(module);
    if (!config.precise_audio_clock) {
        reason = "the microsecond timeline requires precise_audio_clock";
        return false;
    }
    if (config.native_shadow_feed) {
        reason = "the native shadow feed is incompatible with the microsecond timeline";
        return false;
    }
    if (!PrepareTimedRendering(module, reason)) {
        if (reason.empty()) reason = "native timing preparation failed";
        return false;
    }

    const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(module);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew <= 0) {
        reason = "invalid nostalgia DOS header before transition helper lookup";
        return false;
    }
    const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS64*>(
        g_module_base + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE) {
        reason = "invalid nostalgia NT header before helper lookup";
        return false;
    }
    const auto image_size = nt->OptionalHeader.SizeOfImage;
    const auto validate_helper = [&](DWORD rva, const auto& expected) {
        return rva <= image_size && expected.size() <= image_size - rva &&
            std::memcmp(g_module_base + rva, expected.data(), expected.size()) == 0;
    };
    if (!validate_helper(kHasEntryRva, kHasEntryBytes) ||
        !validate_helper(kProcessDispatchRva, kDispatchBytes) ||
        !validate_helper(kTransitionRva, kTransitionBytes)) {
        reason = "render-tail helper bytes do not match PAN-2025020501";
        return false;
    }
    g_has_entry = reinterpret_cast<HasEntryFunction>(g_module_base + kHasEntryRva);
    g_process_dispatch = reinterpret_cast<ProcessDispatchFunction>(
        g_module_base + kProcessDispatchRva);
    g_transition = reinterpret_cast<TransitionFunction>(g_module_base + kTransitionRva);

    if (!PreparePresentation(module, config, reason)) return false;

    if (!PrepareTimelineConversionPatches(module, reason)) {
        RollbackPresentation();
        return false;
    }

    std::vector<HookSpec> specs{
        {"Render", kRenderRva, kRenderBytes,
         reinterpret_cast<void*>(&RenderDetour),
         reinterpret_cast<void**>(&g_render)},
    };
    // Host prediction hooks are not installed: native microsecond timing is
    // authoritative. Only the read-only normal-judge probe remains optional.

    if (config.diagnostic_probe) {
        specs.push_back({"normal judge probe", kNormalJudgeRva, kNormalJudgeBytes,
            reinterpret_cast<void*>(&NormalJudgeProbe),
            reinterpret_cast<void**>(&g_normal_judge)});
    }
    specs.push_back({"timeline microsecond tick", kTimelineTickRva,
        kTimelineTickBytes,
        reinterpret_cast<void*>(&TimelineTickRuntime),
        reinterpret_cast<void**>(&g_timeline_tick)});
    specs.push_back({"timeline judge offset seed", kSeedJudgeOffsetRva,
        kSeedJudgeOffsetBytes,
        reinterpret_cast<void*>(&SeedJudgeOffsetDetour),
        reinterpret_cast<void**>(&g_seed_judge_offset)});
    specs.push_back({"timeline judge offset UI", kUiJudgeOffsetRva,
        kUiJudgeOffsetBytes,
        reinterpret_cast<void*>(&UiJudgeOffsetDetour),
        reinterpret_cast<void**>(&g_ui_judge_offset)});
    if (config.skip_monitor_check) AppendMonitorCheckHook(specs);

    std::vector<CallPatchSpec> calls{
        {"render-tail hasentry", 0x1C3883, kHasEntryCall,
         reinterpret_cast<void*>(&HasEntryCallsite)},
        {"render-tail dispatch", 0x1C390C, kDispatchCall,
         reinterpret_cast<void*>(&ProcessDispatchCallsite)},
    };
    if (!PrepareAudioClockPatch(reason)) {
        RollbackTimelineConversionPatches();
        RollbackPresentation();
        return false;
    }
    if (!InstallHooks(module, specs, reason)) {
        RollbackTimelineConversionPatches();
        RollbackAudioClockPatch();
        RollbackPresentation();
        return false;
    }
    if (!InstallCallPatches(module, calls, reason)) {
        RollbackTimelineConversionPatches();
        RollbackAudioClockPatch();
        RemoveHooks();
        RollbackPresentation();
        return false;
    }
    if (!CommitTimelineConversionPatches(reason)) {
        RollbackTimelineConversionPatches();
        RollbackAudioClockPatch();
        RemoveCallPatches();
        RemoveHooks();
        RollbackPresentation();
        return false;
    }
    if (!CommitAudioClockPatch(reason) || !CommitPresentation(reason)) {
        RollbackTimelineConversionPatches();
        RollbackAudioClockPatch();
        RemoveCallPatches();
        RemoveHooks();
        RollbackPresentation();
        return false;
    }

    HMODULE self = nullptr;
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                                GET_MODULE_HANDLE_EX_FLAG_PIN,
                            reinterpret_cast<LPCWSTR>(&InstallGameHooks), &self)) {
        reason = "cannot pin nosfix.dll while detours are installed";
        RollbackTimelineConversionPatches();
        RollbackAudioClockPatch();
        RemoveCallPatches();
        RemoveHooks();
        RollbackPresentation();
        return false;
    }

    if (config.asio_output) {
        ModuleIdentity xact;
        std::wstring guard_reason;
        std::string asio_reason;
        if (!VerifyLibxactModule(xact, guard_reason)) {
            for (const wchar_t c : guard_reason)
                asio_reason.push_back(c <= 127 ? static_cast<char>(c) : '?');
        } else if (!ConfigureAsioRoute(config, xact.module, asio_reason)) {
            // ASIO is optional: keep the original libxact engine on failure.
        }
        if (!asio_reason.empty())
            Log(LogLevel::Warning, "ASIO route inactive: " + asio_reason);
    } else {
        DisableAsioRoute();
    }
    ConfigureDiagnosticProbe(config.diagnostic_probe);
    ConfigureNativeTimelineRuntime(true);
    DisableShadowLive();
    SetShadowNoteObserver(nullptr);
    SetRuntimeActive(true);
    ConfigureMonitorCheck(module, config.skip_monitor_check);
    ConfigureTimedRendering(false);
    ConfigureSongClock(true);
    ConfigureMotion(module, false);
    ConfigureUiMotion(false);
    if (config.capture_input) {
        ModuleIdentity spice;
        std::wstring guard_reason;
        std::string input_reason;
        if (!VerifySpiceModule(spice, guard_reason)) {
            for (const wchar_t c : guard_reason)
                input_reason.push_back(c <= 127 ? static_cast<char>(c) : '?');
        } else {
            g_input_pending.store(true, std::memory_order_release);
            Log(LogLevel::Info, "raw input observation prepared; capture starts when the game loop begins rendering");
        }
        if (!input_reason.empty())
            Log(LogLevel::Warning, "raw input observation inactive: " + input_reason);
    }
    Log(LogLevel::Info, "display-rate microsecond scheduler active");
    Log(LogLevel::Info, "cue-clock QPC precision corrected; timestamped judgments are not yet integrated");
    if (config.native_shadow) {
        Log(LogLevel::Info, "shadow song clock publication enabled; input packets are mapped for diagnostics only");
        if (!config.capture_input)
            Log(LogLevel::Warning, "native_shadow has no capture_input; the clock publishes but no packets are consumed");
    }
    if (config.diagnostic_probe)
        Log(LogLevel::Info, "diagnostic PROBE enabled: normal judge updates");
    Log(LogLevel::Info, "HIGH REFRESH timeline active: native chart/ticks/windows use absolute microseconds; host prediction disabled");
    if (config.native_shadow_feed)
        Log(LogLevel::Warning, "native_shadow_feed is ignored: microsecond timeline is authoritative");
    Log(LogLevel::Info, "note prediction disabled: native microsecond timing is authoritative");
    Log(LogLevel::Info, "UI motion disabled: native microsecond timing is authoritative");
    return true;
}

void RemoveGameHooks() noexcept {
    g_input_pending.store(false, std::memory_order_release);
    if (g_input_observation.exchange(false, std::memory_order_acq_rel)) StopRawInputCapture();
    ConfigureDiagnosticProbe(false);
    ConfigureNativeTimelineRuntime(false);
    DisableAsioRoute();
    SetShadowNoteObserver(nullptr);
    DisableShadowLive();
    SetRuntimeActive(false);
    ConfigureMonitorCheck(nullptr, false);
    ConfigureSongClock(false);
    ConfigureTimedRendering(false);
    ConfigureMotion(nullptr, false);
    ConfigureUiMotion(false);
    RollbackAudioClockPatch();
    RemoveCallPatches();
    RemoveHooks();
    RollbackTimelineConversionPatches();
    RollbackPresentation();
}

} // namespace nosfix
