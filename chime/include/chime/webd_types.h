#ifndef CHIME_WEBD_TYPES_H
#define CHIME_WEBD_TYPES_H

#include <optional>
#include <string>
#include <vector>

#include "chime/generated/config_types.h"
#include "oc/apply/job_runner.h"
#include "oc/config/validation.h"

namespace chime::webd {

using ValidationError = oc::config::ValidationError;
using ApplyStatus = oc::apply::Status;

struct CoreConfigSnapshot {
    CoreConfig config;
    bool wifi_password_set = false;
    bool mqtt_password_set = false;
};

struct SaveRequest {
    CoreConfig config;
    std::optional<std::string> wifi_password;
    std::optional<std::string> mqtt_password;
};

struct SaveResult {
    bool success = false;
    std::string error;
    std::vector<ValidationError> validation_errors;
    CoreConfigSnapshot snapshot;
};

} // namespace chime::webd

#endif
