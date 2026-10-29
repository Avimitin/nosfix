#include "nosfix/raw_input_capture.hpp"

#include <dbt.h>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <thread>

namespace {
constexpr UINT kProbe = WM_APP + 91;
constexpr UINT kFinish = WM_APP + 92;
constexpr LPARAM kInvalidRawHandle = 1;
HANDLE ready = nullptr;
HANDLE processed = nullptr;
std::atomic<HWND> window{nullptr};
std::atomic<unsigned> raw_forwarded{0};
std::atomic<bool> unchanged{true};

[[noreturn]] void Fail(const char* text) {
    std::fprintf(stderr, "%s (error=%lu)\n", text, GetLastError());
    std::exit(1);
}
LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
    if (message == kProbe) {
        // No hardware input is synthesized. The deliberately invalid handle
        // exercises read failure while proving the original message survives.
        PostMessageW(hwnd, WM_INPUT, RIM_INPUT, kInvalidRawHandle);
        MSG peek{};
        if (!PeekMessageW(&peek, hwnd, WM_INPUT, WM_INPUT, PM_NOREMOVE))
            unchanged.store(false);
        SetEvent(processed);
        return 0;
    }
    if (message == WM_INPUT && lparam == kInvalidRawHandle) {
        if (wparam != RIM_INPUT) unchanged.store(false);
        raw_forwarded.fetch_add(1);
        // This is a synthetic message, not an OS-owned HRAWINPUT to clean up.
        SetEvent(processed);
        return 0;
    }
    if (message == WM_DEVICECHANGE) return 1;
    if (message == kFinish) {
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, message, wparam, lparam);
}

void InputThread() {
    WNDCLASSEXW wc{sizeof(wc)};
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpfnWndProc = WindowProc;
    wc.lpszClassName = L"SpiceTools Input";
    if (!RegisterClassExW(&wc)) Fail("could not register isolated test class");
    const auto hwnd = CreateWindowExW(0, wc.lpszClassName, L"isolated input test",
        0, 0, 0, 0, 0, nullptr, nullptr, wc.hInstance, nullptr);
    if (!hwnd) Fail("could not create isolated input window");
    RAWINPUTDEVICE registration{1, 6, RIDEV_INPUTSINK, hwnd};
    if (!RegisterRawInputDevices(&registration, 1, sizeof(registration)))
        Fail("could not register keyboard in isolated test process");
    window.store(hwnd, std::memory_order_release);
    SetEvent(ready);
    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0) DispatchMessageW(&message);
    registration.dwFlags = RIDEV_REMOVE;
    registration.hwndTarget = nullptr;
    RegisterRawInputDevices(&registration, 1, sizeof(registration));
    DestroyWindow(hwnd);
    UnregisterClassW(wc.lpszClassName, wc.hInstance);
}
}

int main() {
    ready = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    processed = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    if (!ready || !processed) Fail("cannot create synchronization events");
    std::thread thread(InputThread);
    if (WaitForSingleObject(ready, 5000) != WAIT_OBJECT_0) Fail("input thread not ready");
    const auto hwnd = window.load(std::memory_order_acquire);
    std::string reason;
    if (!nosfix::StartRawInputCapture(reason)) {
        std::fprintf(stderr, "%s\n", reason.c_str());
        Fail("could not attach observer to the test input thread");
    }

    // Synchronous send returns after the probe has peeked but not removed.
    SendMessageW(hwnd, kProbe, 0, 0);
    // Queue a synchronous barrier. The input thread can handle it before the
    // posted WM_INPUT, so wait explicitly for that original message as well.
    for (unsigned i = 0; i < 100 && raw_forwarded.load() == 0; ++i)
        WaitForSingleObject(processed, 10);
    if (raw_forwarded.load() != 1 || !unchanged.load())
        Fail("input observer changed or suppressed the original message");
    const auto before = nosfix::RawInputCaptureStatsNow();
    if (!before.active || before.read_failures != 1)
        Fail("PM_NOREMOVE was double-counted or removed input was not observed");

    // Windows validates this message's payload before delivering it across
    // threads. A null lParam would test rejected SendMessage, not the hook.
    DEV_BROADCAST_DEVICEINTERFACE_W notice{};
    notice.dbcc_size = sizeof(notice);
    notice.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
    if (SendMessageW(hwnd, WM_DEVICECHANGE, DBT_DEVICEREMOVECOMPLETE,
                     reinterpret_cast<LPARAM>(&notice)) != 1)
        Fail("test disconnect notification did not reach its window");
    const auto after = nosfix::RawInputCaptureStatsNow();
    if (after.device_change_boundaries <= before.device_change_boundaries)
        Fail("sent disconnect notification did not invalidate the stream");

    nosfix::StopRawInputCapture();
    if (nosfix::RawInputCaptureStatsNow().active) Fail("observer did not stop");
    SendMessageW(hwnd, WM_DEVICECHANGE, DBT_DEVICEARRIVAL,
                 reinterpret_cast<LPARAM>(&notice));
    if (nosfix::RawInputCaptureStatsNow().device_change_boundaries !=
        after.device_change_boundaries) Fail("stopped observer still processed messages");
    if (nosfix::StartRawInputCapture(reason)) Fail("unsupported restart was accepted");
    PostMessageW(hwnd, kFinish, 0, 0);
    thread.join();
    CloseHandle(ready);
    CloseHandle(processed);
    std::puts("Actual input thread hook filtering, forwarding, sent disconnect and unhook passed.");
}
