#ifndef OC_UTIL_STRINGS_H
#define OC_UTIL_STRINGS_H

#include <string>
#include <string_view>
#include <vector>

namespace oc::util {

std::string BoolToString(bool value);
std::string Join(const std::vector<std::string>& values, std::string_view separator);
std::string EscapeShellDoubleQuotes(const std::string& value);
std::string SanitizePayloadForLog(std::string_view payload);

}  // namespace oc::util

#endif
