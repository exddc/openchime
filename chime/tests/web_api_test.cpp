#include <filesystem>
#include <string>

#include "chime/webd_api.h"
#include "chime/webd_apply_manager.h"
#include "chime/webd_config_store.h"
#include "chime/webd_http.h"
#include "chime/webd_json.h"
#include "chime/webd_json_http.h"
#include "chime/webd_wifi_scan.h"
#include "doctest.h"
#include "test_support.h"

namespace {

constexpr const char *kCoreConfig = R"(
mqtt_host=broker.local
mqtt_port=1883
mqtt_client_id=chime
mqtt_username=
mqtt_password=
mqtt_tls_enabled=false
mqtt_tls_validate_certificate=true
mqtt_tls_ca_file=
mqtt_tls_cert_file=
mqtt_tls_key_file=
mqtt_topics=doorbell/ring
ring_topic=doorbell/ring
notification_success_sound_path=/usr/local/share/chime/test.wav
notification_failure_sound_path=/usr/local/share/chime/ring.wav
volume_bell=80
volume_notifications=70
volume_other=70
)";

std::string CorePostBody() {
    return R"({
      "wifi_ssid": "lab-net",
      "wifi_password": "supersecret",
      "mqtt_host": "mqtt.example",
      "mqtt_port": 1883,
      "mqtt_client_id": "chime-lab",
      "mqtt_username": "user",
      "mqtt_password": "mqtt-secret",
      "mqtt_tls_enabled": false,
      "mqtt_tls_validate_certificate": true,
      "mqtt_tls_ca_file": "",
      "mqtt_tls_cert_file": "",
      "mqtt_tls_key_file": "",
      "mqtt_topics": ["doorbell/ring"],
      "ring_topic": "doorbell/ring",
      "notification_success_sound_path": "/usr/local/share/chime/test.wav",
      "notification_failure_sound_path": "/usr/local/share/chime/ring.wav",
      "volume_bell": 40,
      "volume_notifications": 30,
      "volume_other": 20
    })";
}

chime::webd::JsonValue ParseBody(const chime::webd::HttpResponse &response) {
    const auto parsed = chime::webd::ParseJson(response.body);
    REQUIRE(parsed.success);
    return parsed.value;
}

bool HasField(const chime::webd::JsonValue &value, const std::string &key) {
    return chime::webd::GetObjectField(value, key).has_value();
}

chime::webd::JsonValue RequireField(const chime::webd::JsonValue &value, const std::string &key) {
    const auto field = chime::webd::GetObjectField(value, key);
    REQUIRE(field.has_value());
    return *field;
}

std::string MinimalWav() {
    return std::string("RIFF") + std::string("\x24\x00\x00\x00", 4) + "WAVE";
}

std::string RequireError(const chime::webd::HttpResponse &response) {
    std::string error;
    REQUIRE(RequireField(ParseBody(response), "error").AsString(&error));
    return error;
}

} // namespace

