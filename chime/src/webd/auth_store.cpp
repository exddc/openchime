#include "chime/webd_auth.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fcntl.h>
#include <filesystem>
#include <optional>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

#include "chime/webd_json.h"
#include "chime/webd_json_http.h"
#include "chime/webd_json_validate.h"
#include "oc/config/kv_config.h"
#include "oc/logging/logger.h"
#include "oc/util/filesystem.h"

namespace chime::webd {
namespace {

constexpr mode_t kAuthFileMode = 0600;
constexpr mode_t kAuthDirMode = 0700;
constexpr std::size_t kSaltBytes = 16;
constexpr std::size_t kHashBytes = 32;
constexpr std::size_t kTokenBytes = 32;
constexpr std::size_t kPairingCodeLength = 8;
constexpr std::size_t kMinPasswordBytes = 8;
constexpr std::size_t kMaxPasswordBytes = 128;
constexpr const char *kPairingAlphabet = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789";
constexpr const char *kVerifierScheme = "pbkdf2-sha256";

std::string ToHex(const unsigned char *data, std::size_t length) {
    static const char *kDigits = "0123456789abcdef";
    std::string out;
    out.resize(length * 2);
    for (std::size_t i = 0; i < length; ++i) {
        out[i * 2] = kDigits[data[i] >> 4];
        out[i * 2 + 1] = kDigits[data[i] & 0x0f];
    }
    return out;
}

bool FromHex(const std::string &hex, std::string *out) {
    if (out == nullptr || hex.size() % 2 != 0) {
        return false;
    }
    std::string decoded;
    decoded.resize(hex.size() / 2);
    for (std::size_t i = 0; i < decoded.size(); ++i) {
        const auto nybble = [](char c) -> int {
            if (c >= '0' && c <= '9') {
                return c - '0';
            }
            if (c >= 'a' && c <= 'f') {
                return c - 'a' + 10;
            }
            if (c >= 'A' && c <= 'F') {
                return c - 'A' + 10;
            }
            return -1;
        };
        const int high = nybble(hex[i * 2]);
        const int low = nybble(hex[i * 2 + 1]);
        if (high < 0 || low < 0) {
            return false;
        }
        decoded[i] = static_cast<char>((high << 4) | low);
    }
    *out = std::move(decoded);
    return true;
}

bool RandomBytes(unsigned char *data, std::size_t length) {
    return RAND_bytes(data, static_cast<int>(length)) == 1;
}

std::string RandomHex(std::size_t byte_count) {
    std::vector<unsigned char> bytes(byte_count);
    if (!RandomBytes(bytes.data(), bytes.size())) {
        return "";
    }
    return ToHex(bytes.data(), bytes.size());
}

std::string GeneratePairingCode() {
    unsigned char bytes[kPairingCodeLength];
    if (!RandomBytes(bytes, kPairingCodeLength)) {
        return "";
    }
    std::string code;
    code.resize(kPairingCodeLength);
    for (std::size_t i = 0; i < kPairingCodeLength; ++i) {
        code[i] = kPairingAlphabet[bytes[i] % 32];
    }
    return code;
}

std::string NormalizePairingCode(const std::string &raw) {
    std::string out;
    out.reserve(raw.size());
    for (const char c : raw) {
        if (static_cast<unsigned char>(c) <= ' ') {
            continue;
        }
        out.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(c))));
    }
    return out;
}

bool ConstantTimeEqual(const std::string &left, const std::string &right) {
    const std::size_t size = left.size() < right.size() ? right.size() : left.size();
    std::string left_pad(size, '\0');
    std::string right_pad(size, '\0');
    if (!left.empty()) {
        std::copy(left.begin(), left.end(), left_pad.begin());
    }
    if (!right.empty()) {
        std::copy(right.begin(), right.end(), right_pad.begin());
    }
    const int cmp = CRYPTO_memcmp(left_pad.data(), right_pad.data(), size);
    return cmp == 0 && left.size() == right.size();
}

std::string CookieValue(const std::string &header, const std::string &name) {
    std::size_t start = 0;
    while (start < header.size()) {
        while (start < header.size() && (header[start] == ' ' || header[start] == ';')) {
            ++start;
        }
        if (start >= header.size()) {
            break;
        }
        const std::size_t eq = header.find('=', start);
        if (eq == std::string::npos) {
            break;
        }
        const std::string key = oc::config::trim(header.substr(start, eq - start));
        const std::size_t end = header.find(';', eq + 1);
        const std::string value =
            oc::config::trim(end == std::string::npos ? header.substr(eq + 1) : header.substr(eq + 1, end - eq - 1));
        if (key == name) {
            return value;
        }
        start = end == std::string::npos ? header.size() : end + 1;
    }
    return "";
}

