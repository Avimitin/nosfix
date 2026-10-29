#include "nosfix/ui_hooks.hpp"
#include "nosfix/log.hpp"

#include <Windows.h>

#include <array>
#include <atomic>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <vector>

namespace {

constexpr std::size_t kImageSize = 0xC0000;
constexpr std::size_t kComposeRva = 0x3080;
constexpr std::size_t kCallRva = 0xB6DFB;
constexpr std::size_t kNodeSize = 328;

std::uint64_t g_original_calls = 0;
const void* g_last_node = nullptr;
std::array<float, 16> g_last_matrix{};
std::atomic<unsigned> g_presentation_rate{120};

template <typename T>
void Store(void* node, std::size_t offset, T value) {
    std::memcpy(static_cast<std::byte*>(node) + offset, &value, sizeof(value));
}

template <typename T>
T Load(const void* node, std::size_t offset) {
    T value{};
    std::memcpy(&value, static_cast<const std::byte*>(node) + offset, sizeof(value));
    return value;
}

std::array<float, 16> Identity(float x, float y, float z) {
    std::array<float, 16> matrix{};
    matrix[0] = matrix[5] = matrix[10] = matrix[15] = 1.0f;
    matrix[12] = x;
    matrix[13] = y;
    matrix[14] = z;
    return matrix;
}

void __fastcall MockCompose(void* node) {
    ++g_original_calls;
    g_last_node = node;
    std::memcpy(g_last_matrix.data(),
        static_cast<const std::byte*>(node) + 128, sizeof(g_last_matrix));
}

HMODULE MakeFakeModule() {
    auto* memory = static_cast<unsigned char*>(VirtualAlloc(
        nullptr, kImageSize, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE));
    assert(memory != nullptr);
    std::memset(memory, 0, kImageSize);

    auto* dos = reinterpret_cast<IMAGE_DOS_HEADER*>(memory);
    dos->e_magic = IMAGE_DOS_SIGNATURE;
    dos->e_lfanew = 0x100;
    auto* nt = reinterpret_cast<IMAGE_NT_HEADERS64*>(memory + dos->e_lfanew);
    nt->Signature = IMAGE_NT_SIGNATURE;
    nt->OptionalHeader.Magic = IMAGE_NT_OPTIONAL_HDR64_MAGIC;
    nt->OptionalHeader.SizeOfImage = static_cast<DWORD>(kImageSize);

    constexpr std::array<unsigned char, 24> helper{
        0x48,0x83,0xEC,0x48,0x49,0x89,0xC8,0x49,0x81,0xC0,0x80,0x00,
        0x00,0x00,0x48,0x8B,0x0D,0x2B,0x2A,0x18,0x00,0x45,0x0F,0xB6};
    constexpr std::array<unsigned char, 5> call{
        0xE8, 0x80, 0xC2, 0xF4, 0xFF};
    std::memcpy(memory + kComposeRva, helper.data(), helper.size());
    std::memcpy(memory + kCallRva, call.data(), call.size());
    return reinterpret_cast<HMODULE>(memory);
}

void InstallMockAt(HMODULE module) {
    // The call-patch shim invokes the target through the normal x64 ABI. This
    // small absolute-jump stub routes that call into MockCompose.
    std::array<unsigned char, 14> code{
        0xFF,0x25,0x00,0x00,0x00,0x00,
        0,0,0,0,0,0,0,0};
    const std::uintptr_t destination = reinterpret_cast<std::uintptr_t>(&MockCompose);
    std::memcpy(code.data() + 6, &destination, sizeof(destination));
    std::memcpy(reinterpret_cast<unsigned char*>(module) + kComposeRva,
                code.data(), code.size());
    FlushInstructionCache(GetCurrentProcess(),
        reinterpret_cast<unsigned char*>(module) + kComposeRva, code.size());
}

} // namespace

namespace nosfix {
void Log(LogLevel, std::string_view) noexcept {}
unsigned PresentationRate() noexcept {
    return g_presentation_rate.load(std::memory_order_relaxed);
}
} // namespace nosfix

