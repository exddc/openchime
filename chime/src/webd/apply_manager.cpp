#include "chime/webd_apply_manager.h"

#include <stdexcept>
#include <utility>

namespace chime::webd {
namespace {

oc::process::Command ParseOrThrow(const char *label, const std::string &spec) {
    oc::process::Command command;
    std::string error;
    if (!oc::process::ParseCommand(spec, &command, &error)) {
        throw std::invalid_argument(std::string(label) + ": " + error);
    }
    return command;
}

} // namespace

ApplyManager::ApplyManager(oc::logging::Logger &logger, oc::process::Runner &process_runner,
                           oc::process::Command network_restart, oc::process::Command chime_restart)
    : runner_(logger, process_runner, "webd"), network_restart_(std::move(network_restart)),
      chime_restart_(std::move(chime_restart)) {
    if (network_restart_.executable.empty() || chime_restart_.executable.empty()) {
        throw std::invalid_argument("restart command executable is empty");
    }
}

ApplyManager::ApplyManager(oc::logging::Logger &logger, oc::process::Runner &process_runner,
                           std::string network_restart_command, std::string chime_restart_command)
    : ApplyManager(logger, process_runner, ParseOrThrow("network restart", network_restart_command),
                   ParseOrThrow("chime restart", chime_restart_command)) {}

std::vector<oc::apply::Step> ApplyManager::Steps() const {
    return {
        oc::apply::ArgvCommand("network restart", network_restart_.executable, network_restart_.arguments),
        oc::apply::ArgvCommand("chime restart", chime_restart_.executable, chime_restart_.arguments),
    };
}

oc::apply::Status ApplyManager::StartApply() {
    return runner_.Start(*this);
}

oc::apply::Status ApplyManager::CurrentStatus() const {
    return runner_.Current();
}

void ApplyManager::Stop() {
    runner_.Stop();
}

} // namespace chime::webd
