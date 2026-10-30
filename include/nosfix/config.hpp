#pragma once

#include <filesystem>
#include <string>

namespace nosfix {

struct Config {
    bool enabled = true;
    bool precise_audio_clock = true;
    bool skip_monitor_check = true;
    bool capture_input = false; // Experimental raw event observation; no judgments yet.
    bool native_shadow = false; // Publish the shadow song clock; no judgments yet.
    bool diagnostic_probe = false; // Read-only PROBE log lines for live samples.
    bool asio_output = false; // Experimental native ASIO route; off by default.
    std::string asio_driver_clsid;
    std::string asio_driver_name;
    std::string asio_output_channels = "0,1";
    unsigned asio_sample_rate = 44100;
    unsigned asio_buffer_frames = 128;
    unsigned target_hz = 0; // 0 selects the primary display's current rate.
    int log_level = 2;
};

[[nodiscard]] std::filesystem::path DllDirectory();
[[nodiscard]] Config LoadConfig(const std::filesystem::path& path);

} // namespace nosfix
