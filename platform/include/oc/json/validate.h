#ifndef OC_JSON_VALIDATE_H
#define OC_JSON_VALIDATE_H

#include <optional>
#include <string>
#include <vector>

#include "oc/config/validation.h"
#include "oc/json/json.h"

namespace oc::json {

std::optional<std::string> ReadRequiredString(const JsonValue &object, const std::string &key,
                                              std::vector<oc::config::ValidationError> *errors);
std::optional<int> ReadRequiredInt(const JsonValue &object, const std::string &key,
                                   std::vector<oc::config::ValidationError> *errors);
std::optional<bool> ReadRequiredBool(const JsonValue &object, const std::string &key,
                                     std::vector<oc::config::ValidationError> *errors);
std::optional<std::vector<std::string>> ReadRequiredStringArray(const JsonValue &object, const std::string &key,
                                                                std::vector<oc::config::ValidationError> *errors);
std::optional<std::string> ReadOptionalString(const JsonValue &object, const std::string &key,
                                              std::vector<oc::config::ValidationError> *errors);

} // namespace oc::json

#endif
