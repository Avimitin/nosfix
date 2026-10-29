#include "nosfix/asio_output.hpp"
#include "asio_native_abi.hpp"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstring>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <objbase.h>

namespace nosfix::audio {

class AsioCallbackDispatcher {
public:
    static void BufferSwitch(void* context, std::int32_t index,
                             bool direct_process) noexcept;
    static void BufferSwitchTimeInfo(void* context, AsioTime* time_info,
                                     std::int32_t index,
                                     bool direct_process) noexcept;
    static void SampleRateChanged(void* context, double rate) noexcept;
    static std::int32_t Message(void* context, std::int32_t selector,
                                std::int32_t value, void* message,
                                double* opt) noexcept;
};

namespace {

constexpr std::int32_t kAsioTrue = 1;
constexpr std::int32_t kAsioFalse = 0;
constexpr std::int32_t kAseOk = 0;

// asioMessage selectors from the published ASIO 2.3 interface.
constexpr std::int32_t kSelectorSupported = 1;
constexpr std::int32_t kEngineVersion = 2;
constexpr std::int32_t kResetRequest = 3;
constexpr std::int32_t kBufferSizeChange = 4;
constexpr std::int32_t kResyncRequest = 5;
constexpr std::int32_t kLatenciesChanged = 6;
constexpr std::int32_t kSupportsTimeInfo = 7;
constexpr std::int32_t kSupportsTimeCode = 8;
constexpr std::int32_t kOverload = 15;

constexpr std::uint32_t kSystemTimeValid = 1;
constexpr std::uint32_t kSamplePositionValid = 2;
constexpr std::uint32_t kSampleRateValid = 4;

constexpr std::uint32_t kMaxOutputChannels = 8;

// ---------------------------------------------------------------------------
// ASIO 2.3 ABI declarations (native layouts in asio_native_abi.hpp).
// ---------------------------------------------------------------------------

using asio_native::Asio64Abi;
using asio_native::AsioBufferInfoAbi;
using asio_native::AsioCallbacksAbi;
using asio_native::AsioChannelInfoAbi;
using asio_native::AsioDriverInfoAbi;
using asio_native::AsioTimeAbi;
using asio_native::AsioTimeCodeAbi;
using asio_native::AsioTimeInfoAbi;

struct IASIO : IUnknown {
    virtual std::int32_t STDMETHODCALLTYPE init(void* sys_handle) = 0;
    virtual void STDMETHODCALLTYPE getDriverName(char* name) = 0;
    virtual std::int32_t STDMETHODCALLTYPE getDriverVersion() = 0;
    virtual void STDMETHODCALLTYPE getErrorMessage(char* text) = 0;
    virtual std::int32_t STDMETHODCALLTYPE start() = 0;
    virtual std::int32_t STDMETHODCALLTYPE stop() = 0;
    virtual std::int32_t STDMETHODCALLTYPE getChannels(std::int32_t* inputs,
                                                       std::int32_t* outputs) = 0;
    virtual std::int32_t STDMETHODCALLTYPE getLatencies(std::int32_t* input_latency,
                                                        std::int32_t* output_latency) = 0;
    virtual std::int32_t STDMETHODCALLTYPE getBufferSize(std::int32_t* min_size,
                                                         std::int32_t* max_size,
                                                         std::int32_t* preferred_size,
                                                         std::int32_t* granularity) = 0;
    virtual std::int32_t STDMETHODCALLTYPE canSampleRate(double rate) = 0;
    virtual std::int32_t STDMETHODCALLTYPE getSampleRate(double* rate) = 0;
    virtual std::int32_t STDMETHODCALLTYPE setSampleRate(double rate) = 0;
    virtual std::int32_t STDMETHODCALLTYPE getClockSources(void* clocks,
                                                           std::int32_t* count) = 0;
    virtual std::int32_t STDMETHODCALLTYPE setClockSource(std::int32_t reference) = 0;
    virtual std::int32_t STDMETHODCALLTYPE getSamplePosition(Asio64Abi* sample,
                                                             Asio64Abi* timestamp) = 0;
    virtual std::int32_t STDMETHODCALLTYPE getChannelInfo(AsioChannelInfoAbi* info) = 0;
    virtual std::int32_t STDMETHODCALLTYPE createBuffers(AsioBufferInfoAbi* infos,
                                                         std::int32_t count,
                                                         std::int32_t frames,
                                                         AsioCallbacksAbi* callbacks) = 0;
    virtual std::int32_t STDMETHODCALLTYPE disposeBuffers() = 0;
    virtual std::int32_t STDMETHODCALLTYPE controlPanel() = 0;
    virtual std::int32_t STDMETHODCALLTYPE future(std::int32_t selector, void* opt) = 0;
    virtual std::int32_t STDMETHODCALLTYPE outputReady() = 0;
};

std::string FixedString(const char* text, std::size_t maximum) {
    if (text == nullptr) return {};
    std::size_t length = 0;
    while (length < maximum && text[length] != '\0') ++length;
    return std::string(text, length);
}

std::string Utf16ToUtf8(const std::wstring& text) {
    if (text.empty()) return {};
    const int length = WideCharToMultiByte(CP_UTF8, 0, text.c_str(),
                                           static_cast<int>(text.size()),
                                           nullptr, 0, nullptr, nullptr);
    if (length <= 0) return {};
    std::string result(static_cast<std::size_t>(length), '\0');
    WideCharToMultiByte(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()),
                        result.data(), length, nullptr, nullptr);
    return result;
}

std::wstring Utf8ToUtf16(const std::string& text) {
    if (text.empty()) return {};
    const int length = MultiByteToWideChar(CP_UTF8, 0, text.c_str(),
                                           static_cast<int>(text.size()), nullptr, 0);
    if (length <= 0) return {};
    std::wstring result(static_cast<std::size_t>(length), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()),
                        result.data(), length);
    return result;
}

const char* AsioErrorName(std::int32_t code) noexcept {
    switch (code) {
    case 0: return "ASE_OK";
    case 0x3f4847a0: return "ASE_SUCCESS";
    case -1000: return "ASE_NotPresent";
    case -1001: return "ASE_HWMalfunction";
    case -1002: return "ASE_InvalidParameter";
    case -1003: return "ASE_InvalidMode";
    case -1004: return "ASE_SPNotAdvancing";
    case -1005: return "ASE_NoClock";
    case -1006: return "ASE_NoMemory";
    default: return "ASE_unknown";
    }
}

std::string AsioErrorText(const char* method, std::int32_t code) {
    std::string text = "ASIO ";
    text += method;
    text += " failed: ";
    text += AsioErrorName(code);
    text += " (";
    text += std::to_string(code);
    text += ")";
    return text;
}

bool IsPowerOfTwo(std::int32_t value) noexcept {
    return value > 0 && (value & (value - 1)) == 0;
}

bool BufferSizeAllowed(const AsioBufferRange& range, std::int32_t frames,
                       std::string& error) {
    if (range.min_frames <= 0 || range.max_frames < range.min_frames) {
        error = "ASIO driver returned an invalid buffer size range";
        return false;
    }
    if (frames < range.min_frames || frames > range.max_frames) {
        error = "requested ASIO buffer size " + std::to_string(frames) +
                " is outside the driver range " + std::to_string(range.min_frames) +
                ".." + std::to_string(range.max_frames);
        return false;
    }
    if (range.granularity == -1) {
        if (frames != range.min_frames && !IsPowerOfTwo(frames)) {
            error = "ASIO driver only accepts powers of two from " +
                    std::to_string(range.min_frames) + " to " +
                    std::to_string(range.max_frames) + " (requested " +
                    std::to_string(frames) + ")";
            return false;
        }
    } else if (range.granularity > 0) {
        if ((frames % range.granularity) != 0) {
            error = "requested ASIO buffer size " + std::to_string(frames) +
                    " is not a multiple of the driver granularity " +
                    std::to_string(range.granularity);
            return false;
        }
    }
    return true;
}

