#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>

#include "chime/webd_auth.h"
#include "chime/webd_http.h"
#include "chime/webd_json.h"
#include "doctest.h"
#include "web_test_harness.h"

namespace {

chime::webd::JsonValue ParseBody(const chime::webd::HttpResponse &response) {
    const auto parsed = chime::webd::ParseJson(response.body);
    REQUIRE(parsed.success);
    return parsed.value;
}

chime::webd::JsonValue RequireField(const chime::webd::JsonValue &value, const std::string &key) {
    const auto field = chime::webd::GetObjectField(value, key);
    REQUIRE(field.has_value());
    return *field;
}

std::string RequireError(const chime::webd::HttpResponse &response) {
    std::string error;
    REQUIRE(RequireField(ParseBody(response), "error").AsString(&error));
    return error;
}

bool HasField(const chime::webd::JsonValue &value, const std::string &key) {
    return chime::webd::GetObjectField(value, key).has_value();
}

std::string MinimalWav() {
    return std::string("RIFF") + std::string("\x24\x00\x00\x00", 4) + "WAVE";
}

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

} // namespace

TEST_SUITE("web_auth") {
    TEST_CASE("unpaired device permits pairing and rejects administration") {
        WebHarness harness(WebHarness::Mode::Unpaired, "ABCD2345");

        const auto status = harness.api().Handle(harness.Request("GET", "/api/v1/auth/status", "", false));
        REQUIRE(status.status == 200);
        bool paired = true;
        REQUIRE(RequireField(ParseBody(status), "paired").AsBool(&paired));
        CHECK_FALSE(paired);

        CHECK(harness.api().Handle(harness.Request("GET", "/api/v1/config/core", "", false)).status == 401);
        CHECK(RequireError(harness.api().Handle(harness.Request("GET", "/api/v1/config/core", "", false))) ==
              "unpaired");
        CHECK(harness.api().Handle(harness.Request("POST", "/api/v1/config/core", CorePostBody(), false)).status ==
              401);
        CHECK(harness.api().Handle(harness.Request("GET", "/api/v1/wifi/scan", "", false)).status == 401);
        CHECK(harness.api().Handle(harness.Request("GET", "/api/v1/ring/sounds", "", false)).status == 401);
        CHECK(harness.api()
                  .Handle(harness.Request("PUT", "/api/v1/ring/sounds/ring-lab.wav", MinimalWav(), false))
                  .status == 401);
        CHECK(harness.api()
                  .Handle(harness.Request("POST", "/api/v1/ring/sounds/select", R"({"name":"ring-lab.wav"})", false))
                  .status == 401);

        const auto version = harness.api().Handle(harness.Request("GET", "/api/v1/system/version", "", false));
        CHECK(version.status == 200);

        chime::webd::HttpRequest pair;
        pair.method = "POST";
        pair.path = "/api/v1/auth/pair";
        pair.body = R"({"pairing_code":"ABCD2345","password":"admin-secret-1"})";
        const auto paired_response = harness.api().Handle(pair);
        REQUIRE(paired_response.status == 200);
        CHECK(paired_response.body.find("ABCD2345") == std::string::npos);
        CHECK(paired_response.body.find("admin-secret-1") == std::string::npos);
        CHECK(paired_response.body.find("pbkdf2") == std::string::npos);
        CHECK_FALSE(CookieFromResponse(paired_response, chime::webd::kSessionCookieName).empty());
        bool http_only = false;
        bool secure = false;
        bool strict = false;
        for (const auto &cookie : paired_response.set_cookies) {
            if (cookie.rfind("chime_session=", 0) == 0) {
                http_only = cookie.find("HttpOnly") != std::string::npos;
                secure = cookie.find("Secure") != std::string::npos;
                strict = cookie.find("SameSite=Strict") != std::string::npos;
            }
        }
        CHECK(http_only);
        CHECK(secure);
        CHECK(strict);

        const auto closed = harness.api().Handle(pair);
        CHECK(closed.status == 403);
        CHECK(RequireError(closed) == "pairing_closed");

        CHECK(harness.api().Handle(harness.Request("GET", "/api/v1/config/core", "", false)).status == 401);
        CHECK(RequireError(harness.api().Handle(harness.Request("GET", "/api/v1/config/core", "", false))) ==
              "unauthorized");
    }

    TEST_CASE("login succeeds with the admin password and rejects a wrong password") {
        WebHarness harness(WebHarness::Mode::Paired, "correct-horse");

        chime::webd::HttpRequest wrong;
        wrong.method = "POST";
        wrong.path = "/api/v1/auth/login";
        wrong.body = R"({"password":"wrong-password"})";
        const auto wrong_response = harness.api().Handle(wrong);
        CHECK(wrong_response.status == 401);
        CHECK(RequireError(wrong_response) == "invalid_credentials");
        CHECK(wrong_response.body.find("wrong-password") == std::string::npos);
        CHECK(wrong_response.body.find("correct-horse") == std::string::npos);

        chime::webd::HttpRequest login;
        login.method = "POST";
        login.path = "/api/v1/auth/login";
        login.body = R"({"password":"correct-horse"})";
        const auto login_response = harness.api().Handle(login);
        REQUIRE(login_response.status == 200);
        CHECK(login_response.body.find("correct-horse") == std::string::npos);
        CHECK(login_response.body.find("pbkdf2") == std::string::npos);

        chime::webd::HttpRequest get;
        get.method = "GET";
        get.path = "/api/v1/config/core";
        get.headers["cookie"] = std::string(chime::webd::kSessionCookieName) + "=" +
                                CookieFromResponse(login_response, chime::webd::kSessionCookieName);
        CHECK(harness.api().Handle(get).status == 200);
    }

    TEST_CASE("mutating routes require a session and CSRF token") {
        WebHarness harness;

        CHECK(harness.api().Handle(harness.Request("POST", "/api/v1/config/core", CorePostBody(), false)).status ==
              401);
        CHECK(harness.api()
                  .Handle(harness.Request("PUT", "/api/v1/ring/sounds/ring-lab.wav", MinimalWav(), false))
                  .status == 401);
        CHECK(harness.api()
                  .Handle(harness.Request("POST", "/api/v1/ring/sounds/select", R"({"name":"ring-lab.wav"})", false))
                  .status == 401);
        CHECK(harness.api().Handle(harness.Request("GET", "/api/v1/wifi/scan", "", false)).status == 401);
        CHECK(harness.api().Handle(harness.Request("GET", "/api/v1/mqtt/topics", "", false)).status == 401);
        CHECK(harness.api().Handle(harness.Request("GET", "/api/v1/ring/sounds", "", false)).status == 401);

        auto missing_csrf = harness.Request("POST", "/api/v1/config/core", CorePostBody(), true, false);
        CHECK(harness.api().Handle(missing_csrf).status == 403);
        CHECK(RequireError(harness.api().Handle(missing_csrf)) == "csrf_failed");

        auto bad_csrf = harness.Request("POST", "/api/v1/config/core", CorePostBody());
        bad_csrf.headers[chime::webd::kCsrfHeaderName] = "deadbeef";
        CHECK(harness.api().Handle(bad_csrf).status == 403);

        REQUIRE(harness.api().Handle(harness.Request("POST", "/api/v1/config/core", CorePostBody())).status == 200);

        auto upload = harness.Request("PUT", "/api/v1/ring/sounds/ring-lab.wav", MinimalWav());
        upload.has_content_type = true;
        upload.content_type = "audio/wav";
        REQUIRE(harness.api().Handle(upload).status == 200);
        REQUIRE(harness.api()
                    .Handle(harness.Request("POST", "/api/v1/ring/sounds/select", R"({"name":"ring-lab.wav"})"))
                    .status == 200);
    }

    TEST_CASE("logout and session expiry reject later administration") {
        chime::webd::AuthStoreOptions options;
        const ScopedTempDir tmp;
        NullLogger logger;
        options.auth_dir = (tmp.path() / "auth").string();
        options.bootstrap_password = "test-password-ok";
        options.pbkdf2_iterations = 2;
        options.session_ttl = std::chrono::seconds(1);
        const auto conf = tmp.WriteFile("chime.conf", kWebHarnessConfig);
        chime::webd::ConfigStore store(logger, conf.string(), (tmp.path() / "wpa.conf").string());
        chime::webd::WifiScanner scanner(logger, "wlan0");
        chime::webd::ApplyManager apply(logger, "true", "true");
        chime::webd::AuthStore auth(logger, options);
        chime::webd::WebApi api(logger, store, scanner, apply, auth, "", (tmp.path() / "topics.txt").string(),
                                (tmp.path() / "sounds").string(), (tmp.path() / "ring.wav").string());

        chime::webd::HttpRequest login;
        login.method = "POST";
        login.path = "/api/v1/auth/login";
        login.body = R"({"password":"test-password-ok"})";
        const auto login_response = api.Handle(login);
        REQUIRE(login_response.status == 200);
        const std::string session = CookieFromResponse(login_response, chime::webd::kSessionCookieName);
        const std::string csrf = CookieFromResponse(login_response, chime::webd::kCsrfCookieName);

        chime::webd::HttpRequest logout;
        logout.method = "POST";
        logout.path = "/api/v1/auth/logout";
        logout.headers["cookie"] = std::string(chime::webd::kSessionCookieName) + "=" + session;
        logout.headers[chime::webd::kCsrfHeaderName] = csrf;
        const auto logout_response = api.Handle(logout);
        REQUIRE(logout_response.status == 200);
        bool authenticated = true;
        REQUIRE(RequireField(ParseBody(logout_response), "authenticated").AsBool(&authenticated));
        CHECK_FALSE(authenticated);

        chime::webd::HttpRequest get;
        get.method = "GET";
        get.path = "/api/v1/config/core";
        get.headers["cookie"] = std::string(chime::webd::kSessionCookieName) + "=" + session;
        CHECK(api.Handle(get).status == 401);

        const auto login_again = api.Handle(login);
        REQUIRE(login_again.status == 200);
        std::this_thread::sleep_for(std::chrono::milliseconds(1100));
        chime::webd::HttpRequest expired;
        expired.method = "GET";
        expired.path = "/api/v1/config/core";
        expired.headers["cookie"] = std::string(chime::webd::kSessionCookieName) + "=" +
                                    CookieFromResponse(login_again, chime::webd::kSessionCookieName);
        CHECK(api.Handle(expired).status == 401);
    }

    TEST_CASE("wiping auth state restores pairing without deleting config") {
        WebHarness harness;
        REQUIRE(harness.api().Handle(harness.Request("POST", "/api/v1/config/core", CorePostBody())).status == 200);
        const auto conf_path = harness.path() / "chime.conf";
        REQUIRE(std::filesystem::is_regular_file(conf_path));

        std::string error;
        REQUIRE(harness.auth().ClearAuthenticationState(&error));
        CHECK(error.empty());
        CHECK_FALSE(harness.auth().IsPaired());
        REQUIRE(harness.auth().UnpairedSetupSecret().has_value());
        CHECK(std::filesystem::is_regular_file(conf_path));
        std::ifstream conf(conf_path);
        std::string conf_text((std::istreambuf_iterator<char>(conf)), std::istreambuf_iterator<char>());
        CHECK(conf_text.find("mqtt_host=mqtt.example") != std::string::npos);

        CHECK(harness.api().Handle(harness.Request("GET", "/api/v1/config/core")).status == 401);
        CHECK(RequireError(harness.api().Handle(harness.Request("GET", "/api/v1/config/core", "", false))) ==
              "unpaired");

        const std::string pairing_code = *harness.auth().UnpairedSetupSecret();
        chime::webd::HttpRequest pair;
        pair.method = "POST";
        pair.path = "/api/v1/auth/pair";
        pair.body = std::string("{\"pairing_code\":\"") + pairing_code + "\",\"password\":\"new-admin-pass\"}";
        REQUIRE(harness.api().Handle(pair).status == 200);
        CHECK(harness.auth().IsPaired());

        const auto auth_dir = harness.path() / "auth";
        REQUIRE(::chmod(auth_dir.c_str(), 0555) == 0);
        std::string blocked;
        CHECK_FALSE(harness.auth().ClearAuthenticationState(&blocked));
        CHECK(harness.auth().IsPaired());
        REQUIRE(::chmod(auth_dir.c_str(), 0700) == 0);
        REQUIRE(harness.auth().ClearAuthenticationState(&blocked));
        CHECK_FALSE(harness.auth().IsPaired());
    }

    TEST_CASE("login rate limit rejects extra guesses") {
        WebHarness harness(WebHarness::Mode::Paired, "correct-horse");
        chime::webd::HttpRequest wrong;
        wrong.method = "POST";
        wrong.path = "/api/v1/auth/login";
        wrong.peer_address = "192.0.2.10";
        wrong.body = R"({"password":"nope-nope-nope"})";
        int last_status = 0;
        for (int i = 0; i < 6; ++i) {
            last_status = harness.api().Handle(wrong).status;
        }
        CHECK(last_status == 429);
        CHECK(RequireError(harness.api().Handle(wrong)) == "rate_limited");

        chime::webd::HttpRequest other;
        other.method = "POST";
        other.path = "/api/v1/auth/login";
        other.peer_address = "192.0.2.11";
        other.body = R"({"password":"correct-horse"})";
        CHECK(harness.api().Handle(other).status == 200);

        chime::webd::HttpRequest malformed;
        malformed.method = "POST";
        malformed.path = "/api/v1/auth/login";
        malformed.peer_address = "192.0.2.12";
        malformed.body = "{not json";
        for (int i = 0; i < 6; ++i) {
            CHECK(harness.api().Handle(malformed).status == 400);
        }
        chime::webd::HttpRequest still_allowed = wrong;
        still_allowed.peer_address = "192.0.2.12";
        CHECK(harness.api().Handle(still_allowed).status == 401);
    }

    TEST_CASE("pairing rate limit rejects extra guesses") {
        WebHarness harness(WebHarness::Mode::Unpaired, "ABCD2345");
        chime::webd::HttpRequest wrong;
        wrong.method = "POST";
        wrong.path = "/api/v1/auth/pair";
        wrong.peer_address = "192.0.2.20";
        wrong.body = R"({"pairing_code":"ZZZZZZZZ","password":"admin-secret-1"})";
        int last_status = 0;
        for (int i = 0; i < 6; ++i) {
            last_status = harness.api().Handle(wrong).status;
        }
        CHECK(last_status == 429);
        CHECK(RequireError(harness.api().Handle(wrong)) == "rate_limited");

        chime::webd::HttpRequest other;
        other.method = "POST";
        other.path = "/api/v1/auth/pair";
        other.peer_address = "192.0.2.21";
        other.body = R"({"pairing_code":"ABCD2345","password":"admin-secret-1"})";
        CHECK(harness.api().Handle(other).status == 200);

        WebHarness second(WebHarness::Mode::Unpaired, "ABCD2345");
        chime::webd::HttpRequest malformed;
        malformed.method = "POST";
        malformed.path = "/api/v1/auth/pair";
        malformed.peer_address = "192.0.2.22";
        malformed.body = "{not json";
        for (int i = 0; i < 6; ++i) {
            CHECK(second.api().Handle(malformed).status == 400);
        }
        chime::webd::HttpRequest still_allowed = wrong;
        still_allowed.peer_address = "192.0.2.22";
        CHECK(second.api().Handle(still_allowed).status == 401);
    }

    TEST_CASE("admin verifier authenticates after recreating AuthStore") {
        const ScopedTempDir tmp;
        NullLogger logger;
        chime::webd::AuthStoreOptions options;
        options.auth_dir = (tmp.path() / "auth").string();
        options.pairing_code_override = "ABCD2345";
        options.pbkdf2_iterations = 2;
        {
            chime::webd::AuthStore auth(logger, options);
            REQUIRE(auth.Ready());
            CHECK_FALSE(auth.IsPaired());
            chime::webd::HttpRequest pair;
            pair.method = "POST";
            pair.path = "/api/v1/auth/pair";
            pair.body = R"({"pairing_code":"ABCD2345","password":"persist-secret"})";
            REQUIRE(auth.HandlePair(pair).status == 200);
            CHECK(auth.IsPaired());
        }

        options.pairing_code_override.clear();
        chime::webd::AuthStore restarted(logger, options);
        REQUIRE(restarted.Ready());
        CHECK(restarted.IsPaired());
        chime::webd::HttpRequest login;
        login.method = "POST";
        login.path = "/api/v1/auth/login";
        login.body = R"({"password":"persist-secret"})";
        CHECK(restarted.HandleLogin(login).status == 200);
    }

    TEST_CASE("production cookies include Secure and tests may omit it") {
        CHECK(chime::webd::AuthStoreOptions{}.cookie_secure);
        WebHarness harness(WebHarness::Mode::Paired, "correct-horse");
        const auto login = harness.api().Handle(
            harness.Request("POST", "/api/v1/auth/login", R"({"password":"correct-horse"})", false));
        REQUIRE(login.status == 200);
        bool saw_secure = false;
        for (const auto &cookie : login.set_cookies) {
            if (cookie.rfind("chime_session=", 0) == 0) {
                saw_secure = cookie.find("Secure") != std::string::npos;
            }
        }
        CHECK(saw_secure);

        const ScopedTempDir tmp;
        NullLogger logger;
        chime::webd::AuthStoreOptions options;
        options.auth_dir = (tmp.path() / "auth").string();
        options.bootstrap_password = "test-password-ok";
        options.pbkdf2_iterations = 2;
        options.cookie_secure = false;
        chime::webd::AuthStore auth(logger, options);
        chime::webd::HttpRequest login_insecure;
        login_insecure.method = "POST";
        login_insecure.path = "/api/v1/auth/login";
        login_insecure.body = R"({"password":"test-password-ok"})";
        const auto insecure = auth.HandleLogin(login_insecure);
        REQUIRE(insecure.status == 200);
        for (const auto &cookie : insecure.set_cookies) {
            CHECK(cookie.find("Secure") == std::string::npos);
        }
    }

    TEST_CASE("production PBKDF2 work factor is the AuthStore default") {
        CHECK(chime::webd::kDefaultPbkdf2Iterations == 600000);
        CHECK(chime::webd::AuthStoreOptions{}.pbkdf2_iterations == chime::webd::kDefaultPbkdf2Iterations);
    }

    TEST_CASE("auth files are mode 0600 and omit plaintext") {
        WebHarness harness(WebHarness::Mode::Paired, "correct-horse");
        const auto verifier = harness.path() / "auth" / chime::webd::kVerifierFileName;
        REQUIRE(std::filesystem::is_regular_file(verifier));
        struct stat st {};
        REQUIRE(::stat(verifier.c_str(), &st) == 0);
        CHECK((st.st_mode & 0777) == 0600);
        std::ifstream file(verifier);
        std::string text((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        CHECK(text.find("correct-horse") == std::string::npos);
        CHECK(text.find("pbkdf2-sha256$") == 0);
        CHECK_FALSE(HasField(ParseBody(harness.api().Handle(harness.Request("GET", "/api/v1/auth/status", "", false))),
                             "password"));
    }

    TEST_CASE("session and rate-limit maps stay bounded and drop expired entries") {
        const ScopedTempDir tmp;
        NullLogger logger;
        chime::webd::AuthStoreOptions options;
        options.auth_dir = (tmp.path() / "auth").string();
        options.bootstrap_password = "test-password-ok";
        options.pbkdf2_iterations = 2;
        options.session_ttl = std::chrono::seconds(1);
        options.auth_window = std::chrono::seconds(1);
        chime::webd::AuthStore auth(logger, options);

        chime::webd::HttpRequest login;
        login.method = "POST";
        login.path = "/api/v1/auth/login";
        login.body = R"({"password":"test-password-ok"})";
        for (int i = 0; i < 20; ++i) {
            REQUIRE(auth.HandleLogin(login).status == 200);
        }
        CHECK(auth.SessionEntryCount() == chime::webd::kMaxAuthSessions);

        std::this_thread::sleep_for(std::chrono::milliseconds(1100));
        chime::webd::HttpRequest status;
        status.method = "GET";
        status.path = "/api/v1/auth/status";
        REQUIRE(auth.HandleStatus(status).status == 200);
        CHECK(auth.SessionEntryCount() == 0);

        chime::webd::HttpRequest wrong;
        wrong.method = "POST";
        wrong.path = "/api/v1/auth/login";
        wrong.body = R"({"password":"nope-nope-nope"})";
        for (int i = 0; i < 80; ++i) {
            wrong.peer_address = "198.51.100." + std::to_string(i);
            CHECK(auth.HandleLogin(wrong).status == 401);
        }
        CHECK(auth.RateLimitEntryCount() <= chime::webd::kMaxAuthAttemptClients);
        CHECK(auth.RateLimitEntryCount() > 0);

        std::this_thread::sleep_for(std::chrono::milliseconds(1100));
        REQUIRE(auth.HandleStatus(status).status == 200);
        CHECK(auth.RateLimitEntryCount() == 0);
    }

    TEST_CASE("invalid auth directory does not advertise pairing") {
        const ScopedTempDir tmp;
        NullLogger logger;
        chime::webd::AuthStoreOptions options;
        options.auth_dir = tmp.WriteFile("not-a-directory", "blocked").string();
        options.pbkdf2_iterations = 2;
        chime::webd::AuthStore auth(logger, options);
        CHECK_FALSE(auth.Ready());

        chime::webd::HttpRequest status;
        status.method = "GET";
        status.path = "/api/v1/auth/status";
        const auto status_response = auth.HandleStatus(status);
        CHECK(status_response.status == 503);
        CHECK(RequireError(status_response) == "auth_unavailable");
        CHECK_FALSE(HasField(ParseBody(status_response), "paired"));

        chime::webd::HttpRequest pair;
        pair.method = "POST";
        pair.path = "/api/v1/auth/pair";
        pair.body = R"({"pairing_code":"ABCD2345","password":"admin-secret-1"})";
        const auto pair_response = auth.HandlePair(pair);
        CHECK(pair_response.status == 503);
        CHECK(RequireError(pair_response) == "auth_unavailable");
    }
}
