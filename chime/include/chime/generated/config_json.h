#ifndef CHIME_GENERATED_CONFIG_JSON_H
#define CHIME_GENERATED_CONFIG_JSON_H

#include <map>
#include <string>
#include <vector>

#include "chime/generated/config_types.h"
#include "oc/json/json.h"
#include "oc/json/validate.h"
#include "chime/webd_types.h"

namespace chime::webd {
namespace generated_config_json {

using oc::json::JsonValue;
using oc::json::ReadOptionalString;
using oc::json::ReadRequiredBool;
using oc::json::ReadRequiredInt;
using oc::json::ReadRequiredString;
using oc::json::ReadRequiredStringArray;

inline JsonValue SerializeStringArray(const std::vector<std::string> &items) {
    std::vector<JsonValue> output;
    output.reserve(items.size());
    for (const auto &item : items) {
        output.push_back(JsonValue::String(item));
    }
    return JsonValue::Array(std::move(output));
}

inline std::map<std::string, JsonValue> CoreConfigFieldsToJson(const CoreConfig &config) {
    return {
        {"mqtt_host", JsonValue::String(config.mqtt_host)},
        {"mqtt_port", JsonValue::Number(static_cast<double>(config.mqtt_port))},
        {"mqtt_client_id", JsonValue::String(config.mqtt_client_id)},
        {"mqtt_username", JsonValue::String(config.mqtt_username)},
        {"mqtt_tls_enabled", JsonValue::Bool(config.mqtt_tls_enabled)},
        {"mqtt_tls_validate_certificate", JsonValue::Bool(config.mqtt_tls_validate_certificate)},
        {"mqtt_tls_ca_file", JsonValue::String(config.mqtt_tls_ca_file)},
        {"mqtt_tls_cert_file", JsonValue::String(config.mqtt_tls_cert_file)},
        {"mqtt_tls_key_file", JsonValue::String(config.mqtt_tls_key_file)},
        {"mqtt_topics", SerializeStringArray(config.mqtt_topics)},
        {"ring_topic", JsonValue::String(config.ring_topic)},
        {"notification_success_sound_path", JsonValue::String(config.notification_success_sound_path)},
        {"notification_failure_sound_path", JsonValue::String(config.notification_failure_sound_path)},
        {"volume_bell", JsonValue::Number(static_cast<double>(config.volume_bell))},
        {"volume_notifications", JsonValue::Number(static_cast<double>(config.volume_notifications))},
        {"wifi_ssid", JsonValue::String(config.wifi_ssid)},
    };
}

inline void ReadSaveRequestFromJson(const JsonValue &object, SaveRequest *request, std::vector<ValidationError> *errors) {
    if (request == nullptr) {
        return;
    }

    const auto mqtt_host = ReadRequiredString(object, "mqtt_host", errors);
    if (mqtt_host.has_value()) {
        request->config.mqtt_host = *mqtt_host;
    }

    const auto mqtt_port = ReadRequiredInt(object, "mqtt_port", errors);
    if (mqtt_port.has_value()) {
        request->config.mqtt_port = *mqtt_port;
    }

    const auto mqtt_client_id = ReadRequiredString(object, "mqtt_client_id", errors);
    if (mqtt_client_id.has_value()) {
        request->config.mqtt_client_id = *mqtt_client_id;
    }

    const auto mqtt_username = ReadRequiredString(object, "mqtt_username", errors);
    if (mqtt_username.has_value()) {
        request->config.mqtt_username = *mqtt_username;
    }

    const auto mqtt_password = ReadOptionalString(object, "mqtt_password", errors);
    if (mqtt_password.has_value()) {
        request->mqtt_password = mqtt_password;
    }

    const auto mqtt_tls_enabled = ReadRequiredBool(object, "mqtt_tls_enabled", errors);
    if (mqtt_tls_enabled.has_value()) {
        request->config.mqtt_tls_enabled = *mqtt_tls_enabled;
    }

    const auto mqtt_tls_validate_certificate = ReadRequiredBool(object, "mqtt_tls_validate_certificate", errors);
    if (mqtt_tls_validate_certificate.has_value()) {
        request->config.mqtt_tls_validate_certificate = *mqtt_tls_validate_certificate;
    }

    const auto mqtt_tls_ca_file = ReadRequiredString(object, "mqtt_tls_ca_file", errors);
    if (mqtt_tls_ca_file.has_value()) {
        request->config.mqtt_tls_ca_file = *mqtt_tls_ca_file;
    }

    const auto mqtt_tls_cert_file = ReadRequiredString(object, "mqtt_tls_cert_file", errors);
    if (mqtt_tls_cert_file.has_value()) {
        request->config.mqtt_tls_cert_file = *mqtt_tls_cert_file;
    }

    const auto mqtt_tls_key_file = ReadRequiredString(object, "mqtt_tls_key_file", errors);
    if (mqtt_tls_key_file.has_value()) {
        request->config.mqtt_tls_key_file = *mqtt_tls_key_file;
    }

    const auto mqtt_topics = ReadRequiredStringArray(object, "mqtt_topics", errors);
    if (mqtt_topics.has_value()) {
        request->config.mqtt_topics = *mqtt_topics;
    }

    const auto ring_topic = ReadRequiredString(object, "ring_topic", errors);
    if (ring_topic.has_value()) {
        request->config.ring_topic = *ring_topic;
    }

    const auto notification_success_sound_path = ReadOptionalString(object, "notification_success_sound_path", errors);
    if (notification_success_sound_path.has_value()) {
        request->config.notification_success_sound_path = *notification_success_sound_path;
    }

    const auto notification_failure_sound_path = ReadOptionalString(object, "notification_failure_sound_path", errors);
    if (notification_failure_sound_path.has_value()) {
        request->config.notification_failure_sound_path = *notification_failure_sound_path;
    }

    const auto volume_bell = ReadRequiredInt(object, "volume_bell", errors);
    if (volume_bell.has_value()) {
        request->config.volume_bell = *volume_bell;
    }

    const auto volume_notifications = ReadRequiredInt(object, "volume_notifications", errors);
    if (volume_notifications.has_value()) {
        request->config.volume_notifications = *volume_notifications;
    }

    const auto wifi_ssid = ReadRequiredString(object, "wifi_ssid", errors);
    if (wifi_ssid.has_value()) {
        request->config.wifi_ssid = *wifi_ssid;
    }

    const auto wifi_password = ReadOptionalString(object, "wifi_password", errors);
    if (wifi_password.has_value()) {
        request->wifi_password = wifi_password;
    }
}

inline bool ContainsWhitespace(const std::string &value) {
    return oc::config::contains_whitespace(value);
}

inline bool ContainsNewline(const std::string &value) {
    return oc::config::contains_newline(value);
}

inline void ValidateApiString(const ::chime::ConfigFieldSpec *spec, const std::string &value,
                              std::vector<ValidationError> *errors) {
    if (spec == nullptr || errors == nullptr) {
        return;
    }
    if (spec->api_required && !spec->api_empty_ok && value.empty()) {
        errors->push_back({spec->key, std::string(spec->key) + " is required"});
        return;
    }
    if (!oc::config::string_value_valid(value, spec->min_len, spec->max_len, spec->forbid_whitespace,
                                        spec->forbid_newline)) {
        if (ContainsNewline(value)) {
            errors->push_back({spec->key, std::string(spec->key) + " must not contain newline characters"});
            return;
        }
        if (spec->min_len > 0 && value.size() < static_cast<std::size_t>(spec->min_len)) {
            errors->push_back({spec->key, std::string(spec->key) + " must be >= " + std::to_string(spec->min_len) + " chars"});
            return;
        }
        if (spec->max_len > 0 && value.size() > static_cast<std::size_t>(spec->max_len)) {
            errors->push_back({spec->key, std::string(spec->key) + " must be <= " + std::to_string(spec->max_len) + " chars"});
            return;
        }
        if (spec->forbid_whitespace && ContainsWhitespace(value)) {
            errors->push_back({spec->key, std::string(spec->key) + " must not contain spaces"});
        }
    }
}

inline void ValidateApiInt(const ::chime::ConfigFieldSpec *spec, int value, std::vector<ValidationError> *errors) {
    if (spec == nullptr || errors == nullptr) {
        return;
    }
    if (value < spec->min_value || value > spec->max_value) {
        errors->push_back({spec->key, std::string(spec->key) + " must be " + std::to_string(spec->min_value) + "-" +
                                           std::to_string(spec->max_value)});
    }
}

inline void ValidateApiCsv(const ::chime::ConfigFieldSpec *spec, const std::vector<std::string> &items,
                           std::vector<ValidationError> *errors) {
    if (spec == nullptr || errors == nullptr) {
        return;
    }
    if (spec->api_required && items.empty()) {
        errors->push_back({spec->key, std::string(spec->key) + " must contain at least one topic"});
        return;
    }
    for (std::size_t i = 0; i < items.size(); ++i) {
        if (items[i].empty() || !oc::config::string_value_valid(items[i], spec->min_len, spec->max_len,
                                                                spec->forbid_whitespace, spec->forbid_newline)) {
            errors->push_back({spec->key, std::string(spec->key) + "[" + std::to_string(i) + "] is invalid"});
        }
    }
}

inline void ValidateSaveRequest(const SaveRequest &request, std::vector<ValidationError> *errors) {
    if (errors == nullptr) {
        return;
    }
    ValidateApiString(::chime::FindConfigField("mqtt_host"), request.config.mqtt_host, errors);
    ValidateApiInt(::chime::FindConfigField("mqtt_port"), request.config.mqtt_port, errors);
    ValidateApiString(::chime::FindConfigField("mqtt_client_id"), request.config.mqtt_client_id, errors);
    ValidateApiString(::chime::FindConfigField("mqtt_username"), request.config.mqtt_username, errors);
    if (request.mqtt_password.has_value()) {
        ValidateApiString(::chime::FindConfigField("mqtt_password"), *request.mqtt_password, errors);
    }
    ValidateApiString(::chime::FindConfigField("mqtt_tls_ca_file"), request.config.mqtt_tls_ca_file, errors);
    ValidateApiString(::chime::FindConfigField("mqtt_tls_cert_file"), request.config.mqtt_tls_cert_file, errors);
    ValidateApiString(::chime::FindConfigField("mqtt_tls_key_file"), request.config.mqtt_tls_key_file, errors);
    ValidateApiCsv(::chime::FindConfigField("mqtt_topics"), request.config.mqtt_topics, errors);
    ValidateApiString(::chime::FindConfigField("ring_topic"), request.config.ring_topic, errors);
    ValidateApiString(::chime::FindConfigField("notification_success_sound_path"), request.config.notification_success_sound_path, errors);
    ValidateApiString(::chime::FindConfigField("notification_failure_sound_path"), request.config.notification_failure_sound_path, errors);
    ValidateApiInt(::chime::FindConfigField("volume_bell"), request.config.volume_bell, errors);
    ValidateApiInt(::chime::FindConfigField("volume_notifications"), request.config.volume_notifications, errors);
    ValidateApiString(::chime::FindConfigField("wifi_ssid"), request.config.wifi_ssid, errors);
    if (request.wifi_password.has_value() && !request.wifi_password->empty()) {
        ValidateApiString(::chime::FindConfigField("wifi_password"), *request.wifi_password, errors);
    }
}

} // namespace generated_config_json
} // namespace chime::webd

#endif
