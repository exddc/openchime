#ifndef OC_APPLY_JOB_RUNNER_H
#define OC_APPLY_JOB_RUNNER_H

#include <chrono>
#include <functional>
#include <mutex>
#include <stop_token>
#include <string>
#include <thread>
#include <vector>

#include "oc/process/runner.h"

namespace oc::logging {
class Logger;
}

namespace oc::apply {

struct Status {
    unsigned long long job_id = 0;
    std::string state = "idle";
    std::string started_at_utc;
    std::string finished_at_utc;
    std::string error;
};

struct StepContext {
    oc::process::Runner &runner;
    std::stop_token stop;
};

struct Step {
    std::string name;
    std::function<bool(const StepContext &context, std::string *error)> run;
};

class ProductApply {
  public:
    virtual ~ProductApply() = default;
    virtual std::vector<Step> Steps() const = 0;
};

Step ArgvCommand(std::string name, std::string executable, std::vector<std::string> arguments,
                 std::chrono::milliseconds timeout = oc::process::kDefaultTimeout);

class JobRunner {
  public:
    JobRunner(oc::logging::Logger &logger, oc::process::Runner &process_runner, std::string log_component = "apply");
    ~JobRunner();

    JobRunner(const JobRunner &) = delete;
    JobRunner &operator=(const JobRunner &) = delete;
    JobRunner(JobRunner &&) = delete;
    JobRunner &operator=(JobRunner &&) = delete;

    Status Start(std::vector<Step> steps);
    Status Start(const ProductApply &product);
    Status Current() const;
    void Stop();

  private:
    void RunJob(std::stop_token stop, unsigned long long job_id, std::vector<Step> steps);
    void FinishLocked(const std::string &state, const std::string &error);
    bool CancelIfStopping(std::stop_token stop, unsigned long long job_id);

    oc::logging::Logger &logger_;
    oc::process::Runner &process_runner_;
    std::string log_component_;
    std::mutex lifecycle_mutex_;
    mutable std::mutex mutex_;
    Status status_;
    unsigned long long next_job_id_ = 1;
    bool accepting_ = true;
    std::jthread worker_;
};

} // namespace oc::apply

#endif
