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

    if (rename(buffer.data(), path.c_str()) != 0) {
        *error = "rename failed for '" + path + "': " + std::strerror(errno);
        std::remove(buffer.data());
        return false;
    }

    return true;
}

} // namespace oc::util