AsioSampleFormat MapAsioSampleType(std::int32_t type) noexcept {
    switch (type) {
    case 16: return AsioSampleFormat::Int16Lsb;
    case 17: return AsioSampleFormat::Int24Lsb;
    case 18: return AsioSampleFormat::Int32Lsb;
    case 19: return AsioSampleFormat::Float32Lsb;
    case 20: return AsioSampleFormat::Float64Lsb;
    default: return AsioSampleFormat::Unsupported;
    }
}

std::uint32_t BytesPerSample(AsioSampleFormat format) noexcept {
    switch (format) {
    case AsioSampleFormat::Int16Lsb: return 2;
    case AsioSampleFormat::Int24Lsb: return 3;
    case AsioSampleFormat::Int32Lsb: return 4;
    case AsioSampleFormat::Float32Lsb: return 4;
    case AsioSampleFormat::Float64Lsb: return 8;
    default: return 0;
    }
}

void ZeroAsioBuffer(AsioSampleFormat format, void* destination,
                    std::uint32_t frames) noexcept {
    const std::uint32_t bytes = BytesPerSample(format);
    if (destination == nullptr || bytes == 0) return;
    std::memset(destination, 0,
                static_cast<std::size_t>(bytes) * static_cast<std::size_t>(frames));
}

// ---------------------------------------------------------------------------
// Real COM driver
// ---------------------------------------------------------------------------

class SystemAsioDriver final : public AsioDriver {
public:
    explicit SystemAsioDriver(IASIO* iasio) noexcept : iasio_(iasio) {}
    ~SystemAsioDriver() override { Shutdown(); }

    bool Init(void* sys_ref, AsioDriverInfo& info, std::string& error) noexcept override;
    void Shutdown() noexcept override;
    bool GetChannelCounts(std::int32_t& inputs, std::int32_t& outputs,
                          std::string& error) noexcept override;
    bool CanSampleRate(double rate, std::string& error) noexcept override;
    bool GetSampleRate(double& rate, std::string& error) noexcept override;
    bool SetSampleRate(double rate, std::string& error) noexcept override;
    bool GetBufferRange(AsioBufferRange& range, std::string& error) noexcept override;
    bool GetChannelInfo(std::int32_t channel, bool input,
                        AsioChannelDescription& out,
                        std::string& error) noexcept override;
    bool CreateBuffers(AsioBufferInfo* infos, std::int32_t count,
                       std::int32_t frames, const AsioCallbacks& callbacks,
                       std::string& error) noexcept override;
    void DisposeBuffers() noexcept override;
    bool GetLatencies(std::int32_t& input_frames, std::int32_t& output_frames,
                      std::string& error) noexcept override;
    bool Start(std::string& error) noexcept override;
    void Stop() noexcept override;
    bool GetSamplePosition(std::int64_t& sample, std::int64_t& system_time_ns,
                           std::string& error) noexcept override;
    bool GetSamplePositionFast(std::int64_t& sample,
                               std::int64_t& system_time_ns) noexcept override;
    void ControlPanel() noexcept override;
    void OutputReady() noexcept override;

private:
    void EmitBufferSwitch(std::int32_t index, AsioTimeAbi* time_info,
                          bool have_time_info, bool direct_process) noexcept;

    static void STDMETHODCALLTYPE AbiBufferSwitch(std::int32_t index,
                                                  std::int32_t direct_process) noexcept;
    static void STDMETHODCALLTYPE AbiSampleRateChanged(double rate) noexcept;
    static std::int32_t STDMETHODCALLTYPE AbiMessage(std::int32_t selector,
                                                    std::int32_t value,
                                                    void* message,
                                                    double* opt) noexcept;
    static AsioTimeAbi* STDMETHODCALLTYPE AbiBufferSwitchTimeInfo(
        AsioTimeAbi* time_info, std::int32_t index,
        std::int32_t direct_process) noexcept;

public:
    // Process-wide guard: ASIO drivers are single-instance COM servers here.
    static std::atomic<SystemAsioDriver*> instance_;

private:
    IASIO* iasio_ = nullptr;
    AsioCallbacks callbacks_{};
};

std::atomic<SystemAsioDriver*> SystemAsioDriver::instance_{nullptr};

bool SystemAsioDriver::Init(void* sys_ref, AsioDriverInfo& info,
                            std::string& error) noexcept {
    if (iasio_ == nullptr) {
        error = "ASIO driver object is null";
        return false;
    }
    AsioDriverInfoAbi abi{};
    abi.asio_version = 2;
    abi.sys_ref = sys_ref;
    const std::int32_t result = iasio_->init(&abi);
    info.asio_version = abi.asio_version;
    info.driver_version = abi.driver_version;
    char name[32] = {};
    iasio_->getDriverName(name);
    info.name = FixedString(name, sizeof(name));
    if (result != kAsioTrue) {
        info.init_error = FixedString(abi.error_message, sizeof(abi.error_message));
        error = "ASIOInit failed: " + info.init_error;
        return false;
    }
    return true;
}

void SystemAsioDriver::Shutdown() noexcept {
    if (iasio_ != nullptr) {
        iasio_->Release();
        iasio_ = nullptr;
    }
    SystemAsioDriver* expected = this;
    instance_.compare_exchange_strong(expected, nullptr);
    callbacks_ = {};
}

bool SystemAsioDriver::GetChannelCounts(std::int32_t& inputs, std::int32_t& outputs,
                                        std::string& error) noexcept {
    std::int32_t in = 0;
    std::int32_t out = 0;
    const std::int32_t result = iasio_->getChannels(&in, &out);
    if (result != kAseOk) {
        error = AsioErrorText("getChannels", result);
        return false;
    }
    inputs = in;
    outputs = out;
    return true;
}

bool SystemAsioDriver::CanSampleRate(double rate, std::string& error) noexcept {
    const std::int32_t result = iasio_->canSampleRate(rate);
    if (result != kAseOk) {
        error = AsioErrorText("canSampleRate", result);
        return false;
    }
    return true;
}

bool SystemAsioDriver::GetSampleRate(double& rate, std::string& error) noexcept {
    double value = 0.0;
    const std::int32_t result = iasio_->getSampleRate(&value);
    if (result != kAseOk) {
        error = AsioErrorText("getSampleRate", result);
        return false;
    }
    rate = value;
    return true;
}

bool SystemAsioDriver::SetSampleRate(double rate, std::string& error) noexcept {
    const std::int32_t result = iasio_->setSampleRate(rate);
    if (result != kAseOk) {
        error = AsioErrorText("setSampleRate", result);
        return false;
    }
    return true;
}

bool SystemAsioDriver::GetBufferRange(AsioBufferRange& range,
                                      std::string& error) noexcept {
    std::int32_t min_size = 0;
    std::int32_t max_size = 0;
    std::int32_t preferred = 0;
    std::int32_t granularity = 0;
    const std::int32_t result = iasio_->getBufferSize(&min_size, &max_size,
                                                      &preferred, &granularity);
    if (result != kAseOk) {
        error = AsioErrorText("getBufferSize", result);
        return false;
    }
    range.min_frames = min_size;
    range.max_frames = max_size;
    range.preferred_frames = preferred;
    range.granularity = granularity;
    return true;
}

