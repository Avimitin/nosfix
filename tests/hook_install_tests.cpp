#include "nosfix/hooks.hpp"
#include "nosfix/module_guard.hpp"

#include <Windows.h>
#include <array>
#include <cstdio>
#include <cstring>
#include <string>

namespace {
[[noreturn]] void Fail(const char* text) {
    std::fprintf(stderr, "%s\n", text);
    std::exit(1);
}
std::intptr_t __fastcall UnusedDetour(void*) { return 0; }
constexpr std::array<DWORD, 4> kTargets{0x1C31C0, 0x355CD0, 0x355170, 0x2FC9D0};
}

// Never runs any game instruction or resolves its imported dependencies. This
// disposable process checks real PE mapping, the hash guard, and MinHook's x64
// instruction relocation/rollback against the actual analyzed build.
int wmain(int argc, wchar_t** argv) {
    if (argc != 2) Fail("usage: hook_install_tests path-to-analyzed-nostalgia.dll");
    HMODULE module = LoadLibraryExW(argv[1], nullptr, DONT_RESOLVE_DLL_REFERENCES);
    if (!module) Fail("could not map analyzed DLL without initialization");
    nosfix::ModuleIdentity identity;
    std::wstring guard_reason;
    if (!nosfix::VerifyNostalgiaModule(identity, guard_reason)) {
        std::fwprintf(stderr, L"guard: %ls\n", guard_reason.c_str());
        Fail("analyzed DLL was rejected by version guard");
    }
    if (identity.module != module) Fail("guard resolved a different module");
    auto* base = reinterpret_cast<unsigned char*>(module);
    std::array<std::array<unsigned char, 24>, kTargets.size()> bytes{};
    std::array<void*, kTargets.size()> originals{};
    std::array<nosfix::HookSpec, kTargets.size()> specs{};
    for (size_t i = 0; i < kTargets.size(); ++i) {
        std::memcpy(bytes[i].data(), base + kTargets[i], bytes[i].size());
        specs[i] = {"offline target", kTargets[i], bytes[i],
                    reinterpret_cast<void*>(&UnusedDetour), &originals[i]};
    }

    // A mismatch in the last target must reject the entire set before writing
    // the first target, even though the first two expected prologues are valid.
    bytes.back()[0] ^= 1;
    std::string reason;
    if (nosfix::InstallHooks(module, specs, reason)) Fail("mismatched target accepted");
    bytes.back()[0] ^= 1;
    for (size_t i = 0; i < kTargets.size(); ++i)
        if (std::memcmp(base + kTargets[i], bytes[i].data(), bytes[i].size()))
            Fail("failed validation modified a target");

    // Real hooks are installed in this inert image only; no target is called.
    if (!nosfix::InstallHooks(module, specs, reason)) {
        std::fprintf(stderr, "%s\n", reason.c_str());
        Fail("MinHook could not relocate the real target prologues");
    }
    for (size_t i = 0; i < kTargets.size(); ++i) {
        if (!originals[i]) Fail("missing trampoline");
        if (!std::memcmp(base + kTargets[i], bytes[i].data(), bytes[i].size()))
            Fail("target remained unpatched");
    }
    nosfix::RemoveHooks();
    for (size_t i = 0; i < kTargets.size(); ++i) {
        if (originals[i]) Fail("rollback left a trampoline pointer");
        if (std::memcmp(base + kTargets[i], bytes[i].data(), bytes[i].size()))
            Fail("rollback did not restore original bytes");
    }
    FreeLibrary(module);
    std::puts("Real-image hash, mismatch rejection, x64 relocation and rollback passed.");
}
