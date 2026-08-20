#include <atomic>
#include <chrono>
#include <string>
#include <thread>
#include <vector>

#include "doctest.h"
#include "oc/apply/job_runner.h"
#include "test_support.h"

TEST_SUITE("apply_job") {
    TEST_CASE("runs product steps in order and records success") {
        NullLogger logger;
        oc::apply::JobRunner runner(logger, "test");
        const auto started = runner.Start({
            oc::apply::ShellCommand("first", "true"),
            oc::apply::ShellCommand("second", "true"),
        });
        CHECK(started.job_id == 1);
        CHECK(started.state == "pending");

        oc::apply::Status status;
        for (int i = 0; i < 50; ++i) {
            status = runner.Current();
            if (status.state == "succeeded" || status.state == "failed") {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
        CHECK(status.state == "succeeded");
        CHECK(status.error.empty());
        CHECK_FALSE(status.finished_at_utc.empty());
    }

    TEST_CASE("keeps the in-flight job and records the failing step") {
        NullLogger logger;
        oc::apply::JobRunner runner(logger, "test");
        std::atomic<bool> release{false};
        const auto started = runner.Start({
            {"hold",
             [&release](std::string *) {
                 while (!release.load()) {
                     std::this_thread::sleep_for(std::chrono::milliseconds(5));
                 }
                 return true;
             }},
            oc::apply::ShellCommand("fail", "false"),
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

        const auto ignored = runner.Start({oc::apply::ShellCommand("other", "true")});
        CHECK(ignored.job_id == started.job_id);
        CHECK(ignored.state == "running");

        release.store(true);
        oc::apply::Status status;
        for (int i = 0; i < 50; ++i) {
            status = runner.Current();
            if (status.state == "failed" || status.state == "succeeded") {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
        CHECK(status.state == "failed");
        CHECK(status.error.find("fail failed:") == 0);
    }

    TEST_CASE("ProductApply supplies the step list") {
        NullLogger logger;
        oc::apply::JobRunner runner(logger, "test");

        class TrueApply final : public oc::apply::ProductApply {
          public:
            std::vector<oc::apply::Step> Steps() const override { return {oc::apply::ShellCommand("ok", "true")}; }
        } product;

        const auto started = runner.Start(product);
        CHECK(started.job_id == 1);
        oc::apply::Status status;
        for (int i = 0; i < 50; ++i) {
            status = runner.Current();
            if (status.state == "succeeded" || status.state == "failed") {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
        CHECK(status.state == "succeeded");
    }
}
