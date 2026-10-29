#pragma once

#include "nosfix/hooks.hpp"

namespace nosfix {

// Skips only the startup FPS calibration. The existing scene completion path
// still runs. Legacy chart units remain 60 until their consumers are migrated.
void AppendMonitorCheckHook(std::vector<HookSpec>& specs);
void ConfigureMonitorCheck(HMODULE game, bool enabled) noexcept;

} // namespace nosfix
