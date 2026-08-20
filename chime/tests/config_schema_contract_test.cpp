#include <iterator>
#include <string>
#include <vector>

#include "chime/chime_config.h"
#include "chime/generated/config_json.h"
#include "chime/generated/config_types.h"
#include "chime/webd_types.h"
#include "doctest.h"

namespace {

bool HasFieldError(const std::vector<chime::webd::ValidationError> &errors, const std::string &field) {
    for (const auto &error : errors) {
        if (error.field == field) {
            return true;
        }
    }
    return false;
}

chime::webd::SaveRequest ValidApiSaveRequest() {
    chime::webd::SaveRequest request;
    request.config.wifi_ssid = "net";
    request.config.mqtt_host = "broker";
    request.config.mqtt_topics = {"doorbell/ring"};
    return request;
}

} // namespace

TEST_SUITE("config_schema_contract") {
    TEST_CASE("schema version matches generated constant") {
        CHECK(chime::kConfigSchemaVersion == 5);
        CHECK(chime::kLegacyUnversionedSchema == 4);
        CHECK(chime::FileConfig{}.schema_version == chime::kConfigSchemaVersion);
    }

    TEST_CASE("runtime, file, and API structs share defaults for overlapping fields") {
        const chime::ChimeConfig runtime;
        const chime::FileConfig file;
        const chime::webd::CoreConfig core;
        CHECK(runtime.mqtt_host == core.mqtt_host);
        CHECK(runtime.mqtt_host == file.mqtt_host);
        CHECK(runtime.mqtt_port == core.mqtt_port);
        CHECK(runtime.mqtt_port == file.mqtt_port);
        CHECK(runtime.mqtt_client_id == core.mqtt_client_id);
        CHECK(runtime.mqtt_client_id == file.mqtt_client_id);
        CHECK(runtime.mqtt_username == core.mqtt_username);
        CHECK(runtime.mqtt_username == file.mqtt_username);
        CHECK(runtime.mqtt_tls_enabled == core.mqtt_tls_enabled);
        CHECK(runtime.mqtt_tls_enabled == file.mqtt_tls_enabled);
        CHECK(runtime.mqtt_tls_validate_certificate == core.mqtt_tls_validate_certificate);
        CHECK(runtime.mqtt_tls_validate_certificate == file.mqtt_tls_validate_certificate);
        CHECK(runtime.mqtt_tls_ca_file == core.mqtt_tls_ca_file);
        CHECK(runtime.mqtt_tls_ca_file == file.mqtt_tls_ca_file);
        CHECK(runtime.mqtt_tls_cert_file == core.mqtt_tls_cert_file);
        CHECK(runtime.mqtt_tls_cert_file == file.mqtt_tls_cert_file);
        CHECK(runtime.mqtt_tls_key_file == core.mqtt_tls_key_file);
        CHECK(runtime.mqtt_tls_key_file == file.mqtt_tls_key_file);
        CHECK(runtime.mqtt_topics == core.mqtt_topics);
        CHECK(runtime.mqtt_topics == file.mqtt_topics);
        CHECK(runtime.ring_topic == core.ring_topic);
        CHECK(runtime.ring_topic == file.ring_topic);
        CHECK(runtime.notification_success_sound_path == core.notification_success_sound_path);
        CHECK(runtime.notification_success_sound_path == file.notification_success_sound_path);
        CHECK(runtime.notification_failure_sound_path == core.notification_failure_sound_path);
        CHECK(runtime.notification_failure_sound_path == file.notification_failure_sound_path);
        CHECK(runtime.volume_bell == core.volume_bell);
        CHECK(runtime.volume_bell == file.volume_bell);
        CHECK(runtime.volume_notifications == core.volume_notifications);
        CHECK(runtime.volume_notifications == file.volume_notifications);
    }

    TEST_CASE("removed keys are listed for migration") {
        bool found = false;
        for (const char *key : chime::kRemovedConfigKeys) {
            if (std::string(key) == "volume_other") {
                found = true;
            }
        }
        CHECK(found);
        CHECK(chime::FindConfigField("volume_other") == nullptr);
        CHECK(chime::FindConfigField("mqtt_host") != nullptr);
        CHECK(chime::FindConfigField("ntp_servers")->init_only);
        CHECK_FALSE(chime::FindConfigField("ntp_servers")->runtime);
        REQUIRE(std::size(chime::kConfigMigrationSteps) >= 1);
        CHECK(chime::kConfigMigrationSteps[0].to_version == 5);
        CHECK(chime::kConfigMigrationSteps[std::size(chime::kConfigMigrationSteps) - 1].to_version ==
              chime::kConfigSchemaVersion);
    }

    TEST_CASE("schema field specs expose API validation metadata") {
        const auto *host = chime::FindConfigField("mqtt_host");
        REQUIRE(host != nullptr);
        CHECK(host->api_required);
        CHECK_FALSE(host->api_empty_ok);
        CHECK(host->forbid_whitespace);
        CHECK(host->forbid_newline);
        CHECK(host->max_len == 256);

        const auto *volume = chime::FindConfigField("volume_bell");
        REQUIRE(volume != nullptr);
        CHECK(volume->min_value == 0);
        CHECK(volume->max_value == 100);
        CHECK(std::string(chime::FindConfigField("mqtt_topics")->repair_text) == "doorbell/ring,doorbell/status");
        CHECK(std::string(chime::FindConfigField("mqtt_topics")->default_text).empty());
    }

    TEST_CASE("schema constraints affect API validation") {
        const auto valid = ValidApiSaveRequest();
        auto request = valid;
        std::vector<chime::webd::ValidationError> errors;
        chime::webd::generated_config_json::ValidateSaveRequest(request, &errors);
        CHECK(errors.empty());

        request = valid;
        request.config.mqtt_host = "bad value";
        errors.clear();
        chime::webd::generated_config_json::ValidateSaveRequest(request, &errors);
        CHECK(HasFieldError(errors, "mqtt_host"));
        request = valid;
        request.config.mqtt_host = "";
        errors.clear();
        chime::webd::generated_config_json::ValidateSaveRequest(request, &errors);
        CHECK(HasFieldError(errors, "mqtt_host"));
        request = valid;
        request.config.mqtt_host = std::string(257, 'x');
        errors.clear();
        chime::webd::generated_config_json::ValidateSaveRequest(request, &errors);
        CHECK(HasFieldError(errors, "mqtt_host"));
        request = valid;
        request.config.mqtt_port = 65536;
        errors.clear();
        chime::webd::generated_config_json::ValidateSaveRequest(request, &errors);
        CHECK(HasFieldError(errors, "mqtt_port"));
        request = valid;
        request.config.mqtt_client_id = "";
        errors.clear();
        chime::webd::generated_config_json::ValidateSaveRequest(request, &errors);
        CHECK(HasFieldError(errors, "mqtt_client_id"));
        request = valid;
        request.config.mqtt_client_id = std::string(129, 'x');
        errors.clear();
        chime::webd::generated_config_json::ValidateSaveRequest(request, &errors);
        CHECK(HasFieldError(errors, "mqtt_client_id"));
        request = valid;
        request.config.mqtt_username = std::string(129, 'x');
        errors.clear();
        chime::webd::generated_config_json::ValidateSaveRequest(request, &errors);
        CHECK(HasFieldError(errors, "mqtt_username"));
        request = valid;
        request.config.mqtt_tls_ca_file = std::string(257, 'x');
        errors.clear();
        chime::webd::generated_config_json::ValidateSaveRequest(request, &errors);
        CHECK(HasFieldError(errors, "mqtt_tls_ca_file"));
        request = valid;
        request.config.mqtt_tls_cert_file = std::string(257, 'x');
        errors.clear();
        chime::webd::generated_config_json::ValidateSaveRequest(request, &errors);
        CHECK(HasFieldError(errors, "mqtt_tls_cert_file"));
        request = valid;
        request.config.mqtt_tls_key_file = std::string(257, 'x');
        errors.clear();
        chime::webd::generated_config_json::ValidateSaveRequest(request, &errors);
        CHECK(HasFieldError(errors, "mqtt_tls_key_file"));
        request = valid;
        request.config.mqtt_topics = {"bad topic"};
        errors.clear();
        chime::webd::generated_config_json::ValidateSaveRequest(request, &errors);
        CHECK(HasFieldError(errors, "mqtt_topics"));
        request = valid;
        request.config.ring_topic = "bad value";
        errors.clear();
        chime::webd::generated_config_json::ValidateSaveRequest(request, &errors);
        CHECK(HasFieldError(errors, "ring_topic"));
        request = valid;
        request.config.ring_topic = "";
        errors.clear();
        chime::webd::generated_config_json::ValidateSaveRequest(request, &errors);
        CHECK(HasFieldError(errors, "ring_topic"));
        request = valid;
        request.config.ring_topic = std::string(257, 'x');
        errors.clear();
        chime::webd::generated_config_json::ValidateSaveRequest(request, &errors);
        CHECK(HasFieldError(errors, "ring_topic"));
        request = valid;
        request.config.notification_success_sound_path = std::string(257, 'x');
        errors.clear();
        chime::webd::generated_config_json::ValidateSaveRequest(request, &errors);
        CHECK(HasFieldError(errors, "notification_success_sound_path"));
        request = valid;
        request.config.notification_failure_sound_path = std::string(257, 'x');
        errors.clear();
        chime::webd::generated_config_json::ValidateSaveRequest(request, &errors);
        CHECK(HasFieldError(errors, "notification_failure_sound_path"));
        request = valid;
        request.config.volume_bell = 101;
        errors.clear();
        chime::webd::generated_config_json::ValidateSaveRequest(request, &errors);
        CHECK(HasFieldError(errors, "volume_bell"));
        request = valid;
        request.config.volume_notifications = 101;
        errors.clear();
        chime::webd::generated_config_json::ValidateSaveRequest(request, &errors);
        CHECK(HasFieldError(errors, "volume_notifications"));
        request = valid;
        request.config.wifi_ssid = "";
        errors.clear();
        chime::webd::generated_config_json::ValidateSaveRequest(request, &errors);
        CHECK(HasFieldError(errors, "wifi_ssid"));
        request = valid;
        request.config.wifi_ssid = std::string(33, 'x');
        errors.clear();
        chime::webd::generated_config_json::ValidateSaveRequest(request, &errors);
        CHECK(HasFieldError(errors, "wifi_ssid"));

        request = valid;
        request.config.mqtt_client_id = "x\naudio_enabled=false";
        errors.clear();
        chime::webd::generated_config_json::ValidateSaveRequest(request, &errors);
        CHECK(HasFieldError(errors, "mqtt_client_id"));

        request = valid;
        request.config.mqtt_topics = {"doorbell/ring\naudio_enabled=false"};
        errors.clear();
        chime::webd::generated_config_json::ValidateSaveRequest(request, &errors);
        CHECK(HasFieldError(errors, "mqtt_topics"));
    }

    TEST_CASE("shared invalid fixtures fail generated field validation") {
        for (const auto &example : chime::kConfigInvalidValueExamples) {
            const auto *spec = chime::FindConfigField(example.key);
            REQUIRE(spec != nullptr);
            CHECK_FALSE(chime::ConfigFieldValueValid(*spec, example.value));
        }
    }
}
