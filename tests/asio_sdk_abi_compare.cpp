#include "test_support.hpp"

#include "asio_native_abi.hpp"

#include <cstddef>

#include <windows.h>
#include <objbase.h>

// Optional local-only comparison against the official ASIO SDK headers that
// already exist in the read-only Spice reference tree. This is not vendored:
// the build only adds this TU when that directory is present. It catches the
// case where our native declarations and our fake driver agree with each other
// but both disagree with the real SDK.
#include "asio.h"
#include "iasiodrv.h"

int main() {
    using namespace nosfix::audio::asio_native;

    CHECK(sizeof(::ASIOSamples) == sizeof(Asio64Abi));
    CHECK(sizeof(::ASIOTimeStamp) == sizeof(Asio64Abi));
    CHECK(sizeof(::AsioDriverInfo) == sizeof(AsioDriverInfoAbi));
    CHECK(offsetof(::AsioDriverInfo, sys_ref) ==
          offsetof(AsioDriverInfoAbi, sys_ref));
    CHECK(sizeof(::AsioBufferInfo) == sizeof(AsioBufferInfoAbi));
    CHECK(sizeof(::AsioChannelInfo) == sizeof(AsioChannelInfoAbi));
    CHECK(sizeof(::AsioChannelInfo) == 52);
    CHECK(sizeof(::AsioTimeInfo) == sizeof(AsioTimeInfoAbi));
    CHECK(sizeof(::AsioTimeCode) == sizeof(AsioTimeCodeAbi));
    CHECK(sizeof(::AsioTime) == sizeof(AsioTimeAbi));
    CHECK(offsetof(::AsioTime, timeInfo) == offsetof(AsioTimeAbi, time_info));
    CHECK(offsetof(::AsioTime, timeCode) == offsetof(AsioTimeAbi, time_code));
    CHECK(sizeof(::AsioCallbacks) == sizeof(AsioCallbacksAbi));
    CHECK(offsetof(::AsioCallbacks, buffer_switch) ==
          offsetof(AsioCallbacksAbi, buffer_switch));
    CHECK(offsetof(::AsioCallbacks, sample_rate_did_change) ==
          offsetof(AsioCallbacksAbi, sample_rate_did_change));
    CHECK(offsetof(::AsioCallbacks, asio_message) ==
          offsetof(AsioCallbacksAbi, asio_message));
    CHECK(offsetof(::AsioCallbacks, buffer_switch_time_info) ==
          offsetof(AsioCallbacksAbi, buffer_switch_time_info));

    // The official ASIOSamples/ASIOTimeStamp fields are hi then lo, both 32-bit.
    CHECK(sizeof(::ASIOSamples) == 8);
    CHECK(offsetof(::ASIOSamples, hi) == 0);
    CHECK(offsetof(::ASIOSamples, lo) == 4);
    CHECK(offsetof(::ASIOTimeStamp, hi) == 0);
    CHECK(offsetof(::ASIOTimeStamp, lo) == 4);

    return test::Finish("asio_sdk_abi_compare");
}
