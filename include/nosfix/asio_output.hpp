#pragma once

#include "nosfix/audio_timeline.hpp"

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

// ASIO output lifecycle and buffer conversion.
//
// The ASIO 2.3 ABI declarations in asio_output.cpp are an independent
// transcription of the publicly documented host/driver interface
// (IASIO vtable order, ASIODriverInfo/ASIOBufferInfo/AsioTimeInfo layout).
// No Steinberg SDK source is included or redistributed. Anyone redistributing
// an ASIO host must comply with the Steinberg ASIO SDK license and obtain the
// SDK from Steinberg; the interface facts themselves are not copied code.
//
// This module deliberately does not select a device. Open() requires an exact
// driver CLSID and an explicit output-channel list. It does not fall back to
// the first registered driver, the default device or channels 0/1.
namespace nosfix::audio {

enum class AsioSampleFormat {
    Int16Lsb,
    Int24Lsb,
    Int32Lsb,
    Float32Lsb,
    Float64Lsb,
    Unsupported,
};

[[nodiscard]] const char* AsioSampleFormatName(AsioSampleFormat format) noexcept;

struct AsioDriverRegistration {
    std::string name;        // HKLM\SOFTWARE\ASIO subkey name
    std::string description; // "Description" value, if present
    std::string clsid;       // "CLSID" value, e.g. {00000000-0000-0000-0000-000000000000}
    bool registry64 = true;  // KEY_WOW64_64KEY view (required for an x64 host)
};

// Control-thread only. Enumerates registered ASIO drivers without opening one.
[[nodiscard]] bool EnumerateSystemAsioDrivers(
    std::vector<AsioDriverRegistration>& out, std::string& error);

struct AsioDriverInfo {
    std::string name;
    std::int32_t asio_version = 0;
    std::int32_t driver_version = 0;
    std::string init_error;
};

struct AsioBufferRange {
    std::int32_t min_frames = 0;
    std::int32_t max_frames = 0;
    std::int32_t preferred_frames = 0;
    std::int32_t granularity = 0; // -1: powers of two; 0: any; >0: multiple
};

struct AsioChannelDescription {
    std::int32_t channel = 0;
    bool is_input = false;
    bool is_active = false;
    std::int32_t group = 0;
    AsioSampleFormat format = AsioSampleFormat::Unsupported;
    std::string name;
};

// Host-side logical ASIO time info. This is NOT the packed native Windows
// layout: the SDK uses NATIVE_INT64=0 and #pragma pack(4), so native
// ASIOTimeStamp/ASIOSamples are hi/lo word pairs and ASIOTime is 148 bytes.
// asio_output.cpp converts the native packed values at the COM boundary.
struct AsioTimeInfo {
    double speed = 1.0;
    std::int64_t system_time_ns = 0;
    std::int64_t sample_position = 0;
    double sample_rate = 0.0;
    std::uint32_t flags = 0;
    char reserved[12] = {};
};

// Logical host-side time code (native ASIOTimeCode is 84 packed bytes).
struct AsioTimeCode {
    double speed = 1.0;
    std::int64_t time_code_samples = 0;
    std::uint32_t flags = 0;
    char future[64] = {};
};

// Logical host-side ASIOTime (native packed ASIOTime is 148 bytes).
// system_time_ns is nanoseconds from the driver (on Windows derived from
// timeGetTime per the SDK), not QPC. sample_position is the position of the
// buffer being switched, reset to zero by ASIOStart.
struct AsioTime {
    std::int32_t reserved[4] = {};
    AsioTimeInfo time_info{};
    AsioTimeCode time_code{};
};

struct AsioBufferInfo {
    bool is_input = false;
    std::int32_t channel = 0;
    void* buffers[2] = {nullptr, nullptr};
};

struct AsioCallbacks {
    void* context = nullptr;
    void (*buffer_switch)(void* context, std::int32_t index,
                          bool direct_process) noexcept = nullptr;
    void (*sample_rate_changed)(void* context, double rate) noexcept = nullptr;
    std::int32_t (*message)(void* context, std::int32_t selector,
                            std::int32_t value, void* message,
                            double* opt) noexcept = nullptr;
    void (*buffer_switch_time_info)(void* context, AsioTime* time_info,
                                    std::int32_t index,
                                    bool direct_process) noexcept = nullptr;
};

// Implemented by the real COM driver and by fake test drivers. Control-thread
// methods may allocate (error strings); the only method called from the ASIO
// callback thread is OutputReady(), which must not allocate or block.
class AsioDriver {
public:
    virtual ~AsioDriver() = default;

