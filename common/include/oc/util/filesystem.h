#ifndef OC_UTIL_FILESYSTEM_H
#define OC_UTIL_FILESYSTEM_H

#include <string>

namespace oc::util {

bool FileExists(const std::string& path);
std::string ReadTrimmedFile(const std::string& path);

}  // namespace oc::util

#endif
