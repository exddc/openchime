#ifndef CHIME_WEBD_AUTH_H
#define CHIME_WEBD_AUTH_H

#include <chrono>
#include <cstddef>
#include <map>
#include <mutex>
#include <optional>
#include <string>

#include "oc/http/http.h"

namespace oc::logging {
class Logger;
}

namespace chime::webd {

using oc::http::HttpRequest;
using oc::http::HttpResponse;

constexpr const char *kSessionCookieName = "chime_session";
constexpr const char *kCsrfCookieName = "chime_csrf";
constexpr const char *kCsrfHeaderName = "x-csrf-token";
constexpr const char *kDefaultAuthDir = "/var/lib/chime/auth";
constexpr const char *kVerifierFileName = "admin.verifier";
constexpr const char *kPairingFileName = "pairing.code";
constexpr int kDefaultPbkdf2Iterations = 600000;
constexpr std::size_t kMaxAuthSessions = 8;
constexpr std::size_t kMaxAuthAttemptClients = 64;

struct AuthStoreOptions {
    std::string auth_dir = kDefaultAuthDir;
    std::string bootstrap_password;
    std::string pairing_code_override;
    // Test-only. Production cookies always include Secure; the Vite proxy strips it for HTTP :5173.
    bool cookie_secure = true;
    std::chrono::seconds session_ttl{std::chrono::hours(12)};
    int pbkdf2_iterations = kDefaultPbkdf2Iterations;
    int max_auth_failures = 5;
    std::chrono::seconds auth_window{std::chrono::seconds(60)};
};

struct SessionView {
    bool authenticated = false;
    bool csrf_valid = false;
    std::string session_id;
};

class AuthStore {
  public:
    AuthStore(oc::logging::Logger &logger, AuthStoreOptions options);

    bool Ready() const;
    bool IsPaired() const;
    std::optional<std::string> UnpairedSetupSecret() const;
    void AnnounceSetupSecret() const;

    HttpResponse HandleStatus(const HttpRequest &request);
    HttpResponse HandlePair(const HttpRequest &request);
    HttpResponse HandleLogin(const HttpRequest &request);
    HttpResponse HandleLogout(const HttpRequest &request);

    SessionView Inspect(const HttpRequest &request) const;
    bool ClearAuthenticationState(std::string *error);
    std::size_t SessionEntryCount() const;
    std::size_t RateLimitEntryCount() const;

  private:
    struct Session {
        std::string csrf_token;
        std::chrono::steady_clock::time_point expires_at;
    };

    struct AttemptWindow {
        std::chrono::steady_clock::time_point window_start{};
        int failures = 0;
    };

    struct Verifier {
        int iterations = 0;
        std::string salt;
        std::string hash;
    };

    std::string VerifierPath() const;
    std::string PairingPath() const;
    bool EnsureAuthDirectory(std::string *error) const;
    bool Initialize(std::string *error);
    bool LoadOrCreatePairingSecret(std::string *error);
    bool DeriveVerifierPayload(const std::string &password, std::string *encoded, std::string *error) const;
    bool WriteVerifier(const std::string &password, std::string *error) const;
    bool ReadVerifier(Verifier *verifier) const;
    bool VerifyPassword(const std::string &password) const;
    bool PairingCodeMatches(const std::string &provided, const std::string &expected) const;
    std::string ClientKey(const HttpRequest &request) const;
    void ReapLocked() const;
    void ReapAttemptWindowsLocked(std::map<std::string, AttemptWindow> &windows) const;
    void TrimAttemptWindowsLocked(std::map<std::string, AttemptWindow> &windows) const;
    bool IsLimitedLocked(const std::map<std::string, AttemptWindow> &windows, const std::string &key) const;
    bool AllowAttempt(std::map<std::string, AttemptWindow> &windows, const std::string &key);
    void ResetAttempts(std::map<std::string, AttemptWindow> &windows, const std::string &key);
    SessionView InspectLocked(const HttpRequest &request) const;
    std::string CreateSessionLocked();
    void ClearSessionLocked(const std::string &session_id);
    void AttachSessionCookies(HttpResponse *response, const std::string &session_id,
                              const std::string &csrf_token) const;
    void AttachClearedCookies(HttpResponse *response) const;
    std::string CookieFlags(bool http_only) const;
    std::chrono::steady_clock::time_point Now() const;

    oc::logging::Logger &logger_;
    AuthStoreOptions options_;
    mutable std::mutex mutex_;
    bool ready_ = false;
    std::string pairing_code_;
    mutable std::map<std::string, Session> sessions_;
    mutable std::map<std::string, AttemptWindow> pair_attempts_;
    mutable std::map<std::string, AttemptWindow> login_attempts_;
};

} // namespace chime::webd

#endif
