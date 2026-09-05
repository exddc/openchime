#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

#include <unistd.h>

#include "doctest.h"
#include "oc/process/posix_runner.h"
#include "test_support.h"

#ifndef OC_PROCESS_HELPER
#error "OC_PROCESS_HELPER is required"
#endif

namespace {

oc::process::Request Helper(std::vector<std::string> arguments) {
    oc::process::Request request;
    request.command.executable = OC_PROCESS_HELPER;
    request.command.arguments = std::move(arguments);
    return request;
}

#if defined(__linux__)
bool IsExitedOrZombie(pid_t pid) {
    std::ifstream stat("/proc/" + std::to_string(pid) + "/stat");
    if (!stat.is_open()) {
        return true;
    }
    std::string line;
    std::getline(stat, line);
    const auto name_end = line.rfind(')');
    return name_end != std::string::npos && name_end + 2 < line.size() && line[name_end + 2] == 'Z';
}
#endif

} // namespace

TEST_SUITE("process_runner") {
    TEST_CASE("pre-cancelled work never spawns") {
        oc::process::PosixRunner runner;
        auto request = Helper({"exit", "0"});
        request.command.executable = "/no/such/executable";
        std::stop_source stop;
        stop.request_stop();
        request.stop = stop.get_token();
        CHECK(runner.Run(request).outcome == oc::process::Outcome::Cancelled);
    }

    TEST_CASE("large output drains both streams without breaking child writes") {
        oc::process::PosixRunner runner;
        auto request = Helper({"write-both", "1000000"});
        request.capture_stdout = true;
        request.capture_stderr = true;
        request.max_output_bytes = 4096;
        auto result = runner.Run(request);
        CHECK(result.outcome == oc::process::Outcome::Exited);
        CHECK(result.exit_code == 0);
        CHECK(result.stdout_data.size() == 4096);
        CHECK(result.stderr_data.size() == 4096);
        CHECK(result.stdout_truncated);
        CHECK(result.stderr_truncated);
    }

    TEST_CASE("continuous output cannot starve timeout") {
        oc::process::PosixRunner runner;
        auto request = Helper({"flood"});
        request.capture_stdout = true;
        request.max_output_bytes = 0;
        request.timeout = std::chrono::milliseconds(100);
        request.graceful_shutdown = std::chrono::milliseconds(50);
        const auto started = std::chrono::steady_clock::now();
        const auto result = runner.Run(request);
        CHECK(result.outcome == oc::process::Outcome::TimedOut);
        CHECK(result.stdout_data.empty());
        CHECK(result.stdout_truncated);
        CHECK(std::chrono::steady_clock::now() - started < std::chrono::seconds(2));
    }

    TEST_CASE("ParseCommand keeps argv and rejects a shell") {
        oc::process::Command restart;
        std::string error;
        REQUIRE(oc::process::ParseCommand("/etc/init.d/S40network restart", &restart, &error));
        CHECK(restart.executable == "/etc/init.d/S40network");
        REQUIRE(restart.arguments.size() == 1);
        CHECK(restart.arguments[0] == "restart");

        oc::process::Command truth;
        REQUIRE(oc::process::ParseCommand("true", &truth, &error));
        CHECK(truth.executable == "true");
        CHECK(truth.arguments.empty());

        oc::process::Command empty;
        CHECK_FALSE(oc::process::ParseCommand("  ", &empty, &error));
        CHECK(error == "empty command");
        CHECK(empty.executable.empty());

        oc::process::Command redirected;
        CHECK_FALSE(oc::process::ParseCommand("/etc/init.d/S40network restart >/dev/null 2>&1", &redirected, &error));
        CHECK(error == "shell metacharacters are not supported");

        oc::process::Command chained;
        CHECK_FALSE(oc::process::ParseCommand("true && false", &chained, &error));
        CHECK(error == "shell metacharacters are not supported");
    }

    TEST_CASE("default timeout is 30s and zero is invalid") {
        oc::process::Request request;
        CHECK(request.timeout == oc::process::kDefaultTimeout);

        oc::process::PosixRunner runner;
        request.command.executable = OC_PROCESS_HELPER;
        request.command.arguments = {"exit", "0"};
        request.timeout = std::chrono::milliseconds{0};
        const auto result = runner.Run(request);
        CHECK(result.outcome == oc::process::Outcome::SpawnFailed);
        CHECK(result.error.find("timeout") != std::string::npos);
    }

    TEST_CASE("child is a process group leader") {
        oc::process::PosixRunner runner;
        auto request = Helper({"print-ids"});
        request.capture_stdout = true;
        const auto result = runner.Run(request);
        REQUIRE(result.outcome == oc::process::Outcome::Exited);
        int pid = 0;
        int pgid = 0;
        REQUIRE(std::sscanf(result.stdout_data.c_str(), "%d %d", &pid, &pgid) == 2);
        CHECK(pid > 0);
        CHECK(pgid == pid);
    }

    TEST_CASE("success, non-zero exit, missing executable") {
        oc::process::PosixRunner runner;

        const auto ok = runner.Run(Helper({"exit", "0"}));
        CHECK(ok.outcome == oc::process::Outcome::Exited);
        CHECK(ok.exit_code == 0);

        const auto failed = runner.Run(Helper({"exit", "3"}));
        CHECK(failed.outcome == oc::process::Outcome::Exited);
        CHECK(failed.exit_code == 3);
        CHECK(oc::process::Describe(failed) == "exit code 3");

        oc::process::Request missing;
        missing.command.executable = "/no/such/openchime-process-runner";
        const auto spawned = runner.Run(missing);
        CHECK(spawned.outcome == oc::process::Outcome::SpawnFailed);
        CHECK(spawned.error.find("spawn failed:") == 0);
    }

    TEST_CASE("timeout then hard kill of a SIGTERM-ignoring child") {
        oc::process::PosixRunner runner;
        auto request = Helper({"ignore-term"});
        request.timeout = std::chrono::milliseconds(200);
        request.graceful_shutdown = std::chrono::milliseconds(200);
        const auto result = runner.Run(request);
        CHECK(result.outcome == oc::process::Outcome::TimedOut);
        CHECK(result.error == "timed out");
    }

    TEST_CASE("timeout kills a forked grandchild") {
        const ScopedTempDir tmp;
        const auto pid_path = tmp.path() / "grandchild.pid";
        oc::process::PosixRunner runner;
        auto request = Helper({"fork-sleep", pid_path.string()});
        request.timeout = std::chrono::seconds(2);
        request.graceful_shutdown = std::chrono::milliseconds(200);
        const auto result = runner.Run(request);
        CHECK(result.outcome == oc::process::Outcome::TimedOut);

        std::ifstream in(pid_path);
        REQUIRE(in.is_open());
        int parent = 0;
        int parent_pgid = 0;
        int grandchild = 0;
        int grandchild_pgid = 0;
        REQUIRE(static_cast<bool>(in >> parent >> parent_pgid >> grandchild >> grandchild_pgid));
        REQUIRE(grandchild > 0);
        CHECK(parent_pgid == parent);
        CHECK(grandchild_pgid == parent);
#if defined(__linux__)
        CHECK(IsExitedOrZombie(grandchild));
#endif
    }

    TEST_CASE("cancellation stops a long child") {
        oc::process::PosixRunner runner;
        std::stop_source source;
        oc::process::Result result;
        std::jthread worker([&]() {
            auto request = Helper({"sleep-ms", "5000"});
            request.timeout = std::chrono::seconds(10);
            request.stop = source.get_token();
            result = runner.Run(request);
        });
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        source.request_stop();
        worker.join();
        CHECK(result.outcome == oc::process::Outcome::Cancelled);
        CHECK(result.error == "cancelled");
    }

    TEST_CASE("stdout capture is truncated at the configured bound") {
        oc::process::PosixRunner runner;
        auto request = Helper({"write-stdout", "10000"});
        request.capture_stdout = true;
        request.max_output_bytes = 1024;
        request.timeout = std::chrono::seconds(2);
        const auto result = runner.Run(request);
        CHECK(result.outcome == oc::process::Outcome::Exited);
        CHECK(result.exit_code == 0);
        CHECK(result.stdout_data.size() == 1024);
        CHECK(result.stdout_truncated);
    }

    TEST_CASE("self-termination is reported as a signal") {
        oc::process::PosixRunner runner;
        const auto result = runner.Run(Helper({"self-term"}));
        CHECK(result.outcome == oc::process::Outcome::Signaled);
        CHECK(result.terminating_signal == SIGTERM);
    }
}
