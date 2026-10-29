#pragma once

#include "nosfix/asio_output.hpp"

#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <string>
#include <thread>
#include <utility>
#include <vector>

// Deterministic in-process ASIO driver double. It implements the same
// AsioDriver interface as the COM adapter and invokes the host callbacks with
// preallocated planar buffers, exactly where a real driver would.
class FakeAsioDriver final : public nosfix::audio::AsioDriver {
public:
    struct Config {
        std::int32_t outputs = 8;
        std::int32_t inputs = 0;
        double sample_rate = 44100.0;
        bool can_sample_rate = true;
        nosfix::audio::AsioBufferRange range{64, 1024, 256, -1};
        nosfix::audio::AsioSampleFormat format =
            nosfix::audio::AsioSampleFormat::Float32Lsb;
        std::vector<nosfix::audio::AsioSampleFormat> channel_formats;
        std::int32_t output_latency = 256;
        std::int32_t input_latency = 0;
        bool fail_latency = false;
        bool fail_init = false;
        bool fail_create = false;
        bool fail_start = false;
        bool callback_inside_start = false;
        bool callback_thread_inside_start = false;
        std::string name = "Fake ASIO";
    };

    explicit FakeAsioDriver(Config config = {}) : config_(std::move(config)) {}

    // --- inspection -------------------------------------------------------
    std::int32_t start_count = 0;
    std::int32_t stop_count = 0;
    std::int32_t dispose_count = 0;
    std::int32_t create_count = 0;
    std::int32_t output_ready_count = 0;
    std::int32_t control_panel_count = 0;
    std::int32_t shutdown_count = 0;
    std::int32_t init_count = 0;
    std::int32_t last_create_frames = 0;
    std::int32_t last_create_channels = 0;

    [[nodiscard]] std::int32_t frames() const noexcept { return frames_; }
    [[nodiscard]] std::int32_t fire_count() const noexcept { return fire_count_; }
    [[nodiscard]] std::int32_t next_index() const noexcept { return buffer_index_; }

    void SetOutputLatency(std::int32_t frames) noexcept {
        config_.output_latency = frames;
    }

    void SetFailStart(bool fail) noexcept { config_.fail_start = fail; }
    void SetFailCreate(bool fail) noexcept { config_.fail_create = fail; }

    [[nodiscard]] const std::uint8_t* Data(std::int32_t slot,
                                           std::int32_t index) const noexcept {
        if (slot < 0 || static_cast<std::size_t>(slot) >= buffer_storage_.size()) {
            return nullptr;
        }
        if (index < 0 || index > 1) return nullptr;
        return buffer_storage_[static_cast<std::size_t>(slot)]
                              [static_cast<std::size_t>(index)]
                                  .data();
    }

    template <typename T>
    [[nodiscard]] T Value(std::int32_t slot, std::int32_t index,
                          std::int32_t frame) const noexcept {
        T value{};
        const std::uint8_t* data = Data(slot, index);
        if (data != nullptr) {
            std::memcpy(&value, data + static_cast<std::size_t>(frame) * sizeof(T),
                        sizeof(T));
        }
        return value;
    }

    // --- driver simulation -------------------------------------------------
    void Fire(bool with_time_info = true, std::int64_t sample_position = -1) {
        if (callbacks_.buffer_switch_time_info == nullptr ||
            callbacks_.buffer_switch == nullptr) {
            return;
        }
        if (sample_position < 0) sample_position = next_sample_position_;
        nosfix::audio::AsioTime time{};
        time.time_info.speed = 1.0;
        time.time_info.sample_rate = config_.sample_rate;
        time.time_info.system_time_ns = SampleToNanoseconds(sample_position);
        time.time_info.sample_position = sample_position;
        time.time_info.flags = 1U | 2U | 4U;
        if (with_time_info) {
            callbacks_.buffer_switch_time_info(callbacks_.context, &time,
                                               buffer_index_, 0);
        } else {
            callbacks_.buffer_switch(callbacks_.context, buffer_index_, 0);
        }
        buffer_index_ ^= 1;
        if (sample_position <= std::numeric_limits<std::int64_t>::max() - frames_) {
            next_sample_position_ = sample_position + frames_;
        } else {
            next_sample_position_ = sample_position;
        }
        ++fire_count_;
    }

    void FireLegacy() { Fire(false); }

    void FireSampleRate(double rate) {
        config_.sample_rate = rate;
        if (callbacks_.sample_rate_changed != nullptr) {
            callbacks_.sample_rate_changed(callbacks_.context, rate);
        }
    }

    std::int32_t MessageToHost(std::int32_t selector, std::int32_t value = 0) {
        if (callbacks_.message == nullptr) return 0;
        return callbacks_.message(callbacks_.context, selector, value, nullptr,
                                  nullptr);
    }

    // --- AsioDriver --------------------------------------------------------
    bool Init(void* sys_ref, nosfix::audio::AsioDriverInfo& info,
              std::string& error) noexcept override {
        (void)sys_ref;
        ++init_count;
        if (config_.fail_init) {
            error = "fake init failure";
            return false;
        }
        info.name = config_.name;
        info.asio_version = 2;
        info.driver_version = 0x1234;
        return true;
    }

    void Shutdown() noexcept override { ++shutdown_count; }

    bool GetChannelCounts(std::int32_t& inputs, std::int32_t& outputs,
                          std::string& error) noexcept override {
        (void)error;
        inputs = config_.inputs;
        outputs = config_.outputs;
        return true;
    }

    bool CanSampleRate(double rate, std::string& error) noexcept override {
        if (!config_.can_sample_rate ||
            std::fabs(rate - config_.sample_rate) > 1e-9) {
            error = "fake sample rate not supported";
            return false;
        }
        return true;
    }

