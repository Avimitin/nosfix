#include "nosfix/native_timeline_patch.hpp"

#include <Windows.h>

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {

[[noreturn]] void Fail(const char* text) {
    std::fprintf(stderr, "%s\n", text);
    std::exit(1);
}

template <class T>
T Read(const unsigned char* base, std::size_t rva) {
    T value{};
    std::memcpy(&value, base + rva, sizeof(T));
    return value;
}

} // namespace

int wmain(int argc, wchar_t** argv) {
    if (argc != 2) Fail("usage: timeline_patch_tests path-to-analyzed-nostalgia.dll");
    HMODULE module = LoadLibraryExW(argv[1], nullptr, DONT_RESOLVE_DLL_REFERENCES);
    if (!module) Fail("could not map analyzed DLL without initialization");
    auto* base = reinterpret_cast<const unsigned char*>(module);

    const std::uint32_t reset_immediate_before = Read<std::uint32_t>(base, 0x2FC55A + 6);
    const std::uint32_t init_immediate_before = Read<std::uint32_t>(base, 0x2FC8EC + 2);
    const std::uint32_t threshold_before = Read<std::uint32_t>(base, 0x2FDBB0 + 2);
    const std::uint32_t threshold3000_before = Read<std::uint32_t>(base, 0x21AD42 + 2);
    std::array<unsigned char, 6> threshold80_before{};
    std::memcpy(threshold80_before.data(), base + 0x1963D5, threshold80_before.size());
    std::array<unsigned char, 5> threshold1_before{};
    std::memcpy(threshold1_before.data(), base + 0x1A5928, threshold1_before.size());
    const std::int32_t window_before = Read<std::int32_t>(base, 0x13668E0);
    std::array<unsigned char, 8> float_load_before{};
    std::array<unsigned char, 8> double_load_before{};
    std::memcpy(float_load_before.data(), base + 0x197204, float_load_before.size());
    std::memcpy(double_load_before.data(), base + 0x1A4935, double_load_before.size());

    std::string reason;
    if (!nosfix::PrepareTimelineConversionPatches(module, reason)) {
        std::fprintf(stderr, "%s\n", reason.c_str());
        Fail("timeline conversion preparation failed");
    }
    if (Read<std::uint32_t>(base, 0x2FC55A + 6) != reset_immediate_before ||
        Read<std::int32_t>(base, 0x13668E0) != window_before ||
        std::memcmp(base + 0x197204, float_load_before.data(), float_load_before.size()) ||
        std::memcmp(base + 0x1A4935, double_load_before.data(), double_load_before.size())) {
        Fail("preparation modified the image");
    }

    if (!nosfix::CommitTimelineConversionPatches(reason)) {
        std::fprintf(stderr, "%s\n", reason.c_str());
        Fail("timeline conversion commit failed");
    }
    if (Read<std::uint32_t>(base, 0x2FC55A + 6) != 0xFFCD232Bu ||
        Read<std::uint32_t>(base, 0x2FC8EC + 2) != 0xFFCD232Bu ||
        Read<std::uint32_t>(base, 0x2FDBB0 + 2) != 0xFFD74F55u ||
        Read<std::uint32_t>(base, 0x21AD42 + 2) != 0x02FAF080u ||
        Read<std::int32_t>(base, 0x13668E0) != 150'000 ||
        base[0x1963D5] != 0xE9 || base[0x1963D5 + 5] != 0x90 ||
        base[0x1A5928] != 0xE9 ||
        Read<std::int32_t>(base, 0x13668E4) != -183'333 ||
        !std::memcmp(base + 0x197204, float_load_before.data(), float_load_before.size()) ||
        !std::memcmp(base + 0x1A4935, double_load_before.data(), double_load_before.size())) {
        Fail("committed image does not contain the expected conversion");
    }

    nosfix::RollbackTimelineConversionPatches();
    if (Read<std::uint32_t>(base, 0x2FC55A + 6) != reset_immediate_before ||
        Read<std::uint32_t>(base, 0x2FC8EC + 2) != init_immediate_before ||
        Read<std::uint32_t>(base, 0x2FDBB0 + 2) != threshold_before ||
        Read<std::uint32_t>(base, 0x21AD42 + 2) != threshold3000_before ||
        Read<std::int32_t>(base, 0x13668E0) != window_before ||
        std::memcmp(base + 0x1963D5, threshold80_before.data(), threshold80_before.size()) ||
        std::memcmp(base + 0x1A5928, threshold1_before.data(), threshold1_before.size()) ||
        std::memcmp(base + 0x197204, float_load_before.data(), float_load_before.size()) ||
        std::memcmp(base + 0x1A4935, double_load_before.data(), double_load_before.size())) {
        Fail("rollback did not restore the image");
    }

    FreeLibrary(module);
    std::puts("timeline conversion prepare/commit/rollback passed on the real image");
    return 0;
}
