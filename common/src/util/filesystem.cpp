#include "oc/util/filesystem.h"

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

#include "oc/config/kv_config.h"

namespace oc::util {
namespace {

bool WriteFullyFromOffsetZero(int fd, const std::string &content, std::string *error) {
    std::size_t offset = 0;
    while (offset < content.size()) {
        const ssize_t written =
            pwrite(fd, content.data() + offset, content.size() - offset, static_cast<off_t>(offset));
        if (written < 0) {
            if (errno == EINTR) {
                continue;
            }
            *error = "write failed: " + std::string(std::strerror(errno));
            return false;
        }
        offset += static_cast<std::size_t>(written);
    }
    if (ftruncate(fd, static_cast<off_t>(content.size())) != 0) {
        *error = "ftruncate failed: " + std::string(std::strerror(errno));
        return false;
    }
    if (fsync(fd) != 0) {
        *error = "fsync failed: " + std::string(std::strerror(errno));
        return false;
    }
    return true;
}

bool ParentOnSameDevice(const std::string &path) {
    struct stat file_stat;
    struct stat dir_stat;
    if (stat(path.c_str(), &file_stat) != 0) {
        return true;
    }
    const auto parent = std::filesystem::path(path).parent_path();
    if (parent.empty() || stat(parent.c_str(), &dir_stat) != 0) {
        return true;
    }
    return file_stat.st_dev == dir_stat.st_dev;
}

bool WriteInPlace(const std::string &path, const std::string &content, mode_t mode, std::string *error) {
    const int fd = open(path.c_str(), O_WRONLY);
    if (fd < 0) {
        *error = "failed to open '" + path + "' for in-place write: " + std::strerror(errno);
        return false;
    }
    if (fchmod(fd, mode) != 0) {
        *error = "fchmod failed: " + std::string(std::strerror(errno));
        close(fd);
        return false;
    }
    if (!WriteFullyFromOffsetZero(fd, content, error)) {
        close(fd);
        return false;
    }
    if (close(fd) != 0) {
        *error = "close failed: " + std::string(std::strerror(errno));
        return false;
    }
    return true;
}

} // namespace

bool FileExists(const std::string &path) {
    std::ifstream file(path);
    return file.good();
}

std::string ReadTrimmedFile(const std::string &path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return "";
    }
    std::string line;
    std::getline(file, line);
    return oc::config::trim(line);
}

bool AtomicWriteFile(const std::string &path, const std::string &content, mode_t mode, std::string *error) {
    if (error == nullptr) {
        return false;
    }

    if (std::filesystem::exists(path) && !ParentOnSameDevice(path)) {
        // File bind-mounts (S31persistent) live on a different device than their
        // parent directory. rename(2) cannot replace that mount point, and renaming
        // the backing file would leave the mount attached to the old inode.
        return WriteInPlace(path, content, mode, error);
    }

    std::filesystem::path target(path);
    const std::filesystem::path directory = target.parent_path();
    std::error_code ec;
    std::filesystem::create_directories(directory, ec);
    if (ec) {
        *error = "failed to create directory '" + directory.string() + "': " + ec.message();
        return false;
    }

    std::string template_path = (directory / (target.filename().string() + ".tmpXXXXXX")).string();
    std::vector<char> buffer(template_path.begin(), template_path.end());
    buffer.push_back('\0');

    const int fd = mkstemp(buffer.data());
    if (fd < 0) {
        *error = "mkstemp failed for '" + path + "': " + std::strerror(errno);
        return false;
    }

    if (fchmod(fd, mode) != 0) {
        *error = "fchmod failed for temp file: " + std::string(std::strerror(errno));
        close(fd);
        std::remove(buffer.data());
        return false;
    }

    std::size_t offset = 0;
    while (offset < content.size()) {
        const ssize_t written = write(fd, content.data() + offset, content.size() - offset);
        if (written < 0) {
            if (errno == EINTR) {
                continue;
            }
            *error = "write failed: " + std::string(std::strerror(errno));
            close(fd);
            std::remove(buffer.data());
            return false;
        }
        offset += static_cast<std::size_t>(written);
    }

    if (fsync(fd) != 0) {
        *error = "fsync failed: " + std::string(std::strerror(errno));
        close(fd);
        std::remove(buffer.data());
        return false;
    }

    if (close(fd) != 0) {
        *error = "close failed: " + std::string(std::strerror(errno));
        std::remove(buffer.data());
        return false;
    }

    if (rename(buffer.data(), path.c_str()) == 0) {
        return true;
    }

    const int rename_errno = errno;
    if (rename_errno == EXDEV || rename_errno == EBUSY) {
        const bool ok = WriteInPlace(path, content, mode, error);
        std::remove(buffer.data());
        return ok;
    }

    *error = "rename failed for '" + path + "': " + std::strerror(rename_errno);
    std::remove(buffer.data());
    return false;
}

} // namespace oc::util