    virtual bool Init(void* sys_ref, AsioDriverInfo& info, std::string& error) noexcept = 0;
    virtual void Shutdown() noexcept = 0;
    virtual bool GetChannelCounts(std::int32_t& inputs, std::int32_t& outputs,
                                  std::string& error) noexcept = 0;
    virtual bool CanSampleRate(double rate, std::string& error) noexcept = 0;
    virtual bool GetSampleRate(double& rate, std::string& error) noexcept = 0;
    virtual bool SetSampleRate(double rate, std::string& error) noexcept = 0;
    virtual bool GetBufferRange(AsioBufferRange& range, std::string& error) noexcept = 0;
    virtual bool GetChannelInfo(std::int32_t channel, bool input,
                                AsioChannelDescription& out,
                                std::string& error) noexcept = 0;
    virtual bool CreateBuffers(AsioBufferInfo* infos, std::int32_t count,
                               std::int32_t frames, const AsioCallbacks& callbacks,
                               std::string& error) noexcept = 0;
    virtual void DisposeBuffers() noexcept = 0;
    virtual bool GetLatencies(std::int32_t& input_frames,
                              std::int32_t& output_frames,
                              std::string& error) noexcept = 0;
    virtual bool Start(std::string& error) noexcept = 0;
    virtual void Stop() noexcept = 0;
    virtual bool GetSamplePosition(std::int64_t& sample,
                                   std::int64_t& system_time_ns,
                                   std::string& error) noexcept = 0;
    // Callback-safe variant used only when a driver supplies no AsioTime.
    // Must not allocate or block; returns false and leaves outputs untouched.
    virtual bool GetSamplePositionFast(std::int64_t& sample,
                                       std::int64_t& system_time_ns) noexcept = 0;
    virtual void ControlPanel() noexcept = 0;
    virtual void OutputReady() noexcept = 0;
};

// Creates the COM driver for an exact CLSID. The caller must initialize COM
// first (AsioOutput::OpenSystem does). Returns nullptr and sets error if the
// class is not registered, is not a 64-bit in-proc server, or does not expose
// IASIO. Does not open audio hardware until Init() is called.
[[nodiscard]] std::unique_ptr<AsioDriver> CreateSystemAsioDriver(
    const std::string& clsid, std::string& error);

struct AsioOpenOptions {
    // Exact driver class. Required; no implicit device selection.
    std::string driver_clsid;

    // Optional cross-check after ASIOInit. If non-empty it must match the
    // driver-reported name, otherwise Open fails.
    std::string expected_driver_name;

    double engine_sample_rate = 44100.0;

    // Exact buffer size requested. Must satisfy the driver's range/granularity;
    // Open fails rather than silently clamping to the preferred size.
    std::int32_t buffer_frames = 128;

    // Explicit ASIO output channel indices, in mix order. Required.
    std::vector<std::int32_t> output_channels;