std::string PasswordPolicyError(const std::string &password) {
    if (password.size() < kMinPasswordBytes) {
        return "password must be at least 8 characters";
    }
    if (password.size() > kMaxPasswordBytes) {
        return "password must be at most 128 characters";
    }
    return "";
}

bool DeriveKey(const std::string &password, const std::string &salt, int iterations, std::string *out) {
    if (out == nullptr || salt.size() != kSaltBytes || iterations < 1) {
        return false;
    }
    std::string hash(kHashBytes, '\0');
    if (PKCS5_PBKDF2_HMAC(password.data(), static_cast<int>(password.size()),
                          reinterpret_cast<const unsigned char *>(salt.data()), static_cast<int>(salt.size()),
                          iterations, EVP_sha256(), static_cast<int>(hash.size()),
                          reinterpret_cast<unsigned char *>(hash.data())) != 1) {
        return false;
    }
    *out = std::move(hash);
    return true;
}

HttpResponse AuthOk() {
    return JsonHttpBody(200, JsonValue::Object({
                                 {"ok", JsonValue::Bool(true)},
                                 {"paired", JsonValue::Bool(true)},
                                 {"authenticated", JsonValue::Bool(true)},
                             }));
}

} // namespace

AuthStore::AuthStore(oc::logging::Logger &logger, AuthStoreOptions options)
    : logger_(logger), options_(std::move(options)) {
    if (options_.pbkdf2_iterations < 1) {
        options_.pbkdf2_iterations = 1;
    }
    if (options_.max_auth_failures < 1) {
        options_.max_auth_failures = 1;
    }
    std::string error;
    ready_ = Initialize(&error);
    if (!ready_) {
        logger_.Error("webd", "auth store init failed");
    }
}

bool AuthStore::Ready() const {
    return ready_;
}

bool AuthStore::IsPaired() const {
    return oc::util::FileExists(VerifierPath());
}

std::optional<std::string> AuthStore::UnpairedSetupSecret() const {
    const std::lock_guard<std::mutex> lock(mutex_);
    if (IsPaired() || pairing_code_.empty()) {
        return std::nullopt;
    }
    return pairing_code_;
}

void AuthStore::AnnounceSetupSecret() const {
    const auto secret = UnpairedSetupSecret();
    if (!secret.has_value()) {
        return;
    }
    const std::string line = "Open Chime pairing code: " + *secret + "\n";
    if (isatty(STDOUT_FILENO) != 0) {
        const ssize_t written = write(STDOUT_FILENO, line.data(), line.size());
        (void)written;
    }
    const int fd = open("/dev/console", O_WRONLY | O_NOCTTY);
    if (fd >= 0) {
        const ssize_t written = write(fd, line.data(), line.size());
        (void)written;
        close(fd);
    }
}

HttpResponse AuthStore::HandleStatus(const HttpRequest &request) {
    if (!ready_) {
        return JsonHttpError(503, "auth_unavailable");
    }
    const std::lock_guard<std::mutex> lock(mutex_);
    ReapLocked();
    const SessionView session = InspectLocked(request);
    return JsonHttpBody(200, JsonValue::Object({
                                 {"paired", JsonValue::Bool(IsPaired())},
                                 {"authenticated", JsonValue::Bool(session.authenticated)},
                             }));
}

