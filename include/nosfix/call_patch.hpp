#pragma once

#include <Windows.h>

#include <span>
#include <string>
#include <vector>

namespace nosfix {

struct CallPatchSpec {
    const char* name;
    DWORD rva;
    std::span<const unsigned char> expected_call;
    void* destination;
    HMODULE module = nullptr; // Null uses InstallCallPatches' default module.
};

// Redirects a complete set of verified five-byte x64 CALL instructions through
// nearby absolute-jump stubs. Validation/allocation completes before code is
// written; any failure restores bytes and releases stubs already committed.
[[nodiscard]] bool InstallCallPatches(HMODULE module,
                                      std::span<const CallPatchSpec> specs,
                                      std::string& reason);
void RemoveCallPatches() noexcept;

} // namespace nosfix
