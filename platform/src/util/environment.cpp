#include "oc/util/environment.h"

#include <cstdlib>

namespace oc::util {

std::string GetEnv(const char *name) {
    const char *value = std::getenv(name);
    if (value == nullptr || value[0] == '\0') {
        return "";
    }
    return std::string(value);
}

} // namespace oc::util
