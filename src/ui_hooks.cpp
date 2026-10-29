#include "nosfix/ui_hooks.hpp"

#include "nosfix/presentation.hpp"
#include "nosfix/log.hpp"
#include "nosfix/ui_motion.hpp"

#include <Windows.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <string>
#include <unordered_map>

namespace nosfix {
namespace {

using ComposeFunction = void(__fastcall*)(void* node);

constexpr DWORD kComposeCallRva = 0xB6DFB;
constexpr DWORD kComposeFunctionRva = 0x3080;
constexpr std::size_t kCopiedNodeBytes = 208;
constexpr std::size_t kNodeLocalMatrixOffset = 128;
constexpr std::size_t kNodeStatusFlagsOffset = 32;
constexpr std::size_t kNodeFlagsOffset = 204;
constexpr std::size_t kNodeTypeOffset = 8;
constexpr std::size_t kNodeResourceOffset = 48;
constexpr std::size_t kNodeParentOffset = 64;
constexpr std::size_t kNodeAllocationStampOffset = 312;
constexpr std::size_t kTrackCapacity = 8192;
constexpr std::uint64_t kPruneAfterTicks = 2;

constexpr std::array<unsigned char, 5> kExpectedComposeCall{
    0xE8, 0x80, 0xC2, 0xF4, 0xFF};
constexpr std::array<unsigned char, 24> kExpectedComposeBytes{
    0x48,0x83,0xEC,0x48,0x49,0x89,0xC8,0x49,0x81,0xC0,0x80,0x00,
    0x00,0x00,0x48,0x8B,0x0D,0x2B,0x2A,0x18,0x00,0x45,0x0F,0xB6};

std::atomic<bool> g_enabled{false};
std::atomic<std::uint64_t> g_config_generation{0};
unsigned char* g_afp_base = nullptr;
bool g_prepared = false;

struct Track {
    UiNodeKey key{};
    UiTransformPredictor predictor;
    std::uint64_t last_seen_tick = 0;
    std::uint64_t last_counted_tick = 0;
    bool has_counted_tick = false;
};

struct ThreadState {
    std::unordered_map<const void*, Track> tracks;
    UiMotionMetrics metrics{};
    std::uint64_t tick_serial = 0;
    std::uint64_t config_generation = 0;
    double phase = 0.0;
    bool presentation_active = false;
    unsigned diagnostic_samples = 0;
};

thread_local ThreadState g_thread;

void SyncConfiguration() noexcept {
    const std::uint64_t generation = g_config_generation.load(std::memory_order_acquire);
    if (g_thread.config_generation == generation) return;
    g_thread.tracks.clear();
    g_thread.metrics = {};
    g_thread.tick_serial = 0;
    g_thread.phase = 0.0;
    g_thread.presentation_active = false;
    g_thread.config_generation = generation;
}

template <typename T>
T ReadNodeField(const void* node, std::size_t offset) noexcept {
    T value{};
    std::memcpy(&value, static_cast<const std::byte*>(node) + offset, sizeof(value));
    return value;
}

UiNodeKey MakeKey(const void* node) noexcept {
    return {
        reinterpret_cast<std::uintptr_t>(node),
        ReadNodeField<std::uint32_t>(node, kNodeAllocationStampOffset),
        ReadNodeField<std::uintptr_t>(node, kNodeParentOffset),
        ReadNodeField<std::uint16_t>(node, kNodeTypeOffset),
        ReadNodeField<std::uintptr_t>(node, kNodeResourceOffset),
    };
}

ComposeFunction OriginalCompose() noexcept {
    if (g_afp_base == nullptr) return nullptr;
    return reinterpret_cast<ComposeFunction>(g_afp_base + kComposeFunctionRva);
}

bool ValidateModuleBytes(HMODULE module, std::string& reason) {
    auto* base = reinterpret_cast<unsigned char*>(module);
    const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew <= 0) {
        reason = "AFP module has an invalid DOS header";
        return false;
    }
    const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS64*>(base + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE ||
        nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
        reason = "AFP module is not a valid x64 PE image";
        return false;
    }
    const std::size_t image_size = nt->OptionalHeader.SizeOfImage;
    const auto matches = [&](DWORD rva, std::size_t size, const auto& expected) {
        return size == expected.size() && rva <= image_size &&
            size <= image_size - rva &&
            std::memcmp(base + rva, expected.data(), size) == 0;
    };
    if (!matches(kComposeCallRva, kExpectedComposeCall.size(),
                 kExpectedComposeCall)) {
        reason = "AFP node-matrix callsite bytes do not match the verified target";
        return false;
    }
    if (!matches(kComposeFunctionRva, kExpectedComposeBytes.size(),
                 kExpectedComposeBytes)) {
        reason = "AFP matrix helper bytes do not match the verified target";
        return false;
    }
    reason.clear();
    return true;
}

Track* GetTrack(const void* node, const UiNodeKey& key) noexcept {
    auto found = g_thread.tracks.find(node);
    if (found != g_thread.tracks.end()) {
        Track& track = found->second;
        if (track.key != key) {
            track.key = key;
            track.predictor.Reset();
            track.has_counted_tick = false;
        }
        return &track;
    }
    if (g_thread.tracks.size() >= kTrackCapacity) return nullptr;
    try {
        auto [inserted, did_insert] = g_thread.tracks.try_emplace(node);
        (void)did_insert;
        inserted->second.key = key;
        return &inserted->second;
    } catch (...) {
        return nullptr;
    }
}

void EraseTrack(const void* node) noexcept {
    g_thread.tracks.erase(node);
}

void __fastcall ComposeCallsite(void* node) noexcept {
    const ComposeFunction original = OriginalCompose();
    if (original == nullptr) return;
    SyncConfiguration();
    if (!g_enabled.load(std::memory_order_acquire) ||
        !g_thread.presentation_active || PresentationRate() <= 60 || node == nullptr) {
        original(node);
        return;
    }

    if (g_thread.diagnostic_samples < 4) {
        ++g_thread.diagnostic_samples;
        char line[320]{};
        sprintf_s(line, "UI transform sample: status=%08x modes=%u,%u "
            "affine row=%.5g,%.5g,%.5g,%.5g translation=%.3f,%.3f,%.3f pivot=%.3f,%.3f,%.3f",
            ReadNodeField<std::uint32_t>(node, 32),
            static_cast<unsigned>(ReadNodeField<std::uint8_t>(node, 204)),
            static_cast<unsigned>(ReadNodeField<std::uint8_t>(node, 205)),
            ReadNodeField<float>(node, 140), ReadNodeField<float>(node, 156),
            ReadNodeField<float>(node, 172), ReadNodeField<float>(node, 188),
            ReadNodeField<float>(node, 176), ReadNodeField<float>(node, 180),
            ReadNodeField<float>(node, 184), ReadNodeField<float>(node, 192),
            ReadNodeField<float>(node, 196), ReadNodeField<float>(node, 200));
        Log(LogLevel::Debug, line);
    }

    // Bit 4 is the target traversal's verified retired/disabled status bit.
    // Preserve that behavior if a call ever reaches this site with such a node.
    if ((ReadNodeField<std::uint32_t>(node, kNodeStatusFlagsOffset) & 4u) != 0) {
        EraseTrack(node);
        ++g_thread.metrics.fallback_draws;
        original(node);
        return;
    }

    const std::uint16_t transform_flags =
        ReadNodeField<std::uint8_t>(node, kNodeFlagsOffset) |
        (static_cast<std::uint16_t>(ReadNodeField<std::uint8_t>(
            node, kNodeFlagsOffset + 1)) << 1);
    if (transform_flags != 0) {
        EraseTrack(node);
        ++g_thread.metrics.fallback_draws;
        original(node);
        return;
    }

    const UiNodeKey key = MakeKey(node);
    Track* track = GetTrack(node, key);
    if (track == nullptr) {
        ++g_thread.metrics.fallback_draws;
        original(node);
        return;
    }
    track->last_seen_tick = g_thread.tick_serial;

    alignas(16) std::array<std::byte, kCopiedNodeBytes> shadow{};
    std::memcpy(shadow.data(), node, shadow.size());
    std::array<float, UiTransformPredictor::kElementCount> source_matrix{};
    std::memcpy(source_matrix.data(),
        static_cast<const std::byte*>(node) + kNodeLocalMatrixOffset,
        sizeof(source_matrix));
    const std::span<const float, UiTransformPredictor::kElementCount> sample(
        source_matrix.data(), source_matrix.size());
    if (!track->predictor.Capture(key, g_thread.tick_serial, sample)) {
        ++g_thread.metrics.fallback_draws;
        original(node);
        return;
    }
    if (!track->has_counted_tick || track->last_counted_tick != g_thread.tick_serial) {
        ++g_thread.metrics.captures;
        track->last_counted_tick = g_thread.tick_serial;
        track->has_counted_tick = true;
    }

    if (g_thread.phase <= 0.0 || !track->predictor.HasHistory()) {
        ++g_thread.metrics.fallback_draws;
        original(node);
        return;
    }

    std::array<float, UiTransformPredictor::kElementCount> predicted{};
    if (!track->predictor.Evaluate(key, g_thread.tick_serial,
                                  g_thread.phase, predicted)) {
        ++g_thread.metrics.fallback_draws;
        original(node);
        return;
    }
    if (std::equal(predicted.begin(), predicted.end(), source_matrix.begin())) {
        ++g_thread.metrics.fallback_draws;
        original(node);
        return;
    }

    ++g_thread.metrics.predicted_draws;
    std::memcpy(shadow.data() + kNodeLocalMatrixOffset,
                predicted.data(), sizeof(predicted));
    // sub_180003080 reads only transform fields through byte +205. This
    // aligned shadow keeps its game-owned node and parent links untouched.
    original(shadow.data());
}

} // namespace