int main() {
    using namespace nosfix;

    HMODULE fake_module = MakeFakeModule();
    std::vector<CallPatchSpec> patches;
    std::string reason;
    auto* fake_bytes = reinterpret_cast<unsigned char*>(fake_module);
    fake_bytes[kCallRva] ^= 1;
    assert(!PrepareUiMotion(fake_module, true, patches, reason));
    assert(patches.empty() && !reason.empty());
    fake_bytes[kCallRva] ^= 1;
    assert(PrepareUiMotion(fake_module, true, patches, reason));
    assert(patches.size() == 1);
    assert(patches[0].rva == kCallRva);
    assert(patches[0].module == fake_module);
    assert(patches[0].expected_call.size() == 5);
    InstallMockAt(fake_module);

    ConfigureUiMotion(true);
    const auto invoke = reinterpret_cast<void(__fastcall*)(void*)>(patches[0].destination);

    alignas(16) std::array<std::byte, kNodeSize> node{};
    Store<std::uint16_t>(node.data(), 8, 0x79);
    Store<std::uintptr_t>(node.data(), 48, 0x9000);
    Store<std::uintptr_t>(node.data(), 64, 0xA000);
    const std::uint32_t stamp = 0x20010002;
    Store(node.data(), 312, stamp);

    auto first = Identity(10.0f, 20.0f, 0.0f);
    std::memcpy(node.data() + 128, first.data(), sizeof(first));
    const auto source_first = node;
    BeginUiPresentation(0.0);
    invoke(node.data());
    EndUiPresentation();
    assert(g_original_calls == 1 && g_last_node == node.data());
    assert(g_last_matrix == first);
    assert(node == source_first);

    RetireUiTick();
    auto second = Identity(12.0f, 24.0f, 1.0f);
    second[0] = 0.75f;
    std::memcpy(node.data() + 128, second.data(), sizeof(second));
    const auto source_second = node;

    BeginUiPresentation(0.5);
    invoke(node.data());
    EndUiPresentation();
    assert(g_original_calls == 2);
    assert(g_last_node != node.data()); // The game object is never modified.
    assert(g_last_matrix[12] == 13.0f && g_last_matrix[13] == 26.0f &&
           g_last_matrix[14] == 1.5f);
    assert(g_last_matrix[0] == second[0] && g_last_matrix[15] == 1.0f);
    assert(node == source_second);

    // Repeated renders in one simulation tick use the same latest/history pair;
    // they do not integrate or drift the source state.
    for (int i = 0; i < 20; ++i) {
        BeginUiPresentation(0.5);
        invoke(node.data());
        EndUiPresentation();
        assert(g_last_matrix[12] == 13.0f && g_last_matrix[13] == 26.0f);
        assert(node == source_second);
    }
    assert(g_original_calls == 22);

    // A changed allocation stamp/resource/parent/type selects a fresh track.
    Store(node.data(), 312, stamp + 1);
    BeginUiPresentation(0.5);
    invoke(node.data());
    EndUiPresentation();
    assert(g_last_matrix[12] == second[12]);
    Store(node.data(), 312, stamp);
    BeginUiPresentation(0.5);
    invoke(node.data());
    EndUiPresentation();
    assert(g_last_matrix[12] == second[12]); // Old address identity cannot resurrect history.

    Store<std::uintptr_t>(node.data(), 48, 0x9008);
    BeginUiPresentation(0.5);
    invoke(node.data());
    EndUiPresentation();
    assert(g_last_matrix[12] == second[12]);
    Store<std::uintptr_t>(node.data(), 48, 0x9000);
    Store<std::uintptr_t>(node.data(), 64, 0xA008);
    BeginUiPresentation(0.5);
    invoke(node.data());
    EndUiPresentation();
    assert(g_last_matrix[12] == second[12]);
    Store<std::uintptr_t>(node.data(), 64, 0xA000);
    Store<std::uint16_t>(node.data(), 8, 0x7B);
    BeginUiPresentation(0.5);
    invoke(node.data());
    EndUiPresentation();
    assert(g_last_matrix[12] == second[12]);

    // Pivot/no-op transform modes are passed through unchanged and reset the
    // keyed history until the mode returns to ordinary affine composition.
    Store<std::uint16_t>(node.data(), 8, 0x79);
    Store<std::uint8_t>(node.data(), 204, 1);
    BeginUiPresentation(0.75);
    invoke(node.data());
    EndUiPresentation();
    assert(g_last_node == node.data());
    assert(g_last_matrix[12] == second[12]);
    Store<std::uint8_t>(node.data(), 204, 0);

    // No render scope and disabled mode both preserve direct forwarding.
    invoke(node.data());
    assert(g_last_node == node.data());
    const UiMotionMetrics metrics = TakeUiMetrics();
    assert(metrics.captures > 0);
    assert(metrics.predicted_draws > 0);
    assert(metrics.fallback_draws > 0);

    // At 60 Hz the hook remains a byte-for-byte direct forwarder and does not
    // capture or prune UI motion history.
    g_presentation_rate.store(60, std::memory_order_relaxed);
    BeginUiPresentation(0.75);
    invoke(node.data());
    EndUiPresentation();
    assert(g_last_node == node.data());
    const UiMotionMetrics low_rate_metrics = TakeUiMetrics();
    assert(low_rate_metrics.captures == 0 && low_rate_metrics.predicted_draws == 0 &&
           low_rate_metrics.fallback_draws == 0);
    g_presentation_rate.store(120, std::memory_order_relaxed);

    ConfigureUiMotion(false);
    BeginUiPresentation(0.5);
    invoke(node.data());
    EndUiPresentation();
    assert(g_last_node == node.data());

    ConfigureUiMotion(false);
    VirtualFree(fake_module, 0, MEM_RELEASE);
}