bool SystemAsioDriver::GetChannelInfo(std::int32_t channel, bool input,
                                      AsioChannelDescription& out,
                                      std::string& error) noexcept {
    AsioChannelInfoAbi abi{};
    abi.channel = channel;
    abi.is_input = input ? kAsioTrue : kAsioFalse;
    const std::int32_t result = iasio_->getChannelInfo(&abi);
    if (result != kAseOk) {
        error = AsioErrorText("getChannelInfo", result);
        return false;
    }
    out.channel = abi.channel;
    out.is_input = abi.is_input != kAsioFalse;
    out.is_active = abi.is_active != kAsioFalse;
    out.group = abi.channel_group;
    out.format = MapAsioSampleType(abi.type);
    out.name = FixedString(abi.name, sizeof(abi.name));
    return true;
}

bool SystemAsioDriver::CreateBuffers(AsioBufferInfo* infos, std::int32_t count,
                                     std::int32_t frames,
                                     const AsioCallbacks& callbacks,
                                     std::string& error) noexcept {
    if (infos == nullptr || count <= 0 || frames <= 0) {
        error = "invalid ASIOCreateBuffers arguments";
        return false;
    }
    std::vector<AsioBufferInfoAbi> abi(static_cast<std::size_t>(count));
    for (std::int32_t i = 0; i < count; ++i) {
        abi[static_cast<std::size_t>(i)].is_input =
            infos[i].is_input ? kAsioTrue : kAsioFalse;
        abi[static_cast<std::size_t>(i)].channel_num = infos[i].channel;
        abi[static_cast<std::size_t>(i)].buffers[0] = nullptr;
        abi[static_cast<std::size_t>(i)].buffers[1] = nullptr;
    }
    callbacks_ = callbacks;
    AsioCallbacksAbi abi_callbacks{};
    abi_callbacks.buffer_switch = &SystemAsioDriver::AbiBufferSwitch;
    abi_callbacks.sample_rate_did_change = &SystemAsioDriver::AbiSampleRateChanged;
    abi_callbacks.asio_message = &SystemAsioDriver::AbiMessage;
    abi_callbacks.buffer_switch_time_info = &SystemAsioDriver::AbiBufferSwitchTimeInfo;
    const std::int32_t result = iasio_->createBuffers(
        abi.data(), count, frames, &abi_callbacks);
    if (result != kAseOk) {
        error = AsioErrorText("createBuffers", result);
        return false;
    }
    for (std::int32_t i = 0; i < count; ++i) {
        infos[i].buffers[0] = abi[static_cast<std::size_t>(i)].buffers[0];
        infos[i].buffers[1] = abi[static_cast<std::size_t>(i)].buffers[1];
    }
    return true;
}

void SystemAsioDriver::DisposeBuffers() noexcept {
    if (iasio_ != nullptr) iasio_->disposeBuffers();
}

bool SystemAsioDriver::GetLatencies(std::int32_t& input_frames,
                                    std::int32_t& output_frames,
                                    std::string& error) noexcept {
    std::int32_t input = 0;
    std::int32_t output = 0;
    const std::int32_t result = iasio_->getLatencies(&input, &output);
    if (result != kAseOk) {
        error = AsioErrorText("getLatencies", result);
        return false;
    }
    input_frames = input;
    output_frames = output;
    return true;
}

bool SystemAsioDriver::Start(std::string& error) noexcept {
    const std::int32_t result = iasio_->start();
    if (result != kAseOk) {
        error = AsioErrorText("start", result);
        return false;
    }
    return true;
}

void SystemAsioDriver::Stop() noexcept {
    if (iasio_ != nullptr) iasio_->stop();
}

bool SystemAsioDriver::GetSamplePosition(std::int64_t& sample,
                                         std::int64_t& system_time_ns,
                                         std::string& error) noexcept {
    Asio64Abi position{};
    Asio64Abi timestamp{};
    const std::int32_t result = iasio_->getSamplePosition(&position, &timestamp);
    if (result != kAseOk) {
        error = AsioErrorText("getSamplePosition", result);
        return false;
    }
    std::int64_t sample_value = 0;
    std::int64_t timestamp_value = 0;
    if (!asio_native::Asio64ToInt64(position, sample_value) ||
        !asio_native::Asio64ToInt64(timestamp, timestamp_value)) {
        error = "ASIO sample position/timestamp exceeds int64 range";
        return false;
    }
    sample = sample_value;
    system_time_ns = timestamp_value;
    return true;
}

bool SystemAsioDriver::GetSamplePositionFast(std::int64_t& sample,
                                             std::int64_t& system_time_ns) noexcept {
    Asio64Abi position{};
    Asio64Abi timestamp{};
    if (iasio_->getSamplePosition(&position, &timestamp) != kAseOk) return false;
    std::int64_t sample_value = 0;
    std::int64_t timestamp_value = 0;
    if (!asio_native::Asio64ToInt64(position, sample_value) ||
        !asio_native::Asio64ToInt64(timestamp, timestamp_value)) {
        return false;
    }
    sample = sample_value;
    system_time_ns = timestamp_value;
    return true;
}

void SystemAsioDriver::ControlPanel() noexcept {
    if (iasio_ != nullptr) iasio_->controlPanel();
}

void SystemAsioDriver::OutputReady() noexcept {
    if (iasio_ != nullptr) iasio_->outputReady();
}

void SystemAsioDriver::EmitBufferSwitch(std::int32_t index, AsioTimeAbi* time_info,
                                        bool have_time_info,
                                        bool direct_process) noexcept {
    if (have_time_info) {
        if (callbacks_.buffer_switch_time_info != nullptr) {
            AsioTime logical{};
            if (time_info != nullptr) {
                asio_native::ConvertToLogicalTime(*time_info, logical);
            }
            callbacks_.buffer_switch_time_info(callbacks_.context, &logical,
                                               index, direct_process);
        }
    } else if (callbacks_.buffer_switch != nullptr) {
        callbacks_.buffer_switch(callbacks_.context, index, direct_process);
    }
}

void STDMETHODCALLTYPE SystemAsioDriver::AbiBufferSwitch(
    std::int32_t index, std::int32_t direct_process) noexcept {
    SystemAsioDriver* driver = instance_.load(std::memory_order_acquire);
    if (driver != nullptr) {
        driver->EmitBufferSwitch(index, nullptr, false, direct_process != kAsioFalse);
    }
}

void STDMETHODCALLTYPE SystemAsioDriver::AbiSampleRateChanged(double rate) noexcept {
    SystemAsioDriver* driver = instance_.load(std::memory_order_acquire);
    if (driver != nullptr && driver->callbacks_.sample_rate_changed != nullptr) {
        driver->callbacks_.sample_rate_changed(driver->callbacks_.context, rate);
    }
}

std::int32_t STDMETHODCALLTYPE SystemAsioDriver::AbiMessage(
    std::int32_t selector, std::int32_t value, void* message, double* opt) noexcept {
    SystemAsioDriver* driver = instance_.load(std::memory_order_acquire);
    if (driver != nullptr && driver->callbacks_.message != nullptr) {
        return driver->callbacks_.message(driver->callbacks_.context, selector,
                                          value, message, opt);
    }
    return 0;
}

AsioTimeAbi* STDMETHODCALLTYPE SystemAsioDriver::AbiBufferSwitchTimeInfo(
    AsioTimeAbi* time_info, std::int32_t index,
    std::int32_t direct_process) noexcept {
    SystemAsioDriver* driver = instance_.load(std::memory_order_acquire);
    if (driver != nullptr) {
        driver->EmitBufferSwitch(index, time_info, true,
                                 direct_process != kAsioFalse);
    }
    return time_info;
}

} // namespace

void AsioCallbackDispatcher::BufferSwitch(void* context, std::int32_t index,
                                          bool direct_process) noexcept {
    (void)direct_process;
    static_cast<AsioOutput*>(context)->OnBufferSwitch(index, nullptr, false);
}

