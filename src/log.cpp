#include "nosfix/log.hpp"
#include "nosfix/config.hpp"

#include <Windows.h>

#include <fstream>
#include <mutex>

namespace nosfix {
namespace {

std::mutex g_mutex;
LogLevel g_level = LogLevel::Info;

const wchar_t* LevelName(LogLevel level) noexcept {
    switch (level) {
    case LogLevel::Error: return L"ERROR";
    case LogLevel::Warning: return L"WARN";
    case LogLevel::Info: return L"INFO";
    case LogLevel::Debug: return L"DEBUG";
    }
    return L"?";
}

} // namespace

void SetLogLevel(LogLevel level) noexcept {
    std::lock_guard lock(g_mutex);
    g_level = level;
}

void Log(LogLevel level, std::string_view message) noexcept {
    try {
        std::lock_guard lock(g_mutex);
        if (static_cast<int>(level) > static_cast<int>(g_level)) return;

        const auto path = DllDirectory() / L"nosfix.log";
        std::wofstream file(path, std::ios::app);
        if (!file) return;

        SYSTEMTIME now{};
        GetLocalTime(&now);
        const int wide_length = MultiByteToWideChar(CP_UTF8, 0, message.data(),
            static_cast<int>(message.size()), nullptr, 0);
        std::wstring wide_message(static_cast<size_t>(wide_length), L'\0');
        if (wide_length > 0) {
            MultiByteToWideChar(CP_UTF8, 0, message.data(),
                static_cast<int>(message.size()), wide_message.data(), wide_length);
        }
        file << L'[' << now.wYear << L'-' << now.wMonth << L'-' << now.wDay
             << L' ' << now.wHour << L':' << now.wMinute << L':' << now.wSecond
             << L'.' << now.wMilliseconds << L"] [" << LevelName(level) << L"] "
             << wide_message << L'\n';
    } catch (...) {
    }
}

} // namespace nosfix