HttpResponse AuthStore::HandlePair(const HttpRequest &request) {
    if (!ready_) {
        return JsonHttpError(503, "auth_unavailable");
    }
    if (request.body.size() > 64 * 1024) {
        return JsonHttpError(400, "payload_too_large");
    }

    const JsonParseResult parsed = ParseJson(request.body);
    if (!parsed.success || parsed.value.type() != JsonValue::Type::kObject) {
        return JsonHttpError(400, "invalid_json", "payload must be an object");
    }

    std::vector<ValidationError> errors;
    const auto pairing_code = ReadRequiredString(parsed.value, "pairing_code", &errors);
    const auto password = ReadRequiredString(parsed.value, "password", &errors);
    if (!errors.empty() || !pairing_code.has_value() || !password.has_value()) {
        return JsonHttpError(400, "validation_failed", "pairing_code and password are required");
    }

    const std::string policy_error = PasswordPolicyError(*password);
    if (!policy_error.empty()) {
        return JsonHttpError(400, "validation_failed", policy_error);
    }

    const std::string client = ClientKey(request);
    std::string expected_code;
    {
        const std::lock_guard<std::mutex> lock(mutex_);
        ReapLocked();
        if (IsPaired()) {
            return JsonHttpError(403, "pairing_closed");
        }
        if (IsLimitedLocked(pair_attempts_, client)) {
            return JsonHttpError(429, "rate_limited");
        }
        expected_code = pairing_code_;
    }

    if (!PairingCodeMatches(*pairing_code, expected_code)) {
        const std::lock_guard<std::mutex> lock(mutex_);
        ReapLocked();
        if (!AllowAttempt(pair_attempts_, client)) {
            return JsonHttpError(429, "rate_limited");
        }
        return JsonHttpError(401, "invalid_credentials");
    }

    std::string encoded;
    std::string write_error;
    if (!DeriveVerifierPayload(*password, &encoded, &write_error)) {
        logger_.Error("webd", "failed to write admin verifier");
        return JsonHttpError(500, "pair_failed");
    }

    const std::lock_guard<std::mutex> lock(mutex_);
    ReapLocked();
    if (IsPaired()) {
        return JsonHttpError(403, "pairing_closed");
    }
    if (!oc::util::AtomicWriteFile(VerifierPath(), encoded, kAuthFileMode, &write_error)) {
        logger_.Error("webd", "failed to write admin verifier");
        return JsonHttpError(500, "pair_failed");
    }

    std::error_code ec;
    std::filesystem::remove(PairingPath(), ec);
    pairing_code_.clear();
    pair_attempts_.clear();
    ResetAttempts(login_attempts_, client);
    const std::string session_id = CreateSessionLocked();
    if (session_id.empty()) {
        return JsonHttpError(500, "session_failed");
    }
    HttpResponse response = AuthOk();
    AttachSessionCookies(&response, session_id, sessions_[session_id].csrf_token);
    return response;
}

HttpResponse AuthStore::HandleLogin(const HttpRequest &request) {
    if (!ready_) {
        return JsonHttpError(503, "auth_unavailable");
    }
    if (!IsPaired()) {
        return JsonHttpError(403, "unpaired");
    }
    if (request.body.size() > 64 * 1024) {
        return JsonHttpError(400, "payload_too_large");
    }

    const JsonParseResult parsed = ParseJson(request.body);
    if (!parsed.success || parsed.value.type() != JsonValue::Type::kObject) {
        return JsonHttpError(400, "invalid_json", "payload must be an object");
    }

    std::vector<ValidationError> errors;
    const auto password = ReadRequiredString(parsed.value, "password", &errors);
    if (!errors.empty() || !password.has_value()) {
        return JsonHttpError(400, "validation_failed", "password is required");
    }

    const std::string client = ClientKey(request);
    {
        const std::lock_guard<std::mutex> lock(mutex_);
        ReapLocked();
        if (IsLimitedLocked(login_attempts_, client)) {
            return JsonHttpError(429, "rate_limited");
        }
    }

    const bool accepted = VerifyPassword(*password);
    const std::lock_guard<std::mutex> lock(mutex_);
    ReapLocked();
    if (!accepted) {
        if (!AllowAttempt(login_attempts_, client)) {
            return JsonHttpError(429, "rate_limited");
        }
        return JsonHttpError(401, "invalid_credentials");
    }

    ResetAttempts(login_attempts_, client);
    const std::string session_id = CreateSessionLocked();
    if (session_id.empty()) {
        return JsonHttpError(500, "session_failed");
    }
    HttpResponse response = AuthOk();
    AttachSessionCookies(&response, session_id, sessions_[session_id].csrf_token);
    return response;
}

HttpResponse AuthStore::HandleLogout(const HttpRequest &request) {
    const std::lock_guard<std::mutex> lock(mutex_);
    ReapLocked();
    const SessionView session = InspectLocked(request);
    if (session.authenticated) {
        ClearSessionLocked(session.session_id);
    }
    HttpResponse response = JsonHttpBody(200, JsonValue::Object({
                                                  {"ok", JsonValue::Bool(true)},
                                                  {"paired", JsonValue::Bool(IsPaired())},
                                                  {"authenticated", JsonValue::Bool(false)},
                                              }));
    AttachClearedCookies(&response);
    return response;
}