void AsioCallbackDispatcher::BufferSwitchTimeInfo(void* context, AsioTime* time_info,
                                                   std::int32_t index,
                                                   bool direct_process) noexcept {
    (void)direct_process;
    static_cast<AsioOutput*>(context)->OnBufferSwitch(index, time_info, true);
}

void AsioCallbackDispatcher::SampleRateChanged(void* context, double rate) noexcept {
    static_cast<AsioOutput*>(context)->OnSampleRateChanged(rate);
}

std::int32_t AsioCallbackDispatcher::Message(void* context, std::int32_t selector,
                                             std::int32_t value, void* message,
                                             double* opt) noexcept {
    return static_cast<AsioOutput*>(context)->OnMessage(selector, value, message, opt);
}

const char* AsioSampleFormatName(AsioSampleFormat format) noexcept {
    switch (format) {
    case AsioSampleFormat::Int16Lsb: return "Int16LSB";
    case AsioSampleFormat::Int24Lsb: return "Int24LSB";
    case AsioSampleFormat::Int32Lsb: return "Int32LSB";
    case AsioSampleFormat::Float32Lsb: return "Float32LSB";
    case AsioSampleFormat::Float64Lsb: return "Float64LSB";
    default: return "unsupported";
    }
}

bool EnumerateSystemAsioDrivers(std::vector<AsioDriverRegistration>& out,
                                std::string& error) {
    out.clear();
    HKEY root = nullptr;
    const LONG open_result = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\ASIO", 0,
                                           KEY_READ | KEY_WOW64_64KEY, &root);
    if (open_result != ERROR_SUCCESS) {
        error = "HKLM\\SOFTWARE\\ASIO is not readable (error " +
                std::to_string(open_result) + ")";
        return false;
    }
    for (DWORD index = 0;; ++index) {
        wchar_t name[256] = {};
        DWORD name_length = static_cast<DWORD>(std::size(name));
        const LONG enum_result = RegEnumKeyExW(root, index, name, &name_length,
                                               nullptr, nullptr, nullptr, nullptr);
        if (enum_result == ERROR_NO_MORE_ITEMS) break;
        if (enum_result != ERROR_SUCCESS) {
            error = "enumerating HKLM\\SOFTWARE\\ASIO failed (error " +
                    std::to_string(enum_result) + ")";
            RegCloseKey(root);
            return false;
        }
        HKEY subkey = nullptr;
        if (RegOpenKeyExW(root, name, 0, KEY_READ | KEY_WOW64_64KEY, &subkey) !=
            ERROR_SUCCESS) {
            continue;
        }
        auto read_value = [subkey](const wchar_t* value_name) -> std::string {
            wchar_t buffer[512] = {};
            DWORD bytes = sizeof(buffer) - sizeof(wchar_t);
            DWORD type = 0;
            if (RegQueryValueExW(subkey, value_name, nullptr, &type,
                                 reinterpret_cast<BYTE*>(buffer), &bytes) !=
                    ERROR_SUCCESS ||
                (type != REG_SZ && type != REG_EXPAND_SZ)) {
                return {};
            }
            buffer[(sizeof(buffer) / sizeof(buffer[0])) - 1] = L'\0';
            return Utf16ToUtf8(buffer);
        };
        AsioDriverRegistration registration;
        registration.name = Utf16ToUtf8(std::wstring(name, name_length));
        registration.clsid = read_value(L"CLSID");
        registration.description = read_value(L"Description");
        registration.registry64 = true;
        RegCloseKey(subkey);
        if (!registration.clsid.empty()) out.push_back(std::move(registration));
    }
    RegCloseKey(root);
    return true;
}

std::unique_ptr<AsioDriver> CreateSystemAsioDriver(const std::string& clsid,
                                                   std::string& error) {
    if (SystemAsioDriver::instance_.load(std::memory_order_acquire) != nullptr) {
        error = "another ASIO driver is already open in this process";
        return nullptr;
    }
    const std::wstring wide_clsid = Utf8ToUtf16(clsid);
    CLSID class_id{};
    if (wide_clsid.empty() || FAILED(CLSIDFromString(wide_clsid.c_str(), &class_id))) {
        error = "invalid ASIO driver CLSID: " + clsid;
        return nullptr;
    }
    IASIO* iasio = nullptr;
    HRESULT result = CoCreateInstance(class_id, nullptr, CLSCTX_INPROC_SERVER,
                                      class_id, reinterpret_cast<void**>(&iasio));
    if (FAILED(result)) {
        IUnknown* unknown = nullptr;
        result = CoCreateInstance(class_id, nullptr, CLSCTX_INPROC_SERVER,
                                  IID_IUnknown, reinterpret_cast<void**>(&unknown));
        if (SUCCEEDED(result) && unknown != nullptr) {
            result = unknown->QueryInterface(class_id,
                                             reinterpret_cast<void**>(&iasio));
            unknown->Release();
        }
    }
    if (FAILED(result) || iasio == nullptr) {
        error = "ASIO driver class is not registered as a 64-bit in-proc server (HRESULT " +
                std::to_string(static_cast<unsigned long>(result)) + ")";
        return nullptr;
    }
    auto driver = std::make_unique<SystemAsioDriver>(iasio);
    SystemAsioDriver* expected = nullptr;
    if (!SystemAsioDriver::instance_.compare_exchange_strong(expected,
                                                             driver.get())) {
        error = "another ASIO driver is already open in this process";
        return nullptr;
    }
    return driver;
}

// ---------------------------------------------------------------------------
// Sample format conversion
// ---------------------------------------------------------------------------

void ConvertPlanarFloatToAsio(AsioSampleFormat format, const float* source,
                              void* destination, std::uint32_t frames,
                              std::uint64_t& clipped_samples) noexcept {
    if (source == nullptr || destination == nullptr || frames == 0) return;
    for (std::uint32_t i = 0; i < frames; ++i) {
        float value = source[i];
        if (std::isnan(value)) {
            value = 0.0F;
        } else if (value > 1.0F) {
            value = 1.0F;
            ++clipped_samples;
        } else if (value < -1.0F) {
            value = -1.0F;
            ++clipped_samples;
        }
        switch (format) {
        case AsioSampleFormat::Int16Lsb: {
            const std::int32_t scaled = static_cast<std::int32_t>(
                std::llround(static_cast<double>(value) * 32767.0));
            static_cast<std::int16_t*>(destination)[i] =
                static_cast<std::int16_t>(scaled);
            break;
        }
        case AsioSampleFormat::Int24Lsb: {
            std::int32_t scaled = static_cast<std::int32_t>(
                std::llround(static_cast<double>(value) * 8388607.0));
            if (scaled > 8388607) scaled = 8388607;
            if (scaled < -8388607) scaled = -8388607;
            const std::uint32_t bits = static_cast<std::uint32_t>(scaled) & 0x00FFFFFFU;
            auto* bytes = static_cast<std::uint8_t*>(destination) + i * 3U;
            bytes[0] = static_cast<std::uint8_t>(bits & 0xFFU);
            bytes[1] = static_cast<std::uint8_t>((bits >> 8) & 0xFFU);
            bytes[2] = static_cast<std::uint8_t>((bits >> 16) & 0xFFU);
            break;
        }
        case AsioSampleFormat::Int32Lsb: {
            std::int64_t scaled = std::llround(static_cast<double>(value) * 2147483647.0);
            if (scaled > 2147483647LL) scaled = 2147483647LL;
            if (scaled < -2147483647LL) scaled = -2147483647LL;
            static_cast<std::int32_t*>(destination)[i] =
                static_cast<std::int32_t>(scaled);
            break;
        }
        case AsioSampleFormat::Float32Lsb:
            static_cast<float*>(destination)[i] = value;
            break;
        case AsioSampleFormat::Float64Lsb:
            static_cast<double*>(destination)[i] = static_cast<double>(value);
            break;
        default:
            break;
        }
    }
}

