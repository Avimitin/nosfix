#include "nosfix/audio_clock_patch.hpp"
#include "nosfix/module_guard.hpp"
#include "nosfix/log.hpp"

#include <Windows.h>
#include <cstring>

namespace nosfix {
namespace {
unsigned char* g_target = nullptr;
bool g_installed = false;
DWORD g_original_protection = 0;

bool Write(const std::array<unsigned char, 44>& bytes) noexcept {
    DWORD protection = 0;
    if (!VirtualProtect(g_target, bytes.size(), PAGE_EXECUTE_READWRITE, &protection))
        return false;
    if (g_original_protection == 0) g_original_protection = protection;
    std::memcpy(g_target, bytes.data(), bytes.size());
    const bool flushed = FlushInstructionCache(GetCurrentProcess(), g_target, bytes.size()) != FALSE;
    DWORD discarded = 0;
    const bool restored = VirtualProtect(g_target, bytes.size(), g_original_protection, &discarded) != FALSE;
    return flushed && restored;
}
} // namespace

bool PrepareAudioClockPatch(std::string& reason) {
    if (g_target) { reason = "audio clock patch already prepared"; return false; }
    ModuleIdentity identity;
    std::wstring guard_reason;
    if (!VerifyLibxactModule(identity, guard_reason)) {
        reason.clear();
        for (const wchar_t c : guard_reason)
            reason.push_back(c <= 127 ? static_cast<char>(c) : '?');
        return false;
    }
    if (identity.image_size < kAudioClockMathRva + kAudioClockOriginal.size()) {
        reason = "libxact cue clock is outside the image";
        return false;
    }
    auto* target = reinterpret_cast<unsigned char*>(identity.module) + kAudioClockMathRva;
    if (std::memcmp(target, kAudioClockOriginal.data(), kAudioClockOriginal.size())) {
        reason = "libxact cue clock instructions do not match the analyzed build";
        return false;
    }
    g_target = target;
    return true;
}

bool CommitAudioClockPatch(std::string& reason) {
    if (!g_target || g_installed) {
        reason = "audio clock patch is not ready to commit";
        return false;
    }
    if (std::memcmp(g_target, kAudioClockOriginal.data(), kAudioClockOriginal.size())) {
        reason = "libxact cue clock changed after validation";
        return false;
    }
    // Set first so failure after copying also restores the old instructions.
    g_installed = true;
    if (!Write(kAudioClockPrecise)) {
        reason = "cannot write/flush/protect libxact cue clock patch";
        RollbackAudioClockPatch();
        return false;
    }
    return true;
}

void RollbackAudioClockPatch() noexcept {
    if (g_installed && !Write(kAudioClockOriginal)) {
        Log(LogLevel::Error, "could not completely restore libxact cue clock patch");
        return;
    }
    g_installed = false;
    g_target = nullptr;
    g_original_protection = 0;
}
} // namespace nosfix
