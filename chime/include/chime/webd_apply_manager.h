#ifndef CHIME_WEBD_APPLY_MANAGER_H
#define CHIME_WEBD_APPLY_MANAGER_H

#include <string>
#include <vector>

#include "oc/apply/job_runner.h"

namespace oc::logging {
class Logger;
}

namespace chime::webd {

class ApplyManager final : public oc::apply::ProductApply {
  public:
    ApplyManager(oc::logging::Logger &logger, std::string network_restart_command, std::string chime_restart_command);
    ApplyManager(const ApplyManager &) = delete;
    ApplyManager &operator=(const ApplyManager &) = delete;

    std::vector<oc::apply::Step> Steps() const override;
    oc::apply::Status StartApply();
    oc::apply::Status CurrentStatus() const;

  private:
    oc::apply::JobRunner runner_;
    std::string network_restart_command_;
    std::string chime_restart_command_;
};

} // namespace chime::webd

#endif
