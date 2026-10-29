#include "nosfix/config.hpp"
#include "nosfix/game_hooks.hpp"
#include "nosfix/log.hpp"
#include "nosfix/module_guard.hpp"

#include <Windows.h>

#include <cstdint>
#include <exception>
#include <string>

namespace {

using SpiceDestroyCallback = void(__cdecl*)();
using SpiceInitFunction = int(__cdecl)(std::uint32_t,
                                        SpiceDestroyCallback,
                                        void*);

std::string NarrowUtf8(const std::wstring& text) {
    if (text.empty()) return {};
    const int count = WideCharToMultiByte(CP_UTF8, 0, text.data(),
        static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
    if (count <= 0) return "(message conversion failed)";
    std::string result(static_cast<size_t>(count), '\0');
    WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()),
                        result.data(), count, nullptr, nullptr);
    return result;
}

int InitializeFromSpiceSdk() {
    using namespace nosfix;
    const Config config = LoadConfig(DllDirectory() / L"nosfix.ini");
    SetLogLevel(static_cast<LogLevel>(config.log_level));
    Log(LogLevel::Info, "nosfix " NOSFIX_VERSION_STRING " initialized synchronously by the Spice SDK");

    if (!config.enabled) {
        Log(LogLevel::Info, "hooks disabled by adjacent nosfix.ini");
        return 0;
    }

    ModuleIdentity identity;
    std::wstring guard_reason;
    if (!VerifyNostalgiaModule(identity, guard_reason)) {
        Log(LogLevel::Error, "version guard rejected target: " + NarrowUtf8(guard_reason));
        return 1;
    }

    std::string reason;
    if (!InstallGameHooks(identity.module, config, reason)) {
        Log(LogLevel::Error, "nosfix initialization failed: " + reason);
        return 1;
    }
    return 0;
}

} // namespace

// spice2x resolves this SDK export and invokes it outside the loader lock,
// before game main and before Nostalgia creates its D3D device.
extern "C" __declspec(dllexport) int __cdecl
spice_sdk_entry_point(SpiceInitFunction* init) {
    (void)init; // nosfix does not need SDK services or callbacks.
    try {
        return InitializeFromSpiceSdk();
    } catch (const std::exception& exception) {
        nosfix::RemoveGameHooks();
        nosfix::Log(nosfix::LogLevel::Error,
                    std::string("exception during Spice SDK initialization: ") + exception.what());
        return 1;
    } catch (...) {
        nosfix::RemoveGameHooks();
        nosfix::Log(nosfix::LogLevel::Error,
                    "unknown exception during Spice SDK initialization");
        return 1;
    }
}

BOOL WINAPI DllMain(HINSTANCE, DWORD, LPVOID) {
    // /MT and thread_local state require CRT thread notifications. All actual
    // plugin initialization belongs to the synchronous SDK entry point above.
    return TRUE;
}
