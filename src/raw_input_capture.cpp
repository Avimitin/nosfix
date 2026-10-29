#include "nosfix/raw_input_capture.hpp"

#include <dbt.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstring>
#include <mutex>
#include <new>
#include <thread>
#include <type_traits>
#include <vector>

namespace nosfix {
namespace {

constexpr std::uint64_t kQueueMask = kRawInputCaptureCapacity - 1;
static_assert((kRawInputCaptureCapacity & (kRawInputCaptureCapacity - 1)) == 0);
static_assert(std::is_trivially_copyable_v<RawInputPacket>);

struct CaptureState final {
    RawInputPacketQueue queue;
    std::atomic<HHOOK> hook{nullptr};
    std::atomic<HHOOK> call_hook{nullptr};
    HWND target = nullptr;
    const SongClockPublication* clock = nullptr;
    std::atomic<DWORD> thread_id{0};
    std::atomic<std::int64_t> qpc_frequency{0};
    std::atomic<bool> active{false};
    std::atomic<bool> ever_started{false};
    std::atomic<bool> heartbeat_stop{false};
    std::thread heartbeat;
    std::mutex lifecycle;
    // Producer-only.
    bool has_fence = false;
    std::int64_t last_fence_qpc = 0;
};

CaptureState& State() noexcept {
    alignas(CaptureState) static std::byte storage[sizeof(CaptureState)];
    static CaptureState* const state =
        ::new (static_cast<void*>(storage)) CaptureState();
    return *state;
}

// Producer-side snapshot. Must be taken before QueryPerformanceCounter so the
// retained segment cannot have been published after the packet's QPC.
RawInputCaptureClock CaptureClock() noexcept {
    RawInputCaptureClock clock;
    const SongClockPublication* publication = State().clock;
    if (publication != nullptr) {
        SongClockSnapshot snapshot;
        if (publication->TryRead(snapshot)) {
            clock.segment = snapshot.segment;
            clock.version = snapshot.version;
            clock.continuity = snapshot.continuity;
            clock.valid = true;
        }
    }
    return clock;
}

// ---------------------------------------------------------------------------
// Serialized producer callbacks
// ---------------------------------------------------------------------------
//
// WH_CALLWNDPROC runs in the receiving thread when a sent message is dispatched,
// which can happen while the WH_GETMESSAGE hook is inside GetRawInputData. Both
// callbacks mutate the same SPSC producer state, so the nested callback never
// writes directly: it records one coalesced boundary that is flushed after the
// outer callback has finished. A nested WM_INPUT is not read reentrantly (the
// HRAWINPUT lifetime is not ours); it becomes a Reentrant loss boundary.

struct DeferredBoundary final {
    bool pending = false;
    RawInputBoundary boundary{};
    RawInputCaptureClock clock{};
    std::int64_t qpc = 0;
    bool qpc_exact = true;
    std::int64_t observed_qpc = 0;
};

thread_local bool g_in_producer = false;
thread_local DeferredBoundary g_deferred;

void FlushDeferredBoundary() noexcept {
    if (!g_deferred.pending) return;
    const DeferredBoundary deferred = g_deferred;
    g_deferred.pending = false;
    State().queue.MarkGap(deferred.qpc, deferred.boundary, deferred.clock,
                          deferred.qpc_exact, deferred.observed_qpc);
}

void DeferBoundary(const RawInputBoundary& boundary,
                   const RawInputCaptureClock& clock) noexcept {
    if (g_deferred.pending) return; // one boundary is enough; it ends the stream
    std::int64_t qpc = 0;
    bool exact = false;
    LARGE_INTEGER counter{};
    if (QueryPerformanceCounter(&counter)) {
        qpc = static_cast<std::int64_t>(counter.QuadPart);
        exact = true;
    } else {
        const RawInputProducerCursor cursor = State().queue.ProducerCursor();
        if (cursor.valid && cursor.qpc_exact) qpc = cursor.qpc;
    }
    g_deferred.pending = true;
    g_deferred.boundary = boundary;
    g_deferred.clock = clock;
    g_deferred.qpc = qpc;
    g_deferred.qpc_exact = exact;
    g_deferred.observed_qpc = 0;
}

struct ProducerGuard final {
    const bool outer;
    ProducerGuard() noexcept : outer(!g_in_producer) {
        if (outer) g_in_producer = true;
    }
    ~ProducerGuard() {
        if (!outer) return;
        FlushDeferredBoundary();
        g_in_producer = false;
    }
    explicit operator bool() const noexcept { return outer; }
};

bool IsTargetWindow(HWND hwnd) noexcept {
    if (!hwnd || !IsWindow(hwnd)) return false;

    std::array<char, 256> class_name{};
    const int length = GetClassNameA(hwnd, class_name.data(),
                                     static_cast<int>(class_name.size()));
    if (length <= 0 || length >= static_cast<int>(class_name.size())) return false;
    return std::strcmp(class_name.data(), "SpiceTools Input") == 0;
}

bool FindInputTarget(HWND& target, DWORD& thread_id, std::string& reason) {
    UINT count = 0;
    const UINT query = GetRegisteredRawInputDevices(nullptr, &count,
                                                     sizeof(RAWINPUTDEVICE));
    if (query == static_cast<UINT>(-1) || count == 0) {
        reason = "no registered raw-input devices are visible";
        return false;
    }

    std::vector<RAWINPUTDEVICE> registrations(count);
    UINT actual = count;
    if (GetRegisteredRawInputDevices(registrations.data(), &actual,
                                     sizeof(RAWINPUTDEVICE)) == static_cast<UINT>(-1)) {
        reason = "could not enumerate registered raw-input devices";
        return false;
    }
    if (actual > registrations.size()) {
        reason = "raw-input registration list changed during discovery";
        return false;
    }

    HWND candidate = nullptr;
    for (UINT index = 0; index < actual; ++index) {
        const auto& entry = registrations[index];
        if (entry.usUsagePage != 0x01 || entry.usUsage != 0x06) continue;
        if (!entry.hwndTarget) {
            reason = "registered keyboard target is null";
            return false;
        }
        if (candidate && candidate != entry.hwndTarget) {
            reason = "multiple registered keyboard target windows found";
            return false;
        }
        candidate = entry.hwndTarget;
    }

    if (!candidate || !IsTargetWindow(candidate)) {
        reason = "could not identify the unique SpiceTools Input keyboard window";
        return false;
    }

    DWORD process_id = 0;
    const DWORD candidate_thread = GetWindowThreadProcessId(candidate, &process_id);
    if (!candidate_thread || process_id != GetCurrentProcessId() ||
        candidate_thread == GetCurrentThreadId()) {
        reason = "Spice input window is not on a separate thread in this process";
        return false;
    }

    target = candidate;
    thread_id = candidate_thread;
    return true;
}

void CaptureRawInputMessage(const MSG& message) noexcept {
    auto& state = State();
    ProducerGuard guard;
    if (!guard) {
        RawInputBoundary boundary;
        boundary.reason = RawInputResetReason::Reentrant;
        DeferBoundary(boundary, CaptureClock());
        return;
    }

    const RawInputCaptureClock clock = CaptureClock();
    LARGE_INTEGER counter{};
    if (!QueryPerformanceCounter(&counter)) {
        state.queue.MarkGap(0, RawInputResetReason::ReadFailure, clock,
                            RawInputDeviceChangeKind::None, 0, false, 0);
        return;
    }
    const auto qpc = static_cast<std::int64_t>(counter.QuadPart);
    const auto input_handle = reinterpret_cast<HRAWINPUT>(message.lParam);

    alignas(RAWINPUTHEADER) RAWINPUTHEADER header{};
    UINT header_size = sizeof(header);
    const UINT header_result = GetRawInputData(
        input_handle, RID_HEADER, &header, &header_size, sizeof(RAWINPUTHEADER));
    if (header_result == static_cast<UINT>(-1) ||
        header_result != sizeof(RAWINPUTHEADER) ||
        header_size != sizeof(RAWINPUTHEADER)) {
        state.queue.MarkGap(qpc, RawInputResetReason::ReadFailure, clock);
        return;
    }

    if (header.dwType == RIM_TYPEMOUSE) {
        state.queue.NoteIgnoredMouse();
        return;
    }

    UINT payload_size = 0;
    if (GetRawInputData(input_handle, RID_INPUT, nullptr, &payload_size,
                        sizeof(RAWINPUTHEADER)) == static_cast<UINT>(-1)) {
        state.queue.MarkGap(qpc, RawInputResetReason::ReadFailure, clock);
        return;
    }
    if (payload_size < sizeof(RAWINPUTHEADER)) {
        state.queue.MarkGap(qpc, RawInputResetReason::Malformed, clock);
        return;
    }
    if (payload_size > kRawInputCapturePayloadBytes) {
        state.queue.MarkGap(qpc, RawInputResetReason::Oversize, clock);
        return;
    }

    alignas(RAWINPUT) std::array<std::byte, kRawInputCapturePayloadBytes> buffer{};
    const UINT read_size = GetRawInputData(input_handle, RID_INPUT, buffer.data(),
                                           &payload_size, sizeof(RAWINPUTHEADER));
    if (read_size == static_cast<UINT>(-1) || read_size != payload_size) {
        state.queue.MarkGap(qpc, RawInputResetReason::ReadFailure, clock);
        return;
    }
    (void)state.queue.PushRawInput(qpc, {buffer.data(), read_size}, clock, true);
}

void ObserveBoundary(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) noexcept {
    if (hwnd != State().target) return;
    const bool raw_change = message == WM_INPUT_DEVICE_CHANGE &&
        (wparam == GIDC_ARRIVAL || wparam == GIDC_REMOVAL);
    const bool device_change = message == WM_DEVICECHANGE &&
        (wparam == DBT_DEVICEARRIVAL || wparam == DBT_DEVICEREMOVECOMPLETE);
    if (!raw_change && !device_change) return;

    RawInputBoundary boundary;
    boundary.reason = RawInputResetReason::DeviceChange;
    if (raw_change) {
        boundary.device_change = wparam == GIDC_ARRIVAL
            ? RawInputDeviceChangeKind::Arrival
            : RawInputDeviceChangeKind::Removal;
        boundary.device_handle =
            static_cast<std::uint64_t>(static_cast<std::uintptr_t>(lparam));
    }

    ProducerGuard guard;
    if (!guard) {
        DeferBoundary(boundary, CaptureClock());
        return;
    }

    const RawInputCaptureClock clock = CaptureClock();
    LARGE_INTEGER counter{};
    const bool exact = QueryPerformanceCounter(&counter) != 0;
    const std::int64_t qpc = exact ? static_cast<std::int64_t>(counter.QuadPart) : 0;
    State().queue.MarkGap(qpc, boundary, clock, exact, 0);
}

// One heartbeat fence per kFenceIntervalMs milliseconds, on any message the
// producer thread retrieves. The helper thread only posts WM_NULL to this
// thread; the QPC and queue mutation stay on the producer thread.
constexpr std::int64_t kFenceIntervalMs = 2;
constexpr std::int64_t kHeartbeatSleepMs = 1;

void MaybeEmitFence() noexcept {
    auto& state = State();
    if (!state.active.load(std::memory_order_acquire)) return;
    ProducerGuard guard;
    if (!guard) return;

    const RawInputCaptureClock clock = CaptureClock();
    LARGE_INTEGER counter{};
    if (!QueryPerformanceCounter(&counter)) return; // no exact time: no fence
    const std::int64_t qpc = static_cast<std::int64_t>(counter.QuadPart);
    const std::int64_t frequency = state.qpc_frequency.load(std::memory_order_relaxed);
    if (frequency <= 0) return;
    const std::int64_t interval =
        std::max<std::int64_t>(1, frequency * kFenceIntervalMs / 1000);
    if (state.has_fence && qpc - state.last_fence_qpc < interval) return;
    if (state.queue.PushFence(qpc, clock, true)) {
        state.has_fence = true;
        state.last_fence_qpc = qpc;
    }
}

void HeartbeatLoop(HWND target, std::atomic<bool>* stop) noexcept {
    while (!stop->load(std::memory_order_acquire)) {
        if (target != nullptr) {
            (void)PostMessageW(target, WM_NULL, 0, 0);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(kHeartbeatSleepMs));
    }
}

LRESULT CALLBACK GetMessageHook(int code, WPARAM action, LPARAM parameter) {
    auto& state = State();
    const auto hook = state.hook.load(std::memory_order_acquire);
    if (code == HC_ACTION && state.active.load(std::memory_order_acquire) &&
        parameter != 0) {
        const auto& message = *reinterpret_cast<const MSG*>(parameter);
        if (IsRawInputCaptureMessage(code, action, message, state.target)) {
            CaptureRawInputMessage(message);
        }
        if (action == PM_REMOVE && state.clock != nullptr) MaybeEmitFence();
    }
    return CallNextHookEx(hook, code, action, parameter);
}

LRESULT CALLBACK CallWndProcHook(int code, WPARAM action, LPARAM parameter) {
    auto& state = State();
    const auto hook = state.call_hook.load(std::memory_order_acquire);
    if (code == HC_ACTION && state.active.load(std::memory_order_acquire) &&
        parameter != 0) {
        const auto& message = *reinterpret_cast<const CWPSTRUCT*>(parameter);
        if (message.hwnd == state.target) {
            ObserveBoundary(message.hwnd, message.message, message.wParam,
                            message.lParam);
        }
    }
    return CallNextHookEx(hook, code, action, parameter);
}

} // namespace

RawInputPacketQueue::RawInputPacketQueue() noexcept
    : slots_(new (std::nothrow) RawInputPacket[kRawInputCaptureCapacity]) {}

RawInputPacketQueue::~RawInputPacketQueue() = default;

bool RawInputPacketQueue::Ready() const noexcept { return slots_ != nullptr; }

bool RawInputPacketQueue::PushRawInput(
    std::int64_t qpc, std::span<const std::byte> payload,
    const RawInputCaptureClock& clock, bool qpc_exact) noexcept {
    if (!slots_) return false;

    if (payload.size() < sizeof(RAWINPUTHEADER)) {
        MarkGap(qpc, RawInputResetReason::Malformed, clock,
                RawInputDeviceChangeKind::None, 0, qpc_exact, 0);
        return false;
    }
    if (payload.size() > kRawInputCapturePayloadBytes) {
        MarkGap(qpc, RawInputResetReason::Oversize, clock,
                RawInputDeviceChangeKind::None, 0, qpc_exact, 0);
        return false;
    }

    RAWINPUTHEADER header{};
    std::memcpy(&header, payload.data(), sizeof(header));
    if (header.dwSize != payload.size()) {
        MarkGap(qpc, RawInputResetReason::Malformed, clock,
                RawInputDeviceChangeKind::None, 0, qpc_exact, 0);
        return false;
    }
    if (header.dwType == RIM_TYPEMOUSE) {
        NoteIgnoredMouse();
        return false;
    }
    if (header.dwType == RIM_TYPEKEYBOARD) {
        constexpr auto minimum = offsetof(RAWINPUT, data.keyboard) + sizeof(RAWKEYBOARD);
        if (payload.size() < minimum) {
            MarkGap(qpc, RawInputResetReason::Malformed, clock,
                    RawInputDeviceChangeKind::None, 0, qpc_exact, 0);
            return false;
        }
    } else if (header.dwType == RIM_TYPEHID) {
        constexpr auto data_offset = offsetof(RAWINPUT, data.hid.bRawData);
        if (payload.size() < data_offset) {
            MarkGap(qpc, RawInputResetReason::Malformed, clock,
                    RawInputDeviceChangeKind::None, 0, qpc_exact, 0);
            return false;
        }
        DWORD report_size = 0;
        DWORD report_count = 0;
        std::memcpy(&report_size,
                    payload.data() + offsetof(RAWINPUT, data.hid.dwSizeHid),
                    sizeof(report_size));
        std::memcpy(&report_count,
                    payload.data() + offsetof(RAWINPUT, data.hid.dwCount),
                    sizeof(report_count));
        const auto available = payload.size() - data_offset;
        if (report_size == 0 || report_count == 0 ||
            static_cast<std::uint64_t>(report_size) * report_count > available) {
            MarkGap(qpc, RawInputResetReason::Malformed, clock,
                    RawInputDeviceChangeKind::None, 0, qpc_exact, 0);
            return false;
        }
    } else {
        MarkGap(qpc, RawInputResetReason::Malformed, clock,
                RawInputDeviceChangeKind::None, 0, qpc_exact, 0);
        return false;
    }

    if (qpc_exact) {
        if (has_last_qpc_ && qpc < last_qpc_) {
            NoteReversal(qpc, clock);
            return false;
        }
        last_qpc_ = qpc;
        has_last_qpc_ = true;
    } else {
        qpc = has_last_qpc_ ? last_qpc_ : 0;
    }

    const auto write = write_index_.load(std::memory_order_relaxed);
    const auto read = read_index_.load(std::memory_order_acquire);
    if (write - read >= kRawInputCaptureMaxInputPackets) {
        MarkGap(qpc, RawInputResetReason::QueueOverflow, clock,
                RawInputDeviceChangeKind::None, 0, qpc_exact, 0);
        return false;
    }

    const auto sequence = next_sequence_.fetch_add(1, std::memory_order_relaxed);
    auto& packet = slots_[write & kQueueMask];
    packet.kind = RawInputPacketKind::Input;
    packet.reset_reason = RawInputResetReason::None;
    packet.sequence = sequence;
    packet.stream_epoch = stream_epoch_.load(std::memory_order_relaxed);
    packet.qpc = qpc;
    packet.qpc_exact = qpc_exact;
    packet.observed_qpc = 0;
    packet.raw_type = header.dwType;
    packet.payload_size = static_cast<std::uint32_t>(payload.size());
    packet.clock = clock;
    packet.change_kind = RawInputDeviceChangeKind::None;
    packet.change_device = 0;
    std::memcpy(packet.payload.data(), payload.data(), payload.size());
    write_index_.store(write + 1, std::memory_order_release);
    packets_captured_.fetch_add(1, std::memory_order_relaxed);
    if (clock.valid) capture_clock_snapshots_.fetch_add(1, std::memory_order_relaxed);
    PublishProducerCursor(qpc, qpc_exact, sequence, packet.stream_epoch);
    return true;
}

bool RawInputPacketQueue::PushFence(std::int64_t qpc,
                                    const RawInputCaptureClock& clock,
                                    bool qpc_exact) noexcept {
    if (!slots_) return false;
    if (qpc_exact) {
        if (has_last_qpc_ && qpc < last_qpc_) {
            NoteReversal(qpc, clock);
            return false;
        }
        last_qpc_ = qpc;
        has_last_qpc_ = true;
    } else {
        qpc = has_last_qpc_ ? last_qpc_ : 0;
    }

    const auto write = write_index_.load(std::memory_order_relaxed);
    const auto read = read_index_.load(std::memory_order_acquire);
    if (write - read >= kRawInputCaptureMaxInputPackets) {
        fences_dropped_.fetch_add(1, std::memory_order_relaxed);
        return false;
    }

    const auto sequence = next_sequence_.fetch_add(1, std::memory_order_relaxed);
    auto& packet = slots_[write & kQueueMask];
    packet = {};
    packet.kind = RawInputPacketKind::Fence;
    packet.sequence = sequence;
    packet.stream_epoch = stream_epoch_.load(std::memory_order_relaxed);
    packet.qpc = qpc;
    packet.qpc_exact = qpc_exact;
    packet.clock = clock;
    write_index_.store(write + 1, std::memory_order_release);
    fences_captured_.fetch_add(1, std::memory_order_relaxed);
    if (clock.valid) capture_clock_snapshots_.fetch_add(1, std::memory_order_relaxed);
    PublishProducerCursor(qpc, qpc_exact, sequence, packet.stream_epoch);
    return true;
}

void RawInputPacketQueue::NoteIgnoredMouse() noexcept {
    ignored_mouse_packets_.fetch_add(1, std::memory_order_relaxed);
}

void RawInputPacketQueue::NoteReversal(std::int64_t observed_qpc,
                                       const RawInputCaptureClock& clock) noexcept {
    MarkGap(has_last_qpc_ ? last_qpc_ : 0, RawInputResetReason::ClockReversal, clock,
            RawInputDeviceChangeKind::None, 0, false, observed_qpc);
}

void RawInputPacketQueue::MarkGap(std::int64_t qpc, RawInputResetReason reason,
                                  const RawInputCaptureClock& clock,
                                  RawInputDeviceChangeKind change,
                                  std::uint64_t change_device,
                                  bool qpc_exact, std::int64_t observed_qpc) noexcept {
    if (reason == RawInputResetReason::None) return;
    switch (reason) {
        case RawInputResetReason::QueueOverflow:
            dropped_packets_.fetch_add(1, std::memory_order_relaxed);
            break;
        case RawInputResetReason::ReadFailure:
            read_failures_.fetch_add(1, std::memory_order_relaxed);
            break;
        case RawInputResetReason::Oversize:
            oversize_packets_.fetch_add(1, std::memory_order_relaxed);
            break;
        case RawInputResetReason::Malformed:
            malformed_packets_.fetch_add(1, std::memory_order_relaxed);
            break;
        case RawInputResetReason::DeviceChange:
            device_change_boundaries_.fetch_add(1, std::memory_order_relaxed);
            break;
        case RawInputResetReason::ClockReversal:
            clock_reversals_.fetch_add(1, std::memory_order_relaxed);
            break;
        case RawInputResetReason::Reentrant:
            reentrant_boundaries_.fetch_add(1, std::memory_order_relaxed);
            break;
        case RawInputResetReason::None:
            return;
    }

    // Exact-stamp rules: never clamp or fabricate a new timestamp. If the
    // caller's QPC is not exact, or is earlier than the last accepted one, the
    // record keeps the last exact QPC only as an ordering stamp, is marked
    // non-exact, and preserves the rejected raw value for diagnosis.
    if (qpc_exact) {
        if (has_last_qpc_ && qpc < last_qpc_) {
            observed_qpc = qpc;
            qpc = last_qpc_;
            qpc_exact = false;
        } else {
            last_qpc_ = qpc;
            has_last_qpc_ = true;
        }
    } else {
        qpc = has_last_qpc_ ? last_qpc_ : 0;
    }

    const auto write = write_index_.load(std::memory_order_relaxed);
    const auto read = read_index_.load(std::memory_order_acquire);
    if (write - read >= kRawInputCaptureCapacity) {
        // A full queue ends in the reset record that consumed the reserved
        // slot. Coalesce consecutive failures into its reset boundary until
        // the consumer frees room for a later reset record.
        return;
    }

    const auto sequence = next_sequence_.fetch_add(1, std::memory_order_relaxed);
    auto& packet = slots_[write & kQueueMask];
    packet = {};
    packet.kind = RawInputPacketKind::StreamReset;
    packet.reset_reason = reason;
    packet.sequence = sequence;
    packet.stream_epoch = stream_epoch_.fetch_add(1, std::memory_order_relaxed) + 1;
    packet.qpc = qpc;
    packet.qpc_exact = qpc_exact;
    packet.observed_qpc = qpc_exact ? 0 : observed_qpc;
    packet.clock = clock;
    packet.change_kind = change;
    packet.change_device = change_device;
    write_index_.store(write + 1, std::memory_order_release);
    if (clock.valid) capture_clock_snapshots_.fetch_add(1, std::memory_order_relaxed);
    PublishProducerCursor(qpc, qpc_exact, sequence, packet.stream_epoch);
}

void RawInputPacketQueue::MarkGap(std::int64_t qpc, const RawInputBoundary& boundary,
                                  const RawInputCaptureClock& clock, bool qpc_exact,
                                  std::int64_t observed_qpc) noexcept {
    MarkGap(qpc, boundary.reason, clock, boundary.device_change,
            boundary.device_handle, qpc_exact, observed_qpc);
}

bool RawInputPacketQueue::TryPop(RawInputPacket& packet) noexcept {
    if (!slots_) return false;
    const auto read = read_index_.load(std::memory_order_relaxed);
    const auto write = write_index_.load(std::memory_order_acquire);
    if (read == write) return false;

    // Copy the entire record before publishing space to the producer. Do not
    // touch the slot after advancing read_index_; the producer may overwrite it.
    packet = slots_[read & kQueueMask];
    read_index_.store(read + 1, std::memory_order_release);
    return true;
}

RawInputCaptureStats RawInputPacketQueue::Stats() const noexcept {
    RawInputCaptureStats stats;
    stats.stream_epoch = stream_epoch_.load(std::memory_order_acquire);
    stats.packets_captured = packets_captured_.load(std::memory_order_relaxed);
    stats.fences_captured = fences_captured_.load(std::memory_order_relaxed);
    stats.fences_dropped = fences_dropped_.load(std::memory_order_relaxed);
    stats.ignored_mouse_packets = ignored_mouse_packets_.load(std::memory_order_relaxed);
    stats.dropped_packets = dropped_packets_.load(std::memory_order_relaxed);
    stats.oversize_packets = oversize_packets_.load(std::memory_order_relaxed);
    stats.read_failures = read_failures_.load(std::memory_order_relaxed);
    stats.malformed_packets = malformed_packets_.load(std::memory_order_relaxed);
    stats.device_change_boundaries =
        device_change_boundaries_.load(std::memory_order_relaxed);
    stats.clock_reversals = clock_reversals_.load(std::memory_order_relaxed);
    stats.reentrant_boundaries = reentrant_boundaries_.load(std::memory_order_relaxed);
    stats.capture_clock_snapshots =
        capture_clock_snapshots_.load(std::memory_order_relaxed);
    return stats;
}

RawInputProducerCursor RawInputPacketQueue::ProducerCursor() const noexcept {
    for (unsigned attempt = 0; attempt < kCursorReadAttempts; ++attempt) {
        const std::uint64_t before = cursor_sequence_.load(std::memory_order_acquire);
        if (before == 0) return {};
        if ((before & 1U) != 0) continue;

        std::atomic_thread_fence(std::memory_order_acquire);
        const std::int64_t qpc = cursor_qpc_.load(std::memory_order_relaxed);
        const std::uint64_t record = cursor_record_.load(std::memory_order_relaxed);
        const std::uint64_t epoch = cursor_epoch_.load(std::memory_order_relaxed);
        const std::uint64_t flags = cursor_flags_.load(std::memory_order_relaxed);
        std::atomic_thread_fence(std::memory_order_acquire);
        const std::uint64_t after = cursor_sequence_.load(std::memory_order_acquire);
        if (before != after) continue;

        RawInputProducerCursor cursor;
        cursor.valid = (flags & 1U) != 0;
        cursor.qpc_exact = (flags & 2U) != 0;
        cursor.qpc = qpc;
        cursor.sequence = record;
        cursor.stream_epoch = epoch;
        return cursor;
    }
    return {};
}

void RawInputPacketQueue::PublishProducerCursor(std::int64_t qpc, bool qpc_exact,
                                                std::uint64_t sequence,
                                                std::uint64_t stream_epoch) noexcept {
    cursor_qpc_writer_ = qpc;
    cursor_exact_writer_ = qpc_exact;
    cursor_record_writer_ = sequence;
    cursor_epoch_writer_ = stream_epoch;
    has_cursor_ = true;

    const std::uint64_t before = cursor_sequence_.load(std::memory_order_relaxed);
    cursor_sequence_.store(before + 1, std::memory_order_relaxed);
    std::atomic_thread_fence(std::memory_order_release);
    cursor_qpc_.store(cursor_qpc_writer_, std::memory_order_relaxed);
    cursor_record_.store(cursor_record_writer_, std::memory_order_relaxed);
    cursor_epoch_.store(cursor_epoch_writer_, std::memory_order_relaxed);
    cursor_flags_.store((has_cursor_ ? 1U : 0U) | (cursor_exact_writer_ ? 2U : 0U),
                        std::memory_order_relaxed);
    cursor_sequence_.store(before + 2, std::memory_order_release);
}

bool IsRawInputCaptureMessage(int hook_code, WPARAM action,
                              const MSG& message, HWND target) noexcept {
    return hook_code == HC_ACTION && action == PM_REMOVE && target &&
        message.hwnd == target && message.message == WM_INPUT;
}

bool StartRawInputCapture(std::string& reason,
                          const SongClockPublication* clock) {
    auto& state = State();
    std::lock_guard lock(state.lifecycle);
    reason.clear();
    if (state.active.load(std::memory_order_acquire)) return true;
    if (state.ever_started.load(std::memory_order_acquire)) {
        reason = "raw-input capture cannot be restarted after it has stopped";
        return false;
    }
    if (!state.queue.Ready()) {
        reason = "could not allocate the bounded raw-input queue";
        return false;
    }

    HWND target = nullptr;
    DWORD thread_id = 0;
    if (!FindInputTarget(target, thread_id, reason)) return false;

    LARGE_INTEGER frequency{};
    if (!QueryPerformanceFrequency(&frequency) || frequency.QuadPart <= 0) {
        reason = "QueryPerformanceFrequency failed";
        return false;
    }

    state.target = target;
    state.clock = clock;
    state.thread_id.store(thread_id, std::memory_order_relaxed);
    state.qpc_frequency.store(frequency.QuadPart, std::memory_order_relaxed);
    // A failed partial installation may still have an in-flight callback.
    // Never mutate its target through a later installation attempt.
    state.ever_started.store(true, std::memory_order_release);
    state.active.store(true, std::memory_order_release);
    const HHOOK hook = SetWindowsHookExW(WH_GETMESSAGE, GetMessageHook, nullptr, thread_id);
    if (!hook) {
        state.active.store(false, std::memory_order_release);
        state.target = nullptr;
        state.clock = nullptr;
        state.thread_id.store(0, std::memory_order_relaxed);
        state.qpc_frequency.store(0, std::memory_order_relaxed);
        reason = "SetWindowsHookEx(WH_GETMESSAGE) failed with error " +
                 std::to_string(GetLastError());
        return false;
    }
    state.hook.store(hook, std::memory_order_release);

    const HHOOK call_hook =
        SetWindowsHookExW(WH_CALLWNDPROC, CallWndProcHook, nullptr, thread_id);
    if (!call_hook) {
        state.active.store(false, std::memory_order_release);
        const auto installed = state.hook.exchange(nullptr, std::memory_order_acq_rel);
        if (installed) (void)UnhookWindowsHookEx(installed);
        state.clock = nullptr;
        reason = "SetWindowsHookEx(WH_CALLWNDPROC) failed with error " +
                 std::to_string(GetLastError());
        return false;
    }
    state.call_hook.store(call_hook, std::memory_order_release);

    // The heartbeat only posts WM_NULL to the input thread; every QPC read and
    // queue mutation still happens on that producer thread.
    // It starts only when a clock publication is supplied, so the existing
    // diagnostic capture keeps its previous message load.
    if (clock == nullptr) return true;
    try {
        state.heartbeat_stop.store(false, std::memory_order_relaxed);
        state.heartbeat = std::thread(HeartbeatLoop, target, &state.heartbeat_stop);
    } catch (...) {
        state.active.store(false, std::memory_order_release);
        const auto installed_call =
            state.call_hook.exchange(nullptr, std::memory_order_acq_rel);
        const auto installed = state.hook.exchange(nullptr, std::memory_order_acq_rel);
        if (installed_call) (void)UnhookWindowsHookEx(installed_call);
        if (installed) (void)UnhookWindowsHookEx(installed);
        state.clock = nullptr;
        reason = "could not start the producer heartbeat thread";
        return false;
    }
    return true;
}

void StopRawInputCapture() noexcept {
    auto& state = State();
    std::lock_guard lock(state.lifecycle);
    if (!state.active.exchange(false, std::memory_order_acq_rel)) return;
    state.heartbeat_stop.store(true, std::memory_order_release);
    if (state.heartbeat.joinable()) state.heartbeat.join();
    const auto hook = state.hook.exchange(nullptr, std::memory_order_acq_rel);
    const auto call_hook = state.call_hook.exchange(nullptr, std::memory_order_acq_rel);
    if (hook) (void)UnhookWindowsHookEx(hook);
    if (call_hook) (void)UnhookWindowsHookEx(call_hook);
}

bool TryPopRawInputPacket(RawInputPacket& packet) noexcept {
    return State().queue.TryPop(packet);
}

RawInputPacketQueue& RawInputCaptureQueue() noexcept {
    return State().queue;
}

RawInputCaptureStats RawInputCaptureStatsNow() noexcept {
    auto& state = State();
    auto stats = state.queue.Stats();
    stats.active = state.active.load(std::memory_order_acquire);
    stats.input_thread_id = state.thread_id.load(std::memory_order_relaxed);
    stats.qpc_frequency = state.qpc_frequency.load(std::memory_order_relaxed);
    return stats;
}

} // namespace nosfix
