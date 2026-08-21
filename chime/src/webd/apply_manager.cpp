#include "chime/webd_apply_manager.h"

#include <utility>

namespace chime::webd {

ApplyManager::ApplyManager(oc::logging::Logger &logger, std::string network_restart_command,
                           std::string chime_restart_command)
    : runner_(logger, "webd"), network_restart_command_(std::move(network_restart_command)),
      chime_restart_command_(std::move(chime_restart_command)) {}

std::vector<oc::apply::Step> ApplyManager::Steps() const {
    return {
        oc::apply::ShellCommand("network restart", network_restart_command_),
        oc::apply::ShellCommand("chime restart", chime_restart_command_),
    };
}

oc::apply::Status ApplyManager::StartApply() {
    return runner_.Start(*this);
}

oc::apply::Status ApplyManager::CurrentStatus() const {
    return runner_.Current();
}

} // namespace chime::webd
