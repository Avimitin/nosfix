#ifdef NDEBUG
#undef NDEBUG
#endif
#include "nosfix/audio_clock_patch.hpp"
#include "nosfix/log.hpp"
#include <Windows.h>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace nosfix { void Log(LogLevel, std::string_view) noexcept {} }

struct QpcInputs { std::int64_t now, start, frequency; };
extern "C" float RunQpcBlock(void* code, const QpcInputs* inputs);

float Run(const std::array<unsigned char, 44>& bytes, const QpcInputs& input) {
    auto* block = static_cast<unsigned char*>(VirtualAlloc(nullptr, 4096,
        MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
    assert(block);
    std::memcpy(block, bytes.data(), bytes.size());
    block[bytes.size()] = 0xC3; // return to our ABI harness
    DWORD old = 0;
    assert(VirtualProtect(block, 4096, PAGE_EXECUTE_READ, &old));
    assert(FlushInstructionCache(GetCurrentProcess(), block, bytes.size() + 1));
    const float result = RunQpcBlock(block, &input);
    assert(VirtualFree(block, 0, MEM_RELEASE));
    return result;
}

int wmain(int argc, wchar_t** argv) {
    using namespace nosfix;
    // These are the actual machine instructions installed in libxact, tested
    // against an independent arithmetic reference at short and long uptimes.
    for (const std::int64_t start : {0LL, 10'000'000'000'000LL,
                                     7'000'000'000'000'000'000LL}) {
        for (const std::int64_t delta : {1LL, 2'500LL, 83'333LL, 4'321'098'765LL}) {
            const QpcInputs inputs{start + delta, start, 10'000'000};
            const float expected = static_cast<float>(static_cast<double>(delta) *
                                                      1000.0 / inputs.frequency);
            assert(Run(kAudioClockPrecise, inputs) == expected);
        }
    }
    const QpcInputs tiny{10'000'000'002'500LL, 10'000'000'000'000LL, 10'000'000};
    assert(Run(kAudioClockPrecise, tiny) == 0.25f);
    assert(Run(kAudioClockOriginal, tiny) != 0.25f);

    assert(argc == 2);
    // Inert image only: no DllMain or imports run. Check guard, installation,
    // instruction protection, rollback, and conflict rejection on real bytes.
    const auto module = LoadLibraryExW(argv[1], nullptr, DONT_RESOLVE_DLL_REFERENCES);
    assert(module);
    auto* target = reinterpret_cast<unsigned char*>(module) + kAudioClockMathRva;
    std::string reason;
    assert(PrepareAudioClockPatch(reason));
    assert(CommitAudioClockPatch(reason));
    assert(std::memcmp(target, kAudioClockPrecise.data(), kAudioClockPrecise.size()) == 0);
    MEMORY_BASIC_INFORMATION memory{};
    assert(VirtualQuery(target, &memory, sizeof(memory)));
    assert(memory.Protect == PAGE_EXECUTE_READ);
    RollbackAudioClockPatch();
    assert(std::memcmp(target, kAudioClockOriginal.data(), kAudioClockOriginal.size()) == 0);
    assert(PrepareAudioClockPatch(reason));
    DWORD protection = 0;
    assert(VirtualProtect(target, 1, PAGE_EXECUTE_READWRITE, &protection));
    target[0] ^= 1;
    assert(!CommitAudioClockPatch(reason));
    RollbackAudioClockPatch();
    assert(target[0] == (kAudioClockOriginal[0] ^ 1)); // do not overwrite another patch
    assert(!PrepareAudioClockPatch(reason));
    target[0] ^= 1;
    DWORD ignored = 0;
    assert(VirtualProtect(target, 1, protection, &ignored));
    assert(FreeLibrary(module));
    std::puts("Actual QPC instructions, precision, image guard and rollback passed.");
}
