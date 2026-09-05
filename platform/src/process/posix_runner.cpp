#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "oc/process/posix_runner.h"

#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstring>
#include <fcntl.h>
#include <poll.h>
#include <spawn.h>
#include <string>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
#include <vector>

extern char **environ;

namespace oc::process {
namespace {

constexpr std::chrono::milliseconds kWaitQuantum{20};

class UniqueFd {
  public:
    UniqueFd() = default;
    explicit UniqueFd(int fd) : fd_(fd) {}
    ~UniqueFd() { Close(); }

    UniqueFd(const UniqueFd &) = delete;
    UniqueFd &operator=(const UniqueFd &) = delete;
    UniqueFd(UniqueFd &&other) noexcept : fd_(other.fd_) { other.fd_ = -1; }
    UniqueFd &operator=(UniqueFd &&other) noexcept {
        if (this != &other) {
            Close();
            fd_ = other.fd_;
            other.fd_ = -1;
        }
        return *this;
    }

    int get() const { return fd_; }
    bool valid() const { return fd_ >= 0; }

    void Close() {
        if (fd_ >= 0) {
            close(fd_);
            fd_ = -1;
        }
    }

  private:
    int fd_ = -1;
};

#if !defined(__linux__)
bool SetCloexecNonblock(int fd) {
    if (fcntl(fd, F_SETFD, FD_CLOEXEC) != 0) {
        return false;
    }
    const int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
        return false;
    }
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK) == 0;
}

#endif

bool CreatePipe(UniqueFd *read_end, UniqueFd *write_end) {
    int fds[2] = {-1, -1};
#if defined(__linux__)
    if (pipe2(fds, O_CLOEXEC | O_NONBLOCK) != 0) {
        return false;
    }
#else
    if (pipe(fds) != 0) {
        return false;
    }
    if (!SetCloexecNonblock(fds[0]) || !SetCloexecNonblock(fds[1])) {
        close(fds[0]);
        close(fds[1]);
        return false;
    }
#endif
    const int flags = fcntl(fds[1], F_GETFL, 0);
    if (flags < 0 || fcntl(fds[1], F_SETFL, flags & ~O_NONBLOCK) != 0) {
        close(fds[0]);
        close(fds[1]);
        return false;
    }
    *read_end = UniqueFd(fds[0]);
    *write_end = UniqueFd(fds[1]);
    return true;
}

Result SpawnFailed(const std::string &error) {
    Result result;
    result.outcome = Outcome::SpawnFailed;
    result.error = error;
    return result;
}

int PollTimeoutMs(std::chrono::milliseconds remaining) {
    if (remaining.count() <= 0) {
        return 0;
    }
    if (remaining > kWaitQuantum) {
        return static_cast<int>(kWaitQuantum.count());
    }
    return static_cast<int>(remaining.count());
}

std::chrono::milliseconds Until(std::chrono::steady_clock::time_point deadline) {
    const auto now = std::chrono::steady_clock::now();
    if (now >= deadline) {
        return std::chrono::milliseconds{0};
    }
    return std::chrono::duration_cast<std::chrono::milliseconds>(deadline - now);
}

void ConsumeOutput(UniqueFd &fd, std::string *out, bool *truncated, std::size_t max_bytes) {
    if (!fd.valid()) {
        return;
    }
    char buffer[4096];
    for (int reads = 0; reads < 64; ++reads) {
        const ssize_t n = read(fd.get(), buffer, sizeof(buffer));
        if (n > 0) {
            const auto bytes = static_cast<std::size_t>(n);
            if (out->size() >= max_bytes) {
                *truncated = true;
                continue;
            }
            const std::size_t room = max_bytes - out->size();
            const std::size_t take = bytes < room ? bytes : room;
            out->append(buffer, take);
            if (take < bytes) {
                *truncated = true;
            }
            continue;
        }
        if (n == 0) {
            fd.Close();
            return;
        }
        if (errno == EINTR) {
            continue;
        }
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return;
        }
        fd.Close();
        return;
    }
}

void SendSignal(pid_t pid, int sig) {
    kill(-pid, sig);
}

void KillGroupAndReap(pid_t pid, int *wait_status) {
    SendSignal(pid, SIGKILL);
    int status = 0;
    for (;;) {
        const pid_t waited = waitpid(pid, &status, 0);
        if (waited == pid) {
            if (wait_status != nullptr) {
                *wait_status = status;
            }
            return;
        }
        if (waited < 0 && errno == ECHILD) {
            return;
        }
        if (waited < 0 && errno != EINTR) {
            return;
        }
    }
}

} // namespace