// ---------------------------------------------------------------------------
// AsioOutput
// ---------------------------------------------------------------------------

AsioOutput::AsioOutput() = default;

AsioOutput::~AsioOutput() { Close(); }

void AsioOutput::SetRenderCallback(AsioRenderCallback callback,
                                   void* context) noexcept {
    render_context_.store(context, std::memory_order_release);
    render_callback_.store(callback, std::memory_order_release);
}

void AsioOutput::SetPaused(bool paused) noexcept {
    paused_.store(paused, std::memory_order_release);
}

bool AsioOutput::ConfigureDriver(const AsioOpenOptions& options,
                                 std::string& error) {
    if (!driver_) {
        error = "no ASIO driver";
        return false;
    }
    if (options.output_channels.empty() ||
        options.output_channels.size() > kMaxOutputChannels) {
        error = "output_channels must list 1..8 explicit ASIO output channels";
        return false;
    }
    for (std::size_t i = 0; i < options.output_channels.size(); ++i) {
        if (options.output_channels[i] < 0) {
            error = "output channel indices must be nonnegative";
            return false;
        }
        for (std::size_t j = i + 1; j < options.output_channels.size(); ++j) {
            if (options.output_channels[i] == options.output_channels[j]) {
                error = "output channel indices must be unique";
                return false;
            }
        }
    }
    if (!(options.engine_sample_rate >= 8000.0) ||
        !(options.engine_sample_rate <= 384000.0)) {
        error = "engine sample rate is outside 8000..384000 Hz";
        return false;
    }
    if (options.buffer_frames <= 0 || options.buffer_frames > 65536) {
        error = "buffer_frames must be 1..65536";
        return false;
    }

    AsioDriverInfo driver_info;
    if (!driver_->Init(nullptr, driver_info, error)) return false;
    info_.driver_name = driver_info.name;
    info_.asio_version = driver_info.asio_version;
    info_.driver_version = driver_info.driver_version;
    if (!options.expected_driver_name.empty() &&
        driver_info.name != options.expected_driver_name) {
        error = "ASIO driver name mismatch: expected '" +
                options.expected_driver_name + "', got '" + driver_info.name + "'";
        return false;
    }

    std::int32_t inputs = 0;
    std::int32_t outputs = 0;
    if (!driver_->GetChannelCounts(inputs, outputs, error)) return false;
    if (outputs <= 0) {
        error = "ASIO driver reports no output channels";
        return false;
    }
    info_.channels.clear();
    AsioSampleFormat format = AsioSampleFormat::Unsupported;
    for (std::size_t i = 0; i < options.output_channels.size(); ++i) {
        const std::int32_t channel = options.output_channels[i];
        if (channel >= outputs) {
            error = "output channel " + std::to_string(channel) +
                    " is outside the driver's " + std::to_string(outputs) +
                    " outputs";
            return false;
        }
        AsioChannelDescription description;
        if (!driver_->GetChannelInfo(channel, false, description, error)) return false;
        if (description.is_input) {
            error = "channel " + std::to_string(channel) +
                    " is an input channel, not an output";
            return false;
        }
        if (description.format == AsioSampleFormat::Unsupported) {
            error = "channel " + std::to_string(channel) +
                    " uses a sample type this build does not convert";
            return false;
        }
        if (i == 0) {
            format = description.format;
        } else if (description.format != format) {
            error = "selected ASIO output channels use mixed sample types; "
                    "mixed-type output is not implemented";
            return false;
        }
        info_.channels.push_back(std::move(description));
    }
    format_ = format;
    info_.format = format;

    if (!driver_->GetBufferRange(buffer_range_, error)) return false;
    if (!BufferSizeAllowed(buffer_range_, options.buffer_frames, error)) return false;
    if (!driver_->CanSampleRate(options.engine_sample_rate, error)) return false;
    if (!driver_->SetSampleRate(options.engine_sample_rate, error)) return false;
    double actual_rate = 0.0;
    if (!driver_->GetSampleRate(actual_rate, error)) return false;
    const double rate_delta = std::fabs(actual_rate - options.engine_sample_rate);
    if (rate_delta > options.engine_sample_rate * 1e-6) {
        error = "ASIO driver is running at " + std::to_string(actual_rate) +
                " Hz, not the requested " +
                std::to_string(options.engine_sample_rate) + " Hz";
        return false;
    }
    info_.sample_rate = actual_rate;
    info_.buffer_frames = options.buffer_frames;
    info_.driver_clsid = options.driver_clsid;
    driver_sample_rate_.store(actual_rate, std::memory_order_relaxed);

    LARGE_INTEGER frequency{};
    if (!QueryPerformanceFrequency(&frequency) || frequency.QuadPart <= 0) {
        error = "QueryPerformanceFrequency failed";
        return false;
    }
    qpc_frequency_ = frequency.QuadPart;

    buffer_frames_ = options.buffer_frames;
    output_latency_frames_.store(0, std::memory_order_relaxed);
    output_latency_reported_ = false;
    mix_buffers_.assign(options.output_channels.size(),
                        std::vector<float>(static_cast<std::size_t>(buffer_frames_),
                                           0.0F));
    mix_planar_.resize(options.output_channels.size());
    for (std::size_t i = 0; i < mix_buffers_.size(); ++i) {
        mix_planar_[i] = mix_buffers_[i].data();
    }
    buffer_infos_.assign(options.output_channels.size(), AsioBufferInfo{});
    for (std::size_t i = 0; i < buffer_infos_.size(); ++i) {
        buffer_infos_[i].is_input = false;
        buffer_infos_[i].channel = options.output_channels[i];
        buffer_infos_[i].buffers[0] = nullptr;
        buffer_infos_[i].buffers[1] = nullptr;
    }
    return true;
}

bool AsioOutput::CreateDriverBuffers(std::string& error) {
    AsioCallbacks callbacks{};
    callbacks.context = this;
    callbacks.buffer_switch = &AsioCallbackDispatcher::BufferSwitch;
    callbacks.sample_rate_changed = &AsioCallbackDispatcher::SampleRateChanged;
    callbacks.message = &AsioCallbackDispatcher::Message;
    callbacks.buffer_switch_time_info = &AsioCallbackDispatcher::BufferSwitchTimeInfo;
    if (!driver_->CreateBuffers(buffer_infos_.data(),
                                static_cast<std::int32_t>(buffer_infos_.size()),
                                buffer_frames_, callbacks, error)) {
        return false;
    }
    buffers_created_ = true;

    std::int32_t input_latency = 0;
    std::int32_t output_latency = 0;
    if (!driver_->GetLatencies(input_latency, output_latency, error)) {
        DestroyDriverBuffers();
        return false;
    }
    if (output_latency < 0 ||
        output_latency > static_cast<std::int32_t>(options_.engine_sample_rate * 10.0)) {
        DestroyDriverBuffers();
        error = "ASIO driver reported an implausible output latency";
        return false;
    }
    output_latency_frames_.store(static_cast<std::uint32_t>(output_latency),
                                 std::memory_order_relaxed);
    output_latency_reported_ = true;
    info_.output_latency_frames = output_latency;
    info_.output_latency_reported = true;

    AudioTimelineConfig timeline_config;
    timeline_config.qpc_frequency = qpc_frequency_;
    timeline_config.sample_rate =
        static_cast<std::uint32_t>(std::llround(options_.engine_sample_rate));
    timeline_config.nominal_buffer_frames =
        static_cast<std::uint32_t>(buffer_frames_);
    timeline_config.output_latency_frames =
        static_cast<std::uint32_t>(output_latency);
    if (!timeline_.Configure(timeline_config)) {
        DestroyDriverBuffers();
        error = "could not configure the device sample timeline";
        return false;
    }
    return true;
}