    bool GetSampleRate(double& rate, std::string& error) noexcept override {
        (void)error;
        rate = config_.sample_rate;
        return true;
    }

    bool SetSampleRate(double rate, std::string& error) noexcept override {
        if (!config_.can_sample_rate) {
            error = "fake setSampleRate rejected";
            return false;
        }
        config_.sample_rate = rate;
        return true;
    }

    bool GetBufferRange(nosfix::audio::AsioBufferRange& range,
                        std::string& error) noexcept override {
        (void)error;
        range = config_.range;
        return true;
    }

    bool GetChannelInfo(std::int32_t channel, bool input,
                        nosfix::audio::AsioChannelDescription& out,
                        std::string& error) noexcept override {
        (void)error;
        out.channel = channel;
        out.is_input = input;
        out.is_active = true;
        out.group = 0;
        out.format = FormatFor(channel);
        out.name = "Fake Out";
        return true;
    }

    bool CreateBuffers(nosfix::audio::AsioBufferInfo* infos, std::int32_t count,
                       std::int32_t frames,
                       const nosfix::audio::AsioCallbacks& callbacks,
                       std::string& error) noexcept override {
        ++create_count;
        last_create_frames = frames;
        last_create_channels = count;
        if (config_.fail_create) {
            error = "fake createBuffers failure";
            return false;
        }
        frames_ = frames;
        callbacks_ = callbacks;
        buffer_storage_.assign(static_cast<std::size_t>(count),
                               {std::vector<std::uint8_t>(),
                                std::vector<std::uint8_t>()});
        for (std::int32_t i = 0; i < count; ++i) {
            const std::uint32_t bytes = BytesFor(FormatFor(infos[i].channel));
            for (int index = 0; index < 2; ++index) {
                buffer_storage_[static_cast<std::size_t>(i)]
                               [static_cast<std::size_t>(index)]
                                   .assign(static_cast<std::size_t>(bytes) *
                                               static_cast<std::size_t>(frames),
                                           0U);
                infos[i].buffers[index] =
                    buffer_storage_[static_cast<std::size_t>(i)]
                                   [static_cast<std::size_t>(index)]
                                       .data();
            }
        }
        buffer_index_ = 0;
        next_sample_position_ = 0;
        return true;
    }

    void DisposeBuffers() noexcept override {
        ++dispose_count;
        buffer_storage_.clear();
    }

    bool GetLatencies(std::int32_t& input_frames, std::int32_t& output_frames,
                      std::string& error) noexcept override {
        if (config_.fail_latency) {
            error = "fake latency failure";
            return false;
        }
        input_frames = config_.input_latency;
        output_frames = config_.output_latency;
        return true;
    }

    bool Start(std::string& error) noexcept override {
        (void)error;
        ++start_count;
        next_sample_position_ = 0;
        buffer_index_ = 0;
        if (config_.fail_start) {
            error = "fake start failure";
            return false;
        }
        if (config_.callback_thread_inside_start) {
            std::thread callback([this]() { Fire(true, 0); });
            callback.join();
        } else if (config_.callback_inside_start) {
            Fire(true, 0);
        }
        return true;
    }

    void Stop() noexcept override { ++stop_count; }

    bool GetSamplePosition(std::int64_t& sample, std::int64_t& system_time_ns,
                           std::string& error) noexcept override {
        (void)error;
        sample = next_sample_position_;
        system_time_ns = SampleToNanoseconds(sample);
        return true;
    }

    bool GetSamplePositionFast(std::int64_t& sample,
                               std::int64_t& system_time_ns) noexcept override {
        sample = next_sample_position_;
        system_time_ns = SampleToNanoseconds(sample);
        return true;
    }

    void ControlPanel() noexcept override { ++control_panel_count; }

    void OutputReady() noexcept override { ++output_ready_count; }

private:
    [[nodiscard]] nosfix::audio::AsioSampleFormat FormatFor(
        std::int32_t channel) const noexcept {
        if (!config_.channel_formats.empty() && channel >= 0 &&
            static_cast<std::size_t>(channel) < config_.channel_formats.size()) {
            return config_.channel_formats[static_cast<std::size_t>(channel)];
        }
        return config_.format;
    }

    static std::uint32_t BytesFor(
        nosfix::audio::AsioSampleFormat format) noexcept {
        switch (format) {
        case nosfix::audio::AsioSampleFormat::Int16Lsb: return 2;
        case nosfix::audio::AsioSampleFormat::Int24Lsb: return 3;
        case nosfix::audio::AsioSampleFormat::Int32Lsb: return 4;
        case nosfix::audio::AsioSampleFormat::Float32Lsb: return 4;
        case nosfix::audio::AsioSampleFormat::Float64Lsb: return 8;
        default: return 0;
        }
    }

    [[nodiscard]] std::int64_t SampleToNanoseconds(
        std::int64_t sample) const noexcept {
        if (config_.sample_rate <= 0.0 || sample < 0) return 0;
        const long double value =
            static_cast<long double>(sample) * 1'000'000'000.0L /
            static_cast<long double>(config_.sample_rate);
        if (value >= static_cast<long double>(
                         std::numeric_limits<std::int64_t>::max())) {
            return std::numeric_limits<std::int64_t>::max();
        }
        return static_cast<std::int64_t>(value);
    }

    Config config_;
    nosfix::audio::AsioCallbacks callbacks_{};
    std::vector<std::vector<std::vector<std::uint8_t>>> buffer_storage_;
    std::int32_t frames_ = 0;
    std::int32_t buffer_index_ = 0;
    std::int64_t next_sample_position_ = 0;
    std::int32_t fire_count_ = 0;
};