SessionView AuthStore::Inspect(const HttpRequest &request) const {
    const std::lock_guard<std::mutex> lock(mutex_);
    ReapLocked();
    return InspectLocked(request);
}

bool AuthStore::ClearAuthenticationState(std::string *error) {
    const std::lock_guard<std::mutex> lock(mutex_);
    std::error_code verifier_ec;
    std::filesystem::remove(VerifierPath(), verifier_ec);
    if (verifier_ec) {
        if (error != nullptr) {
            *error = "failed to remove admin verifier";
        }
        return false;
    }
    std::error_code pairing_ec;
    std::filesystem::remove(PairingPath(), pairing_ec);
    if (pairing_ec) {
        if (error != nullptr) {
            *error = "failed to remove pairing secret";
        }
        return false;
    }
    sessions_.clear();
    pairing_code_.clear();
    pair_attempts_.clear();
    login_attempts_.clear();
    if (!LoadOrCreatePairingSecret(error)) {
        return false;
    }
    if (IsPaired()) {
        if (error != nullptr) {
            *error = "device is still paired after reset";
        }
        return false;
    }
    return true;
}

std::size_t AuthStore::SessionEntryCount() const {
    const std::lock_guard<std::mutex> lock(mutex_);
    ReapLocked();
    return sessions_.size();
}

std::size_t AuthStore::RateLimitEntryCount() const {
    const std::lock_guard<std::mutex> lock(mutex_);
    ReapLocked();
    return pair_attempts_.size() + login_attempts_.size();
}

std::string AuthStore::VerifierPath() const {
    return (std::filesystem::path(options_.auth_dir) / kVerifierFileName).string();
}

std::string AuthStore::PairingPath() const {
    return (std::filesystem::path(options_.auth_dir) / kPairingFileName).string();
}

bool AuthStore::EnsureAuthDirectory(std::string *error) const {
    std::error_code ec;
    std::filesystem::create_directories(options_.auth_dir, ec);
    if (ec) {
        if (error != nullptr) {
            *error = "failed to create auth directory";
        }
        return false;
    }
    if (chmod(options_.auth_dir.c_str(), kAuthDirMode) != 0) {
        if (error != nullptr) {
            *error = "failed to set auth directory mode";
        }
        return false;
    }
    return true;
}

bool AuthStore::Initialize(std::string *error) {
    const std::lock_guard<std::mutex> lock(mutex_);
    if (!EnsureAuthDirectory(error)) {
        return false;
    }
    if (IsPaired()) {
        pairing_code_.clear();
        std::error_code ec;
        std::filesystem::remove(PairingPath(), ec);
        return true;
    }
    if (!options_.bootstrap_password.empty()) {
        const std::string policy_error = PasswordPolicyError(options_.bootstrap_password);
        if (!policy_error.empty()) {
            logger_.Error("webd", "bootstrap password rejected");
            return LoadOrCreatePairingSecret(error);
        }
        if (!WriteVerifier(options_.bootstrap_password, error)) {
            logger_.Error("webd", "bootstrap pairing failed");
            return false;
        }
        pairing_code_.clear();
        return true;
    }
    return LoadOrCreatePairingSecret(error);
}

bool AuthStore::LoadOrCreatePairingSecret(std::string *error) {
    const std::string existing = oc::util::ReadTrimmedFile(PairingPath());
    if (!existing.empty()) {
        pairing_code_ = NormalizePairingCode(existing);
        if (pairing_code_.size() == kPairingCodeLength) {
            return true;
        }
    }
    if (!options_.pairing_code_override.empty()) {
        pairing_code_ = NormalizePairingCode(options_.pairing_code_override);
    } else {
        pairing_code_ = GeneratePairingCode();
    }
    if (pairing_code_.size() != kPairingCodeLength) {
        if (error != nullptr) {
            *error = "failed to create pairing secret";
        }
        pairing_code_.clear();
        return false;
    }
    return oc::util::AtomicWriteFile(PairingPath(), pairing_code_ + "\n", kAuthFileMode, error);
}