Result PosixRunner::Run(const Request &request) {
    if (request.command.executable.empty()) {
        return SpawnFailed("empty executable");
    }
    if (request.timeout.count() <= 0) {
        return SpawnFailed("timeout must be positive");
    }

    if (request.command.executable.find('\0') != std::string::npos) {
        return SpawnFailed("executable contains NUL");
    }
    for (const auto &argument : request.command.arguments) {
        if (argument.find('\0') != std::string::npos) {
            return SpawnFailed("argument contains NUL");
        }
    }
    if (request.graceful_shutdown.count() < 0) {
        return SpawnFailed("graceful shutdown must be nonnegative");
    }
    if (request.stop.stop_requested()) {
        Result result;
        result.outcome = Outcome::Cancelled;
        return result;
    }

    UniqueFd dev_null(open("/dev/null", O_RDWR | O_CLOEXEC));
    if (!dev_null.valid()) {
        return SpawnFailed(std::string("failed to open /dev/null: ") + std::strerror(errno));
    }

    UniqueFd stdout_read;
    UniqueFd stdout_write;
    UniqueFd stderr_read;
    UniqueFd stderr_write;
    if (request.capture_stdout && !CreatePipe(&stdout_read, &stdout_write)) {
        return SpawnFailed(std::string("failed to create stdout pipe: ") + std::strerror(errno));
    }
    if (request.capture_stderr && !CreatePipe(&stderr_read, &stderr_write)) {
        return SpawnFailed(std::string("failed to create stderr pipe: ") + std::strerror(errno));
    }

    posix_spawn_file_actions_t actions;
    if (posix_spawn_file_actions_init(&actions) != 0) {
        return SpawnFailed("posix_spawn_file_actions_init failed");
    }

    auto add_dup2 = [&](int source, int target) {
        return posix_spawn_file_actions_adddup2(&actions, source, target) == 0;
    };
    auto add_close = [&](int fd) {
        if (fd < 0 || fd == STDIN_FILENO || fd == STDOUT_FILENO || fd == STDERR_FILENO) {
            return true;
        }
        return posix_spawn_file_actions_addclose(&actions, fd) == 0;
    };

    bool actions_ok = add_dup2(dev_null.get(), STDIN_FILENO);
    if (request.capture_stdout) {
        actions_ok = actions_ok && add_dup2(stdout_write.get(), STDOUT_FILENO);
        actions_ok = actions_ok && add_close(stdout_read.get());
        actions_ok = actions_ok && add_close(stdout_write.get());
    } else {
        actions_ok = actions_ok && add_dup2(dev_null.get(), STDOUT_FILENO);
    }
    if (request.capture_stderr) {
        actions_ok = actions_ok && add_dup2(stderr_write.get(), STDERR_FILENO);
        actions_ok = actions_ok && add_close(stderr_read.get());
        actions_ok = actions_ok && add_close(stderr_write.get());
    } else {
        actions_ok = actions_ok && add_dup2(dev_null.get(), STDERR_FILENO);
    }
    actions_ok = actions_ok && add_close(dev_null.get());

    posix_spawnattr_t attr;
    const bool attr_inited = posix_spawnattr_init(&attr) == 0;
    sigset_t mask;
    sigemptyset(&mask);
    sigset_t defaults;
    sigemptyset(&defaults);
    sigaddset(&defaults, SIGTERM);
    sigaddset(&defaults, SIGINT);
    sigaddset(&defaults, SIGPIPE);
    const bool attr_ok =
        attr_inited &&
        posix_spawnattr_setflags(&attr, POSIX_SPAWN_SETPGROUP | POSIX_SPAWN_SETSIGMASK | POSIX_SPAWN_SETSIGDEF) == 0 &&
        posix_spawnattr_setpgroup(&attr, 0) == 0 && posix_spawnattr_setsigmask(&attr, &mask) == 0 &&
        posix_spawnattr_setsigdefault(&attr, &defaults) == 0;

    if (!actions_ok || !attr_ok) {
        if (attr_inited) {
            posix_spawnattr_destroy(&attr);
        }
        posix_spawn_file_actions_destroy(&actions);
        return SpawnFailed("posix_spawn file actions or attributes failed");
    }

    std::vector<char *> argv;
    argv.reserve(request.command.arguments.size() + 2);
    argv.push_back(const_cast<char *>(request.command.executable.c_str()));
    for (const auto &argument : request.command.arguments) {
        argv.push_back(const_cast<char *>(argument.c_str()));
    }
    argv.push_back(nullptr);

    pid_t pid = -1;
    const int spawn_rc =
        request.command.executable.find('/') == std::string::npos
            ? posix_spawnp(&pid, request.command.executable.c_str(), &actions, &attr, argv.data(), environ)
            : posix_spawn(&pid, request.command.executable.c_str(), &actions, &attr, argv.data(), environ);
    posix_spawnattr_destroy(&attr);
    posix_spawn_file_actions_destroy(&actions);
    stdout_write.Close();
    stderr_write.Close();
    dev_null.Close();

    if (spawn_rc != 0) {
        return SpawnFailed(std::string("spawn failed: ") + std::strerror(spawn_rc));
    }

    Result result;
    const auto started = std::chrono::steady_clock::now();
    const auto timeout_at = started + request.timeout;
    const auto graceful = request.graceful_shutdown;

    enum class StopReason { None, Timeout, Cancel };
    StopReason stop_reason = StopReason::None;
    bool sent_term = false;
    bool sent_kill = false;
    auto term_deadline = std::chrono::steady_clock::time_point::max();
    int wait_status = 0;
    bool reaped = false;

    auto consume = [&]() {
        ConsumeOutput(stdout_read, &result.stdout_data, &result.stdout_truncated, request.max_output_bytes);
        ConsumeOutput(stderr_read, &result.stderr_data, &result.stderr_truncated, request.max_output_bytes);
    };

    while (!reaped || (stop_reason != StopReason::None && !sent_kill)) {
        const auto now = std::chrono::steady_clock::now();
        if (stop_reason == StopReason::None) {
            if (request.stop.stop_requested()) {
                stop_reason = StopReason::Cancel;
            } else if (now >= timeout_at) {
                stop_reason = StopReason::Timeout;
            }
        }
        if (stop_reason != StopReason::None && !sent_term) {
            SendSignal(pid, SIGTERM);
            sent_term = true;
            term_deadline = now + graceful;
        }
        if (sent_term && !sent_kill && now >= term_deadline) {
            SendSignal(pid, SIGKILL);
            sent_kill = true;
        }

        consume();

        if (!reaped) {
            const pid_t waited = waitpid(pid, &wait_status, WNOHANG);
            if (waited == pid) {
                reaped = true;
                continue;
            }
            if (waited < 0) {
                if (errno == EINTR) {
                    continue;
                }
                const int wait_error = errno;
                KillGroupAndReap(pid, &wait_status);
                result.outcome = Outcome::SpawnFailed;
                result.error = std::string("waitpid failed: ") + std::strerror(wait_error);
                return result;
            }
        }

        if (reaped && (stop_reason == StopReason::None || sent_kill)) {
            break;
        }

        auto wake_in = kWaitQuantum;
        const auto until_timeout = Until(timeout_at);
        if (stop_reason == StopReason::None && until_timeout < wake_in) {
            wake_in = until_timeout;
        }
        if (sent_term && !sent_kill) {
            const auto until_kill = Until(term_deadline);
            if (until_kill < wake_in) {
                wake_in = until_kill;
            }
        }

        pollfd fds[2];
        nfds_t nfds = 0;
        if (stdout_read.valid()) {
            fds[nfds].fd = stdout_read.get();
            fds[nfds].events = POLLIN;
            fds[nfds].revents = 0;
            ++nfds;
        }
        if (stderr_read.valid()) {
            fds[nfds].fd = stderr_read.get();
            fds[nfds].events = POLLIN;
            fds[nfds].revents = 0;
            ++nfds;
        }
        if (nfds > 0) {
            poll(fds, nfds, PollTimeoutMs(wake_in));
        } else if (wake_in.count() > 0) {
            std::this_thread::sleep_for(wake_in);
        }
    }

    if (!reaped) {
        KillGroupAndReap(pid, &wait_status);
        reaped = true;
    }

    consume();

    if (stop_reason == StopReason::Timeout) {
        result.outcome = Outcome::TimedOut;
        result.error = "timed out";
        if (WIFSIGNALED(wait_status)) {
            result.terminating_signal = WTERMSIG(wait_status);
        } else if (WIFEXITED(wait_status)) {
            result.exit_code = WEXITSTATUS(wait_status);
        }
        return result;
    }
    if (stop_reason == StopReason::Cancel) {
        result.outcome = Outcome::Cancelled;
        result.error = "cancelled";
        if (WIFSIGNALED(wait_status)) {
            result.terminating_signal = WTERMSIG(wait_status);
        } else if (WIFEXITED(wait_status)) {
            result.exit_code = WEXITSTATUS(wait_status);
        }
        return result;
    }
    if (WIFEXITED(wait_status)) {
        result.outcome = Outcome::Exited;
        result.exit_code = WEXITSTATUS(wait_status);
        return result;
    }
    if (WIFSIGNALED(wait_status)) {
        result.outcome = Outcome::Signaled;
        result.terminating_signal = WTERMSIG(wait_status);
        result.error = "signal " + std::to_string(result.terminating_signal);
        return result;
    }

    result.outcome = Outcome::SpawnFailed;
    result.error = "unknown wait status";
    return result;
}

} // namespace oc::process
