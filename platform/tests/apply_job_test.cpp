#include <atomic>
#include <chrono>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "doctest.h"
#include "oc/apply/job_runner.h"
#include "oc/process/fake_runner.h"
#include "process_test_support.h"
#include "test_support.h"

TEST_SUITE("apply_job") {
    TEST_CASE("runs product steps in order and records success") {
        NullLogger logger;
        oc::process::FakeRunner processes;
        oc::apply::JobRunner runner(logger, processes, "test");
        const auto started = runner.Start({
            oc::apply::ArgvCommand("first", "true", {}),
            oc::apply::ArgvCommand("second", "true", {}),
        });
        CHECK(started.job_id == 1);
        CHECK(started.state == "pending");

        const oc::apply::Status status = WaitTerminal(runner);
        CHECK(status.state == "succeeded");
        CHECK(status.error.empty());
        CHECK_FALSE(status.finished_at_utc.empty());

        const auto calls = processes.Calls();
        REQUIRE(calls.size() == 2);
        CHECK(calls[0].command.executable == "true");
        CHECK(calls[0].command.arguments.empty());
        CHECK(calls[0].timeout == oc::process::kDefaultTimeout);
        CHECK(calls[1].command.executable == "true");
        CHECK(calls[1].command.arguments.empty());
    }

    TEST_CASE("keeps the in-flight job and records the failing step") {
        NullLogger logger;
        oc::process::FakeRunner processes;
        processes.Queue(oc::process::Exited(1));
        oc::apply::JobRunner runner(logger, processes, "test");
        std::atomic<bool> release{false};
        const auto started = runner.Start({
            {"hold",
             [&release](const oc::apply::StepContext &, std::string *) {
                 while (!release.load()) {
                     std::this_thread::sleep_for(std::chrono::milliseconds(5));
                 }
                 return true;
             }},
            oc::apply::ArgvCommand("fail", "false", {}),
        });

        oc::apply::Status mid;
        for (int i = 0; i < 50; ++i) {
            mid = runner.Current();
            if (mid.state == "running") {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        CHECK(mid.state == "running");

        const auto ignored = runner.Start({oc::apply::ArgvCommand("other", "true", {})});
        CHECK(ignored.job_id == started.job_id);
        CHECK(ignored.state == "running");

        release.store(true);
        const oc::apply::Status status = WaitTerminal(runner);
        CHECK(status.state == "failed");
        CHECK(status.error.find("fail failed:") == 0);
        CHECK(status.error.find("exit code 1") != std::string::npos);

        const auto calls = processes.Calls();
        REQUIRE(calls.size() == 1);
        CHECK(calls[0].command.executable == "false");
    }

    TEST_CASE("ProductApply supplies the step list") {
        NullLogger logger;
        oc::process::FakeRunner processes;
        oc::apply::JobRunner runner(logger, processes, "test");

        class TrueApply final : public oc::apply::ProductApply {
          public:
            std::vector<oc::apply::Step> Steps() const override { return {oc::apply::ArgvCommand("ok", "true", {})}; }
        } product;

        const auto started = runner.Start(product);
        CHECK(started.job_id == 1);
        CHECK(WaitTerminal(runner).state == "succeeded");
        REQUIRE(processes.Calls().size() == 1);
        CHECK(processes.Calls()[0].command.executable == "true");
    }

    TEST_CASE("records a failed status when a product step throws") {
        NullLogger logger;
        oc::process::FakeRunner processes;
        oc::apply::JobRunner runner(logger, processes, "test");
        const auto started = runner.Start({
            {"explode",
             [](const oc::apply::StepContext &, std::string *) -> bool { throw std::runtime_error("callback boom"); }},
        });
        CHECK(started.job_id == 1);

        const oc::apply::Status status = WaitTerminal(runner);
        CHECK(status.state == "failed");
        CHECK(status.error.find("explode failed: callback boom") == 0);
        CHECK_FALSE(status.finished_at_utc.empty());
    }

    TEST_CASE("records a failed status when a product step throws an unknown exception") {
        NullLogger logger;
        oc::process::FakeRunner processes;
        oc::apply::JobRunner runner(logger, processes, "test");
        const auto started = runner.Start({
            {"explode", [](const oc::apply::StepContext &, std::string *) -> bool { throw 42; }},
        });
        CHECK(started.job_id == 1);

        const oc::apply::Status status = WaitTerminal(runner);
        CHECK(status.state == "failed");
        CHECK(status.error.find("explode failed: unknown exception") == 0);
    }

    TEST_CASE("Stop cancels an in-flight job and refuses later work") {
        NullLogger logger;
        oc::process::FakeRunner processes;
        std::atomic<bool> entered{false};
        processes.SetHandler([&](const oc::process::Request &request) {
            entered.store(true);
            while (!request.stop.stop_requested()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
            oc::process::Result result;
            result.outcome = oc::process::Outcome::Cancelled;
            result.error = "cancelled";
            return result;
        });

        oc::apply::JobRunner runner(logger, processes, "test");
        const auto started = runner.Start({oc::apply::ArgvCommand("hold", "sleep", {"30"})});
        for (int i = 0; i < 50 && !entered.load(); ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        REQUIRE(entered.load());

        runner.Stop();
        const oc::apply::Status status = runner.Current();
        CHECK(status.job_id == started.job_id);
        CHECK(status.state == "failed");
        CHECK(status.error.find("cancelled") != std::string::npos);
        CHECK_FALSE(status.finished_at_utc.empty());

        const auto refused = runner.Start({oc::apply::ArgvCommand("later", "true", {})});
        CHECK(refused.job_id == started.job_id);
        CHECK(refused.state == "failed");
    }

    TEST_CASE("Start overlapping Stop joins the worker and refuses later work") {
        NullLogger logger;
        oc::process::FakeRunner processes;
        processes.SetHandler([](const oc::process::Request &request) {
            while (!request.stop.stop_requested()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
            oc::process::Result result;
            result.outcome = oc::process::Outcome::Cancelled;
            result.error = "cancelled";
            return result;
        });

        for (int i = 0; i < 20; ++i) {
            oc::apply::JobRunner runner(logger, processes, "test");
            std::thread start_thread([&] { runner.Start({oc::apply::ArgvCommand("hold", "sleep", {"30"})}); });
            std::thread stop_thread([&] { runner.Stop(); });
            start_thread.join();
            stop_thread.join();

            const oc::apply::Status status = runner.Current();
            CHECK(status.state != "pending");
            CHECK(status.state != "running");

            const auto refused = runner.Start({oc::apply::ArgvCommand("later", "true", {})});
            CHECK(refused.state != "pending");
            CHECK(refused.state != "running");
        }
    }
}
