#include "oc/apply/job_runner.h"

#include <chrono>
#include <ctime>
#include <exception>
#include <iomanip>
#include <sstream>
#include <utility>

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

} // namespace

Step ArgvCommand(std::string name, std::string executable, std::vector<std::string> arguments,
                 std::chrono::milliseconds timeout) {
    Step step;
    step.name = std::move(name);
    step.run = [executable = std::move(executable), arguments = std::move(arguments),
                timeout](const StepContext &context, std::string *error) {
        if (context.stop.stop_requested()) {
            if (error != nullptr) {
                *error = "cancelled";
            }
            return false;
        }

        oc::process::Request request;
        request.command.executable = executable;
        request.command.arguments = arguments;
        request.timeout = timeout;
        request.stop = context.stop;
        const oc::process::Result result = context.runner.Run(request);
        if (oc::process::Succeeded(result)) {
            return true;
        }
        if (error != nullptr) {
            *error = oc::process::Describe(result);
        }
        return false;
    };
    return step;
}

JobRunner::JobRunner(oc::logging::Logger &logger, oc::process::Runner &process_runner, std::string log_component)
    : logger_(logger), process_runner_(process_runner), log_component_(std::move(log_component)) {
    status_.state = "idle";
}

JobRunner::~JobRunner() {
    Stop();
}

Status JobRunner::Start(const ProductApply &product) {
    return Start(product.Steps());
}

Status JobRunner::Start(std::vector<Step> steps) {
    std::lock_guard<std::mutex> lifecycle_lock(lifecycle_mutex_);
    for (;;) {
        std::jthread previous;
        std::thread::id previous_id;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!accepting_ || status_.state == "pending" || status_.state == "running") {
                return status_;
            }
            if (worker_.joinable()) {
                previous = std::move(worker_);
                previous_id = worker_thread_id_;
            } else {
                status_.job_id = next_job_id_++;
                status_.state = "pending";
                status_.error.clear();
                status_.started_at_utc = NowIso8601Utc();
                status_.finished_at_utc.clear();
                const Status started = status_;
                const unsigned long long job_id = started.job_id;
                try {
                    worker_ = std::jthread([this, job_id, steps = std::move(steps)](std::stop_token stop) {
                        RunJob(std::move(stop), job_id, steps);
                    });
                    worker_thread_id_ = worker_.get_id();
                } catch (const std::exception &ex) {
                    FinishLocked("failed", std::string("failed to start worker: ") + ex.what());
                    return status_;
                } catch (...) {
                    FinishLocked("failed", "failed to start worker");
                    return status_;
                }
                return started;
            }
        }
        if (previous.joinable()) {
            previous.join();
            std::lock_guard<std::mutex> lock(mutex_);
            if (worker_thread_id_ == previous_id) {
                worker_thread_id_ = {};
            }
        }
    }
}

Status JobRunner::Current() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return status_;
}

void JobRunner::Stop() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        accepting_ = false;
        if (worker_thread_id_ == std::this_thread::get_id()) {
            if (worker_.joinable()) {
                worker_.request_stop();
            }
            return;
        }
    }

    std::lock_guard<std::mutex> lifecycle_lock(lifecycle_mutex_);
    std::jthread to_join;
    std::thread::id joining_id;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (worker_.joinable()) {
            worker_.request_stop();
            joining_id = worker_thread_id_;
            to_join = std::move(worker_);
        }
    }
    if (to_join.joinable()) {
        to_join.join();
    }
    std::lock_guard<std::mutex> lock(mutex_);
    if (worker_thread_id_ == joining_id) {
        worker_thread_id_ = {};
    }
    if (status_.state == "pending" || status_.state == "running") {
        FinishLocked("failed", "cancelled");
        logger_.Error(log_component_, "apply job cancelled id=" + std::to_string(status_.job_id));
    }
}

void JobRunner::FinishLocked(const std::string &state, const std::string &error) {
    status_.state = state;
    status_.finished_at_utc = NowIso8601Utc();
    status_.error = error;
}

bool JobRunner::CancelIfStopping(std::stop_token stop, unsigned long long job_id) {
    if (!stop.stop_requested()) {
        return false;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    if (status_.job_id != job_id) {
        return true;
    }
    if (status_.state != "pending" && status_.state != "running") {
        return true;
    }
    FinishLocked("failed", "cancelled");
    logger_.Error(log_component_, "apply job cancelled id=" + std::to_string(job_id));
    return true;
}

void JobRunner::RunJob(std::stop_token stop, unsigned long long job_id, std::vector<Step> steps) {
    if (CancelIfStopping(stop, job_id)) {
        return;
    }
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
        if (CancelIfStopping(stop, job_id)) {
            return;
        }

        std::string error;
        bool ok = false;
        try {
            const StepContext context{process_runner_, stop};
            ok = static_cast<bool>(step.run) && step.run(context, &error);
        } catch (const std::exception &ex) {
            error = ex.what();
        } catch (...) {
            error = "unknown exception";
        }
        if (ok) {
            continue;
        }
        std::lock_guard<std::mutex> lock(mutex_);
        FinishLocked("failed", step.name + " failed: " + error);
        logger_.Error(log_component_,
                      "apply job failed id=" + std::to_string(job_id) + " error='" + status_.error + "'");
        return;
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (stop.stop_requested()) {
            FinishLocked("failed", "cancelled");
            return;
        }
        FinishLocked("succeeded", "");
    }

    logger_.Info(log_component_, "apply job succeeded id=" + std::to_string(job_id));
}

} // namespace oc::apply