bool AuthStore::DeriveVerifierPayload(const std::string &password, std::string *encoded, std::string *error) const {
    if (encoded == nullptr) {
        return false;
    }
    unsigned char salt_bytes[kSaltBytes];
    if (!RandomBytes(salt_bytes, kSaltBytes)) {
        if (error != nullptr) {
            *error = "failed to generate salt";
        }
        return false;
    }
    std::string salt(reinterpret_cast<const char *>(salt_bytes), kSaltBytes);
    std::string hash;
    if (!DeriveKey(password, salt, options_.pbkdf2_iterations, &hash)) {
        if (error != nullptr) {
            *error = "failed to derive verifier";
        }
        return false;
    }
    *encoded = std::string(kVerifierScheme) + "$" + std::to_string(options_.pbkdf2_iterations) + "$" +
               ToHex(salt_bytes, kSaltBytes) + "$" +
               ToHex(reinterpret_cast<const unsigned char *>(hash.data()), hash.size()) + "\n";
    return true;
}

bool AuthStore::WriteVerifier(const std::string &password, std::string *error) const {
    std::string encoded;
    if (!DeriveVerifierPayload(password, &encoded, error)) {
        return false;
    }
    return oc::util::AtomicWriteFile(VerifierPath(), encoded, kAuthFileMode, error);
}

bool AuthStore::ReadVerifier(Verifier *verifier) const {
    if (verifier == nullptr) {
        return false;
    }
    const std::string raw = oc::util::ReadTrimmedFile(VerifierPath());
    const std::size_t first = raw.find('$');
    const std::size_t second = first == std::string::npos ? std::string::npos : raw.find('$', first + 1);
    const std::size_t third = second == std::string::npos ? std::string::npos : raw.find('$', second + 1);
    if (first == std::string::npos || second == std::string::npos || third == std::string::npos) {
        return false;
    }
    if (raw.substr(0, first) != kVerifierScheme) {
        return false;
    }
    char *end = nullptr;
    const long iterations = std::strtol(raw.c_str() + first + 1, &end, 10);
    if (end == nullptr || *end != '$' || iterations < 1 || iterations > 10000000) {
        return false;
    }
    std::string salt;
    std::string hash;
    if (!FromHex(raw.substr(second + 1, third - second - 1), &salt) || !FromHex(raw.substr(third + 1), &hash)) {
        return false;
    }
    if (salt.size() != kSaltBytes || hash.size() != kHashBytes) {
        return false;
    }
    verifier->iterations = static_cast<int>(iterations);
    verifier->salt = std::move(salt);
    verifier->hash = std::move(hash);
    return true;
}

bool AuthStore::VerifyPassword(const std::string &password) const {
    Verifier stored;
    const bool loaded = ReadVerifier(&stored);
    Verifier use = stored;
    if (!loaded) {
        use.iterations = options_.pbkdf2_iterations;
        use.salt.assign(kSaltBytes, '\0');
        use.hash.assign(kHashBytes, '\0');
    }
    std::string derived;
    if (!DeriveKey(password, use.salt, use.iterations, &derived)) {
        return false;
    }
    return loaded && ConstantTimeEqual(derived, use.hash);
}

bool AuthStore::PairingCodeMatches(const std::string &provided, const std::string &expected) const {
    const bool have_secret = expected.size() == kPairingCodeLength;
    const std::string compare = have_secret ? expected : std::string(kPairingCodeLength, 'X');
    return have_secret && ConstantTimeEqual(NormalizePairingCode(provided), compare);
}

std::string AuthStore::ClientKey(const HttpRequest &request) const {
    return request.peer_address.empty() ? "unknown" : request.peer_address;
}

void AuthStore::ReapLocked() const {
    const auto now = Now();
    for (auto it = sessions_.begin(); it != sessions_.end();) {
        if (now >= it->second.expires_at) {
            it = sessions_.erase(it);
        } else {
            ++it;
        }
    }
    ReapAttemptWindowsLocked(pair_attempts_);
    ReapAttemptWindowsLocked(login_attempts_);
}

void AuthStore::ReapAttemptWindowsLocked(std::map<std::string, AttemptWindow> &windows) const {
    const auto now = Now();
    for (auto it = windows.begin(); it != windows.end();) {
        if (it->second.window_start.time_since_epoch().count() == 0 ||
            now - it->second.window_start >= options_.auth_window) {
            it = windows.erase(it);
        } else {
            ++it;
        }
    }
}

void AuthStore::TrimAttemptWindowsLocked(std::map<std::string, AttemptWindow> &windows) const {
    while (windows.size() >= kMaxAuthAttemptClients) {
        auto oldest = windows.begin();
        for (auto it = windows.begin(); it != windows.end(); ++it) {
            if (it->second.window_start < oldest->second.window_start) {
                oldest = it;
            }
        }
        windows.erase(oldest);
    }
}

