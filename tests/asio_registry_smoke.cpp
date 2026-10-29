#include "test_support.hpp"

#include "nosfix/asio_output.hpp"

#include <cstdio>
#include <memory>
#include <string>
#include <vector>

// Non-hardware compatibility smoke test:
//  - the local ASIO ABI structs have the expected x64 layout;
//  - the registry enumerator runs and reports what it finds (missing key is
//    informational, not a failure);
//  - invalid CLSIDs are rejected by the COM factory without touching audio.
// No ASIO driver is opened and no device is started.
int main() {
    using namespace nosfix::audio;

    CHECK(std::string(AsioSampleFormatName(AsioSampleFormat::Int24Lsb)) ==
          "Int24LSB");

    std::vector<AsioDriverRegistration> drivers;
    std::string error;
    const bool enumerated = EnumerateSystemAsioDrivers(drivers, error);
    if (!enumerated) {
        std::printf("INFO ASIO registry unavailable: %s\n", error.c_str());
    } else {
        std::printf("INFO registered 64-bit ASIO drivers: %zu\n", drivers.size());
        for (const auto& driver : drivers) {
            std::printf("INFO   name='%s' clsid=%s description='%s'\n",
                        driver.name.c_str(), driver.clsid.c_str(),
                        driver.description.c_str());
        }
    }

    std::string create_error;
    auto bad_driver = CreateSystemAsioDriver("not-a-clsid", create_error);
    CHECK(!bad_driver);
    CHECK(!create_error.empty());

    return test::Finish("asio_registry_smoke");
}
