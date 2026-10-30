#include "nosfix/native_timeline_patch.hpp"

#include "nosfix/native_timeline_patch_data.hpp"

#include <Windows.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <limits>
#include <span>
#include <vector>

namespace nosfix {
namespace {

constexpr std::uint32_t kPrerollUs = 0xFFCD232Bu; // -3,333,333
constexpr std::uint32_t kThreshold80Rva = 0x1963D5;
constexpr std::uint32_t kThreshold80TargetRva = 0x1963F0;
constexpr std::uint32_t kThreshold80ReturnRva = 0x1963DB;
constexpr std::uint32_t kThresholdMinus1Rva = 0x1A5928;
constexpr std::uint32_t kThresholdMinus1TargetRva = 0x1A593D;
constexpr std::uint32_t kThresholdMinus1ReturnRva = 0x1A592D;
constexpr std::uint32_t kThreshold3000Rva = 0x21AD42;
constexpr std::array<std::pair<std::uint32_t, std::int32_t>, 8> kJudgmentWindows{{
    {0x13668E0u, 150'000},  // 9 frames
    {0x13668E8u, 116'667},  // 7 frames
    {0x13668F0u, 66'667},   // 4 frames
    {0x13668F8u, 33'333},   // 2 frames
    {0x13668FCu, -33'333},  // -2 frames
    {0x13668F4u, -66'667},  // -4 frames
    {0x13668ECu, -100'000}, // -6 frames
    {0x13668E4u, -183'333}, // -11 frames
}};

struct Patch final {
    unsigned char* target = nullptr;
    std::size_t length = 0;
    std::vector<unsigned char> original;
    std::vector<unsigned char> patched;
    bool code = true;
};

HMODULE g_module = nullptr;
unsigned char* g_constants = nullptr;
std::size_t g_constant_size = 0;
std::vector<Patch> g_patches;
std::size_t g_applied = 0;
bool g_prepared = false;
bool g_committed = false;

bool Readable(const void* address, std::size_t length) noexcept {
    if (!address || length == 0) return false;
    MEMORY_BASIC_INFORMATION info{};
    if (VirtualQuery(address, &info, sizeof(info)) == 0) return false;
    if (info.State != MEM_COMMIT) return false;
    if ((info.Protect & (PAGE_NOACCESS | PAGE_GUARD)) != 0) return false;
    const auto start = reinterpret_cast<std::uintptr_t>(address);
    const auto end = start + length;
    const auto region_end = reinterpret_cast<std::uintptr_t>(info.BaseAddress) +
                            info.RegionSize;
    return end >= start && end <= region_end;
}

bool FitsDisp32(const unsigned char* instruction, std::size_t instruction_length,
                const void* destination) noexcept {
    const auto after = reinterpret_cast<std::uintptr_t>(instruction) + instruction_length;
    const auto delta = static_cast<std::int64_t>(
        reinterpret_cast<std::uintptr_t>(destination)) -
        static_cast<std::int64_t>(after);
    return delta >= std::numeric_limits<std::int32_t>::min() &&
           delta <= std::numeric_limits<std::int32_t>::max();
}

void StoreDisp32(unsigned char* instruction, std::size_t instruction_length,
                 const void* destination) noexcept {
    const auto after = reinterpret_cast<std::uintptr_t>(instruction) + instruction_length;
    const auto delta = static_cast<std::int32_t>(
        reinterpret_cast<std::uintptr_t>(destination) - after);
    std::memcpy(instruction + instruction_length - 4, &delta, sizeof(delta));
}

unsigned char* FindConstantPage(HMODULE module) noexcept {
    SYSTEM_INFO system{};
    GetSystemInfo(&system);
    const auto granularity = system.dwAllocationGranularity;
    if (granularity == 0) return nullptr;
    const auto base = reinterpret_cast<std::uintptr_t>(module);
    const auto aligned = base & ~(static_cast<std::uintptr_t>(granularity) - 1);
    constexpr std::uintptr_t kMaxDistance = 0x7FFF0000;
    constexpr std::size_t kPageSize = 0x1000;

    for (std::uintptr_t distance = 0; distance <= kMaxDistance;
         distance += granularity) {
        const std::array<std::uintptr_t, 2> candidates{
            aligned <= std::numeric_limits<std::uintptr_t>::max() - distance
                ? aligned + distance : 0,
            aligned >= distance ? aligned - distance : 0};
        for (const auto candidate : candidates) {
            if (!candidate) continue;
            auto* memory = static_cast<unsigned char*>(VirtualAlloc(
                reinterpret_cast<void*>(candidate), kPageSize,
                MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE));
            if (!memory) continue;
            return memory;
        }
        if (kMaxDistance - distance < granularity) break;
    }
    return nullptr;
}

bool AddPatch(unsigned char* target, std::span<const unsigned char> expected,
              std::span<const unsigned char> replacement, bool code,
              std::string& reason) noexcept {
    if (!target || expected.empty() || expected.size() != replacement.size() ||
        !Readable(target, expected.size())) {
        reason = "timeline patch target is not readable or has a bad length";
        return false;
    }
    if (std::memcmp(target, expected.data(), expected.size()) != 0) {
        reason = "timeline patch bytes do not match the analyzed build";
        return false;
    }
    for (const auto& prior : g_patches) {
        const auto a = reinterpret_cast<std::uintptr_t>(prior.target);
        const auto b = a + prior.length;
        const auto c = reinterpret_cast<std::uintptr_t>(target);
        const auto d = c + expected.size();
        if (c < b && a < d) {
            reason = "timeline patch targets overlap";
            return false;
        }
    }
    Patch patch;
    patch.target = target;
    patch.length = expected.size();
    patch.original.assign(expected.begin(), expected.end());
    patch.patched.assign(replacement.begin(), replacement.end());
    patch.code = code;
    g_patches.push_back(std::move(patch));
    return true;
}

bool WritePatch(const Patch& patch) noexcept {
    DWORD old = 0;
    if (!VirtualProtect(patch.target, patch.length,
                        patch.code ? PAGE_EXECUTE_READWRITE : PAGE_READWRITE,
                        &old)) {
        return false;
    }
    std::memcpy(patch.target, patch.patched.data(), patch.length);
    if (patch.code) {
        FlushInstructionCache(GetCurrentProcess(), patch.target, patch.length);
    }
    DWORD ignored = 0;
    return VirtualProtect(patch.target, patch.length, old, &ignored) != FALSE;
}

bool RestorePatch(const Patch& patch) noexcept {
    DWORD old = 0;
    if (!VirtualProtect(patch.target, patch.length,
                        patch.code ? PAGE_EXECUTE_READWRITE : PAGE_READWRITE,
                        &old)) {
        return false;
    }
    std::memcpy(patch.target, patch.original.data(), patch.length);
    if (patch.code) {
        FlushInstructionCache(GetCurrentProcess(), patch.target, patch.length);
    }
    DWORD ignored = 0;
    return VirtualProtect(patch.target, patch.length, old, &ignored) != FALSE;
}

void ClearPrepared() noexcept {
    g_patches.clear();
    if (g_constants) {
        VirtualFree(g_constants, 0, MEM_RELEASE);
        g_constants = nullptr;
        g_constant_size = 0;
    }
    g_prepared = false;
    g_committed = false;
    g_applied = 0;
    g_module = nullptr;
}

} // namespace

bool PrepareTimelineConversionPatches(HMODULE module, std::string& reason) {
    if (g_prepared || g_committed) {
        reason = "timeline conversion patches are already prepared";
        return false;
    }
    if (!module) {
        reason = "timeline conversion module is null";
        return false;
    }

    const auto* base = reinterpret_cast<const unsigned char*>(module);
    const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew <= 0) {
        reason = "timeline conversion module has an invalid DOS header";
        return false;
    }
    const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS64*>(base + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE ||
        nt->FileHeader.Machine != IMAGE_FILE_MACHINE_AMD64 ||
        nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
        reason = "timeline conversion module is not x64 PE";
        return false;
    }
    const auto image_size = nt->OptionalHeader.SizeOfImage;

    g_module = module;
    g_constants = FindConstantPage(module);
    if (!g_constants) {
        ClearPrepared();
        reason = "cannot allocate a constants page within +/-2 GB of the game";
        return false;
    }
    g_constant_size = 0x1000;
    const float us_per_minute = 60'000'000.0f;
    const double us_per_ms = 1'000'000.0;
    std::memcpy(g_constants, &us_per_minute, sizeof(us_per_minute));
    std::memcpy(g_constants + 8, &us_per_ms, sizeof(us_per_ms));

    // Relocated -80 frame threshold for sub_180196390. The original compare
    // uses an 8-bit immediate, so a nearby six-byte jump enters this stub:
    //   cmp r8d, -1,333,333
    //   jl  original target
    //   jmp back to the instruction after the original compare
    {
        auto* stub = g_constants + 0x100;
        stub[0] = 0x41;
        stub[1] = 0x81;
        stub[2] = 0xF8;
        const std::uint32_t threshold = 0xFFEBA7ABu; // -1,333,333
        std::memcpy(stub + 3, &threshold, sizeof(threshold));
        stub[7] = 0x0F;
        stub[8] = 0x8C;
        const auto jl_after = reinterpret_cast<std::uintptr_t>(stub + 13);
        const auto jl_target = reinterpret_cast<std::uintptr_t>(
            base + kThreshold80TargetRva);
        const auto jl_delta = static_cast<std::int32_t>(jl_target - jl_after);
        std::memcpy(stub + 9, &jl_delta, sizeof(jl_delta));
        stub[13] = 0xE9;
        const auto jmp_after = reinterpret_cast<std::uintptr_t>(stub + 18);
        const auto jmp_target = reinterpret_cast<std::uintptr_t>(
            base + kThreshold80ReturnRva);
        const auto jmp_delta = static_cast<std::int32_t>(jmp_target - jmp_after);
        std::memcpy(stub + 14, &jmp_delta, sizeof(jmp_delta));
        FlushInstructionCache(GetCurrentProcess(), stub, 18);
    }

    // Relocated one-frame late threshold for sub_1801A58C0. The original
    // compare/jge pair is five bytes; jump through this stub:
    //   cmp eax, -16,667
    //   jge original target
    //   jmp back
    {
        auto* stub = g_constants + 0x140;
        stub[0] = 0x3D;
        const std::uint32_t threshold = 0xFFFFBEE5u; // -16,667
        std::memcpy(stub + 1, &threshold, sizeof(threshold));
        stub[5] = 0x0F;
        stub[6] = 0x8D;
        const auto jge_after = reinterpret_cast<std::uintptr_t>(stub + 11);
        const auto jge_target = reinterpret_cast<std::uintptr_t>(
            base + kThresholdMinus1TargetRva);
        const auto jge_delta = static_cast<std::int32_t>(jge_target - jge_after);
        std::memcpy(stub + 7, &jge_delta, sizeof(jge_delta));
        stub[11] = 0xE9;
        const auto jmp_after = reinterpret_cast<std::uintptr_t>(stub + 16);
        const auto jmp_target = reinterpret_cast<std::uintptr_t>(
            base + kThresholdMinus1ReturnRva);
        const auto jmp_delta = static_cast<std::int32_t>(jmp_target - jmp_after);
        std::memcpy(stub + 12, &jmp_delta, sizeof(jmp_delta));
        FlushInstructionCache(GetCurrentProcess(), stub, 16);
    }

    DWORD old = 0;
    if (!VirtualProtect(g_constants, g_constant_size, PAGE_EXECUTE_READ, &old)) {
        ClearPrepared();
        reason = "cannot protect the constants/stub page";
        return false;
    }

    g_patches.reserve(kFloatSquarePatches.size() * 2 +
                      kDoubleRatePatches.size() +
                      kCounterResetPatches.size() + kJudgmentWindows.size());

    for (const auto& spec : kFloatSquarePatches) {
        if (spec.load_rva > image_size || spec.load_length > image_size - spec.load_rva ||
            spec.mul_rva > image_size || spec.mul_length > image_size - spec.mul_rva) {
            ClearPrepared();
            reason = "float rate patch RVA is outside the image";
            return false;
        }
        auto* load = const_cast<unsigned char*>(base + spec.load_rva);
        auto* mul = const_cast<unsigned char*>(base + spec.mul_rva);
        if (!FitsDisp32(load, spec.load_length, g_constants)) {
            ClearPrepared();
            reason = "constants page is outside rel32 range of a float rate site";
            return false;
        }
        std::array<unsigned char, 9> load_patch{};
        std::memcpy(load_patch.data(), spec.load_bytes.data(), spec.load_length);
        StoreDisp32(load_patch.data(), spec.load_length, g_constants);
        std::array<unsigned char, 5> mul_patch{};
        if (spec.mul_length == 5) {
            mul_patch = {0x0F, 0x1F, 0x44, 0x00, 0x00};
        } else {
            mul_patch = {0x0F, 0x1F, 0x40, 0x00, 0x00};
        }
        if (!AddPatch(load, std::span(spec.load_bytes).first(spec.load_length),
                      std::span(load_patch).first(spec.load_length), true, reason) ||
            !AddPatch(mul, std::span(spec.mul_bytes).first(spec.mul_length),
                      std::span(mul_patch).first(spec.mul_length), true, reason)) {
            ClearPrepared();
            return false;
        }
    }

    for (const auto& spec : kDoubleRatePatches) {
        if (spec.rva > image_size || spec.bytes.size() > image_size - spec.rva) {
            ClearPrepared();
            reason = "double rate patch RVA is outside the image";
            return false;
        }
        auto* target = const_cast<unsigned char*>(base + spec.rva);
        if (!FitsDisp32(target, spec.bytes.size(), g_constants + 8)) {
            ClearPrepared();
            reason = "constants page is outside rel32 range of a double rate site";
            return false;
        }
        auto patched = spec.bytes;
        StoreDisp32(patched.data(), patched.size(), g_constants + 8);
        if (!AddPatch(target, spec.bytes, patched, true, reason)) {
            ClearPrepared();
            return false;
        }
    }

    for (const auto& spec : kCounterResetPatches) {
        if (spec.rva > image_size || spec.length > image_size - spec.rva) {
            ClearPrepared();
            reason = "counter reset patch RVA is outside the image";
            return false;
        }
        auto* target = const_cast<unsigned char*>(base + spec.rva);
        const auto expected = std::span(spec.bytes).first(spec.length);
        auto patched = spec.bytes;
        const std::size_t immediate = spec.length == 6 ? 2 : 6;
        if (spec.length != 6 && spec.length != 10) {
            ClearPrepared();
            reason = "unexpected counter reset instruction length";
            return false;
        }
        std::memcpy(patched.data() + immediate, &kPrerollUs, sizeof(kPrerollUs));
        if (!AddPatch(target, expected, std::span(patched).first(spec.length),
                      true, reason)) {
            ClearPrepared();
            return false;
        }
    }

    // sub_1802FDB30 compares the combined song time with -160 frames before
    // starting the stage sound. At -160 frames = -2,666,667 us.
    {
        constexpr std::uint32_t kThresholdRva = 0x2FDBB0;
        if (kThresholdRva > image_size || 6 > image_size - kThresholdRva) {
            ClearPrepared();
            reason = "stage-start threshold RVA is outside the image";
            return false;
        }
        constexpr std::array<unsigned char, 6> expected{
            0x81, 0xFA, 0x60, 0xFF, 0xFF, 0xFF};
        constexpr std::array<unsigned char, 6> patched{
            0x81, 0xFA, 0x55, 0x4F, 0xD7, 0xFF};
        if (!AddPatch(const_cast<unsigned char*>(base + kThresholdRva),
                      expected, patched, true, reason)) {
            ClearPrepared();
            return false;
        }
    }

    // sub_180196390 compares the combined time with -80 frames; the original
    // comparison uses an 8-bit immediate, so jump through the stub above.
    {
        if (kThreshold80Rva > image_size || 6 > image_size - kThreshold80Rva) {
            ClearPrepared();
            reason = "-80 threshold RVA is outside the image";
            return false;
        }
        auto* site = const_cast<unsigned char*>(base + kThreshold80Rva);
        if (!FitsDisp32(site, 5, g_constants + 0x100)) {
            ClearPrepared();
            reason = "-80 threshold stub is outside rel32 range";
            return false;
        }
        constexpr std::array<unsigned char, 6> expected{
            0x41, 0x83, 0xF8, 0xB0, 0x7C, 0x15};
        std::array<unsigned char, 6> patched{0xE9, 0, 0, 0, 0, 0x90};
        const auto after = reinterpret_cast<std::uintptr_t>(site) + 5;
        const auto delta = static_cast<std::int32_t>(
            reinterpret_cast<std::uintptr_t>(g_constants + 0x100) - after);
        std::memcpy(patched.data() + 1, &delta, sizeof(delta));
        if (!AddPatch(site, expected, patched, true, reason)) {
            ClearPrepared();
            return false;
        }
    }

    // sub_1801A58C0 one-frame late marker.
    {
        if (kThresholdMinus1Rva > image_size ||
            5 > image_size - kThresholdMinus1Rva) {
            ClearPrepared();
            reason = "one-frame threshold RVA is outside the image";
            return false;
        }
        auto* site = const_cast<unsigned char*>(base + kThresholdMinus1Rva);
        if (!FitsDisp32(site, 5, g_constants + 0x140)) {
            ClearPrepared();
            reason = "one-frame threshold stub is outside rel32 range";
            return false;
        }
        constexpr std::array<unsigned char, 5> expected{
            0x83, 0xF8, 0xFF, 0x7D, 0x10};
        std::array<unsigned char, 5> patched{0xE9, 0, 0, 0, 0};
        const auto after = reinterpret_cast<std::uintptr_t>(site) + 5;
        const auto delta = static_cast<std::int32_t>(
            reinterpret_cast<std::uintptr_t>(g_constants + 0x140) - after);
        std::memcpy(patched.data() + 1, &delta, sizeof(delta));
        if (!AddPatch(site, expected, patched, true, reason)) {
            ClearPrepared();
            return false;
        }
    }

    // sub_18021ACF0 uses a 3000-frame (50 s) scene/tutorial limit.
    {
        if (kThreshold3000Rva > image_size || 6 > image_size - kThreshold3000Rva) {
            ClearPrepared();
            reason = "3000-frame threshold RVA is outside the image";
            return false;
        }
        constexpr std::array<unsigned char, 6> expected{
            0x81, 0xFA, 0xB8, 0x0B, 0x00, 0x00};
        constexpr std::array<unsigned char, 6> patched{
            0x81, 0xFA, 0x80, 0xF0, 0xFA, 0x02};
        if (!AddPatch(const_cast<unsigned char*>(base + kThreshold3000Rva),
                      expected, patched, true, reason)) {
            ClearPrepared();
            return false;
        }
    }

    for (const auto& [rva, value] : kJudgmentWindows) {
        if (rva > image_size || sizeof(value) > image_size - rva) {
            ClearPrepared();
            reason = "judgment window RVA is outside the image";
            return false;
        }
        auto* target = const_cast<unsigned char*>(base + rva);
        std::array<unsigned char, 4> expected{};
        std::memcpy(expected.data(), target, expected.size());
        std::array<unsigned char, 4> patched{};
        std::memcpy(patched.data(), &value, sizeof(value));
        if (!AddPatch(target, expected, patched, false, reason)) {
            ClearPrepared();
            return false;
        }
    }

    g_prepared = true;
    return true;
}

bool CommitTimelineConversionPatches(std::string& reason) {
    if (!g_prepared || g_committed) {
        reason = "timeline conversion patches are not prepared";
        return false;
    }
    g_committed = true;
    for (const auto& patch : g_patches) {
        if (!WritePatch(patch)) {
            reason = "cannot write a timeline conversion patch";
            RollbackTimelineConversionPatches();
            return false;
        }
        ++g_applied;
    }
    return true;
}

void RollbackTimelineConversionPatches() noexcept {
    if (g_committed) {
        for (std::size_t i = g_applied; i > 0; --i) {
            if (!RestorePatch(g_patches[i - 1])) {
                OutputDebugStringW(L"nosfix: could not restore a timeline conversion patch\n");
            }
        }
    }
    ClearPrepared();
}

} // namespace nosfix
