#include "nosfix/presentation.hpp"
#include "nosfix/config.hpp"
#include "nosfix/log.hpp"

#include <d3d9.h>
#include <algorithm>
#include <atomic>
#include <cstring>
#include <new>

namespace nosfix {
namespace {
using Factory = IDirect3D9* (WINAPI*)(UINT);
// These RVAs are valid only after the PAN-2025020501 SHA-256 guard succeeds.
constexpr DWORD kFactoryImport = 0x3A8C70;
constexpr DWORD kDevice = 0x11CEAF8;
constexpr DWORD kSecondaryParameters = 0x11CED08;
constexpr DWORD kSecondaryCount = 0x11CEE98;
constexpr DWORD kSecondaryEnabled = 0x6FC344;
static_assert(sizeof(D3DPRESENT_PARAMETERS) == 64);

unsigned char* g_base = nullptr;
void** g_import = nullptr;
Factory g_factory = nullptr;
unsigned g_requested_rate = 0;
std::atomic<unsigned> g_rate{60};
bool g_committed = false;

template <typename T> T& At(DWORD rva) noexcept {
    return *reinterpret_cast<T*>(g_base + rva);
}

unsigned DesktopRate() noexcept {
    DEVMODEW mode{};
    mode.dmSize = sizeof(mode);
    if (EnumDisplaySettingsW(nullptr, ENUM_CURRENT_SETTINGS, &mode) &&
        mode.dmDisplayFrequency >= 60 && mode.dmDisplayFrequency <= 1000) {
        return mode.dmDisplayFrequency;
    }
    return 60;
}

UINT SupportedRefresh(IDirect3D9* real, UINT adapter,
                      const D3DPRESENT_PARAMETERS& pp, unsigned requested) {
    if (pp.Windowed) return 0;
    // A fullscreen backbuffer may have alpha even though display modes do not.
    const auto format = pp.BackBufferFormat == D3DFMT_A8R8G8B8
        ? D3DFMT_X8R8G8B8 : pp.BackBufferFormat;
    const UINT count = real->GetAdapterModeCount(adapter, format);
    for (UINT i = 0; i < count; ++i) {
        D3DDISPLAYMODE mode{};
        if (SUCCEEDED(real->EnumAdapterModes(adapter, format, i, &mode)) &&
            mode.Width == pp.BackBufferWidth && mode.Height == pp.BackBufferHeight &&
            mode.RefreshRate == requested) return requested;
    }
    Log(LogLevel::Warning, "requested fullscreen refresh " + std::to_string(requested) +
        " is not enumerated for adapter " + std::to_string(adapter) +
        " at " + std::to_string(pp.BackBufferWidth) + "x" +
        std::to_string(pp.BackBufferHeight) + "; using display default");
    return D3DPRESENT_RATE_DEFAULT;
}

void Configure(IDirect3D9* real, UINT adapter, D3DPRESENT_PARAMETERS& pp,
               bool primary) {
    D3DDISPLAYMODE desktop{};
    unsigned rate = DesktopRate();
    if (SUCCEEDED(real->GetAdapterDisplayMode(adapter, &desktop)) &&
        desktop.RefreshRate >= 60 && desktop.RefreshRate <= 1000) rate = desktop.RefreshRate;
    if (primary && g_requested_rate) rate = g_requested_rate;
    if (primary) g_rate.store(rate, std::memory_order_relaxed);
    pp.FullScreen_RefreshRateInHz = SupportedRefresh(real, adapter, pp, rate);
    // The QPC scheduler owns pacing. Either monitor's blocking vblank would
    // otherwise delay the next simulation tick, especially at non-integer ratios.
    pp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
    Log(LogLevel::Info, "D3D adapter " + std::to_string(adapter) +
        ": immediate presentation, fullscreen refresh=" +
        std::to_string(pp.FullScreen_RefreshRateInHz));
}

// Only the factory imported by nostalgia is wrapped. Existing Spice D3D hooks
// remain underneath this object; no process-wide graphics API is detoured.
class Direct3D final : public IDirect3D9 {
public:
    explicit Direct3D(IDirect3D9* real) noexcept : real_(real) {}
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id, void** out) override {
        if (!out) return E_POINTER;
        *out = nullptr;
        if (id == __uuidof(IUnknown) || id == __uuidof(IDirect3D9)) {
            *out = static_cast<IDirect3D9*>(this);
            AddRef();
            return S_OK;
        }
        return E_NOINTERFACE;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return ++references_; }
    ULONG STDMETHODCALLTYPE Release() override {
        const ULONG remaining = --references_;
        if (!remaining) { real_->Release(); delete this; }
        return remaining;
    }
    HRESULT STDMETHODCALLTYPE RegisterSoftwareDevice(void* p) override { return real_->RegisterSoftwareDevice(p); }
    UINT STDMETHODCALLTYPE GetAdapterCount() override { return real_->GetAdapterCount(); }
    HRESULT STDMETHODCALLTYPE GetAdapterIdentifier(UINT a, DWORD f, D3DADAPTER_IDENTIFIER9* p) override { return real_->GetAdapterIdentifier(a, f, p); }
    UINT STDMETHODCALLTYPE GetAdapterModeCount(UINT a, D3DFORMAT f) override { return real_->GetAdapterModeCount(a, f); }
    HRESULT STDMETHODCALLTYPE EnumAdapterModes(UINT a, D3DFORMAT f, UINT m, D3DDISPLAYMODE* p) override { return real_->EnumAdapterModes(a, f, m, p); }
    HRESULT STDMETHODCALLTYPE GetAdapterDisplayMode(UINT a, D3DDISPLAYMODE* p) override { return real_->GetAdapterDisplayMode(a, p); }
    HRESULT STDMETHODCALLTYPE CheckDeviceType(UINT a, D3DDEVTYPE t, D3DFORMAT f, D3DFORMAT b, BOOL w) override { return real_->CheckDeviceType(a, t, f, b, w); }
    HRESULT STDMETHODCALLTYPE CheckDeviceFormat(UINT a, D3DDEVTYPE t, D3DFORMAT f, DWORD u, D3DRESOURCETYPE r, D3DFORMAT c) override { return real_->CheckDeviceFormat(a, t, f, u, r, c); }
    HRESULT STDMETHODCALLTYPE CheckDeviceMultiSampleType(UINT a, D3DDEVTYPE t, D3DFORMAT f, BOOL w, D3DMULTISAMPLE_TYPE m, DWORD* q) override { return real_->CheckDeviceMultiSampleType(a, t, f, w, m, q); }
    HRESULT STDMETHODCALLTYPE CheckDepthStencilMatch(UINT a, D3DDEVTYPE t, D3DFORMAT f, D3DFORMAT r, D3DFORMAT d) override { return real_->CheckDepthStencilMatch(a, t, f, r, d); }
    HRESULT STDMETHODCALLTYPE CheckDeviceFormatConversion(UINT a, D3DDEVTYPE t, D3DFORMAT s, D3DFORMAT d) override { return real_->CheckDeviceFormatConversion(a, t, s, d); }
    HRESULT STDMETHODCALLTYPE GetDeviceCaps(UINT a, D3DDEVTYPE t, D3DCAPS9* c) override { return real_->GetDeviceCaps(a, t, c); }
    HMONITOR STDMETHODCALLTYPE GetAdapterMonitor(UINT a) override { return real_->GetAdapterMonitor(a); }
    HRESULT STDMETHODCALLTYPE CreateDevice(UINT adapter, D3DDEVTYPE type, HWND window,
        DWORD flags, D3DPRESENT_PARAMETERS* parameters, IDirect3DDevice9** device) override {
        if (!parameters) return D3DERR_INVALIDCALL;
        UINT count = 1;
        if (flags & D3DCREATE_ADAPTERGROUP_DEVICE) {
            D3DCAPS9 caps{};
            if (FAILED(real_->GetDeviceCaps(adapter, type, &caps)) ||
                caps.NumberOfAdaptersInGroup == 0 || caps.NumberOfAdaptersInGroup > 16)
                return D3DERR_INVALIDCALL;
            count = caps.NumberOfAdaptersInGroup;
            // This game allocates the group array using this count global.
            if (count > At<unsigned>(kSecondaryCount)) return D3DERR_INVALIDCALL;
        }
        for (UINT i = 0; i < count; ++i) {
            UINT member = adapter;
            if (i) {
                bool found = false;
                for (UINT candidate = 0; candidate < real_->GetAdapterCount(); ++candidate) {
                    D3DCAPS9 caps{};
                    if (SUCCEEDED(real_->GetDeviceCaps(candidate, type, &caps)) &&
                        caps.MasterAdapterOrdinal == adapter && caps.AdapterOrdinalInGroup == i) {
                        member = candidate;
                        found = true;
                        break;
                    }
                }
                if (!found) return D3DERR_INVALIDCALL;
            }
            Configure(real_, member, parameters[i], i == 0);
        }
        // In the non-group path the game creates secondary swapchains later,
        // directly from this same persistent array. Configure those before use.
        if (count == 1 && At<unsigned char>(kSecondaryEnabled)) {
            auto* secondary = At<D3DPRESENT_PARAMETERS*>(kSecondaryParameters);
            const unsigned secondary_count = At<unsigned>(kSecondaryCount);
            if (secondary && secondary_count > 1 && secondary_count <= 16) {
                for (unsigned i = 1; i < secondary_count; ++i) {
                    secondary[i].PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
                    if (secondary[i].Windowed) secondary[i].FullScreen_RefreshRateInHz = 0;
                }
            }
        }
        const HRESULT result = real_->CreateDevice(adapter, type, window, flags, parameters, device);
        if (SUCCEEDED(result) && device && *device) {
            D3DDISPLAYMODE actual{};
            if (SUCCEEDED((*device)->GetDisplayMode(0, &actual)))
                Log(LogLevel::Info, "device created: reported display=" +
                    std::to_string(actual.RefreshRate) + "Hz, presentation target=" +
                    std::to_string(PresentationRate()) + "Hz");
        }
        return result;
    }
private:
    IDirect3D9* real_;
    std::atomic<ULONG> references_{1};
};

IDirect3D9* WINAPI Create9(UINT version) noexcept {
    IDirect3D9* real = g_factory(version);
    if (!real) return nullptr;
    auto* proxy = new (std::nothrow) Direct3D(real);
    if (!proxy) { real->Release(); return nullptr; }
    return proxy;
}

bool ExchangeImport(void* expected, void* desired, std::string& reason) {
    DWORD old = 0;
    if (!VirtualProtect(g_import, sizeof(void*), PAGE_READWRITE, &old)) {
        reason = "cannot make the Direct3DCreate9 import writable";
        return false;
    }
    void* observed = InterlockedCompareExchangePointer(g_import, desired, expected);
    DWORD ignored = 0;
    const BOOL restored = VirtualProtect(g_import, sizeof(void*), old, &ignored);
    if (observed != expected) {
        if (!restored) {
            VirtualProtect(g_import, sizeof(void*), old, &ignored);
            Log(LogLevel::Error, "Direct3DCreate9 import changed and restoring its page protection failed");
        }
        reason = "Direct3DCreate9 import changed during installation";
        return false;
    }
    if (!restored) {
        InterlockedCompareExchangePointer(g_import, expected, desired);
        VirtualProtect(g_import, sizeof(void*), old, &ignored);
        reason = "cannot restore Direct3DCreate9 import protection";
        return false;
    }
    return true;
}
} // namespace

