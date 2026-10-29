#pragma once

// Packed native ASIO 2.3 ABI, Windows x64.
//
// The Steinberg SDK defines NATIVE_INT64=0 on Windows (asiosys.h), so
// ASIOSamples and ASIOTimeStamp are two 32-bit words (hi then lo), and asio.h
// wraps its declarations in #pragma pack(push,4). This header is an independent
// declaration of those facts, not a copy of SDK source. The public
// nosfix::audio::AsioTime* structs are logical host-side values and are NOT this
// native layout; the COM adapter converts at every driver boundary.
//
// Expected native sizes/offsets are asserted here and in
// tests/asio_native_abi_tests.cpp. No real driver may be opened until those
// fixtures are reviewed.

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "nosfix/asio_output.hpp"

namespace nosfix::audio::asio_native {

#pragma pack(push, 4)

struct Asio64Abi {
    std::uint32_t hi = 0;
    std::uint32_t lo = 0;
};

struct AsioDriverInfoAbi {
    std::int32_t asio_version = 0;
    std::int32_t driver_version = 0;
    char name[32] = {};
    char error_message[124] = {};
    void* sys_ref = nullptr; // offset 164 under pack(4), size 172
};

struct AsioChannelInfoAbi {
    std::int32_t channel = 0;
    std::int32_t is_input = 0;
    std::int32_t is_active = 0;
    std::int32_t channel_group = 0;
    std::int32_t type = 0;
    char name[32] = {};
};

struct AsioBufferInfoAbi {
    std::int32_t is_input = 0;
    std::int32_t channel_num = 0;
    void* buffers[2] = {nullptr, nullptr};
};

struct AsioTimeInfoAbi {
    double speed = 1.0;
    Asio64Abi system_time{};      // nanoseconds; hi/lo
    Asio64Abi sample_position{};  // hi/lo
    double sample_rate = 0.0;
    std::uint32_t flags = 0;
    char reserved[12] = {};
};

struct AsioTimeCodeAbi {
    double speed = 1.0;
    Asio64Abi time_code_samples{};
    std::uint32_t flags = 0;
    char future[64] = {};
};

struct AsioTimeAbi {
    std::int32_t reserved[4] = {};
    AsioTimeInfoAbi time_info{};
    AsioTimeCodeAbi time_code{};
};

struct AsioCallbacksAbi {
    void (*buffer_switch)(std::int32_t index, std::int32_t direct_process) = nullptr;
    void (*sample_rate_did_change)(double rate) = nullptr;
    std::int32_t (*asio_message)(std::int32_t selector, std::int32_t value,
                                 void* message, double* opt) = nullptr;
    AsioTimeAbi* (*buffer_switch_time_info)(AsioTimeAbi* params,
                                            std::int32_t index,
                                            std::int32_t direct_process) = nullptr;
};

#pragma pack(pop)

static_assert(sizeof(Asio64Abi) == 8, "native ASIO 64-bit word pair");
static_assert(offsetof(Asio64Abi, hi) == 0, "ASIOSamples.hi offset");
static_assert(offsetof(Asio64Abi, lo) == 4, "ASIOSamples.lo offset");
static_assert(sizeof(AsioDriverInfoAbi) == 172, "native ASIODriverInfo size");
static_assert(offsetof(AsioDriverInfoAbi, sys_ref) == 164,
              "native ASIODriverInfo.sysRef offset");
static_assert(sizeof(AsioChannelInfoAbi) == 52, "native ASIOChannelInfo size");
static_assert(sizeof(AsioBufferInfoAbi) == 24, "native ASIOBufferInfo size");
static_assert(sizeof(AsioTimeInfoAbi) == 48, "native AsioTimeInfo size");
static_assert(sizeof(AsioTimeCodeAbi) == 84, "native ASIOTimeCode size");
static_assert(sizeof(AsioTimeAbi) == 148, "native ASIOTime size");
static_assert(offsetof(AsioTimeAbi, time_info) == 16, "native ASIOTime.timeInfo");
static_assert(offsetof(AsioTimeAbi, time_code) == 64, "native ASIOTime.timeCode");
static_assert(sizeof(AsioCallbacksAbi) == 32, "native ASIOCallbacks size");

// ASIO values are nonnegative 64-bit quantities in practice. Values with the
// high bit set cannot be represented as int64; callers must fail closed rather
// than wrap. Negative inputs are encoded as two's complement for completeness
// but are not expected from a driver sample position.
[[nodiscard]] inline bool Asio64ToInt64(const Asio64Abi& value,
                                        std::int64_t& out) noexcept {
    if (value.hi > 0x7FFFFFFFU) return false;
    const std::uint64_t bits =
        (static_cast<std::uint64_t>(value.hi) << 32U) | value.lo;
    out = static_cast<std::int64_t>(bits);
    return true;
}

[[nodiscard]] inline Asio64Abi Int64ToAsio64(std::int64_t value) noexcept {
    const std::uint64_t bits = static_cast<std::uint64_t>(value);
    Asio64Abi result;
    result.hi = static_cast<std::uint32_t>(bits >> 32U);
    result.lo = static_cast<std::uint32_t>(bits & 0xFFFFFFFFU);
    return result;
}

// Converts a packed native ASIOTime into the host-side logical representation.
// The COM adapter calls this at every bufferSwitchTimeInfo entry. If a hi/lo
// value cannot be represented as int64, the corresponding validity flag is
// cleared rather than wrapping.
inline void ConvertToLogicalTime(const AsioTimeAbi& native,
                                 nosfix::audio::AsioTime& logical) noexcept {
    constexpr std::uint32_t kSystemTimeValid = 1U;
    constexpr std::uint32_t kSamplePositionValid = 2U;
    constexpr std::uint32_t kTcValid = 1U;

    logical.time_info.speed = native.time_info.speed;
    logical.time_info.sample_rate = native.time_info.sample_rate;
    logical.time_info.flags = native.time_info.flags;
    if (!Asio64ToInt64(native.time_info.system_time,
                       logical.time_info.system_time_ns)) {
        logical.time_info.system_time_ns = 0;
        logical.time_info.flags &= ~kSystemTimeValid;
    }
    if (!Asio64ToInt64(native.time_info.sample_position,
                       logical.time_info.sample_position)) {
        logical.time_info.sample_position = 0;
        logical.time_info.flags &= ~kSamplePositionValid;
    }
    std::memcpy(logical.time_info.reserved, native.time_info.reserved,
                sizeof(logical.time_info.reserved));

    logical.time_code.speed = native.time_code.speed;
    logical.time_code.flags = native.time_code.flags;
    if (!Asio64ToInt64(native.time_code.time_code_samples,
                       logical.time_code.time_code_samples)) {
        logical.time_code.time_code_samples = 0;
        logical.time_code.flags &= ~kTcValid;
    }
    std::memcpy(logical.time_code.future, native.time_code.future,
                sizeof(logical.time_code.future));
}

} // namespace nosfix::audio::asio_native
