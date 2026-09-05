#ifndef OC_PROCESS_POSIX_RUNNER_H
#define OC_PROCESS_POSIX_RUNNER_H

#include "oc/process/runner.h"

namespace oc::process {

class PosixRunner final : public Runner {
  public:
    Result Run(const Request &request) override;
};

} // namespace oc::process

#endif
