#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "chime/webd_api.h"
#include "chime/webd_http.h"
#include "chime/webd_json.h"
#include "chime/webd_json_http.h"
#include "doctest.h"
#include "web_test_harness.h"

namespace {

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
      "volume_notifications": 30
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
        WebHarness harness;

        const auto get_response = harness.api().Handle(harness.Request("GET", "/api/v1/config/core"));
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
        CHECK_FALSE(HasField(get_body, "volume_other"));

        chime::webd::HttpRequest post;
        post.method = "POST";
        post.path = "/api/v1/config/core";
        post.body = CorePostBody();
        harness.Authorize(post);
        const auto post_response = harness.api().Handle(post);
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

        const auto get_after = ParseBody(harness.api().Handle(harness.Request("GET", "/api/v1/config/core")));
        REQUIRE(RequireField(get_after, "mqtt_host").AsString(&host));
        CHECK(host == "mqtt.example");
        CHECK_FALSE(HasField(get_after, "mqtt_password"));
        REQUIRE(RequireField(get_after, "mqtt_password_set").AsBool(&mqtt_password_set));
        CHECK(mqtt_password_set);
    }

    TEST_CASE("rejects malformed JSON, unsupported methods, and missing routes") {
        WebHarness harness;

        const auto bad_json_response =
            harness.api().Handle(harness.Request("POST", "/api/v1/config/core", "{not json"));
        CHECK(bad_json_response.status == 400);
        std::string error;
        REQUIRE(RequireField(ParseBody(bad_json_response), "error").AsString(&error));
        CHECK(error == "invalid_json");

        CHECK(harness.api().Handle(harness.Request("DELETE", "/api/v1/config/core")).status == 405);
        CHECK(harness.api().Handle(harness.Request("GET", "/api/v1/nope")).status == 404);
        CHECK(harness.api().Handle(harness.Request("GET", "/api/v1/diagnostics/ping")).status == 501);

        const auto too_large =
            harness.Request("POST", "/api/v1/config/core", std::string(chime::webd::kMaxJsonBodyBytes + 1, 'x'));
        CHECK(harness.api().Handle(too_large).status == 400);
        CHECK(RequireError(harness.api().Handle(too_large)) == "payload_too_large");
    }

    TEST_CASE("rejects non-finite and out-of-range integers") {
        WebHarness harness;

        const auto post_with_port = [&](const std::string &port) {
            std::string body = CorePostBody();
            const std::string from = "\"mqtt_port\": 1883";
            const auto pos = body.find(from);
            REQUIRE(pos != std::string::npos);
            body.replace(pos, from.size(), "\"mqtt_port\": " + port);
            return harness.api().Handle(harness.Request("POST", "/api/v1/config/core", body));
        };

        for (const char *port : {"1e999", "2147483648", "-2147483649"}) {
            CAPTURE(port);
            const auto response = post_with_port(port);
            CHECK(response.status == 400);
            CHECK(RequireError(response) == "validation_failed");
        }
    }

    TEST_CASE("rejects CR/LF in file-backed API values so extra assignments cannot be injected") {
        WebHarness harness;
        const auto conf = harness.path() / "chime.conf";
        const std::string original = [](const std::filesystem::path &path) {
            std::ifstream file(path);
            REQUIRE(file.is_open());
            return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        }(conf);

        auto post_replaced = [&](const std::string &from, const std::string &to) {
            std::string body = CorePostBody();
            const auto pos = body.find(from);
            REQUIRE(pos != std::string::npos);
            body.replace(pos, from.size(), to);
            return harness.api().Handle(harness.Request("POST", "/api/v1/config/core", body));
        };

        const auto client_id = post_replaced("\"mqtt_client_id\": \"chime-lab\"",
                                             "\"mqtt_client_id\": \"x\\naudio_enabled=false\"");
        CHECK(client_id.status == 400);
        CHECK(RequireError(client_id) == "validation_failed");

        const auto topics = post_replaced("\"mqtt_topics\": [\"doorbell/ring\"]",
                                          "\"mqtt_topics\": [\"doorbell/ring\\naudio_enabled=false\"]");
        CHECK(topics.status == 400);
        CHECK(RequireError(topics) == "validation_failed");

        std::ifstream file(conf);
        REQUIRE(file.is_open());
        const std::string after((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        CHECK(after == original);
        CHECK(after.find("audio_enabled=false") == std::string::npos);
    }

    TEST_CASE("uploads a WAV, rejects bad names and magic, and selects a sound") {
        WebHarness harness;
        const auto sounds_dir = harness.path() / "sounds";
        const auto active_path = harness.path() / "ring.wav";

        auto bad_name = harness.Request("PUT", "/api/v1/ring/sounds/not-a-sound.bin", MinimalWav());
        CHECK(harness.api().Handle(bad_name).status == 400);
        CHECK(RequireError(harness.api().Handle(bad_name)) == "invalid_sound_name");

        auto bad_magic = harness.Request("PUT", "/api/v1/ring/sounds/ring-lab.wav", "not a wav");
        CHECK(harness.api().Handle(bad_magic).status == 415);
        CHECK(RequireError(harness.api().Handle(bad_magic)) == "invalid_payload");

        auto bad_type = harness.Request("PUT", "/api/v1/ring/sounds/ring-lab.wav", MinimalWav());
        bad_type.has_content_type = true;
        bad_type.content_type = "application/json";
        CHECK(harness.api().Handle(bad_type).status == 415);

        auto upload = harness.Request("PUT", "/api/v1/ring/sounds/ring-lab.wav", MinimalWav());
        upload.has_content_type = true;
        upload.content_type = "audio/wav";
        const auto uploaded = harness.api().Handle(upload);
        CHECK(uploaded.status == 200);
        std::string uploaded_name;
        REQUIRE(RequireField(ParseBody(uploaded), "uploaded").AsString(&uploaded_name));
        CHECK(uploaded_name == "ring-lab.wav");
        CHECK(std::filesystem::is_regular_file(sounds_dir / "ring-lab.wav"));

        CHECK(harness.api()
                  .Handle(harness.Request("POST", "/api/v1/ring/sounds/select", R"({"name":"ring-missing.wav"})"))
                  .status == 404);
        CHECK(RequireError(harness.api().Handle(harness.Request("POST", "/api/v1/ring/sounds/select",
                                                                R"({"name":"ring-missing.wav"})"))) == "not_found");

        const auto selected =
            harness.api().Handle(harness.Request("POST", "/api/v1/ring/sounds/select", R"({"name":"ring-lab.wav"})"));
        CHECK(selected.status == 200);
        std::string selected_name;
        REQUIRE(RequireField(ParseBody(selected), "selected").AsString(&selected_name));
        CHECK(selected_name == "ring-lab.wav");
        CHECK(std::filesystem::is_regular_file(active_path));
    }

    TEST_CASE("omitted passwords on update preserve existing secrets") {
        WebHarness harness;
        REQUIRE(harness.api().Handle(harness.Request("POST", "/api/v1/config/core", CorePostBody())).status == 200);

        std::string body = CorePostBody();
        const std::string wifi_field = "\"wifi_password\": \"supersecret\",";
        const std::string mqtt_field = "\"mqtt_password\": \"mqtt-secret\",";
        REQUIRE(body.find(wifi_field) != std::string::npos);
        REQUIRE(body.find(mqtt_field) != std::string::npos);
        body.replace(body.find(wifi_field), wifi_field.size(), "");
        body.replace(body.find(mqtt_field), mqtt_field.size(), "");

        const auto updated = harness.api().Handle(harness.Request("POST", "/api/v1/config/core", body));
        REQUIRE(updated.status == 200);
        CHECK(updated.body.find("supersecret") == std::string::npos);
        CHECK(updated.body.find("mqtt-secret") == std::string::npos);
        bool wifi_password_set = false;
        bool mqtt_password_set = false;
        REQUIRE(RequireField(ParseBody(updated), "wifi_password_set").AsBool(&wifi_password_set));
        REQUIRE(RequireField(ParseBody(updated), "mqtt_password_set").AsBool(&mqtt_password_set));
        CHECK(wifi_password_set);
        CHECK(mqtt_password_set);

        std::ifstream conf(harness.path() / "chime.conf");
        REQUIRE(conf.is_open());
        std::string conf_text((std::istreambuf_iterator<char>(conf)), std::istreambuf_iterator<char>());
        CHECK(conf_text.find("mqtt_password=mqtt-secret") != std::string::npos);
        CHECK(conf_text.find("supersecret") == std::string::npos);
        std::ifstream wpa(harness.path() / "wpa.conf");
        REQUIRE(wpa.is_open());
        std::string wpa_text((std::istreambuf_iterator<char>(wpa)), std::istreambuf_iterator<char>());
        CHECK(wpa_text.find("supersecret") != std::string::npos);
    }

    TEST_CASE("omitted mqtt password is kept when the username changes") {
        WebHarness harness;
        REQUIRE(harness.api().Handle(harness.Request("POST", "/api/v1/config/core", CorePostBody())).status == 200);

        std::string body = CorePostBody();
        const std::string mqtt_field = "\"mqtt_password\": \"mqtt-secret\",";
        const std::string username_field = "\"mqtt_username\": \"user\",";
        REQUIRE(body.find(mqtt_field) != std::string::npos);
        REQUIRE(body.find(username_field) != std::string::npos);
        body.replace(body.find(mqtt_field), mqtt_field.size(), "");
        body.replace(body.find(username_field), username_field.size(), "\"mqtt_username\": \"other-user\",");

        const auto updated = harness.api().Handle(harness.Request("POST", "/api/v1/config/core", body));
        REQUIRE(updated.status == 200);
        bool mqtt_password_set = false;
        REQUIRE(RequireField(ParseBody(updated), "mqtt_password_set").AsBool(&mqtt_password_set));
        CHECK(mqtt_password_set);
        std::string username;
        REQUIRE(RequireField(ParseBody(updated), "mqtt_username").AsString(&username));
        CHECK(username == "other-user");

        std::ifstream conf(harness.path() / "chime.conf");
        REQUIRE(conf.is_open());
        std::string conf_text((std::istreambuf_iterator<char>(conf)), std::istreambuf_iterator<char>());
        CHECK(conf_text.find("mqtt_username=other-user") != std::string::npos);
        CHECK(conf_text.find("mqtt_password=mqtt-secret") != std::string::npos);
    }

    TEST_CASE("omitted mqtt password is kept when the username is cleared") {
        WebHarness harness;
        REQUIRE(harness.api().Handle(harness.Request("POST", "/api/v1/config/core", CorePostBody())).status == 200);

        std::string body = CorePostBody();
        const std::string mqtt_field = "\"mqtt_password\": \"mqtt-secret\",";
        const std::string username_field = "\"mqtt_username\": \"user\",";
        REQUIRE(body.find(mqtt_field) != std::string::npos);
        REQUIRE(body.find(username_field) != std::string::npos);
        body.replace(body.find(mqtt_field), mqtt_field.size(), "");
        body.replace(body.find(username_field), username_field.size(), "\"mqtt_username\": \"\",");

        const auto updated = harness.api().Handle(harness.Request("POST", "/api/v1/config/core", body));
        REQUIRE(updated.status == 200);
        bool mqtt_password_set = false;
        REQUIRE(RequireField(ParseBody(updated), "mqtt_password_set").AsBool(&mqtt_password_set));
        CHECK(mqtt_password_set);

        std::ifstream conf(harness.path() / "chime.conf");
        REQUIRE(conf.is_open());
        std::string conf_text((std::istreambuf_iterator<char>(conf)), std::istreambuf_iterator<char>());
        CHECK(conf_text.find("mqtt_username=") != std::string::npos);
        CHECK(conf_text.find("mqtt_username=user") == std::string::npos);
        CHECK(conf_text.find("mqtt_password=mqtt-secret") != std::string::npos);
    }

    TEST_CASE("config save joins the apply worker before ApplyManager is destroyed") {
        for (int i = 0; i < 8; ++i) {
            WebHarness harness;
            REQUIRE(harness.api().Handle(harness.Request("POST", "/api/v1/config/core", CorePostBody())).status == 200);
        }
    }
}
