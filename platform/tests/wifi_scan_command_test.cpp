#include <chrono>

#include "doctest.h"
#include "wifi/scan_command.h"

TEST_SUITE("wifi_scan_command") {
    TEST_CASE("large scan output does not consume the time budget") {
        const auto result = oc::wifi::detail::RunCommand({OC_PROCESS_HELPER, "write-stdout", "1000000"}, 8000, 4096);
        CHECK(result.success);
        CHECK_FALSE(result.timed_out);
        CHECK(result.exit_code == 0);
        CHECK(result.output.size() == 4096);
    }

    TEST_CASE("continuous scan output still times out after elapsed time") {
        const auto started = std::chrono::steady_clock::now();
        const auto result = oc::wifi::detail::RunCommand({OC_PROCESS_HELPER, "flood"}, 200, 4096);
        const auto elapsed = std::chrono::steady_clock::now() - started;
        CHECK_FALSE(result.success);
        CHECK(result.timed_out);
        CHECK(elapsed >= std::chrono::milliseconds(200));
        CHECK(elapsed < std::chrono::seconds(2));
        CHECK(result.output.size() == 4096);
    }

    TEST_CASE("a silent scan command has a bounded wait") {
        const auto started = std::chrono::steady_clock::now();
        const auto result = oc::wifi::detail::RunCommand({OC_PROCESS_HELPER, "sleep-ms", "3000"}, 200, 4096);
        const auto elapsed = std::chrono::steady_clock::now() - started;
        CHECK_FALSE(result.success);
        CHECK(result.timed_out);
        CHECK(elapsed >= std::chrono::milliseconds(200));
        CHECK(elapsed < std::chrono::seconds(2));
    }
}