TEST_SUITE("web_api") {
    TEST_CASE("JsonHttpBody does not substitute null on serialize failure") {
        const auto ok_null = chime::webd::JsonHttpBody(200, chime::webd::JsonValue::Null());
        CHECK(ok_null.status == 200);
        CHECK(ok_null.body == "null");

        std::string with_nul("a");
        with_nul.push_back('\0');
        with_nul.push_back('b');
        const auto failed = chime::webd::JsonHttpBody(200, chime::webd::JsonValue::String(with_nul));
        CHECK(failed.status != 200);
        CHECK(failed.body != "null");
        const auto parsed = chime::webd::ParseJson(failed.body);
        REQUIRE(parsed.success);
        CHECK(parsed.value.type() != chime::webd::JsonValue::Type::kNull);
    }

    TEST_CASE("config GET/POST round trip redacts passwords and keeps UI field names") {
        const ScopedTempDir tmp;
        const auto conf = tmp.WriteFile("chime.conf", kCoreConfig);
        const auto wpa = tmp.path() / "wpa.conf";
        NullLogger logger;
        chime::webd::ConfigStore store(logger, conf.string(), wpa.string());
        chime::webd::WifiScanner scanner(logger, "wlan0");
        chime::webd::ApplyManager apply(logger, "true", "true");
        chime::webd::WebApi api(logger, store, scanner, apply, (tmp.path() / "ui").string(),
                                (tmp.path() / "topics.txt").string(), (tmp.path() / "sounds").string(),
                                (tmp.path() / "ring.wav").string());

        chime::webd::HttpRequest get;
        get.method = "GET";
        get.path = "/api/v1/config/core";
        const auto get_response = api.Handle(get);
        REQUIRE(get_response.status == 200);
        const auto get_body = ParseBody(get_response);
        CHECK(get_response.content_type == "application/json; charset=utf-8");
        CHECK_FALSE(HasField(get_body, "wifi_password"));
        CHECK_FALSE(HasField(get_body, "mqtt_password"));
        bool wifi_password_set = true;
        REQUIRE(RequireField(get_body, "wifi_password_set").AsBool(&wifi_password_set));
        CHECK_FALSE(wifi_password_set);
        CHECK(HasField(get_body, "wifi_ssid"));
        CHECK(HasField(get_body, "mqtt_host"));
        CHECK(HasField(get_body, "apply"));

        chime::webd::HttpRequest post;
        post.method = "POST";
        post.path = "/api/v1/config/core";
        post.body = CorePostBody();
        const auto post_response = api.Handle(post);
        REQUIRE(post_response.status == 200);
        const auto post_body = ParseBody(post_response);
        CHECK_FALSE(HasField(post_body, "wifi_password"));
        CHECK_FALSE(HasField(post_body, "mqtt_password"));
        bool mqtt_password_set = false;
        REQUIRE(RequireField(post_body, "mqtt_password_set").AsBool(&mqtt_password_set));
        CHECK(mqtt_password_set);
        std::string host;
        REQUIRE(RequireField(post_body, "mqtt_host").AsString(&host));
        CHECK(host == "mqtt.example");
        double volume = 0;
        REQUIRE(RequireField(post_body, "volume_bell").AsNumber(&volume));
        CHECK(volume == 40.0);

        const auto get_after = ParseBody(api.Handle(get));
        REQUIRE(RequireField(get_after, "mqtt_host").AsString(&host));
        CHECK(host == "mqtt.example");
        CHECK_FALSE(HasField(get_after, "mqtt_password"));
        REQUIRE(RequireField(get_after, "mqtt_password_set").AsBool(&mqtt_password_set));
        CHECK(mqtt_password_set);
    }

    TEST_CASE("rejects malformed JSON, unsupported methods, and missing routes") {
        const ScopedTempDir tmp;
        const auto conf = tmp.WriteFile("chime.conf", kCoreConfig);
        NullLogger logger;
        chime::webd::ConfigStore store(logger, conf.string(), (tmp.path() / "wpa.conf").string());
        chime::webd::WifiScanner scanner(logger, "wlan0");
        chime::webd::ApplyManager apply(logger, "true", "true");
        chime::webd::WebApi api(logger, store, scanner, apply, "", (tmp.path() / "topics.txt").string(),
                                (tmp.path() / "sounds").string(), (tmp.path() / "ring.wav").string());

        chime::webd::HttpRequest bad_json;
        bad_json.method = "POST";
        bad_json.path = "/api/v1/config/core";
        bad_json.body = "{not json";
        const auto bad_json_response = api.Handle(bad_json);
        CHECK(bad_json_response.status == 400);
        std::string error;
        REQUIRE(RequireField(ParseBody(bad_json_response), "error").AsString(&error));
        CHECK(error == "invalid_json");

        chime::webd::HttpRequest unsupported;
        unsupported.method = "DELETE";
        unsupported.path = "/api/v1/config/core";
        CHECK(api.Handle(unsupported).status == 405);

        chime::webd::HttpRequest missing;
        missing.method = "GET";
        missing.path = "/api/v1/nope";
        CHECK(api.Handle(missing).status == 404);

        chime::webd::HttpRequest reserved;
        reserved.method = "GET";
        reserved.path = "/api/v1/diagnostics/ping";
        CHECK(api.Handle(reserved).status == 501);

        chime::webd::HttpRequest too_large;
        too_large.method = "POST";
        too_large.path = "/api/v1/config/core";
        too_large.body = std::string(chime::webd::kMaxJsonBodyBytes + 1, 'x');
        CHECK(api.Handle(too_large).status == 400);
        CHECK(RequireError(api.Handle(too_large)) == "payload_too_large");
    }

    TEST_CASE("rejects non-finite and out-of-range integers") {
        const ScopedTempDir tmp;
        const auto conf = tmp.WriteFile("chime.conf", kCoreConfig);
        NullLogger logger;
        chime::webd::ConfigStore store(logger, conf.string(), (tmp.path() / "wpa.conf").string());
        chime::webd::WifiScanner scanner(logger, "wlan0");
        chime::webd::ApplyManager apply(logger, "true", "true");
        chime::webd::WebApi api(logger, store, scanner, apply, "", (tmp.path() / "topics.txt").string(),
                                (tmp.path() / "sounds").string(), (tmp.path() / "ring.wav").string());

        const auto post_with_port = [&](const std::string &port) {
            std::string body = CorePostBody();
            const std::string from = "\"mqtt_port\": 1883";
            const auto pos = body.find(from);
            REQUIRE(pos != std::string::npos);
            body.replace(pos, from.size(), "\"mqtt_port\": " + port);
            chime::webd::HttpRequest post;
            post.method = "POST";
            post.path = "/api/v1/config/core";
            post.body = body;
            return api.Handle(post);
        };

        for (const char *port : {"1e999", "2147483648", "-2147483649"}) {
            CAPTURE(port);
            const auto response = post_with_port(port);
            CHECK(response.status == 400);
            CHECK(RequireError(response) == "validation_failed");
        }
    }

    TEST_CASE("uploads a WAV, rejects bad names and magic, and selects a sound") {
        const ScopedTempDir tmp;
        const auto conf = tmp.WriteFile("chime.conf", kCoreConfig);
        const auto sounds_dir = tmp.path() / "sounds";
        const auto active_path = tmp.path() / "ring.wav";
        NullLogger logger;
        chime::webd::ConfigStore store(logger, conf.string(), (tmp.path() / "wpa.conf").string());
        chime::webd::WifiScanner scanner(logger, "wlan0");
        chime::webd::ApplyManager apply(logger, "true", "true");
        chime::webd::WebApi api(logger, store, scanner, apply, "", (tmp.path() / "topics.txt").string(),
                                sounds_dir.string(), active_path.string());

        chime::webd::HttpRequest bad_name;
        bad_name.method = "PUT";
        bad_name.path = "/api/v1/ring/sounds/not-a-sound.bin";
        bad_name.body = MinimalWav();
        CHECK(api.Handle(bad_name).status == 400);
        CHECK(RequireError(api.Handle(bad_name)) == "invalid_sound_name");

        chime::webd::HttpRequest bad_magic;
        bad_magic.method = "PUT";
        bad_magic.path = "/api/v1/ring/sounds/ring-lab.wav";
        bad_magic.body = "not a wav";
        CHECK(api.Handle(bad_magic).status == 415);
        CHECK(RequireError(api.Handle(bad_magic)) == "invalid_payload");

        chime::webd::HttpRequest bad_type;
        bad_type.method = "PUT";
        bad_type.path = "/api/v1/ring/sounds/ring-lab.wav";
        bad_type.has_content_type = true;
        bad_type.content_type = "application/json";
        bad_type.body = MinimalWav();
        CHECK(api.Handle(bad_type).status == 415);

        chime::webd::HttpRequest upload;
        upload.method = "PUT";
        upload.path = "/api/v1/ring/sounds/ring-lab.wav";
        upload.has_content_type = true;
        upload.content_type = "audio/wav";
        upload.body = MinimalWav();
        const auto uploaded = api.Handle(upload);
        CHECK(uploaded.status == 200);
        std::string uploaded_name;
        REQUIRE(RequireField(ParseBody(uploaded), "uploaded").AsString(&uploaded_name));
        CHECK(uploaded_name == "ring-lab.wav");
        CHECK(std::filesystem::is_regular_file(sounds_dir / "ring-lab.wav"));

        chime::webd::HttpRequest missing;
        missing.method = "POST";
        missing.path = "/api/v1/ring/sounds/select";
        missing.body = R"({"name":"ring-missing.wav"})";
        CHECK(api.Handle(missing).status == 404);
        CHECK(RequireError(api.Handle(missing)) == "not_found");

        chime::webd::HttpRequest select;
        select.method = "POST";
        select.path = "/api/v1/ring/sounds/select";
        select.body = R"({"name":"ring-lab.wav"})";
        const auto selected = api.Handle(select);
        CHECK(selected.status == 200);
        std::string selected_name;
        REQUIRE(RequireField(ParseBody(selected), "selected").AsString(&selected_name));
        CHECK(selected_name == "ring-lab.wav");
        CHECK(std::filesystem::is_regular_file(active_path));
    }
}