void AsioOutput::DestroyDriverBuffers() noexcept {
    if (driver_ && buffers_created_) {
        driver_->DisposeBuffers();
    }
    buffers_created_ = false;
    for (auto& info : buffer_infos_) {
        info.buffers[0] = nullptr;
        info.buffers[1] = nullptr;
    }
}

bool AsioOutput::Open(std::unique_ptr<AsioDriver> driver,
                      const AsioOpenOptions& options,
                      std::string& error) {
    if (driver_ != nullptr || IsRunning()) {
        error = "ASIO output is already open";
        return false;
    }
    if (!driver) {
        error = "no ASIO driver supplied";
        return false;
    }
    driver_ = std::move(driver);
    options_ = options;
    if (options_.initial_generation == 0) options_.initial_generation = 1;
    // Generation is a lifetime floor across Close/Open. Never reuse a value
    // that a previous device timeline or keysound event may still carry.
    const std::uint64_t floor = std::max(
        generation_.load(std::memory_order_relaxed), timeline_.Generation());
    if (floor != 0 && options_.initial_generation <= floor) {
        if (floor == std::numeric_limits<std::uint64_t>::max()) {
            error = "ASIO generation counter exhausted";
            Close();
            return false;
        }
        options_.initial_generation = floor + 1;
    }

    callback_count_.store(0, std::memory_order_relaxed);
    underrun_count_.store(0, std::memory_order_relaxed);
    clipped_samples_.store(0, std::memory_order_relaxed);
    invalid_buffers_.store(0, std::memory_order_relaxed);
    reset_requests_.store(0, std::memory_order_relaxed);
    resync_requests_.store(0, std::memory_order_relaxed);
    overloads_.store(0, std::memory_order_relaxed);
    sample_rate_changes_.store(0, std::memory_order_relaxed);
    buffer_size_changes_.store(0, std::memory_order_relaxed);
    latencies_changed_.store(0, std::memory_order_relaxed);
    requested_buffer_frames_.store(0, std::memory_order_relaxed);
    pending_latency_refresh_.store(false, std::memory_order_relaxed);
    sample_rate_mismatch_.store(false, std::memory_order_release);
    force_timeline_discontinuity_.store(false, std::memory_order_release);
    time_info_mode_.store(false, std::memory_order_release);
    paused_.store(false, std::memory_order_release);
    last_sample_position_.store(0, std::memory_order_relaxed);
    last_qpc_.store(0, std::memory_order_relaxed);
    last_buffer_frames_.store(0, std::memory_order_relaxed);

    if (!ConfigureDriver(options_, error)) {
        Close();
        return false;
    }
    if (!CreateDriverBuffers(error)) {
        Close();
        return false;
    }
    generation_.store(options_.initial_generation, std::memory_order_release);
    begin_generation_pending_.store(true, std::memory_order_release);
    expected_sample_position_ = 0;
    running_.store(false, std::memory_order_release);
    UpdateInfoFromDriver();
    return true;
}

bool AsioOutput::OpenSystem(const AsioOpenOptions& options, std::string& error) {
    if (options.driver_clsid.empty()) {
        error = "driver_clsid is required; no implicit device selection";
        return false;
    }
    // ASIO drivers commonly register ThreadingModel=Apartment (the TOPPING
    // TUSBAudio driver on this host does). Prefer an STA so calls into the
    // in-proc server stay direct; if the caller already initialized an MTA,
    // proceed with it rather than failing. Service() must then be called on
    // this same thread.
    HRESULT com_result = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (com_result == S_OK) {
        com_initialized_ = true;
    } else if (com_result == S_FALSE) {
        com_initialized_ = false; // already an STA on this thread
    } else if (com_result == RPC_E_CHANGED_MODE) {
        com_initialized_ = false; // existing MTA; legal but may marshal
    } else {
        error = "CoInitializeEx failed (HRESULT " +
                std::to_string(static_cast<unsigned long>(com_result)) + ")";
        return false;
    }
    auto system_driver = CreateSystemAsioDriver(options.driver_clsid, error);
    if (!system_driver) {
        if (com_initialized_) {
            CoUninitialize();
            com_initialized_ = false;
        }
        return false;
    }
    return Open(std::move(system_driver), options, error);
}

bool AsioOutput::Start(std::string& error) {
    if (!driver_) {
        error = "ASIO output is not open";
        return false;
    }
    if (IsRunning()) {
        error = "ASIO output is already running";
        return false;
    }
    // ASIO resets samplePosition to zero on every ASIOStart. A stop/start is a
    // new sample timeline, so advance the generation that keysound events use.
    const std::uint64_t previous_generation =
        generation_.load(std::memory_order_relaxed);
    const std::uint64_t floor = std::max(previous_generation,
                                         timeline_.Generation());
    const bool must_advance = started_once_ || timeline_.IsValid();
    if (must_advance) {
        if (floor == std::numeric_limits<std::uint64_t>::max()) {
            error = "ASIO generation counter exhausted";
            return false;
        }
        generation_.store(floor + 1, std::memory_order_release);
    }
    expected_sample_position_ = 0;
    begin_generation_pending_.store(true, std::memory_order_release);
    force_timeline_discontinuity_.store(false, std::memory_order_release);
    last_sample_position_.store(0, std::memory_order_relaxed);
    last_buffer_frames_.store(static_cast<std::uint32_t>(buffer_frames_),
                              std::memory_order_relaxed);
    // ASIOStart may invoke bufferSwitch before returning. Publish running state
    // and the new generation first so that callback renders into prepared
    // buffers instead of being ignored.
    running_.store(true, std::memory_order_release);
    if (!driver_->Start(error)) {
        running_.store(false, std::memory_order_release);
        // A failed Start may still have run a callback. Keep the output's
        // generation at or above the timeline floor; the next Start will pick a
        // value greater than both.
        generation_.store(std::max(previous_generation, timeline_.Generation()),
                          std::memory_order_release);
        return false;
    }
    started_once_ = true;
    return true;
}

void AsioOutput::Stop() noexcept {
    if (driver_ && IsRunning()) {
        driver_->Stop();
    }
    running_.store(false, std::memory_order_release);
    begin_generation_pending_.store(true, std::memory_order_release);
    expected_sample_position_ = 0;
}

void AsioOutput::Close() noexcept {
    if (driver_) {
        if (IsRunning()) driver_->Stop();
        running_.store(false, std::memory_order_release);
        DestroyDriverBuffers();
        driver_->Shutdown();
        driver_.reset();
    }
    mix_buffers_.clear();
    mix_planar_.clear();
    buffer_infos_.clear();
    info_ = {};
    format_ = AsioSampleFormat::Unsupported;
    buffer_frames_ = 0;
    output_latency_frames_.store(0, std::memory_order_relaxed);
    pending_output_latency_valid_.store(false, std::memory_order_relaxed);
    sample_rate_mismatch_.store(false, std::memory_order_release);
    requested_buffer_frames_.store(0, std::memory_order_relaxed);
    pending_latency_refresh_.store(false, std::memory_order_relaxed);
    force_timeline_discontinuity_.store(false, std::memory_order_release);
    time_info_mode_.store(false, std::memory_order_release);
    paused_.store(false, std::memory_order_release);
    render_callback_.store(nullptr, std::memory_order_release);
    render_context_.store(nullptr, std::memory_order_release);
    started_once_ = false;
    // generation_ is intentionally preserved as the lifetime floor.
    if (com_initialized_) {
        CoUninitialize();
        com_initialized_ = false;
    }
}