bool PreparePresentation(HMODULE module, const Config& config, std::string& reason) {
    g_base = reinterpret_cast<unsigned char*>(module);
    if (!g_base || g_committed) { reason = "invalid presentation initialization state"; return false; }
    if (At<void*>(kDevice)) { reason = "D3D device already exists; load with Spice -k before game main"; return false; }
    // Check that the independently verified RVA is the import named in the PE.
    const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(g_base);
    const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS64*>(g_base + dos->e_lfanew);
    const DWORD size = nt->OptionalHeader.SizeOfImage;
    const auto directory = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    bool found = false;
    if (directory.VirtualAddress && directory.VirtualAddress < size && directory.Size <= size - directory.VirtualAddress) {
        const auto* imports = reinterpret_cast<const IMAGE_IMPORT_DESCRIPTOR*>(g_base + directory.VirtualAddress);
        for (size_t i = 0; i < directory.Size / sizeof(*imports) && imports[i].Name; ++i) {
            const auto& entry = imports[i];
            if (!entry.OriginalFirstThunk || entry.OriginalFirstThunk >= size || entry.FirstThunk >= size) continue;
            const auto* names = reinterpret_cast<const IMAGE_THUNK_DATA64*>(g_base + entry.OriginalFirstThunk);
            const size_t max = std::min((size - entry.OriginalFirstThunk) / sizeof(*names),
                                      (size - entry.FirstThunk) / sizeof(*names));
            for (size_t j = 0; j < max && names[j].u1.AddressOfData; ++j) {
                const auto rva = names[j].u1.AddressOfData;
                if (IMAGE_SNAP_BY_ORDINAL64(rva) || rva >= size || size - rva < sizeof(IMAGE_IMPORT_BY_NAME)) continue;
                const auto* name = reinterpret_cast<const IMAGE_IMPORT_BY_NAME*>(g_base + rva);
                constexpr char expected[] = "Direct3DCreate9";
                if (size - rva >= offsetof(IMAGE_IMPORT_BY_NAME, Name) + sizeof(expected) &&
                    std::memcmp(name->Name, expected, sizeof(expected)) == 0 &&
                    entry.FirstThunk + j * sizeof(IMAGE_THUNK_DATA64) == kFactoryImport) found = true;
            }
        }
    }
    if (!found) { reason = "verified Direct3DCreate9 import not found"; return false; }
    g_import = reinterpret_cast<void**>(g_base + kFactoryImport);
    g_factory = reinterpret_cast<Factory>(*g_import);
    if (!g_factory) { reason = "Direct3DCreate9 import is null"; return false; }
    g_requested_rate = config.target_hz;
    g_rate.store(g_requested_rate ? g_requested_rate : DesktopRate(), std::memory_order_relaxed);
    return true;
}

bool CommitPresentation(std::string& reason) {
    if (!g_import || !g_factory || g_committed) { reason = "presentation hook was not prepared"; return false; }
    g_committed = ExchangeImport(reinterpret_cast<void*>(g_factory), reinterpret_cast<void*>(&Create9), reason);
    return g_committed;
}

void RollbackPresentation() noexcept {
    if (!g_committed) return;
    try {
        std::string ignored;
        if (ExchangeImport(reinterpret_cast<void*>(&Create9), reinterpret_cast<void*>(g_factory), ignored))
            g_committed = false;
        else
            Log(LogLevel::Error, "presentation rollback failed: " + ignored);
    } catch (...) {}
}

unsigned PresentationRate() noexcept { return g_rate.load(std::memory_order_relaxed); }
} // namespace nosfix
