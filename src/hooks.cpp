#include "nosfix/hooks.hpp"

#include <MinHook.h>

#include <cstring>
#include <mutex>

namespace nosfix {
namespace {

struct InstalledHook {
    void* target;
    void** original;
};

std::mutex g_hook_mutex;
std::vector<InstalledHook> g_installed;
bool g_initialized = false;

void Rollback() noexcept {
    for (auto it = g_installed.rbegin(); it != g_installed.rend(); ++it) {
        MH_DisableHook(it->target);
    }
    for (auto it = g_installed.rbegin(); it != g_installed.rend(); ++it) {
        MH_RemoveHook(it->target);
        if (it->original) *it->original = nullptr;
    }
    g_installed.clear();
    if (g_initialized) {
        MH_Uninitialize();
        g_initialized = false;
    }
}

} // namespace

bool InstallHooks(HMODULE module, std::span<const HookSpec> specs,
                  std::string& reason) {
    std::lock_guard lock(g_hook_mutex);
    if (!module) {
        reason = "hook target module is null";
        return false;
    }
    if (specs.empty()) return true;
    if (!g_installed.empty()) {
        reason = "hook set is already installed";
        return false;
    }
    // No container growth is allowed after MinHook writes the first target.
    g_installed.reserve(specs.size());

    const auto* base = reinterpret_cast<const unsigned char*>(module);
    auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew <= 0) {
        reason = "invalid target module DOS header";
        return false;
    }
    const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS64*>(base + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE ||
        nt->FileHeader.Machine != IMAGE_FILE_MACHINE_AMD64 ||
        nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
        reason = "target module is not a valid x64 PE image";
        return false;
    }

    // Check all addresses and prologues before creating any detours.
    for (const auto& spec : specs) {
        if (!spec.name || !spec.detour || !spec.original || spec.expected.empty()) {
            reason = "hook specification is incomplete";
            return false;
        }
        if (spec.rva > nt->OptionalHeader.SizeOfImage ||
            spec.expected.size() > nt->OptionalHeader.SizeOfImage - spec.rva) {
            reason = std::string(spec.name) + ": RVA is outside the target image";
            return false;
        }
        const auto* target = base + spec.rva;
        MEMORY_BASIC_INFORMATION memory{};
        if (!VirtualQuery(target, &memory, sizeof(memory)) ||
            memory.State != MEM_COMMIT ||
            reinterpret_cast<const unsigned char*>(memory.BaseAddress) + memory.RegionSize <
                target + spec.expected.size()) {
            reason = std::string(spec.name) + ": target bytes are not readable";
            return false;
        }
        if (std::memcmp(target, spec.expected.data(), spec.expected.size()) != 0) {
            reason = std::string(spec.name) + ": target bytes do not match expected build";
            return false;
        }
    }

    const MH_STATUS init_status = MH_Initialize();
    if (init_status != MH_OK) {
        reason = "MinHook initialization failed with status " +
                 std::to_string(static_cast<int>(init_status));
        return false;
    }
    g_initialized = true;

    for (const auto& spec : specs) {
        void* target = const_cast<unsigned char*>(base + spec.rva);
        const MH_STATUS status = MH_CreateHook(target, spec.detour, spec.original);
        if (status != MH_OK) {
            reason = std::string(spec.name) + ": MinHook create failed with status " +
                     std::to_string(static_cast<int>(status));
            Rollback();
            return false;
        }
        g_installed.push_back({target, spec.original});
    }

    for (size_t i = 0; i < specs.size(); ++i) {
        const MH_STATUS status = MH_QueueEnableHook(g_installed[i].target);
        if (status != MH_OK) {
            reason = std::string(specs[i].name) +
                     ": MinHook queue-enable failed with status " +
                     std::to_string(static_cast<int>(status));
            Rollback();
            return false;
        }
    }
    const MH_STATUS apply_status = MH_ApplyQueued();
    if (apply_status != MH_OK) {
        reason = "MinHook queued enable transaction failed with status " +
                 std::to_string(static_cast<int>(apply_status));
        Rollback();
        return false;
    }
    return true;
}

void RemoveHooks() noexcept {
    std::lock_guard lock(g_hook_mutex);
    Rollback();
}

} // namespace nosfix