bool AuthStore::IsLimitedLocked(const std::map<std::string, AttemptWindow> &windows, const std::string &key) const {
    const auto it = windows.find(key);
    if (it == windows.end()) {
        return false;
    }
    const auto now = Now();
    if (it->second.window_start.time_since_epoch().count() == 0 ||
        now - it->second.window_start >= options_.auth_window) {
        return false;
    }
    return it->second.failures >= options_.max_auth_failures;
}

bool AuthStore::AllowAttempt(std::map<std::string, AttemptWindow> &windows, const std::string &key) {
    ReapAttemptWindowsLocked(windows);
    if (windows.find(key) == windows.end()) {
        TrimAttemptWindowsLocked(windows);
    }
    AttemptWindow &window = windows[key];
    const auto now = Now();
    if (window.window_start.time_since_epoch().count() == 0 || now - window.window_start >= options_.auth_window) {
        window.window_start = now;
        window.failures = 0;
    }
    if (window.failures >= options_.max_auth_failures) {
        return false;
    }
    ++window.failures;
    return true;
}

void AuthStore::ResetAttempts(std::map<std::string, AttemptWindow> &windows, const std::string &key) {
    windows.erase(key);
}

SessionView AuthStore::InspectLocked(const HttpRequest &request) const {
    SessionView view;
    const std::string session_id = CookieValue(RequestHeader(request, "cookie"), kSessionCookieName);
    if (session_id.empty()) {
        return view;
    }
    const auto it = sessions_.find(session_id);
    if (it == sessions_.end()) {
        return view;
    }
    if (Now() >= it->second.expires_at) {
        sessions_.erase(it);
        return view;
    }
    view.authenticated = true;
    view.session_id = session_id;
    const std::string csrf_header = RequestHeader(request, kCsrfHeaderName);
    view.csrf_valid = ConstantTimeEqual(csrf_header, it->second.csrf_token);
    return view;
}

std::string AuthStore::CreateSessionLocked() {
    ReapLocked();
    while (sessions_.size() >= kMaxAuthSessions) {
        auto oldest = sessions_.begin();
        for (auto it = sessions_.begin(); it != sessions_.end(); ++it) {
            if (it->second.expires_at < oldest->second.expires_at) {
                oldest = it;
            }
        }
        sessions_.erase(oldest);
    }
    const std::string session_id = RandomHex(kTokenBytes);
    const std::string csrf_token = RandomHex(kTokenBytes);
    if (session_id.empty() || csrf_token.empty()) {
        return "";
    }
    Session session;
    session.csrf_token = csrf_token;
    session.expires_at = Now() + options_.session_ttl;
    sessions_[session_id] = std::move(session);
    return session_id;
}

void AuthStore::ClearSessionLocked(const std::string &session_id) {
    sessions_.erase(session_id);
}

void AuthStore::AttachSessionCookies(HttpResponse *response, const std::string &session_id,
                                     const std::string &csrf_token) const {
    if (response == nullptr) {
        return;
    }
    const std::string max_age = "Max-Age=" + std::to_string(options_.session_ttl.count());
    response->set_cookies.push_back(std::string(kSessionCookieName) + "=" + session_id + "; Path=/; " + max_age +
                                    CookieFlags(true));
    response->set_cookies.push_back(std::string(kCsrfCookieName) + "=" + csrf_token + "; Path=/; " + max_age +
                                    CookieFlags(false));
}

void AuthStore::AttachClearedCookies(HttpResponse *response) const {
    if (response == nullptr) {
        return;
    }
    response->set_cookies.push_back(std::string(kSessionCookieName) + "=; Path=/; Max-Age=0" + CookieFlags(true));
    response->set_cookies.push_back(std::string(kCsrfCookieName) + "=; Path=/; Max-Age=0" + CookieFlags(false));
}

std::string AuthStore::CookieFlags(bool http_only) const {
    std::string flags = "; SameSite=Strict";
    if (http_only) {
        flags += "; HttpOnly";
    }
    if (options_.cookie_secure) {
        flags += "; Secure";
    }
    return flags;
}

std::chrono::steady_clock::time_point AuthStore::Now() const {
    return std::chrono::steady_clock::now();
}

} // namespace chime::webd
