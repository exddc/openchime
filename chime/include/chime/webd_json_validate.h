#ifndef CHIME_WEBD_JSON_VALIDATE_H
#define CHIME_WEBD_JSON_VALIDATE_H

#include <optional>
#include <string>
#include <vector>

#include "chime/webd_json.h"
#include "chime/webd_types.h"

namespace chime::webd {

std::optional<std::string> ReadRequiredString(const JsonValue &object, const std::string &key,
                                              std::vector<ValidationError> *errors);
std::optional<int> ReadRequiredInt(const JsonValue &object, const std::string &key,
                                   std::vector<ValidationError> *errors);
std::optional<bool> ReadRequiredBool(const JsonValue &object, const std::string &key,
                                     std::vector<ValidationError> *errors);
std::optional<std::vector<std::string>> ReadRequiredStringArray(const JsonValue &object, const std::string &key,
                                                                std::vector<ValidationError> *errors);
std::optional<std::string> ReadOptionalString(const JsonValue &object, const std::string &key,
                                              std::vector<ValidationError> *errors);

} // namespace chime::webd

#endif