    std::uint64_t initial_generation = 1;
};

struct AsioOutputInfo {
    std::string driver_name;
    std::string driver_clsid;
    std::int32_t asio_version = 0;
    std::int32_t driver_version = 0;
    double sample_rate = 0.0;
    std::int32_t buffer_frames = 0;
    std::int32_t output_latency_frames = 0;
    bool output_latency_reported = false;
    AsioSampleFormat format = AsioSampleFormat::Unsupported;
    std::vector<AsioChannelDescription> channels;
};

struct AsioRenderContext {
    std::uint64_t generation = 0;
    std::int64_t block_start_sample = 0; // ASIO samplePosition of this buffer
    std::int64_t qpc_ticks = 0;
    std::int64_t qpc_frequency = 0;
    std::int64_t driver_system_time_ns = 0;
    bool sample_position_valid = false;
    bool system_time_valid = false;
    bool time_info_mode = false;
    std::uint32_t buffer_frames = 0;
    std::uint32_t output_latency_frames = 0;
};

// Called on the ASIO callback thread. Must fill every channel for
// ctx.buffer_frames samples in [-1, 1], without allocating, locking, logging,
// waiting, touching files or calling gameplay code. Return false to request
// silence for this block (counted as an underrun). Starting keysound voices at
// dispatch.offset_in_block is the caller's responsibility.
using AsioRenderCallback = bool (*)(void* context,
                                    const AsioRenderContext& ctx,
                                    float* const* planar_channels) noexcept;

struct AsioOutputEvents {
    std::uint64_t callback_count = 0;
    std::uint64_t underrun_count = 0;
    std::uint64_t clipped_samples = 0;
    std::uint64_t invalid_buffers = 0;
    std::uint32_t reset_requests = 0;
    std::uint32_t resync_requests = 0;
    std::uint32_t overloads = 0;
    std::uint32_t sample_rate_changes = 0;
    std::uint32_t buffer_size_changes = 0;
    std::uint32_t latencies_changed = 0;
    double last_driver_sample_rate = 0.0;
    std::int64_t last_sample_position = 0;
    std::int64_t last_qpc = 0;
    std::uint32_t last_buffer_frames = 0;
};

// Converts planar float [-1, 1] to one ASIO channel buffer. Clips and counts
// clipped samples; NaN becomes 0. Not realtime-safe only if the destination is
// invalid; the function itself allocates nothing.
void ConvertPlanarFloatToAsio(AsioSampleFormat format,
                              const float* source,
                              void* destination,
                              std::uint32_t frames,
                              std::uint64_t& clipped_samples) noexcept;

class AsioOutput final {
public:
    AsioOutput();
    ~AsioOutput();

    AsioOutput(const AsioOutput&) = delete;
    AsioOutput& operator=(const AsioOutput&) = delete;

    // Takes ownership of the driver. The driver must not be running.
    [[nodiscard]] bool Open(std::unique_ptr<AsioDriver> driver,
                            const AsioOpenOptions& options,
                            std::string& error);
    [[nodiscard]] bool OpenSystem(const AsioOpenOptions& options,
                                  std::string& error);

    // Install before Start() or while stopped. The callback/context pair is not
    // an atomic transaction; changing it while the ASIO thread is live can pair
    // a new callback with the previous context.
    void SetRenderCallback(AsioRenderCallback callback, void* context) noexcept;
    // Safe while running: the next block is silent or rendered.
    void SetPaused(bool paused) noexcept;

    [[nodiscard]] bool Start(std::string& error);
    void Stop() noexcept;
    void Close() noexcept;

    // Control thread only, after Stop()/while stopped. Stops, disposes,
    // recreates buffers with the same or a new size, starts, and assigns a new
    // generation. Never called from the ASIO callback.
    [[nodiscard]] bool Restart(std::uint64_t new_generation,
                               std::int32_t new_buffer_frames,
                               std::string& error);

    // Control thread. Applies a driver reset request, buffer-size change or
    // latency change reported by the callback. Returns false if the engine
    // sample rate no longer matches, in which case a full Open is required.
    [[nodiscard]] bool Service(std::string& error);

    [[nodiscard]] AsioOutputEvents TakeEvents() noexcept;
    [[nodiscard]] bool ResetRequested() const noexcept;

