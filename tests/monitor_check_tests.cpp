#include "nosfix/monitor_check.hpp"
#include "nosfix/module_guard.hpp"
#include "nosfix/log.hpp"

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace nosfix {
void Log(LogLevel, std::string_view) noexcept {}
}
namespace {
[[noreturn]] void Fail(const char* message) {
    std::fprintf(stderr, "%s\n", message);
    std::exit(1);
}
template<class T> T Read(const void* base, std::size_t offset) {
    T value{};
    std::memcpy(&value, static_cast<const unsigned char*>(base) + offset, sizeof(value));
    return value;
}
template<class T> void Write(void* base, std::size_t offset, T value) {
    std::memcpy(static_cast<unsigned char*>(base) + offset, &value, sizeof(value));
}
bool completed = false;
std::intptr_t __fastcall Complete(void*, std::intptr_t argument) {
    if (argument != 0) Fail("native completion passed an unexpected argument");
    completed = true;
    return 7;
}
}

// Executes only the original initializer and its completion callback, which
// have no imports. The callback's virtual scene operation is replaced by a
// test double. This validates the real relocated branch and transition ABI.
int wmain(int argc, wchar_t** argv) {
    if (argc != 2) Fail("expected analyzed nostalgia.dll path");
    HMODULE game = LoadLibraryExW(argv[1], nullptr, DONT_RESOLVE_DLL_REFERENCES);
    if (!game) Fail("cannot map analyzed game");
    nosfix::ModuleIdentity identity;
    std::wstring why;
    if (!nosfix::VerifyNostalgiaModule(identity, why) || identity.module != game)
        Fail("game failed version guard");
    auto* base = reinterpret_cast<unsigned char*>(game);
    std::array<unsigned char, 24> original{};
    std::memcpy(original.data(), base + 0x218CF0, original.size());
    std::vector<nosfix::HookSpec> specs;
    nosfix::AppendMonitorCheckHook(specs);
    std::string reason;
    if (!nosfix::InstallHooks(game, specs, reason)) {
        std::fprintf(stderr, "%s\n", reason.c_str());
        Fail("could not install monitor hook");
    }
    using Callback = std::intptr_t(__fastcall*)(void*);
    auto initialize = reinterpret_cast<Callback>(base + 0x218CF0);
    alignas(16) std::array<unsigned char, 320> scene{};
    std::array<void*, 12> vtable{};
    vtable[11] = reinterpret_cast<void*>(&Complete);
    Write(scene.data(), 0, vtable.data());

    // Inactive hook retains original calibration and conversion constants.
    Write(base, 0x12D342F, std::uint8_t{0});
    Write(base, 0x70B2F8, 59.75);
    Write(base, 0x70B300, 59.75f);
    Write(base, 0x70B304, std::int32_t{59});
    initialize(scene.data());
    if (Read<void*>(scene.data(), 208) != base + 0x218AA0 ||
        Read<double>(base, 0x70B2F8) != 59.75 ||
        Read<float>(base, 0x70B300) != 59.75f ||
        Read<std::int32_t>(base, 0x70B304) != 59)
        Fail("inactive hook changed calibration behavior");

    // Active hook uses the game's existing already-completed branch.
    nosfix::ConfigureMonitorCheck(game, true);
    Write(base, 0x12D342F, std::uint8_t{0});
    Write(scene.data(), 120, 123.5); // Calibration accumulator remains untouched.
    if (initialize(scene.data()) != 0) Fail("initializer return value changed");
    if (Read<void*>(scene.data(), 208) != base + 0x218A90 ||
        Read<std::int32_t>(scene.data(), 216) != 0 ||
        Read<std::int32_t>(scene.data(), 224) != 0 ||
        Read<std::int32_t>(scene.data(), 244) != 0 ||
        Read<std::uint8_t>(scene.data(), 260) != 1 ||
        Read<double>(scene.data(), 120) != 123.5)
        Fail("native scene completion was not initialized correctly");
    if (Read<double>(base, 0x70B2F8) != 60.0 ||
        Read<float>(base, 0x70B300) != 60.0f ||
        Read<std::int32_t>(base, 0x70B304) != 60 ||
        Read<std::uint8_t>(base, 0x12D342F) != 1)
        Fail("legacy units or completion flag incorrect");
    const auto callback = Read<Callback>(scene.data(), 208);
    if (callback(scene.data()) != 7 || !completed)
        Fail("native completion callback did not reach the scene operation");
    initialize(scene.data());
    if (Read<Callback>(scene.data(), 208) != callback)
        Fail("repeated initialization changed completion path");

    nosfix::ConfigureMonitorCheck(nullptr, false);
    nosfix::RemoveHooks();
    if (std::memcmp(original.data(), base + 0x218CF0, original.size()))
        Fail("hook removal did not restore initializer");
    FreeLibrary(game);
    std::puts("Native FPS-check branch, completion ABI, legacy constants and hook rollback passed.");
}
