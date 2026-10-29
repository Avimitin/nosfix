#include "nosfix/game_hooks.hpp"
#include "nosfix/audio_clock_patch.hpp"

#include "nosfix/config.hpp"
#include "nosfix/call_patch.hpp"
#include "nosfix/hooks.hpp"
#include "nosfix/log.hpp"
#include "nosfix/module_guard.hpp"
#include "nosfix/motion_hooks.hpp"
#include "nosfix/monitor_check.hpp"
#include "nosfix/presentation.hpp"
#include "nosfix/render_tail.hpp"
#include "nosfix/raw_input_capture.hpp"
#include "nosfix/scheduler.hpp"
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

constexpr DWORD kRenderRva = 0x1C31C0;
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
        if (packet.kind == RawInputPacketKind::StreamReset) {
            ++g_thread.input_resets;
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
    sprintf_s(line, "5s timing: legacy ticks=%.2f Hz, renders=%.2f Hz, "
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
        BeginTimedRendering();
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
        if (!QueryPerformanceFrequency(&frequency) ||
            !g_thread.scheduler.Start(frequency.QuadPart, PresentationRate(), QpcNow())) {
            Log(LogLevel::Error, "cannot start the fixed phase QPC scheduler");
            return g_render(app);
        }
        g_thread.qpc_frequency = frequency.QuadPart;
        g_thread.scheduled = true;
    }

    // The SDK entry point precedes several seconds of game initialization.
    // Begin observation only once the game loop can consume the bounded queue.
    if (g_input_pending.exchange(false, std::memory_order_acq_rel)) {
        std::string input_reason;
        if (StartRawInputCapture(input_reason)) {
            g_input_observation.store(true, std::memory_order_release);
            Log(LogLevel::Info, "raw input observation active on Spice input thread; host QPC dequeue times only, judgments unchanged");
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

void SetRuntimeActive(bool active) noexcept {
    g_runtime_active.store(active, std::memory_order_release);
}

} // namespace

bool InstallGameHooks(HMODULE module, const Config& config, std::string& reason) {
    if (!module) { reason = "nostalgia module handle is null"; return false; }
    g_module_base = reinterpret_cast<unsigned char*>(module);
    if (config.clock_motion && (!config.precise_audio_clock ||
        !PrepareTimedRendering(module, reason))) {
        if (reason.empty()) reason = "clock_motion requires precise_audio_clock";
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

    std::vector<HookSpec> specs{
        {"Render", kRenderRva, kRenderBytes,
         reinterpret_cast<void*>(&RenderDetour),
         reinterpret_cast<void**>(&g_render)},
    };
    if (config.smooth_motion || config.clock_motion) AppendMotionHooks(specs);
    if (config.skip_monitor_check) AppendMonitorCheckHook(specs);

    std::vector<CallPatchSpec> calls{
        {"render-tail hasentry", 0x1C3883, kHasEntryCall,
         reinterpret_cast<void*>(&HasEntryCallsite)},
        {"render-tail dispatch", 0x1C390C, kDispatchCall,
         reinterpret_cast<void*>(&ProcessDispatchCallsite)},
    };
    if (config.smooth_ui) {
        ModuleIdentity afp;
        std::wstring guard_reason;
        if (!VerifyAfpModule(afp, guard_reason)) {
            reason.clear();
            for (const wchar_t c : guard_reason)
                reason.push_back(c <= 127 ? static_cast<char>(c) : '?');
            RollbackPresentation();
            return false;
        }
        if (!PrepareUiMotion(afp.module, true, calls, reason)) {
            RollbackPresentation();
            return false;
        }
    }
    if (config.precise_audio_clock && !PrepareAudioClockPatch(reason)) {
        RollbackPresentation();
        return false;
    }
    if (!InstallHooks(module, specs, reason)) {
        RollbackAudioClockPatch();
        RollbackPresentation();
        return false;
    }
    if (!InstallCallPatches(module, calls, reason)) {
        RollbackAudioClockPatch();
        RemoveHooks();
        RollbackPresentation();
        return false;
    }
    if ((config.precise_audio_clock && !CommitAudioClockPatch(reason)) ||
        !CommitPresentation(reason)) {
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
        RollbackAudioClockPatch();
        RemoveCallPatches();
        RemoveHooks();
        RollbackPresentation();
        return false;
    }

    SetRuntimeActive(true);
    ConfigureMonitorCheck(module, config.skip_monitor_check);
    ConfigureTimedRendering(config.clock_motion);
    ConfigureMotion(module, config.smooth_motion);
    ConfigureUiMotion(config.smooth_ui);
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
    Log(LogLevel::Info, "legacy 60 Hz update scheduler and independent presentation proxy active");
    if (config.precise_audio_clock)
        Log(LogLevel::Info, "cue-clock QPC precision corrected; timestamped judgments are not yet integrated");
    if (config.clock_motion)
        Log(LogLevel::Info, "experimental normal-note projection samples the song clock each presentation");
    Log(LogLevel::Info, config.smooth_motion
        ? "bounded note-head and ribbon prediction enabled above 60 Hz"
        : "note prediction disabled by configuration");
    Log(LogLevel::Info, config.smooth_ui
        ? "bounded AFP UI translation prediction enabled above 60 Hz"
        : "UI motion disabled by configuration");
    return true;
}

void RemoveGameHooks() noexcept {
    g_input_pending.store(false, std::memory_order_release);
    if (g_input_observation.exchange(false, std::memory_order_acq_rel)) StopRawInputCapture();
    SetRuntimeActive(false);
    ConfigureMonitorCheck(nullptr, false);
    ConfigureTimedRendering(false);
    ConfigureMotion(nullptr, false);
    ConfigureUiMotion(false);
    RollbackAudioClockPatch();
    RemoveCallPatches();
    RemoveHooks();
    RollbackPresentation();
}

} // namespace nosfix
