#ifndef OC_APPLY_JOB_RUNNER_H
#define OC_APPLY_JOB_RUNNER_H

#include <atomic>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

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

struct Step {
    std::string name;
    std::function<bool(std::string *error)> run;
};

class ProductApply {
  public:
    virtual ~ProductApply() = default;
    virtual std::vector<Step> Steps() const = 0;
};

Step ShellCommand(std::string name, std::string command);

class JobRunner {
  public:
    explicit JobRunner(oc::logging::Logger &logger, std::string log_component = "apply");
    JobRunner(const JobRunner &) = delete;
    JobRunner &operator=(const JobRunner &) = delete;

    Status Start(std::vector<Step> steps);
    Status Start(const ProductApply &product);
    Status Current() const;

  private:
    void RunJob(unsigned long long job_id, std::vector<Step> steps);

    oc::logging::Logger &logger_;
    std::string log_component_;
    mutable std::mutex mutex_;
    Status status_;
    std::atomic<unsigned long long> next_job_id_{1};
    std::jthread worker_;
};

} // namespace oc::apply

#endif
