#include "nosfix/module_guard.hpp"

#include <bcrypt.h>

#include <array>
#include <iomanip>
#include <sstream>
#include <vector>

namespace nosfix {
namespace {

constexpr LONGLONG kExpectedFileSize = 7'640'728;
constexpr wchar_t kExpectedSha256[] =
    L"339247b90b6de067ab0daa4355c7410487bae9b6f84ad61a4050700fa2d5638b";

bool HashFileSha256(const std::wstring& path, LONGLONG expected_size,
                    const std::wstring& name, std::wstring& hash, std::wstring& reason) {
    HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ,
        nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        reason = L"cannot open " + name + L" for SHA-256 verification";
        return false;
    }
    LARGE_INTEGER file_size{};
    if (!GetFileSizeEx(file, &file_size) || file_size.QuadPart != expected_size) {
        reason = name + L" file size does not match the analyzed build";
        CloseHandle(file);
        return false;
    }

    BCRYPT_ALG_HANDLE algorithm = nullptr;
    BCRYPT_HASH_HANDLE hasher = nullptr;
    DWORD object_size = 0;
    DWORD returned = 0;
    std::vector<UCHAR> object;
    std::array<UCHAR, 32> digest{};
    bool success = false;
    NTSTATUS status = BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM,
                                                   nullptr, 0);
    if (status == 0) status = BCryptGetProperty(algorithm, BCRYPT_OBJECT_LENGTH,
        reinterpret_cast<PUCHAR>(&object_size), sizeof(object_size), &returned, 0);
    if (status == 0) {
        object.resize(object_size);
        status = BCryptCreateHash(algorithm, &hasher, object.data(), object_size,
                                  nullptr, 0, 0);
    }

    std::array<UCHAR, 64 * 1024> buffer{};
    while (status == 0) {
        DWORD bytes_read = 0;
        if (!ReadFile(file, buffer.data(), static_cast<DWORD>(buffer.size()),
                      &bytes_read, nullptr)) {
            reason = L"failed reading " + name + L" during SHA-256 verification";
            break;
        }
        if (bytes_read == 0) {
            status = BCryptFinishHash(hasher, digest.data(),
                                      static_cast<ULONG>(digest.size()), 0);
            success = status == 0;
            break;
        }
        status = BCryptHashData(hasher, buffer.data(), bytes_read, 0);
    }

    if (hasher) BCryptDestroyHash(hasher);
    if (algorithm) BCryptCloseAlgorithmProvider(algorithm, 0);
    CloseHandle(file);

    if (!success) {
        if (reason.empty()) reason = L"Windows SHA-256 provider failed";
        return false;
    }

    std::wostringstream formatted;
    formatted << std::hex << std::setfill(L'0');
    for (const UCHAR byte : digest) formatted << std::setw(2) << byte;
    hash = formatted.str();
    return true;
}

bool VerifyModule(const std::wstring& name, LONGLONG expected_size,
                  const wchar_t* expected_hash, ModuleIdentity& out, std::wstring& reason) {
    HMODULE module = GetModuleHandleW(name.c_str());
    if (!module) {
        reason = name + L" is not loaded";
        return false;
    }

    auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(module);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew <= 0) {
        reason = name + L" has an invalid DOS header";
        return false;
    }
    auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS64*>(
        reinterpret_cast<const BYTE*>(module) + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE ||
        nt->FileHeader.Machine != IMAGE_FILE_MACHINE_AMD64 ||
        nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
        reason = name + L" is not a valid x64 PE image";
        return false;
    }
    std::wstring path(32768, L'\0');
    const DWORD length = GetModuleFileNameW(module, path.data(),
                                             static_cast<DWORD>(path.size()));
    if (length == 0 || length >= path.size()) {
        reason = L"cannot resolve " + name + L" path";
        return false;
    }
    path.resize(length);

    std::wstring actual_hash;
    if (!HashFileSha256(path, expected_size, name, actual_hash, reason)) return false;
    if (actual_hash != expected_hash) {
        reason = name + L" SHA-256 does not match the analyzed build";
        return false;
    }

    out.module = module;
    out.path = std::move(path);
    out.image_size = nt->OptionalHeader.SizeOfImage;
    return true;
}

} // namespace

bool VerifyNostalgiaModule(ModuleIdentity& out, std::wstring& reason) {
    return VerifyModule(L"nostalgia.dll", kExpectedFileSize, kExpectedSha256, out, reason);
}

bool VerifyAfpModule(ModuleIdentity& out, std::wstring& reason) {
    return VerifyModule(L"afp-core.dll", 1'673'880,
        L"03484040f661a8aa49b67d1969c975ff85e4c5d6085347a2da876ac512549873", out, reason);
}

bool VerifyLibxactModule(ModuleIdentity& out, std::wstring& reason) {
    return VerifyModule(L"libxact.dll", 192'664,
        L"6ceee6952cfa636fd96deaaf11bc72e7ca6d7b7499560ef15b033b18cb9bfb9e", out, reason);
}

bool VerifySpiceModule(ModuleIdentity& out, std::wstring& reason) {
    return VerifyModule(L"spice64.exe", 11'171'328,
        L"657ff897b6b2c627ef389c0162a9b4abd935743c45c8cc45e4317d65d1f08409", out, reason);
}

} // namespace nosfix
