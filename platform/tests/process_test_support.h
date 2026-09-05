#ifndef OC_PROCESS_TEST_SUPPORT_H
#define OC_PROCESS_TEST_SUPPORT_H

#include <chrono>
#include <functional>
#include <thread>

#include "oc/apply/job_runner.h"

inline oc::apply::Status WaitTerminal(const std::function<oc::apply::Status()> &poll) {
    oc::apply::Status status;
    for (int i = 0; i < 100; ++i) {
        status = poll();
        if (status.state == "succeeded" || status.state == "failed") {
            return status;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return status;
}

inline oc::apply::Status WaitTerminal(oc::apply::JobRunner &runner) {
    return WaitTerminal([&] { return runner.Current(); });
}

#endif
