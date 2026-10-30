#include "nosfix/asio_route.hpp"

#include "nosfix/asio_audio_host.hpp"
#include "nosfix/log.hpp"

#include <MinHook.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

namespace nosfix {
namespace {

constexpr std::size_t kCreateEngineRva = 0x3730;
constexpr std::array<unsigned char, 24> kCreateEngineBytes{
    0x40,0x53,0x56,0x57,0x48,0x83,0xEC,0x40,0x48,0x8B,0x05,0x01,
    0x63,0x02,0x00,0x48,0x33,0xC4,0x48,0x89,0x44,0x24,0x38,0x48};

using CreateEngine = HRESULT(__fastcall*)(void** out, unsigned flags,
                                          unsigned processor);
using EngineInitialize = std::int32_t(__fastcall*)(void* engine, unsigned flags,
                                                   unsigned processor);

CreateEngine g_original = nullptr;
std::unique_ptr<audio::AsioAudioHost> g_host;
void* g_target = nullptr;
bool g_active = false;
bool g_engine_initialized = false;

bool ResolveDriverClsid(const Config& config, std::string& clsid,
                        std::string& error) {
    clsid = config.asio_driver_clsid;
    if (!clsid.empty()) return true;
    if (config.asio_driver_name.empty()) {
        error = "asio_driver_clsid or asio_driver_name is required";
        return false;
    }
    std::vector<audio::AsioDriverRegistration> drivers;
    if (!audio::EnumerateSystemAsioDrivers(drivers, error)) return false;
    std::vector<std::string> matches;
    for (const auto& driver : drivers) {
        if (driver.name == config.asio_driver_name ||
            driver.description == config.asio_driver_name) {
            matches.push_back(driver.clsid);
        }
    }
    if (matches.size() != 1) {
        error = matches.empty() ? "ASIO driver name not found"
                                : "ASIO driver name is ambiguous";
        return false;
    }
    clsid = matches.front();
    return true;
}

bool ParseChannels(const std::string& text, std::vector<std::int32_t>& out,
                   std::string& error) {
    out.clear();
    std::size_t start = 0;
    while (start <= text.size()) {
        const std::size_t comma = text.find(',', start);
        const std::string piece = text.substr(
            start, comma == std::string::npos ? std::string::npos : comma - start);
        try {
            const int channel = std::stoi(piece);
            if (channel < 0 || channel > 1023 ||
                std::find(out.begin(), out.end(), channel) != out.end()) {
                error = "invalid or duplicate ASIO output channel";
                return false;
            }
            out.push_back(channel);
        } catch (...) {
            error = "invalid asio_output_channels list";
            return false;
        }
        if (comma == std::string::npos) break;
        start = comma + 1;
    }
    if (out.empty() || out.size() > 16) {
        error = "ASIO output channel count must be 1..16";
        return false;
    }
    return true;
}

HRESULT __fastcall CreateEngineDetour(void** out, unsigned flags,
                                       unsigned processor) {
    if (!g_active || !g_host) {
        return g_original ? g_original(out, flags, processor) : E_FAIL;
    }
    void* engine = g_host->Engine();
    if (!engine || !out) return E_FAIL;
    if (!g_engine_initialized) {
        auto** vtable = *reinterpret_cast<void***>(engine);
        const auto initialize = reinterpret_cast<EngineInitialize>(vtable[5]);
        const std::int32_t result = initialize(engine, flags, processor);
        if (result < 0) return result;
        g_engine_initialized = true;
    }
    *out = engine;
    return S_OK;
}

} // namespace

bool ConfigureAsioRoute(const Config& config, HMODULE xact,
                        std::string& reason) {
    DisableAsioRoute();
    if (!config.asio_output) return true;
    if (!xact) {
        reason = "libxact module is null";
        return false;
    }
    const auto* base = reinterpret_cast<const std::byte*>(xact);
    if (std::memcmp(base + kCreateEngineRva, kCreateEngineBytes.data(),
                    kCreateEngineBytes.size()) != 0) {
        reason = "libxact CreateEngine bytes do not match the analyzed build";
        return false;
    }

    std::string clsid;
    if (!ResolveDriverClsid(config, clsid, reason)) return false;
    std::vector<std::int32_t> channels;
    if (!ParseChannels(config.asio_output_channels, channels, reason)) return false;

    audio::AsioHostConfig host_config;
    host_config.output.driver_clsid = clsid;
    host_config.output.expected_driver_name = config.asio_driver_name;
    host_config.output.engine_sample_rate = config.asio_sample_rate;
    host_config.output.buffer_frames = static_cast<std::int32_t>(
        config.asio_buffer_frames);
    host_config.output.output_channels = channels;
    host_config.bridge.device_rate = config.asio_sample_rate;
    host_config.bridge.device_channels = static_cast<std::uint32_t>(channels.size());
    host_config.bridge.max_block_frames = std::max<std::uint32_t>(
        1024, config.asio_buffer_frames);
    host_config.bridge.quantum_frames = config.asio_buffer_frames;
    host_config.bridge.destroy_on_release = false;
    host_config.mixer_max_block_frames = std::max<std::uint32_t>(
        1024, config.asio_buffer_frames);
    host_config.keysound_capacity = 1024;

    auto host = std::make_unique<audio::AsioAudioHost>();
    if (!host->Open(host_config, reason)) return false;
    if (!host->Start(reason)) {
        host->Close();
        return false;
    }

    const MH_STATUS initialized = MH_Initialize();
    if (initialized != MH_OK && initialized != MH_ERROR_ALREADY_INITIALIZED) {
        host->Stop();
        host->Close();
        reason = "MinHook initialization failed";
        return false;
    }
    g_target = const_cast<std::byte*>(base) + kCreateEngineRva;
    const MH_STATUS created = MH_CreateHook(
        g_target, reinterpret_cast<void*>(&CreateEngineDetour),
        reinterpret_cast<void**>(&g_original));
    if (created != MH_OK) {
        host->Stop();
        host->Close();
        reason = "cannot create the libxact CreateEngine hook";
        return false;
    }
    const MH_STATUS enabled = MH_EnableHook(g_target);
    if (enabled != MH_OK) {
        MH_RemoveHook(g_target);
        g_target = nullptr;
        host->Stop();
        host->Close();
        reason = "cannot enable the libxact CreateEngine hook";
        return false;
    }

    g_host = std::move(host);
    g_engine_initialized = false;
    g_active = true;
    Log(LogLevel::Info, "ASIO route active: libxact XAudio2 engine replaced by the bridge");
    return true;
}

void DisableAsioRoute() noexcept {
    g_active = false;
    if (g_target) {
        MH_DisableHook(g_target);
        MH_RemoveHook(g_target);
        g_target = nullptr;
    }
    g_original = nullptr;
    if (g_host) {
        g_host->Stop();
        g_host->Close();
        g_host.reset();
    }
    g_engine_initialized = false;
}

bool AsioRouteActive() noexcept { return g_active; }

} // namespace nosfix
