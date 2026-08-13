#ifndef CHIME_WEBD_APPLY_MANAGER_H
#define CHIME_WEBD_APPLY_MANAGER_H

#include <atomic>
#include <mutex>
#include <string>

#include "chime/webd_types.h"

namespace oc::logging {
class Logger;
}

namespace chime::webd {

class ApplyManager {
 public:
  ApplyManager(oc::logging::Logger& logger, std::string network_restart_command,
               std::string chime_restart_command);

  ApplyStatus StartApply();
  ApplyStatus CurrentStatus() const;

 private:
  void RunApplyJob(unsigned long long job_id);
  bool RunCommand(const std::string& command, std::string* error) const;

  oc::logging::Logger& logger_;
  std::string network_restart_command_;
  std::string chime_restart_command_;
  mutable std::mutex mutex_;
  ApplyStatus status_;
  std::atomic<unsigned long long> next_job_id_{1};
};

}  // namespace chime::webd

#endif
