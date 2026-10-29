#pragma once

#include <string_view>

namespace nosfix {

enum class LogLevel : int { Error = 0, Warning = 1, Info = 2, Debug = 3 };

void SetLogLevel(LogLevel level) noexcept;
void Log(LogLevel level, std::string_view message) noexcept;

} // namespace nosfix
