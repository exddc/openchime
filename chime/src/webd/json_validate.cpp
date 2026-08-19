#include "chime/webd_json_validate.h"

#include <cmath>
#include <limits>

namespace chime::webd {

std::optional<std::string> ReadRequiredString(const JsonValue &object, const std::string &key,
                                              std::vector<ValidationError> *errors) {
    const auto field = GetObjectField(object, key);
    if (!field.has_value()) {
        if (errors != nullptr) {
            errors->push_back({key, key + " is required"});
        }
        return std::nullopt;
    }

    std::string value;
    if (!field->AsString(&value)) {
        if (errors != nullptr) {
            errors->push_back({key, key + " must be a string"});
        }
        return std::nullopt;
    }

    return value;
}

std::optional<int> ReadRequiredInt(const JsonValue &object, const std::string &key,
                                   std::vector<ValidationError> *errors) {
    const auto field = GetObjectField(object, key);
    if (!field.has_value()) {
        if (errors != nullptr) {
            errors->push_back({key, key + " is required"});
        }
        return std::nullopt;
    }

    double value = 0.0;
    if (!field->AsNumber(&value)) {
        if (errors != nullptr) {
            errors->push_back({key, key + " must be a number"});
        }
        return std::nullopt;
    }

    if (!std::isfinite(value) || value < static_cast<double>(std::numeric_limits<int>::min()) ||
        value > static_cast<double>(std::numeric_limits<int>::max())) {
        if (errors != nullptr) {
            errors->push_back({key, key + " must be an integer"});
        }
        return std::nullopt;
    }

    const int rounded = static_cast<int>(value);
    if (static_cast<double>(rounded) != value) {
        if (errors != nullptr) {
            errors->push_back({key, key + " must be an integer"});
        }
        return std::nullopt;
    }

    return rounded;
}

std::optional<bool> ReadRequiredBool(const JsonValue &object, const std::string &key,
                                     std::vector<ValidationError> *errors) {
    const auto field = GetObjectField(object, key);
    if (!field.has_value()) {
        if (errors != nullptr) {
            errors->push_back({key, key + " is required"});
        }
        return std::nullopt;
    }

    bool value = false;
    if (!field->AsBool(&value)) {
        if (errors != nullptr) {
            errors->push_back({key, key + " must be a boolean"});
        }
        return std::nullopt;
    }

    return value;
}

std::optional<std::vector<std::string>> ReadRequiredStringArray(const JsonValue &object, const std::string &key,
                                                                std::vector<ValidationError> *errors) {
    const auto field = GetObjectField(object, key);
    if (!field.has_value()) {
        if (errors != nullptr) {
            errors->push_back({key, key + " is required"});
        }
        return std::nullopt;
    }

    std::vector<JsonValue> items;
    if (!field->AsArray(&items)) {
        if (errors != nullptr) {
            errors->push_back({key, key + " must be an array"});
        }
        return std::nullopt;
    }

    std::vector<std::string> output;
    output.reserve(items.size());
    for (std::size_t i = 0; i < items.size(); ++i) {
        std::string entry;
        if (!items[i].AsString(&entry)) {
            if (errors != nullptr) {
                errors->push_back({key, key + "[" + std::to_string(i) + "] must be a string"});
            }
            continue;
        }
        output.push_back(entry);
    }

    return output;
}

std::optional<std::string> ReadOptionalString(const JsonValue &object, const std::string &key,
                                              std::vector<ValidationError> *errors) {
    const auto field = GetObjectField(object, key);
    if (!field.has_value()) {
        return std::nullopt;
    }

    std::string value;
    if (!field->AsString(&value)) {
        if (errors != nullptr) {
            errors->push_back({key, key + " must be a string"});
        }
        return std::nullopt;
    }

    return value;
}

} // namespace chime::webd
