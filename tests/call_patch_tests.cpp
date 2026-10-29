#include "nosfix/call_patch.hpp"
#include "nosfix/module_guard.hpp"

#include <Windows.h>

#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

namespace {

[[noreturn]] void Fail(const char* message) {
    std::fprintf(stderr, "%s\n", message);
    std::exit(1);
}

unsigned g_stub_calls = 0;
void __fastcall TestCallTarget(void*) { ++g_stub_calls; }

constexpr std::array<unsigned char, 5> kHasEntryCall{
    0xE8,0x48,0x24,0x19,0x00};
constexpr std::array<unsigned char, 5> kDispatchCall{
    0xE8,0x5F,0x18,0x19,0x00};
constexpr std::array<unsigned char, 5> kWrongCall{
    0xE8,0x00,0x00,0x00,0x00};

} // namespace

int wmain(int argc, wchar_t** argv) {
    if (argc != 3) Fail("usage: call_patch_tests nostalgia.dll afp-core.dll");
    HMODULE module = LoadLibraryExW(argv[1], nullptr, DONT_RESOLVE_DLL_REFERENCES);
    if (!module) Fail("could not map analyzed DLL without initialization");
    HMODULE afp = LoadLibraryExW(argv[2], nullptr, DONT_RESOLVE_DLL_REFERENCES);
    if (!afp) Fail("could not map analyzed AFP DLL without initialization");

    nosfix::ModuleIdentity identity;
    std::wstring reason;
    if (!nosfix::VerifyNostalgiaModule(identity, reason) || identity.module != module)
        Fail("version guard rejected or misidentified analyzed DLL");
    if (!nosfix::VerifyAfpModule(identity, reason) || identity.module != afp)
        Fail("version guard rejected or misidentified analyzed AFP DLL");
    constexpr std::array<unsigned char, 5> afp_call{0xE8,0x80,0xC2,0xF4,0xFF};
    auto* afp_base = reinterpret_cast<unsigned char*>(afp);
    if (std::memcmp(afp_base + 0xB6DFB, afp_call.data(), 5))
        Fail("verified AFP call bytes differ");

    auto* base = reinterpret_cast<unsigned char*>(module);
    std::array<unsigned char, 5> original_hasentry{};
    std::array<unsigned char, 5> original_dispatch{};
    std::memcpy(original_hasentry.data(), base + 0x1C3883, original_hasentry.size());
    std::memcpy(original_dispatch.data(), base + 0x1C390C, original_dispatch.size());
    if (original_hasentry != kHasEntryCall || original_dispatch != kDispatchCall)
        Fail("verified render-tail call bytes differ from expected build");

    const std::array<nosfix::CallPatchSpec, 3> bad_specs{{
        {"hasentry", 0x1C3883, kHasEntryCall,
         reinterpret_cast<void*>(&TestCallTarget)},
        {"dispatch", 0x1C390C, kDispatchCall,
         reinterpret_cast<void*>(&TestCallTarget)},
        {"AFP", 0xB6DFB, kWrongCall, reinterpret_cast<void*>(&TestCallTarget), afp},
    }};
    std::string error;
    if (nosfix::InstallCallPatches(module, bad_specs, error))
        Fail("mismatched call patch set was accepted");
    if (std::memcmp(base + 0x1C3883, original_hasentry.data(), 5) ||
        std::memcmp(base + 0x1C390C, original_dispatch.data(), 5))
        Fail("validation failure modified a call instruction");

    const std::array<nosfix::CallPatchSpec, 3> specs{{
        {"hasentry", 0x1C3883, kHasEntryCall,
         reinterpret_cast<void*>(&TestCallTarget)},
        {"dispatch", 0x1C390C, kDispatchCall,
         reinterpret_cast<void*>(&TestCallTarget)},
        {"AFP", 0xB6DFB, afp_call, reinterpret_cast<void*>(&TestCallTarget), afp},
    }};
    if (!nosfix::InstallCallPatches(module, specs, error)) {
        std::fprintf(stderr, "%s\n", error.c_str());
        Fail("could not install near call stubs");
    }
    if (base[0x1C3883] != 0xE8 || base[0x1C390C] != 0xE8)
        Fail("call-site redirection was not written");

    for (const auto& spec : specs) {
        const auto* target_base = spec.module ? reinterpret_cast<unsigned char*>(spec.module) : base;
        std::int32_t displacement = 0;
        std::memcpy(&displacement, target_base + spec.rva + 1, sizeof(displacement));
        const auto* stub = target_base + spec.rva + 5 + displacement;
        constexpr std::array<unsigned char, 6> jump{0xFF, 0x25, 0, 0, 0, 0};
        if (std::memcmp(stub, jump.data(), jump.size())) Fail("invalid absolute-jump stub");
        void* destination = nullptr;
        std::memcpy(&destination, stub + 6, sizeof(destination));
        if (destination != spec.destination) Fail("stub points to the wrong function");
        MEMORY_BASIC_INFORMATION memory{};
        if (!VirtualQuery(stub, &memory, sizeof(memory)) || memory.Protect != PAGE_EXECUTE_READ)
            Fail("call stub must be executable and non-writable");
        // Executes only our generated jump and this test's C++ function. No
        // instruction in the mapped game image is executed.
        reinterpret_cast<void(__fastcall*)(void*)>(stub)(nullptr);
    }
    if (g_stub_calls != specs.size()) Fail("generated stubs did not forward both calls");

    nosfix::RemoveCallPatches();
    if (std::memcmp(afp_base + 0xB6DFB, afp_call.data(), 5))
        Fail("call-patch rollback did not restore AFP call");
    if (std::memcmp(base + 0x1C3883, original_hasentry.data(), 5) ||
        std::memcmp(base + 0x1C390C, original_dispatch.data(), 5))
        Fail("call-patch rollback did not restore both original calls");
    FreeLibrary(module);
    FreeLibrary(afp);
    std::puts("Call-site validation, near stubs and rollback passed.");
}
