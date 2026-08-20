#include "oc/apply/job_runner.h"

#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <utility>

#include <sys/wait.h>

#include "oc/logging/logger.h"

namespace oc::apply {
namespace {

std::string NowIso8601Utc() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t now_time = std::chrono::system_clock::to_time_t(now);

    std::tm utc_tm{};
#if defined(_WIN32)
    gmtime_s(&utc_tm, &now_time);
#else
    gmtime_r(&now_time, &utc_tm);
#endif

    std::ostringstream out;
    out << std::put_time(&utc_tm, "%Y-%m-%dT%H:%M:%SZ");
    return out.str();
}

bool RunShell(const std::string &command, std::string *error) {
    const int rc = std::system(command.c_str());
    if (rc == 0) {
        return true;
    }

    if (error == nullptr) {
        return false;
    }

    if (rc < 0) {
        *error = "system() failed";
        return false;
    }

    if (WIFEXITED(rc)) {
        *error = "exit code " + std::to_string(WEXITSTATUS(rc));
        return false;
    }

    if (WIFSIGNALED(rc)) {
        *error = "signal " + std::to_string(WTERMSIG(rc));
        return false;
    }

    *error = "unknown failure";
    return false;
}

} // namespace

Step ShellCommand(std::string name, std::string command) {
    Step step;
    step.name = std::move(name);
    step.run = [command = std::move(command)](std::string *error) { return RunShell(command, error); };
    return step;
}

JobRunner::JobRunner(oc::logging::Logger &logger, std::string log_component)
    : logger_(logger), log_component_(std::move(log_component)) {
    status_.state = "idle";
}

Status JobRunner::Start(const ProductApply &product) {
    return Start(product.Steps());
}

Status JobRunner::Start(std::vector<Step> steps) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (status_.state == "pending" || status_.state == "running") {
        return status_;
    }

    status_.job_id = next_job_id_.fetch_add(1, std::memory_order_relaxed);
    status_.state = "pending";
    status_.error.clear();
    status_.started_at_utc = NowIso8601Utc();
    status_.finished_at_utc.clear();
    const Status started = status_;
    const unsigned long long job_id = started.job_id;
    worker_ = std::jthread([this, job_id, steps = std::move(steps)]() { RunJob(job_id, steps); });
    return started;
}

Status JobRunner::Current() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return status_;
}

void JobRunner::RunJob(unsigned long long job_id, std::vector<Step> steps) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (status_.job_id != job_id) {
            return;
        }
        status_.state = "running";
        status_.started_at_utc = NowIso8601Utc();
    }

    logger_.Info(log_component_, "apply job started id=" + std::to_string(job_id));

    for (const auto &step : steps) {
        std::string error;
        if (step.run && step.run(&error)) {
            continue;
        }
        std::lock_guard<std::mutex> lock(mutex_);
        status_.state = "failed";
        status_.finished_at_utc = NowIso8601Utc();
        status_.error = step.name + " failed: " + error;
        logger_.Error(log_component_,
                      "apply job failed id=" + std::to_string(job_id) + " error='" + status_.error + "'");
        return;
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        status_.state = "succeeded";
        status_.finished_at_utc = NowIso8601Utc();
        status_.error.clear();
    }

    logger_.Info(log_component_, "apply job succeeded id=" + std::to_string(job_id));
}

} // namespace oc::apply