    [[nodiscard]] bool IsOpen() const noexcept { return driver_ != nullptr; }
    [[nodiscard]] bool IsRunning() const noexcept { return running_.load(std::memory_order_acquire); }
    // Advances on open, Restart/Service reset, every Stop/Start, and a
    // driver-observed sample-position discontinuity. Keysound events must carry
    // this value and are dropped when it no longer matches.
    [[nodiscard]] std::uint64_t Generation() const noexcept {
        return generation_.load(std::memory_order_acquire);
    }
    // Control-thread snapshot; not synchronized with Restart/Close.
    [[nodiscard]] AsioOutputInfo Info() const;

    // Audio-callback-thread view. Cross-thread readers need their own snapshot.
    [[nodiscard]] AudioTimeline& Timeline() noexcept { return timeline_; }
    [[nodiscard]] const AudioTimeline& Timeline() const noexcept { return timeline_; }

private:
    friend class AsioCallbackDispatcher;

    void OnBufferSwitch(std::int32_t index, AsioTime* time_info, bool have_time_info) noexcept;
    void OnSampleRateChanged(double rate) noexcept;
    std::int32_t OnMessage(std::int32_t selector, std::int32_t value,
                           void* message, double* opt) noexcept;

    [[nodiscard]] bool ConfigureDriver(const AsioOpenOptions& options,
                                       std::string& error);
    [[nodiscard]] bool CreateDriverBuffers(std::string& error);
    void DestroyDriverBuffers() noexcept;
    void UpdateInfoFromDriver() noexcept;

    std::unique_ptr<AsioDriver> driver_;
    AsioOpenOptions options_{};
    AsioOutputInfo info_{};
    AsioBufferRange buffer_range_{};

    std::vector<AsioBufferInfo> buffer_infos_;
    std::vector<std::vector<float>> mix_buffers_;
    std::vector<float*> mix_planar_;
    AsioSampleFormat format_ = AsioSampleFormat::Unsupported;

    std::int32_t buffer_frames_ = 0;
    std::atomic<std::uint32_t> output_latency_frames_{0};
    std::atomic<std::uint32_t> pending_output_latency_{0};
    std::atomic<bool> pending_output_latency_valid_{false};
    std::int64_t qpc_frequency_ = 0;
    bool output_latency_reported_ = false;
    bool com_initialized_ = false;
    bool buffers_created_ = false;
    bool started_once_ = false;
    std::atomic<bool> time_info_mode_{false};
    std::int64_t expected_sample_position_ = 0; // callback thread only

    std::atomic<AsioRenderCallback> render_callback_{nullptr};
    std::atomic<void*> render_context_{nullptr};
    std::atomic<bool> paused_{false};
    std::atomic<bool> running_{false};
    std::atomic<std::uint64_t> generation_{0};
    std::atomic<bool> begin_generation_pending_{true};
    std::atomic<bool> force_timeline_discontinuity_{false};

    AudioTimeline timeline_{};

    std::atomic<std::uint64_t> callback_count_{0};
    std::atomic<std::uint64_t> underrun_count_{0};
    std::atomic<std::uint64_t> clipped_samples_{0};
    std::atomic<std::uint64_t> invalid_buffers_{0};
    std::atomic<std::uint32_t> reset_requests_{0};
    std::atomic<std::uint32_t> resync_requests_{0};
    std::atomic<std::uint32_t> overloads_{0};
    std::atomic<std::uint32_t> sample_rate_changes_{0};
    std::atomic<std::uint32_t> buffer_size_changes_{0};
    std::atomic<std::uint32_t> latencies_changed_{0};

    std::atomic<double> driver_sample_rate_{0.0};
    std::atomic<std::int64_t> last_sample_position_{0};
    std::atomic<std::int64_t> last_qpc_{0};
    std::atomic<std::uint32_t> last_buffer_frames_{0};
    std::atomic<std::int32_t> requested_buffer_frames_{0};
    std::atomic<bool> sample_rate_mismatch_{false};
    std::atomic<bool> pending_latency_refresh_{false};
};

} // namespace nosfix::audio
