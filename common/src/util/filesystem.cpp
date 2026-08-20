#include "oc/util/filesystem.h"

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <unistd.h>
#include <vector>

#include "oc/config/kv_config.h"

namespace oc::util {
namespace {

std::filesystem::path ResolveAtomicWritePath(const std::string &path) {
    const std::filesystem::path input(path);
    std::error_code ec;
    if (!std::filesystem::is_symlink(input, ec) || ec) {
        return input;
    }
    std::filesystem::path target = std::filesystem::read_symlink(input, ec);
    if (ec) {
        return input;
    }
    if (target.is_relative()) {
        target = input.parent_path() / target;
    }
    return target.lexically_normal();
}

void FsyncDirectory(const std::filesystem::path &directory) {
    const int dir_fd = open(directory.c_str(), O_RDONLY | O_DIRECTORY);
    if (dir_fd < 0) {
        return;
    }
    fsync(dir_fd);
    close(dir_fd);
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

    const std::filesystem::path target = ResolveAtomicWritePath(path);
    const std::filesystem::path directory =
        target.parent_path().empty() ? std::filesystem::path(".") : target.parent_path();
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
        *error = "mkstemp failed for '" + target.string() + "': " + std::strerror(errno);
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

    if (rename(buffer.data(), target.c_str()) != 0) {
        *error = "rename failed for '" + target.string() + "': " + std::strerror(errno);
        std::remove(buffer.data());
        return false;
    }

    FsyncDirectory(directory);
    return true;
}

} // namespace oc::util