bool AsioOutput::Restart(std::uint64_t new_generation,
                         std::int32_t new_buffer_frames,
                         std::string& error) {
    if (!driver_) {
        error = "ASIO output is not open";
        return false;
    }
    if (sample_rate_mismatch_.load(std::memory_order_acquire)) {
        error = "ASIO driver changed sample rate; close and reopen at the new rate";
        return false;
    }
    if (new_generation == 0 ||
        (generation_.load(std::memory_order_acquire) != 0 &&
         new_generation <= generation_.load(std::memory_order_acquire))) {
        error = "restart generation must advance";
        return false;
    }

    // A reset may have changed the driver's range, rate or channel format.
    AsioBufferRange refreshed_range;
    if (!driver_->GetBufferRange(refreshed_range, error)) return false;
    buffer_range_ = refreshed_range;
    double refreshed_rate = 0.0;
    if (!driver_->GetSampleRate(refreshed_rate, error)) return false;
    if (std::fabs(refreshed_rate - options_.engine_sample_rate) >
        options_.engine_sample_rate * 1e-6) {
        error = "ASIO driver reset to a different sample rate; reopen required";
        return false;
    }
    AsioSampleFormat refreshed_format = AsioSampleFormat::Unsupported;
    for (std::size_t i = 0; i < buffer_infos_.size(); ++i) {
        AsioChannelDescription description;
        if (!driver_->GetChannelInfo(buffer_infos_[i].channel, false,
                                     description, error)) {
            return false;
        }
        if (i == 0) {
            refreshed_format = description.format;
        } else if (description.format != refreshed_format) {
            error = "ASIO reset produced mixed channel sample types";
            return false;
        }
        info_.channels[i] = std::move(description);
    }
    if (refreshed_format == AsioSampleFormat::Unsupported) {
        error = "ASIO reset produced an unsupported channel sample type";
        return false;
    }

    if (new_buffer_frames <= 0) new_buffer_frames = buffer_frames_;
    if (!BufferSizeAllowed(buffer_range_, new_buffer_frames, error)) return false;

    if (IsRunning()) driver_->Stop();
    running_.store(false, std::memory_order_release);
    DestroyDriverBuffers();
    // Publish the new conversion format only after the callback is stopped.
    format_ = refreshed_format;
    info_.format = refreshed_format;

    if (new_buffer_frames != buffer_frames_) {
        buffer_frames_ = new_buffer_frames;
        const std::size_t channels = mix_buffers_.size();
        mix_buffers_.assign(channels,
                            std::vector<float>(static_cast<std::size_t>(buffer_frames_),
                                               0.0F));
        mix_planar_.resize(channels);
        for (std::size_t i = 0; i < channels; ++i) mix_planar_[i] = mix_buffers_[i].data();
        info_.buffer_frames = buffer_frames_;
        options_.buffer_frames = buffer_frames_;
    }
    if (!CreateDriverBuffers(error)) return false;
    const std::uint64_t previous_generation =
        generation_.load(std::memory_order_relaxed);
    const bool previous_started_once = started_once_;
    generation_.store(new_generation, std::memory_order_release);
    begin_generation_pending_.store(true, std::memory_order_release);
    force_timeline_discontinuity_.store(false, std::memory_order_release);
    started_once_ = true;
    expected_sample_position_ = 0;
    last_sample_position_.store(0, std::memory_order_relaxed);
    last_buffer_frames_.store(static_cast<std::uint32_t>(buffer_frames_),
                              std::memory_order_relaxed);
    running_.store(true, std::memory_order_release);
    if (!driver_->Start(error)) {
        running_.store(false, std::memory_order_release);
        generation_.store(std::max(previous_generation, timeline_.Generation()),
                          std::memory_order_release);
        started_once_ = previous_started_once;
        DestroyDriverBuffers();
        return false;
    }
    UpdateInfoFromDriver();
    return true;
}

bool AsioOutput::Service(std::string& error) {
    if (!driver_) {
        error = "ASIO output is not open";
        return false;
    }
    if (sample_rate_mismatch_.load(std::memory_order_acquire)) {
        error = "ASIO driver changed sample rate; close and reopen at the new rate";
        return false;
    }
    const std::int32_t requested =
        requested_buffer_frames_.exchange(0, std::memory_order_acq_rel);
    if (requested > 0 && requested != buffer_frames_) {
        return Restart(generation_.load(std::memory_order_acquire) + 1,
                       requested, error);
    }
    if (reset_requests_.exchange(0, std::memory_order_acq_rel) > 0) {
        return Restart(generation_.load(std::memory_order_acquire) + 1,
                       buffer_frames_, error);
    }
    if (pending_latency_refresh_.exchange(false, std::memory_order_acq_rel)) {
        std::int32_t input_latency = 0;
        std::int32_t output_latency = 0;
        if (!driver_->GetLatencies(input_latency, output_latency, error)) return false;
        if (output_latency < 0 ||
            output_latency > static_cast<std::int32_t>(options_.engine_sample_rate * 10.0)) {
            error = "ASIO driver reported an implausible output latency";
            return false;
        }
        output_latency_frames_.store(static_cast<std::uint32_t>(output_latency),
                                     std::memory_order_relaxed);
        pending_output_latency_.store(static_cast<std::uint32_t>(output_latency),
                                      std::memory_order_relaxed);
        pending_output_latency_valid_.store(true, std::memory_order_release);
        info_.output_latency_frames = output_latency;
    }
    return true;
}

void AsioOutput::OnSampleRateChanged(double rate) noexcept {
    driver_sample_rate_.store(rate, std::memory_order_relaxed);
    sample_rate_changes_.fetch_add(1, std::memory_order_relaxed);
    if (!(rate >= 8000.0) || !(rate <= 384000.0) ||
        std::fabs(rate - options_.engine_sample_rate) >
            options_.engine_sample_rate * 1e-6) {
        sample_rate_mismatch_.store(true, std::memory_order_release);
    }
}

std::int32_t AsioOutput::OnMessage(std::int32_t selector, std::int32_t value,
                                   void* message, double* opt) noexcept {
    (void)message;
    (void)opt;
    switch (selector) {
    case kSelectorSupported:
        switch (value) {
        case kEngineVersion:
        case kResetRequest:
        case kResyncRequest:
        case kLatenciesChanged:
        case kSupportsTimeInfo:
        case kOverload:
            return 1;
        default:
            return 0;
        }
    case kEngineVersion:
        return 2;
    case kResetRequest:
        reset_requests_.fetch_add(1, std::memory_order_relaxed);
        return 1;
    case kBufferSizeChange:
        if (value > 0 && value <= 65536) {
            requested_buffer_frames_.store(value, std::memory_order_release);
        }
        buffer_size_changes_.fetch_add(1, std::memory_order_relaxed);
        return 1;
    case kResyncRequest:
        resync_requests_.fetch_add(1, std::memory_order_relaxed);
        force_timeline_discontinuity_.store(true, std::memory_order_release);
        return 1;
    case kLatenciesChanged:
        latencies_changed_.fetch_add(1, std::memory_order_relaxed);
        pending_latency_refresh_.store(true, std::memory_order_release);
        return 1;
    case kSupportsTimeInfo:
        time_info_mode_.store(true, std::memory_order_release);
        return 1;
    case kSupportsTimeCode:
        return 0;
    case kOverload:
        overloads_.fetch_add(1, std::memory_order_relaxed);
        return 1;
    default:
        return 0;
    }
}

