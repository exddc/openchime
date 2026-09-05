#include <atomic>
#include <chrono>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "chime/webd_apply_manager.h"
#include "doctest.h"
#include "fake_process_runner.h"
#include "process_test_support.h"
#include "test_support.h"

namespace {

oc::process::Command Restart(std::string executable) {
    return oc::process::Command{std::move(executable), {"restart"}};
}

} // namespace

TEST_SUITE("apply_manager") {
    TEST_CASE("restart commands are argv arrays with a 30s step timeout") {
        NullLogger logger;
        oc::process::FakeRunner processes;
        chime::webd::ApplyManager apply(logger, processes, Restart("/etc/init.d/S40network"),
                                        Restart("/etc/init.d/S99chime"));

        const auto started = apply.StartApply();
        CHECK(started.state == "pending");
        const oc::apply::Status status = WaitTerminal([&] { return apply.CurrentStatus(); });
        CHECK(status.state == "succeeded");

        const auto calls = processes.Calls();
        REQUIRE(calls.size() == 2);
        CHECK(calls[0].command.executable == "/etc/init.d/S40network");
        REQUIRE(calls[0].command.arguments.size() == 1);
        CHECK(calls[0].command.arguments[0] == "restart");
        CHECK(calls[0].timeout == oc::process::kDefaultTimeout);
        CHECK(calls[1].command.executable == "/etc/init.d/S99chime");
        REQUIRE(calls[1].command.arguments.size() == 1);
        CHECK(calls[1].command.arguments[0] == "restart");
        CHECK(calls[1].timeout == oc::process::kDefaultTimeout);
    }

    TEST_CASE("an empty executable fails at construction") {
        NullLogger logger;
        oc::process::FakeRunner processes;
        CHECK_THROWS_AS(chime::webd::ApplyManager(logger, processes, Restart(""), Restart("true")),
                        std::invalid_argument);
        CHECK_THROWS_AS(chime::webd::ApplyManager(logger, processes, Restart("true"), Restart("")),
                        std::invalid_argument);
    }

    TEST_CASE("network failure skips the chime restart") {
        NullLogger logger;
        oc::process::FakeRunner processes;
        processes.Queue(oc::process::Exited(1));
        chime::webd::ApplyManager apply(logger, processes, Restart("/etc/init.d/S40network"),
                                        Restart("/etc/init.d/S99chime"));

        const auto started = apply.StartApply();
        const oc::apply::Status status = WaitTerminal([&] { return apply.CurrentStatus(); });
        CHECK(status.job_id == started.job_id);
        CHECK(status.state == "failed");
        CHECK(status.error.find("network restart failed:") == 0);
        CHECK(status.error.find("exit code 1") != std::string::npos);

        const auto calls = processes.Calls();
        REQUIRE(calls.size() == 1);
        CHECK(calls[0].command.executable == "/etc/init.d/S40network");
    }

    TEST_CASE("concurrent apply requests keep the in-flight job") {
        NullLogger logger;
        oc::process::FakeRunner processes;
        std::atomic<bool> entered{false};
        std::atomic<bool> release{false};
        processes.SetHandler([&](const oc::process::Request &) {
            entered.store(true);
            while (!release.load()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
            return oc::process::Exited(0);
        });

        chime::webd::ApplyManager apply(logger, processes, Restart("true"), Restart("true"));
        const auto started = apply.StartApply();
        for (int i = 0; i < 50 && !entered.load(); ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        REQUIRE(entered.load());

        const auto ignored = apply.StartApply();
        CHECK(ignored.job_id == started.job_id);
        CHECK(ignored.state == "running");

        release.store(true);
        CHECK(WaitTerminal([&] { return apply.CurrentStatus(); }).state == "succeeded");
    }

    TEST_CASE("Stop cancels a running apply job and refuses later work") {
        NullLogger logger;
        oc::process::FakeRunner processes;
        std::atomic<bool> entered{false};
        std::atomic<bool> finished{false};
        processes.SetHandler([&](const oc::process::Request &request) {
            entered.store(true);
            while (!request.stop.stop_requested()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
            finished.store(true);
            oc::process::Result result;
            result.outcome = oc::process::Outcome::Cancelled;
            result.error = "cancelled";
            return result;
        });

        chime::webd::ApplyManager apply(logger, processes, Restart("sleep"), Restart("true"));
        const auto started = apply.StartApply();
        for (int i = 0; i < 50 && !entered.load(); ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        REQUIRE(entered.load());

        const auto stop_started = std::chrono::steady_clock::now();
        apply.Stop();
        CHECK(std::chrono::steady_clock::now() - stop_started < std::chrono::seconds(1));
        CHECK(finished.load());

        const oc::apply::Status status = apply.CurrentStatus();
        CHECK(status.job_id == started.job_id);
        CHECK(status.state == "failed");
        CHECK(status.error.find("cancelled") != std::string::npos);
        CHECK_FALSE(status.finished_at_utc.empty());
        CHECK(apply.StartApply().job_id == started.job_id);
        CHECK(processes.Calls().size() == 1);
    }
}
