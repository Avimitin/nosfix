#pragma once

#include <Windows.h>
#include <string>

namespace nosfix {
struct Config;

// Called from Spice's synchronous SDK entry point, before the game creates D3D.
[[nodiscard]] bool PreparePresentation(HMODULE module, const Config& config,
                                       std::string& reason);
[[nodiscard]] bool CommitPresentation(std::string& reason);
void RollbackPresentation() noexcept;
[[nodiscard]] unsigned PresentationRate() noexcept;
} // namespace nosfix