void AsioOutput::OnBufferSwitch(std::int32_t index, AsioTime* time_info,
                                bool have_time_info) noexcept {
    if (!driver_ || !IsRunning()) return;

    if (pending_output_latency_valid_.exchange(false, std::memory_order_acq_rel)) {
        const std::uint32_t latency =
            pending_output_latency_.load(std::memory_order_relaxed);
        if (timeline_.SetOutputLatency(latency)) {
            output_latency_frames_.store(latency, std::memory_order_relaxed);
        }
    }

    LARGE_INTEGER counter{};
    QueryPerformanceCounter(&counter);
    const std::int64_t qpc = counter.QuadPart;
    std::uint64_t generation = generation_.load(std::memory_order_acquire);

    std::int64_t sample_position = expected_sample_position_;
    std::int64_t system_time_ns = 0;
    bool sample_position_valid = false;
    bool system_time_valid = false;
    if (have_time_info && time_info != nullptr) {
        const AsioTimeInfo& info = time_info->time_info;
        if ((info.flags & kSamplePositionValid) != 0) {
            sample_position = info.sample_position;
            sample_position_valid = true;
        }
        if ((info.flags & kSystemTimeValid) != 0) {
            system_time_ns = info.system_time_ns;
            system_time_valid = true;
        }
    } else if (driver_->GetSamplePositionFast(sample_position, system_time_ns)) {
        sample_position_valid = true;
        system_time_valid = true;
    }

    const std::int64_t frame_count = static_cast<std::int64_t>(buffer_frames_);
    if (sample_position_valid &&
        (sample_position < 0 ||
         sample_position > std::numeric_limits<std::int64_t>::max() - frame_count)) {
        sample_position_valid = false;
    }
    if (sample_position_valid) {
        expected_sample_position_ = sample_position + frame_count;
    } else {
        // Keep a plausible block start for silence/rendering, but do not claim
        // a valid driver position. The expected cursor advances only when it
        // can do so without overflow.
        sample_position = expected_sample_position_;
        if (sample_position >= 0 &&
            sample_position <= std::numeric_limits<std::int64_t>::max() - frame_count) {
            expected_sample_position_ = sample_position + frame_count;
        } else {
            sample_position = 0;
            expected_sample_position_ = 0;
        }
    }
    last_sample_position_.store(sample_position, std::memory_order_relaxed);
    last_qpc_.store(qpc, std::memory_order_relaxed);
    last_buffer_frames_.store(static_cast<std::uint32_t>(buffer_frames_),
                              std::memory_order_relaxed);

    if (sample_position_valid && qpc_frequency_ > 0) {
        if (begin_generation_pending_.load(std::memory_order_acquire)) {
            if (timeline_.BeginGeneration(generation, qpc, sample_position)) {
                begin_generation_pending_.store(false, std::memory_order_release);
            }
        } else {
            DeviceClockObservation observation;
            observation.driver_sample_position = sample_position;
            observation.qpc_ticks = qpc;
            observation.qpc_frequency = qpc_frequency_;
            observation.buffer_frames = static_cast<std::uint32_t>(buffer_frames_);
            observation.output_latency_frames =
                output_latency_frames_.load(std::memory_order_relaxed);
            observation.discontinuity =
                force_timeline_discontinuity_.exchange(false, std::memory_order_acq_rel);
            const TimelineStatus status = timeline_.Observe(observation);
            if (status == TimelineStatus::Discontinuity) {
                // A driver-observed jump is a new device generation too. Publish
                // it so the producer stops labeling events with the old one.
                const std::uint64_t timeline_generation = timeline_.Generation();
                if (timeline_generation > generation) {
                    generation = timeline_generation;
                    generation_.store(generation, std::memory_order_release);
                }
            }
        }
    }

    AsioRenderContext context;
    context.generation = generation;
    context.block_start_sample = sample_position;
    context.qpc_ticks = qpc;
    context.qpc_frequency = qpc_frequency_;
    context.driver_system_time_ns = system_time_ns;
    context.sample_position_valid = sample_position_valid;
    context.system_time_valid = system_time_valid;
    context.time_info_mode = time_info_mode_.load(std::memory_order_acquire);
    context.buffer_frames = static_cast<std::uint32_t>(buffer_frames_);
    context.output_latency_frames =
        output_latency_frames_.load(std::memory_order_relaxed);

    for (auto& buffer : mix_buffers_) {
        std::fill(buffer.begin(), buffer.end(), 0.0F);
    }

    const bool paused = paused_.load(std::memory_order_acquire);
    bool rendered = false;
    AsioRenderCallback render = render_callback_.load(std::memory_order_acquire);
    void* render_context = render_context_.load(std::memory_order_acquire);
    if (!paused && render != nullptr) {
        rendered = render(render_context, context, mix_planar_.data());
    }
    if (!rendered) {
        for (auto& buffer : mix_buffers_) {
            std::fill(buffer.begin(), buffer.end(), 0.0F);
        }
        if (!paused) underrun_count_.fetch_add(1, std::memory_order_relaxed);
    }

    std::uint64_t clipped = 0;
    const std::uint32_t frames = static_cast<std::uint32_t>(buffer_frames_);
    for (std::size_t channel = 0; channel < buffer_infos_.size(); ++channel) {
        void* destination = nullptr;
        if (index == 0 || index == 1) {
            destination = buffer_infos_[channel].buffers[index];
        }
        if (destination == nullptr) {
            invalid_buffers_.fetch_add(1, std::memory_order_relaxed);
            continue;
        }
        ConvertPlanarFloatToAsio(format_, mix_buffers_[channel].data(),
                                 destination, frames, clipped);
    }
    if (clipped != 0) {
        clipped_samples_.fetch_add(clipped, std::memory_order_relaxed);
    }
    driver_->OutputReady();
    callback_count_.fetch_add(1, std::memory_order_relaxed);
}

AsioOutputEvents AsioOutput::TakeEvents() noexcept {
    AsioOutputEvents events;
    events.callback_count = callback_count_.load(std::memory_order_relaxed);
    events.underrun_count = underrun_count_.load(std::memory_order_relaxed);
    events.clipped_samples = clipped_samples_.load(std::memory_order_relaxed);
    events.invalid_buffers = invalid_buffers_.load(std::memory_order_relaxed);
    events.reset_requests = reset_requests_.load(std::memory_order_relaxed);
    events.resync_requests = resync_requests_.load(std::memory_order_relaxed);
    events.overloads = overloads_.load(std::memory_order_relaxed);
    events.sample_rate_changes = sample_rate_changes_.load(std::memory_order_relaxed);
    events.buffer_size_changes = buffer_size_changes_.load(std::memory_order_relaxed);
    events.latencies_changed = latencies_changed_.load(std::memory_order_relaxed);
    events.last_driver_sample_rate =
        driver_sample_rate_.load(std::memory_order_relaxed);
    events.last_sample_position = last_sample_position_.load(std::memory_order_relaxed);
    events.last_qpc = last_qpc_.load(std::memory_order_relaxed);
    events.last_buffer_frames = last_buffer_frames_.load(std::memory_order_relaxed);
    return events;
}

bool AsioOutput::ResetRequested() const noexcept {
    return reset_requests_.load(std::memory_order_acquire) > 0;
}

void AsioOutput::UpdateInfoFromDriver() noexcept {
    if (!driver_) return;
    info_.buffer_frames = buffer_frames_;
    info_.output_latency_frames =
        static_cast<std::int32_t>(output_latency_frames_.load(std::memory_order_relaxed));
    info_.output_latency_reported = output_latency_reported_;
    info_.format = format_;
    info_.sample_rate = driver_sample_rate_.load(std::memory_order_relaxed);
    for (std::size_t i = 0; i < info_.channels.size(); ++i) {
        AsioChannelDescription updated;
        std::string ignored;
        if (driver_->GetChannelInfo(info_.channels[i].channel, false, updated, ignored)) {
            info_.channels[i] = std::move(updated);
        }
    }
}

AsioOutputInfo AsioOutput::Info() const { return info_; }

} // namespace nosfix::audio
