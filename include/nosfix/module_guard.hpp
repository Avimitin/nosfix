#pragma once

#include <Windows.h>
#include <string>

namespace nosfix {

struct ModuleIdentity {
    HMODULE module = nullptr;
    std::wstring path;
    DWORD image_size = 0;
};

[[nodiscard]] bool VerifyNostalgiaModule(ModuleIdentity& out, std::wstring& reason);
[[nodiscard]] bool VerifyAfpModule(ModuleIdentity& out, std::wstring& reason);
[[nodiscard]] bool VerifyLibxactModule(ModuleIdentity& out, std::wstring& reason);
[[nodiscard]] bool VerifySpiceModule(ModuleIdentity& out, std::wstring& reason);

} // namespace nosfix
