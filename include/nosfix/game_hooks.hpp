#pragma once

#include <Windows.h>

#include <string>

namespace nosfix {

struct Config;

[[nodiscard]] bool InstallGameHooks(HMODULE module, const Config& config,
                                    std::string& reason);
void RemoveGameHooks() noexcept;

} // namespace nosfix
