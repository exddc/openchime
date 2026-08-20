#ifndef OC_UTIL_FILESYSTEM_H
#define OC_UTIL_FILESYSTEM_H

#include <string>
#include <sys/stat.h>

namespace oc::util {

bool FileExists(const std::string &path);
std::string ReadTrimmedFile(const std::string &path);
bool AtomicWriteFile(const std::string &path, const std::string &content, mode_t mode, std::string *error);

} // namespace oc::util

#endif
