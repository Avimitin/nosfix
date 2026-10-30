#include "nosfix/config.hpp"

#include <Windows.h>

#include <algorithm>
#include <fstream>
#include <string>

namespace nosfix {
namespace {

std::string Trim(std::string value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

} // namespace

std::filesystem::path DllDirectory() {
    HMODULE self = nullptr;
    const auto address = reinterpret_cast<LPCWSTR>(&DllDirectory);
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                                GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                            address, &self)) {
        return std::filesystem::current_path();
    }

    std::wstring path(32768, L'\0');
    const DWORD length = GetModuleFileNameW(self, path.data(),
                                             static_cast<DWORD>(path.size()));
    if (length == 0 || length >= path.size()) return std::filesystem::current_path();
    path.resize(length);
    return std::filesystem::path(path).parent_path();
}

Config LoadConfig(const std::filesystem::path& path) {
    Config config;
    std::ifstream input(path);
    std::string line;
    while (std::getline(input, line)) {
        line = Trim(std::move(line));
        if (line.empty() || line.front() == ';' || line.front() == '#') continue;
        const auto equals = line.find('=');
        if (equals == std::string::npos) continue;
        const auto key = Trim(line.substr(0, equals));
        const auto value = Trim(line.substr(equals + 1));
        if (key == "enabled") {
            config.enabled = value == "1" || value == "true" || value == "yes";
        } else if (key == "smooth_motion") {
            config.smooth_motion = value == "1" || value == "true" || value == "yes";
        } else if (key == "smooth_ui") {
            config.smooth_ui = value == "1" || value == "true" || value == "yes";
        } else if (key == "precise_audio_clock") {
            config.precise_audio_clock = value == "1" || value == "true" || value == "yes";
        } else if (key == "clock_motion") {
            config.clock_motion = value == "1" || value == "true" || value == "yes";
        } else if (key == "skip_monitor_check") {
            config.skip_monitor_check = value == "1" || value == "true" || value == "yes";
        } else if (key == "capture_input") {
            config.capture_input = value == "1" || value == "true" || value == "yes";
        } else if (key == "native_shadow") {
            config.native_shadow = value == "1" || value == "true" || value == "yes";
        } else if (key == "diagnostic_probe") {
            config.diagnostic_probe = value == "1" || value == "true" || value == "yes";
        } else if (key == "timeline_probe") {
            config.timeline_probe = value == "1" || value == "true" || value == "yes";
        } else if (key == "native_shadow_feed") {
            config.native_shadow_feed = value == "1" || value == "true" || value == "yes";
        } else if (key == "asio_output") {
            config.asio_output = value == "1" || value == "true" || value == "yes";
        } else if (key == "asio_driver_clsid") {
            config.asio_driver_clsid = value;
        } else if (key == "asio_driver_name") {
            config.asio_driver_name = value;
        } else if (key == "asio_output_channels") {
            if (!value.empty()) config.asio_output_channels = value;
        } else if (key == "asio_sample_rate") {
            try {
                const auto rate = static_cast<unsigned>(std::stoul(value));
                if (rate >= 8000 && rate <= 384000) config.asio_sample_rate = rate;
            } catch (...) {
            }
        } else if (key == "asio_buffer_frames") {
            try {
                const auto frames = static_cast<unsigned>(std::stoul(value));
                if (frames >= 16 && frames <= 8192) config.asio_buffer_frames = frames;
            } catch (...) {
            }
        } else if (key == "target_hz") {
            if (value == "auto" || value == "0") {
                config.target_hz = 0;
            } else {
                try {
                    const auto rate = static_cast<unsigned>(std::stoul(value));
                    switch (rate) {
                    case 60: case 90: case 120: case 144: case 165: case 240: case 360:
                        config.target_hz = rate;
                        break;
                    default:
                        config.target_hz = 0;
                        break;
                    }
                } catch (...) {
                    config.target_hz = 0;
                }
            }
        } else if (key == "log_level") {
            try {
                config.log_level = std::clamp(std::stoi(value), 0, 3);
            } catch (...) {
                if (value == "error") config.log_level = 0;
                else if (value == "warn" || value == "warning") config.log_level = 1;
                else if (value == "debug") config.log_level = 3;
                else config.log_level = 2;
            }
        }
    }
    return config;
}

} // namespace nosfix
