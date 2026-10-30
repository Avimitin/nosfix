#pragma once

#include "nosfix/config.hpp"

#include <Windows.h>
#include <string>

namespace nosfix {

// Experimental native ASIO route: creates the bridge/host and replaces
// libxact's XAudio2Create helper (0x180003730) with the bridge engine. The
// original engine is used when this is disabled or any step fails.
[[nodiscard]] bool ConfigureAsioRoute(const Config& config, HMODULE xact,
                                      std::string& reason);
void DisableAsioRoute() noexcept;
[[nodiscard]] bool AsioRouteActive() noexcept;

} // namespace nosfix
