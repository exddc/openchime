#ifndef OC_WIFI_SCAN_COMMAND_H
#define OC_WIFI_SCAN_COMMAND_H

#include <cstddef>
#include <string>
#include <vector>

namespace oc::wifi::detail {

struct CommandResult {
    bool success = false;
    bool timed_out = false;
    int exit_code = -1;
    std::string output;
    std::string error;
};

CommandResult RunCommand(const std::vector<std::string> &args, int timeout_ms, std::size_t max_output_bytes);

} // namespace oc::wifi::detail

#endif
