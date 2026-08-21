#ifndef OC_CONFIG_VALIDATION_H
#define OC_CONFIG_VALIDATION_H

#include <string>

namespace oc::config {

struct ValidationError {
    std::string field;
    std::string message;
};

} // namespace oc::config

#endif
