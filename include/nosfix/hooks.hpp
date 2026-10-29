#pragma once

#include <Windows.h>

#include <span>
#include <string>
#include <vector>

namespace nosfix {

struct HookSpec {
    const char* name;
    DWORD rva;
    std::span<const unsigned char> expected;
    void* detour;
    void** original;
};

// Validates every target before changing code, then creates and enables the set
// as one unit. Any failure disables/removes the hooks already installed.
[[nodiscard]] bool InstallHooks(HMODULE module, std::span<const HookSpec> specs,
                                std::string& reason);
void RemoveHooks() noexcept;

} // namespace nosfix
