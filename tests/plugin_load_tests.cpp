#include <Windows.h>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {
[[noreturn]] void Fail(const char* message) {
    std::fprintf(stderr, "%s (GetLastError=%lu)\n", message, GetLastError());
    std::exit(1);
}
using Init = int(__cdecl*)(std::uint32_t, void(__cdecl*)(), void*);
using Entry = int(__cdecl*)(Init);
constexpr std::array<DWORD, 12> kPatchRvas{
    0x1C31C0, 0x1C3883, 0x1C390C, 0x2F6DF0, 0x2F4E80, 0xDE7C0, 0xD61C0,
    0x1A5990, 0x1A6820, 0x1A84F0, 0x1AA4D0, 0x218CF0};
}

int wmain(int argc, wchar_t** argv) {
    if (argc < 2 || argc > 5) Fail("usage: plugin_load_tests nosfix.dll [nostalgia.dll [afp-core.dll [libxact.dll]]]");
    HMODULE game = nullptr;
    HMODULE afp = nullptr;
    HMODULE xact = nullptr;
    std::array<unsigned char, 44> xact_original{};
    std::array<unsigned char, 5> afp_original{};
    std::array<std::array<unsigned char, 5>, kPatchRvas.size()> original{};
    void* original_import = nullptr;
    if (argc >= 3) {
        // No DllMain, game code, dependencies, audio or display operations run.
        game = LoadLibraryExW(argv[2], nullptr, DONT_RESOLVE_DLL_REFERENCES);
        if (!game) Fail("cannot map inert game image");
        auto* base = reinterpret_cast<unsigned char*>(game);
        for (size_t i = 0; i < kPatchRvas.size(); ++i)
            std::memcpy(original[i].data(), base + kPatchRvas[i], 5);
        original_import = *reinterpret_cast<void**>(base + 0x3A8C70);
    }
    if (argc >= 4) {
        afp = LoadLibraryExW(argv[3], nullptr, DONT_RESOLVE_DLL_REFERENCES);
        if (!afp) Fail("cannot map inert AFP image");
        std::memcpy(afp_original.data(), reinterpret_cast<unsigned char*>(afp) + 0xB6DFB, 5);
    }
    if (argc == 5) {
        xact = LoadLibraryExW(argv[4], nullptr, DONT_RESOLVE_DLL_REFERENCES);
        if (!xact) Fail("cannot map inert libxact image");
        std::memcpy(xact_original.data(), reinterpret_cast<unsigned char*>(xact) + 0x410A, 44);
    }
    HMODULE plugin = LoadLibraryW(argv[1]);
    if (!plugin) Fail("DLL dependencies or loader initialization failed");
    const auto entry = reinterpret_cast<Entry>(GetProcAddress(plugin, "spice_sdk_entry_point"));
    if (!entry) Fail("Spice SDK entry export missing");
    const int result = entry(nullptr); // No SDK helpers are used by this plugin.
    if (!game) {
        if (result == 0) Fail("plugin accepted a process without Nostalgia");
        FreeLibrary(plugin);
        std::puts("Plugin loader/export and missing-game rejection passed.");
        return 0;
    }
    auto* base = reinterpret_cast<unsigned char*>(game);
    if (!afp || !xact) {
        if (result == 0) Fail("plugin accepted a missing dependency");
        for (size_t i = 0; i < kPatchRvas.size(); ++i)
            if (std::memcmp(original[i].data(), base + kPatchRvas[i], 5))
                Fail("dependency guard failure modified game code");
        if (original_import != *reinterpret_cast<void**>(base + 0x3A8C70))
            Fail("dependency guard failure changed D3D import");
        if (afp && std::memcmp(afp_original.data(), reinterpret_cast<unsigned char*>(afp) + 0xB6DFB, 5))
            Fail("libxact guard failure modified AFP code");
        std::puts("Missing dependency rejected with game code and imports unchanged.");
        return 0;
    }
    if (result != 0) Fail("plugin rejected the analyzed inert images");
    if (!std::memcmp(xact_original.data(), reinterpret_cast<unsigned char*>(xact) + 0x410A, 44))
        Fail("plugin failed to correct libxact clock arithmetic");
    if (!std::memcmp(afp_original.data(), reinterpret_cast<unsigned char*>(afp) + 0xB6DFB, 5))
        Fail("plugin failed to install the AFP callsite hook");
    for (size_t i = 0; i < kPatchRvas.size(); ++i)
        if (!std::memcmp(original[i].data(), base + kPatchRvas[i], 5))
            Fail("plugin failed to install an expected code hook");
    if (original_import == *reinterpret_cast<void**>(base + 0x3A8C70))
        Fail("plugin failed to install its D3D factory import");
    // The plugin intentionally pins itself; this disposable process owns both
    // inert mappings until exit. None of their patched instructions are called.
    std::puts("Actual DLL SDK initialization and complete inert-image installation passed.");
}
