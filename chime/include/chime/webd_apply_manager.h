#ifndef CHIME_WEBD_APPLY_MANAGER_H
#define CHIME_WEBD_APPLY_MANAGER_H

#include <string>
#include <vector>

#include "oc/apply/job_runner.h"
#include "oc/process/runner.h"

namespace oc::logging {
class Logger;
}

namespace chime::webd {

class ApplyManager final : public oc::apply::ProductApply {
  public:
    ApplyManager(oc::logging::Logger &logger, oc::process::Runner &process_runner, oc::process::Command network_restart,
                 oc::process::Command chime_restart);
    ApplyManager(const ApplyManager &) = delete;
    ApplyManager &operator=(const ApplyManager &) = delete;

    std::vector<oc::apply::Step> Steps() const override;
    oc::apply::Status StartApply();
    oc::apply::Status CurrentStatus() const;
    void Stop();

  private:
    oc::apply::JobRunner runner_;
    oc::process::Command network_restart_;
    oc::process::Command chime_restart_;
};

} // namespace chime::webd

#endif
