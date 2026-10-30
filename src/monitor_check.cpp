#include "nosfix/monitor_check.hpp"
#include "nosfix/log.hpp"

#include <array>
#include <atomic>
#include <cstdint>
#include <cstring>

namespace nosfix {
namespace {
using Initialize = std::intptr_t(__fastcall*)(void*);
Initialize g_initialize = nullptr;
std::atomic<unsigned char*> g_base{nullptr};

constexpr DWORD kInitializeRva = 0x218CF0;
constexpr std::array<unsigned char, 24> kInitializeBytes{
    0x48,0x83,0xEC,0x18,0x80,0x3D,0x34,0xA7,0x0B,0x01,0x00,0x74,
    0x09,0x48,0x8D,0x05,0x8C,0xFD,0xFF,0xFF,0xEB,0x0E,0xC6,0x05};

template<class T> void Write(unsigned char* base, DWORD rva, T value) noexcept {
    std::memcpy(base + rva, &value, sizeof(value));
}

std::intptr_t __fastcall InitializeDetour(void* scene) {
    if (auto* base = g_base.load(std::memory_order_acquire)) {
        // Original initializer chooses the completion callback when this flag
        // is set. No rate constants are written: the native timeline uses the
        // absolute microsecond conversion manifest instead.
        Write(base, 0x12D342F, std::uint8_t{1});
        Log(LogLevel::Info, "startup FPS calibration skipped via native completion path");
    }
    return g_initialize ? g_initialize(scene) : 0;
}
} // namespace

void AppendMonitorCheckHook(std::vector<HookSpec>& specs) {
    specs.push_back({"FPS calibration initializer", kInitializeRva, kInitializeBytes,
        reinterpret_cast<void*>(&InitializeDetour), reinterpret_cast<void**>(&g_initialize)});
}

void ConfigureMonitorCheck(HMODULE game, bool enabled) noexcept {
    g_base.store(enabled ? reinterpret_cast<unsigned char*>(game) : nullptr,
                 std::memory_order_release);
}
} // namespace nosfix
