#include "oc/process/runner.h"

#include <cctype>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace oc::process {
namespace {

bool HasShellMetacharacter(std::string_view token) {
    return token.find_first_of("&|;$`<>(){}!#\"'\\") != std::string_view::npos;
}

} // namespace

bool ParseCommand(std::string_view spec, Command *command, std::string *error) {
    if (command == nullptr) {
        return false;
    }
    command->executable.clear();
    command->arguments.clear();

    std::vector<std::string> tokens;
    std::string current;
    for (const char c : spec) {
        if (std::isspace(static_cast<unsigned char>(c)) != 0) {
            if (!current.empty()) {
                tokens.push_back(std::move(current));
                current.clear();
            }
            continue;
        }
        current.push_back(c);
    }
    if (!current.empty()) {
        tokens.push_back(std::move(current));
    }

    if (tokens.empty()) {
        if (error != nullptr) {
            *error = "empty command";
        }
        return false;
    }

    for (const auto &token : tokens) {
        if (HasShellMetacharacter(token)) {
            if (error != nullptr) {
                *error = "shell metacharacters are not supported";
            }
            command->executable.clear();
            command->arguments.clear();
            return false;
        }
        if (command->executable.empty()) {
            command->executable = token;
        } else {
            command->arguments.push_back(token);
        }
    }

    if (command->executable.empty()) {
        if (error != nullptr) {
            *error = "empty command";
        }
        return false;
    }
    return true;
}

std::string Describe(const Result &result) {
    switch (result.outcome) {
    case Outcome::Exited:
        return "exit code " + std::to_string(result.exit_code);
    case Outcome::Signaled:
        return "signal " + std::to_string(result.terminating_signal);
    case Outcome::TimedOut:
        return "timed out";
    case Outcome::Cancelled:
        return "cancelled";
    case Outcome::SpawnFailed:
        return result.error.empty() ? "spawn failed" : result.error;
    }
    return "unknown failure";
}

} // namespace oc::process