bool PrepareUiMotion(HMODULE afp_module, bool enabled,
                     std::vector<CallPatchSpec>& patches,
                     std::string& reason) {
    g_enabled.store(false, std::memory_order_release);
    g_prepared = false;
    g_afp_base = nullptr;
    if (!enabled) {
        reason.clear();
        return true;
    }
    if (afp_module == nullptr) {
        reason = "smooth UI motion requested but afp-core.dll is not loaded";
        return false;
    }
    if (!ValidateModuleBytes(afp_module, reason)) return false;

    try {
        patches.push_back({"AFP node transform compose", kComposeCallRva,
            kExpectedComposeCall, reinterpret_cast<void*>(&ComposeCallsite),
            afp_module});
    } catch (...) {
        reason = "cannot allocate the AFP UI call-patch specification";
        return false;
    }
    g_afp_base = reinterpret_cast<unsigned char*>(afp_module);
    g_prepared = true;
    reason.clear();
    return true;
}

void ConfigureUiMotion(bool enabled) noexcept {
    g_enabled.store(enabled && g_prepared && g_afp_base != nullptr,
                    std::memory_order_release);
    g_config_generation.fetch_add(1, std::memory_order_acq_rel);
    SyncConfiguration();
}

void BeginUiPresentation(double phase) noexcept {
    SyncConfiguration();
    g_thread.phase = std::isfinite(phase) ? std::clamp(phase, 0.0, 1.0) : 0.0;
    g_thread.presentation_active = g_enabled.load(std::memory_order_acquire) &&
                                   PresentationRate() > 60;
}

void EndUiPresentation() noexcept {
    SyncConfiguration();
    g_thread.presentation_active = false;
    g_thread.phase = 0.0;
}

void RetireUiTick() noexcept {
    SyncConfiguration();
    if (g_thread.tick_serial == UINT64_MAX) {
        g_thread.tick_serial = 0;
        g_thread.tracks.clear();
        return;
    }
    ++g_thread.tick_serial;
    for (auto iterator = g_thread.tracks.begin();
         iterator != g_thread.tracks.end();) {
        const Track& track = iterator->second;
        if (g_thread.tick_serial >= track.last_seen_tick &&
            g_thread.tick_serial - track.last_seen_tick > kPruneAfterTicks) {
            iterator = g_thread.tracks.erase(iterator);
        } else {
            ++iterator;
        }
    }
}

UiMotionMetrics TakeUiMetrics() noexcept {
    SyncConfiguration();
    const UiMotionMetrics result = g_thread.metrics;
    g_thread.metrics = {};
    return result;
}

} // namespace nosfix
