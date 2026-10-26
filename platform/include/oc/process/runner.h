#ifndef OC_PROCESS_RUNNER_H
#define OC_PROCESS_RUNNER_H

#include <chrono>
#include <cstddef>
#include <stop_token>
#include <string>
#include <string_view>
#include <vector>

namespace oc::process {

inline constexpr std::chrono::milliseconds kDefaultTimeout{30000};
inline constexpr std::chrono::milliseconds kDefaultGracefulShutdown{2000};
inline constexpr std::size_t kDefaultMaxOutputBytes = 65536;

enum class Outcome {
    Exited,
    Signaled,
    TimedOut,
    Cancelled,
    SpawnFailed,
};

struct Command {
    std::string executable;
    std::vector<std::string> arguments;
};

struct Request {
    Command command;
    std::chrono::milliseconds timeout{kDefaultTimeout};
    // On timeout or cancellation, SIGTERM goes to the child's process group. SIGKILL follows when
    // this period ends or the child exits, whichever comes first, so descendants that outlive the
    // child lose the rest of their grace period.
    std::chrono::milliseconds graceful_shutdown{kDefaultGracefulShutdown};
    bool capture_stdout = false;
    bool capture_stderr = false;
    std::size_t max_output_bytes = kDefaultMaxOutputBytes;
    std::stop_token stop;
};

struct Result {
    Outcome outcome = Outcome::SpawnFailed;
    int exit_code = -1;
    int terminating_signal = 0;
    std::string stdout_data;
    std::string stderr_data;
    bool stdout_truncated = false;
    bool stderr_truncated = false;
    std::string error;
};

class Runner {
  public:
    virtual ~Runner() = default;
    virtual Result Run(const Request &request) = 0;
};

bool ParseCommand(std::string_view spec, Command *command, std::string *error);
bool Succeeded(const Result &result);
std::string Describe(const Result &result);

} // namespace oc::process

#endif
