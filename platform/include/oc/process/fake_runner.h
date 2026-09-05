#ifndef OC_PROCESS_FAKE_RUNNER_H
#define OC_PROCESS_FAKE_RUNNER_H

#include <functional>
#include <mutex>
#include <utility>
#include <vector>

#include "oc/process/runner.h"

namespace oc::process {

inline Result Exited(int exit_code) {
    Result result;
    result.outcome = Outcome::Exited;
    result.exit_code = exit_code;
    return result;
}

inline Result TimedOut() {
    Result result;
    result.outcome = Outcome::TimedOut;
    result.error = "timed out";
    return result;
}

class FakeRunner final : public Runner {
  public:
    Result Run(const Request &request) override {
        std::function<Result(const Request &)> handler;
        Result queued;
        bool have_queued = false;
        {
            const std::lock_guard<std::mutex> lock(mutex_);
            calls_.push_back(request);
            handler = handler_;
            if (!handler && !queued_.empty()) {
                queued = queued_.front();
                queued_.erase(queued_.begin());
                have_queued = true;
            }
        }
        if (handler) {
            return handler(request);
        }
        if (have_queued) {
            return queued;
        }

        const std::lock_guard<std::mutex> lock(mutex_);
        return default_result_;
    }

    std::vector<Request> Calls() const {
        const std::lock_guard<std::mutex> lock(mutex_);
        return calls_;
    }

    void Queue(Result result) {
        const std::lock_guard<std::mutex> lock(mutex_);
        queued_.push_back(std::move(result));
    }

    void SetDefault(Result result) {
        const std::lock_guard<std::mutex> lock(mutex_);
        default_result_ = std::move(result);
    }

    void SetHandler(std::function<Result(const Request &)> handler) {
        const std::lock_guard<std::mutex> lock(mutex_);
        handler_ = std::move(handler);
    }

  private:
    mutable std::mutex mutex_;
    std::vector<Request> calls_;
    std::vector<Result> queued_;
    Result default_result_{Exited(0)};
    std::function<Result(const Request &)> handler_;
};

} // namespace oc::process

#endif
