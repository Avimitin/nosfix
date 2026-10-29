#include "nosfix/call_patch.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <limits>
#include <mutex>
#include <vector>

namespace nosfix {
namespace {

constexpr std::size_t kCallSize = 5;
constexpr std::size_t kStubSize = 14;
constexpr std::uintptr_t kSearchDistance = 0x7FFF0000;

struct PreparedPatch {
    const CallPatchSpec* spec;
    unsigned char* target;
    unsigned char* stub;
    std::size_t allocation_size;
    std::array<unsigned char, kCallSize> patched{};
};

struct InstalledPatch {
    unsigned char* target;
    unsigned char* stub;
    std::size_t allocation_size;
    std::array<unsigned char, kCallSize> original{};
    std::array<unsigned char, kCallSize> patched{};
};

std::mutex g_mutex;
std::vector<InstalledPatch> g_installed;

bool FitsRel32(std::uintptr_t call_after, std::uintptr_t destination,
               std::int32_t& displacement) noexcept {
    const auto delta = static_cast<std::int64_t>(destination) -
                       static_cast<std::int64_t>(call_after);
    if (delta < std::numeric_limits<std::int32_t>::min() ||
        delta > std::numeric_limits<std::int32_t>::max()) return false;
    displacement = static_cast<std::int32_t>(delta);
    return true;
}

unsigned char* AllocateNear(unsigned char* target, std::size_t& allocation_size) noexcept {
    SYSTEM_INFO system{};
    GetSystemInfo(&system);
    const std::uintptr_t granularity = system.dwAllocationGranularity;
    if (granularity == 0) return nullptr;
    allocation_size = granularity;

    const auto address = reinterpret_cast<std::uintptr_t>(target);
    const auto aligned = address & ~(granularity - 1);
    const auto max_distance = std::min<std::uintptr_t>(kSearchDistance,
        static_cast<std::uintptr_t>(std::numeric_limits<std::int32_t>::max()) - granularity);

    for (std::uintptr_t distance = 0; distance <= max_distance; distance += granularity) {
        const std::array<std::uintptr_t, 2> candidates{
            aligned <= std::numeric_limits<std::uintptr_t>::max() - distance
                ? aligned + distance : 0,
            aligned >= distance ? aligned - distance : 0};
        for (const auto candidate : candidates) {
            if (!candidate) continue;
            std::int32_t ignored = 0;
            if (!FitsRel32(address + kCallSize, candidate, ignored)) continue;
            auto* memory = static_cast<unsigned char*>(VirtualAlloc(
                reinterpret_cast<void*>(candidate), allocation_size,
                MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
            if (memory) return memory;
        }
        if (max_distance - distance < granularity) break;
    }
    return nullptr;
}

void FreePrepared(std::vector<PreparedPatch>& prepared) noexcept {
    for (auto& patch : prepared) {
        if (patch.stub) VirtualFree(patch.stub, 0, MEM_RELEASE);
        patch.stub = nullptr;
    }
}

bool Restore(const InstalledPatch& patch) noexcept {
    DWORD previous = 0;
    if (!VirtualProtect(patch.target, kCallSize, PAGE_EXECUTE_READWRITE, &previous))
        return false;
    bool restored = false;
    if (std::memcmp(patch.target, patch.patched.data(), kCallSize) == 0) {
        std::memcpy(patch.target, patch.original.data(), kCallSize);
        FlushInstructionCache(GetCurrentProcess(), patch.target, kCallSize);
        restored = true;
    } else if (std::memcmp(patch.target, patch.original.data(), kCallSize) == 0) {
        restored = true;
    }
    DWORD ignored = 0;
    VirtualProtect(patch.target, kCallSize, previous, &ignored);
    return restored;
}

void Rollback() noexcept {
    for (auto it = g_installed.rbegin(); it != g_installed.rend(); ++it) {
        if (Restore(*it)) VirtualFree(it->stub, 0, MEM_RELEASE);
        else OutputDebugStringW(L"nosfix: retained call stub after rollback could not restore bytes\n");
    }
    g_installed.clear();
}

bool ValidTargetBytes(const unsigned char* target, std::size_t length) noexcept {
    MEMORY_BASIC_INFORMATION memory{};
    if (!VirtualQuery(target, &memory, sizeof(memory)) || memory.State != MEM_COMMIT)
        return false;
    const auto begin = reinterpret_cast<std::uintptr_t>(memory.BaseAddress);
    const auto end = begin + memory.RegionSize;
    const auto at = reinterpret_cast<std::uintptr_t>(target);
    return at >= begin && at <= end && length <= end - at;
}

} // namespace

bool InstallCallPatches(HMODULE module, std::span<const CallPatchSpec> specs,
                        std::string& reason) {
    std::lock_guard lock(g_mutex);
    if (!module) { reason = "call-patch target module is null"; return false; }
    if (specs.empty()) return true;
    if (!g_installed.empty()) {
        reason = "call-patch set is already installed";
        return false;
    }

    std::vector<PreparedPatch> prepared;
    prepared.reserve(specs.size());
    g_installed.reserve(specs.size());
    for (const auto& spec : specs) {
        const auto* base = reinterpret_cast<const unsigned char*>(spec.module ? spec.module : module);
        const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
        if (dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew <= 0) {
            reason = "invalid call-patch target DOS header";
            return false;
        }
        const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS64*>(base + dos->e_lfanew);
        if (nt->Signature != IMAGE_NT_SIGNATURE ||
            nt->FileHeader.Machine != IMAGE_FILE_MACHINE_AMD64 ||
            nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
            reason = "call-patch target is not a valid x64 PE image";
            return false;
        }

        if (!spec.name || !spec.destination || spec.expected_call.size() != kCallSize ||
            spec.expected_call.front() != 0xE8) {
            reason = "call-patch specification is incomplete";
            return false;
        }
        if (spec.rva > nt->OptionalHeader.SizeOfImage ||
            kCallSize > nt->OptionalHeader.SizeOfImage - spec.rva) {
            reason = std::string(spec.name) + ": call RVA is outside target image";
            return false;
        }
        auto* target = const_cast<unsigned char*>(base + spec.rva);
        if (!ValidTargetBytes(target, kCallSize)) {
            reason = std::string(spec.name) + ": call instruction is not readable";
            return false;
        }
        if (std::memcmp(target, spec.expected_call.data(), kCallSize) != 0) {
            reason = std::string(spec.name) + ": call bytes do not match expected build";
            return false;
        }
        for (const auto& prior : prepared) {
            if (prior.target == target) {
                reason = std::string(spec.name) + ": duplicate call target";
                return false;
            }
        }
        prepared.push_back({&spec, target, nullptr, 0, {}});
    }

    for (auto& patch : prepared) {
        patch.stub = AllocateNear(patch.target, patch.allocation_size);
        if (!patch.stub) {
            reason = std::string(patch.spec->name) + ": could not allocate a nearby call stub";
            FreePrepared(prepared);
            return false;
        }
        std::array<unsigned char, kStubSize> stub_bytes{
            0xFF,0x25,0x00,0x00,0x00,0x00,0,0,0,0,0,0,0,0};
        const auto destination = reinterpret_cast<std::uintptr_t>(patch.spec->destination);
        std::memcpy(stub_bytes.data() + 6, &destination, sizeof(destination));
        std::memcpy(patch.stub, stub_bytes.data(), stub_bytes.size());
        FlushInstructionCache(GetCurrentProcess(), patch.stub, stub_bytes.size());
        DWORD old = 0;
        if (!VirtualProtect(patch.stub, patch.allocation_size, PAGE_EXECUTE_READ, &old)) {
            reason = std::string(patch.spec->name) + ": cannot make call stub executable";
            FreePrepared(prepared);
            return false;
        }

        std::int32_t displacement = 0;
        const auto target_after_call = reinterpret_cast<std::uintptr_t>(patch.target) + kCallSize;
        if (!FitsRel32(target_after_call, reinterpret_cast<std::uintptr_t>(patch.stub),
                       displacement)) {
            reason = std::string(patch.spec->name) + ": nearby stub is outside rel32 range";
            FreePrepared(prepared);
            return false;
        }
        patch.patched[0] = 0xE8;
        std::memcpy(patch.patched.data() + 1, &displacement, sizeof(displacement));
    }

    for (auto& patch : prepared) {
        DWORD old = 0;
        if (!VirtualProtect(patch.target, kCallSize, PAGE_EXECUTE_READWRITE, &old)) {
            reason = std::string(patch.spec->name) + ": cannot make call site writable";
            FreePrepared(prepared);
            Rollback();
            return false;
        }
        InstalledPatch installed{patch.target, patch.stub, patch.allocation_size};
        std::copy(patch.spec->expected_call.begin(), patch.spec->expected_call.end(),
                  installed.original.begin());
        installed.patched = patch.patched;
        std::memcpy(patch.target, patch.patched.data(), kCallSize);
        FlushInstructionCache(GetCurrentProcess(), patch.target, kCallSize);
        g_installed.push_back(installed);
        patch.stub = nullptr; // Ownership moved to the installed set.

        DWORD ignored = 0;
        if (!VirtualProtect(installed.target, kCallSize, old, &ignored)) {
            reason = std::string(patch.spec->name) + ": cannot restore call-site protection";
            FreePrepared(prepared);
            Rollback();
            return false;
        }
    }
    return true;
}

void RemoveCallPatches() noexcept {
    std::lock_guard lock(g_mutex);
    Rollback();
}

} // namespace nosfix
