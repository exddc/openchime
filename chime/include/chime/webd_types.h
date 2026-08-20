#ifndef CHIME_WEBD_TYPES_H
#define CHIME_WEBD_TYPES_H

#include <optional>
#include <string>
#include <vector>

#include "chime/generated/config_types.h"

namespace chime::webd {

struct ValidationError {
    std::string field;
    std::string message;
};

struct CoreConfigSnapshot {
    CoreConfig config;
    bool wifi_password_set = false;
    bool mqtt_password_set = false;
};

struct ApplyStatus {
    unsigned long long job_id = 0;
    std::string state = "idle";
    std::string started_at_utc;
    std::string finished_at_utc;
    std::string error;
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
