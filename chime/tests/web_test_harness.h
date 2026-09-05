#ifndef OC_WEB_TEST_HARNESS_H
#define OC_WEB_TEST_HARNESS_H

#include <memory>
#include <string>
#include <utility>

#include "chime/webd_api.h"
#include "chime/webd_apply_manager.h"
#include "chime/webd_auth.h"
#include "chime/webd_config_store.h"
#include "doctest.h"
#include "fake_process_runner.h"
#include "oc/http/http.h"
#include "oc/wifi/scan.h"
#include "test_support.h"

constexpr const char *kWebHarnessConfig = R"(
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
)";

inline std::string CookieFromResponse(const oc::http::HttpResponse &response, const std::string &name) {
    const std::string prefix = name + "=";
    for (const auto &line : response.set_cookies) {
        if (line.rfind(prefix, 0) != 0) {
            continue;
        }
        const auto end = line.find(';');
        return end == std::string::npos ? line.substr(prefix.size()) : line.substr(prefix.size(), end - prefix.size());
    }
    return "";
}

class WebHarness {
  public:
    enum class Mode { Unpaired, Paired };

    explicit WebHarness(Mode mode = Mode::Paired, std::string secret = "test-password-ok")
        : secret_(std::move(secret)) {
        const auto conf = tmp_.WriteFile("chime.conf", kWebHarnessConfig);
        store_ =
            std::make_unique<chime::webd::ConfigStore>(logger_, conf.string(), (tmp_.path() / "wpa.conf").string());
        scanner_ = std::make_unique<oc::wifi::WifiScanner>(logger_, "wlan0");
        apply_ = std::make_unique<chime::webd::ApplyManager>(logger_, process_runner_, oc::process::Command{"true", {}},
                                                             oc::process::Command{"true", {}});

        chime::webd::AuthStoreOptions options;
        options.auth_dir = (tmp_.path() / "auth").string();
        options.pbkdf2_iterations = 2;
        options.cookie_secure = true;
        if (mode == Mode::Paired) {
            options.bootstrap_password = secret_;
        } else {
            options.pairing_code_override = secret_;
        }
        auth_ = std::make_unique<chime::webd::AuthStore>(logger_, options);
        api_ = std::make_unique<chime::webd::WebApi>(
            logger_, *store_, *scanner_, *apply_, *auth_, (tmp_.path() / "ui").string(),
            (tmp_.path() / "topics.txt").string(), (tmp_.path() / "sounds").string(),
            (tmp_.path() / "ring.wav").string());
        if (mode == Mode::Paired) {
            Login(secret_);
        }
    }

    chime::webd::WebApi &api() { return *api_; }
    chime::webd::AuthStore &auth() { return *auth_; }
    chime::webd::ConfigStore &store() { return *store_; }
    const std::filesystem::path &path() const { return tmp_.path(); }
    const std::string &secret() const { return secret_; }
    const std::string &session_id() const { return session_id_; }
    const std::string &csrf_token() const { return csrf_token_; }

    std::filesystem::path WriteFile(const std::string &name, std::string_view contents) const {
        return tmp_.WriteFile(name, contents);
    }

    void Login(const std::string &password) {
        oc::http::HttpRequest request;
        request.method = "POST";
        request.path = "/api/v1/auth/login";
        request.body = std::string("{\"password\":\"") + password + "\"}";
        const auto response = api_->Handle(request);
        REQUIRE(response.status == 200);
        session_id_ = CookieFromResponse(response, chime::webd::kSessionCookieName);
        csrf_token_ = CookieFromResponse(response, chime::webd::kCsrfCookieName);
        REQUIRE_FALSE(session_id_.empty());
        REQUIRE_FALSE(csrf_token_.empty());
    }

    void Authorize(oc::http::HttpRequest &request, bool with_csrf = true) const {
        request.headers["cookie"] = std::string(chime::webd::kSessionCookieName) + "=" + session_id_ + "; " +
                                    chime::webd::kCsrfCookieName + "=" + csrf_token_;
        if (with_csrf) {
            request.headers[chime::webd::kCsrfHeaderName] = csrf_token_;
        }
    }

    oc::http::HttpRequest Request(const std::string &method, const std::string &path, const std::string &body = "",
                                  bool authed = true, bool with_csrf = true) const {
        oc::http::HttpRequest request;
        request.method = method;
        request.path = path;
        request.body = body;
        if (authed) {
            Authorize(request, with_csrf);
        }
        return request;
    }

  private:
    ScopedTempDir tmp_;
    NullLogger logger_;
    oc::process::FakeRunner process_runner_;
    std::string secret_;
    std::unique_ptr<chime::webd::ConfigStore> store_;
    std::unique_ptr<oc::wifi::WifiScanner> scanner_;
    std::unique_ptr<chime::webd::ApplyManager> apply_;
    std::unique_ptr<chime::webd::AuthStore> auth_;
    std::unique_ptr<chime::webd::WebApi> api_;
    std::string session_id_;
    std::string csrf_token_;
};

#endif
