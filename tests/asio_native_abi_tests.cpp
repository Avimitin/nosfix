#include "test_support.hpp"

#include "asio_native_abi.hpp"

#include <cstdint>
#include <cstring>
#include <limits>

// Exact byte/layout fixtures for the packed native ASIO 2.3 Windows x64 ABI.
// This is the test that would have caught the NATIVE_INT64/pack(4) mistake:
// it does not rely on the host-side logical AsioTime struct.
int main() {
    using namespace nosfix::audio::asio_native;

    CHECK(sizeof(Asio64Abi) == 8);
    CHECK(offsetof(Asio64Abi, hi) == 0);
    CHECK(offsetof(Asio64Abi, lo) == 4);

    CHECK(sizeof(AsioDriverInfoAbi) == 172);
    CHECK(offsetof(AsioDriverInfoAbi, sys_ref) == 164);
    CHECK(sizeof(AsioChannelInfoAbi) == 52);
    CHECK(sizeof(AsioBufferInfoAbi) == 24);
    CHECK(sizeof(AsioTimeInfoAbi) == 48);
    CHECK(sizeof(AsioTimeCodeAbi) == 84);
    CHECK(sizeof(AsioTimeAbi) == 148);
    CHECK(offsetof(AsioTimeAbi, time_info) == 16);
    CHECK(offsetof(AsioTimeAbi, time_code) == 64);
    CHECK(sizeof(AsioCallbacksAbi) == 32);

    std::int64_t value = -1;
    CHECK(Asio64ToInt64(Asio64Abi{0, 0}, value) && value == 0);
    CHECK(Asio64ToInt64(Asio64Abi{0, 1}, value) && value == 1);
    CHECK(Asio64ToInt64(Asio64Abi{1, 0}, value) && value == 4'294'967'296LL);
    CHECK(Asio64ToInt64(Asio64Abi{0x7FFFFFFFU, 0xFFFFFFFFU}, value) &&
          value == std::numeric_limits<std::int64_t>::max());
    CHECK(!Asio64ToInt64(Asio64Abi{0x80000000U, 0}, value));
    CHECK(!Asio64ToInt64(Asio64Abi{0xFFFFFFFFU, 0xFFFFFFFFU}, value));

    const Asio64Abi max_words = Int64ToAsio64(
        std::numeric_limits<std::int64_t>::max());
    CHECK(max_words.hi == 0x7FFFFFFFU && max_words.lo == 0xFFFFFFFFU);
    const Asio64Abi round_trip = Int64ToAsio64(0x123456789ABCDEF0LL);
    CHECK(round_trip.hi == 0x12345678U);
    CHECK(round_trip.lo == 0x9ABCDEF0U);
    std::int64_t decoded = 0;
    CHECK(Asio64ToInt64(round_trip, decoded));
    CHECK(decoded == 0x123456789ABCDEF0LL);

    // Byte fixture: construct a native ASIOTime with fields at the exact packed
    // offsets, then read it through the declared type. A pack(8) layout or an
    // int64 field would fail this test.
    alignas(4) unsigned char raw_time[148] = {};
    const double speed = 1.5;
    const double sample_rate = 44100.0;
    const std::uint32_t flags = 1U | 2U | 4U;
    const std::uint32_t time_flags = 1U;
    const double tc_speed = 0.75;
    std::uint32_t system_hi = 0x12345678U;
    std::uint32_t system_lo = 0x9ABCDEF0U;
    std::uint32_t position_hi = 1U;
    std::uint32_t position_lo = 2U;
    std::uint32_t time_code_hi = 3U;
    std::uint32_t time_code_lo = 4U;
    std::memcpy(raw_time + 16, &speed, sizeof(speed));
    std::memcpy(raw_time + 24, &system_hi, sizeof(system_hi));
    std::memcpy(raw_time + 28, &system_lo, sizeof(system_lo));
    std::memcpy(raw_time + 32, &position_hi, sizeof(position_hi));
    std::memcpy(raw_time + 36, &position_lo, sizeof(position_lo));
    std::memcpy(raw_time + 40, &sample_rate, sizeof(sample_rate));
    std::memcpy(raw_time + 48, &flags, sizeof(flags));
    std::memcpy(raw_time + 64, &tc_speed, sizeof(tc_speed));
    std::memcpy(raw_time + 72, &time_code_hi, sizeof(time_code_hi));
    std::memcpy(raw_time + 76, &time_code_lo, sizeof(time_code_lo));
    std::memcpy(raw_time + 80, &time_flags, sizeof(time_flags));

    const auto* time = reinterpret_cast<const AsioTimeAbi*>(raw_time);
    CHECK(time->time_info.speed == speed);
    CHECK(time->time_info.system_time.hi == 0x12345678U);
    CHECK(time->time_info.system_time.lo == 0x9ABCDEF0U);
    CHECK(time->time_info.sample_position.hi == 1U);
    CHECK(time->time_info.sample_position.lo == 2U);
    CHECK(time->time_info.sample_rate == sample_rate);
    CHECK(time->time_info.flags == flags);
    CHECK(time->time_code.speed == tc_speed);
    CHECK(time->time_code.time_code_samples.hi == 3U);
    CHECK(time->time_code.time_code_samples.lo == 4U);
    CHECK(time->time_code.flags == time_flags);

    nosfix::audio::AsioTime logical{};
    ConvertToLogicalTime(*time, logical);
    CHECK(logical.time_info.speed == speed);
    CHECK(logical.time_info.system_time_ns == 0x123456789ABCDEF0LL);
    CHECK(logical.time_info.sample_position == 0x0000000100000002LL);
    CHECK(logical.time_info.sample_rate == sample_rate);
    CHECK(logical.time_info.flags == flags);
    CHECK(logical.time_code.time_code_samples == 0x0000000300000004LL);
    CHECK(logical.time_code.flags == time_flags);

    AsioTimeAbi invalid_native_time = *time;
    invalid_native_time.time_info.sample_position.hi = 0x80000000U;
    invalid_native_time.time_code.time_code_samples.hi = 0x80000000U;
    nosfix::audio::AsioTime invalid_logical{};
    ConvertToLogicalTime(invalid_native_time, invalid_logical);
    CHECK((invalid_logical.time_info.flags & 2U) == 0U);
    CHECK((invalid_logical.time_code.flags & 1U) == 0U);

    alignas(4) unsigned char raw_driver[172] = {};
    int marker = 7;
    void* pointer = &marker;
    std::memcpy(raw_driver + 164, &pointer, sizeof(pointer));
    const auto* driver = reinterpret_cast<const AsioDriverInfoAbi*>(raw_driver);
    CHECK(driver->sys_ref == &marker);

    // Every callback entry must treat native time as hi/lo. Conversion failure
    // (high bit set) is reported to the host by clearing the validity flag in
    // asio_output.cpp; here we prove the conversion itself fails closed.
    Asio64Abi invalid_time{0x80000000U, 1U};
    CHECK(!Asio64ToInt64(invalid_time, decoded));

    return test::Finish("asio_native_abi");
}
