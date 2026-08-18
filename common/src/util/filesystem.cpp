#include "oc/util/filesystem.h"

#include <fstream>

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

} // namespace oc::util
