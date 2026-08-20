#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <string>
#include <thread>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include <openssl/ssl.h>

#include "chime/webd_api.h"
#include "chime/webd_apply_manager.h"
#include "chime/webd_auth.h"
#include "chime/webd_config_store.h"
#include "doctest.h"
#include "oc/http/tls_server.h"
#include "oc/json/json.h"
#include "oc/wifi/scan.h"
#include "test_support.h"

namespace {

constexpr const char *kCoreConfig = R"(
mqtt_host=broker.local
mqtt_port=1883
mqtt_topics=doorbell/ring
)";

std::string TlsExchange(const std::string &bind_address, int port, const std::string &request,
                        std::chrono::seconds timeout = std::chrono::seconds(5)) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    REQUIRE(fd >= 0);

    struct timeval timeout_tv {};
    timeout_tv.tv_sec = static_cast<time_t>(timeout.count());
    timeout_tv.tv_usec = 0;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout_tv, sizeof(timeout_tv));
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &timeout_tv, sizeof(timeout_tv));

    struct sockaddr_in address {};
    address.sin_family = AF_INET;
    address.sin_port = htons(static_cast<uint16_t>(port));
    REQUIRE(inet_pton(AF_INET, bind_address.c_str(), &address.sin_addr) == 1);
    REQUIRE(connect(fd, reinterpret_cast<struct sockaddr *>(&address), sizeof(address)) == 0);

    SSL_CTX *ctx = SSL_CTX_new(TLS_client_method());
    REQUIRE(ctx != nullptr);
    SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, nullptr);

    SSL *ssl = SSL_new(ctx);
    REQUIRE(ssl != nullptr);
    SSL_set_fd(ssl, fd);
    REQUIRE(SSL_connect(ssl) == 1);

    std::size_t offset = 0;
    while (offset < request.size()) {
        const int written =
            SSL_write(ssl, request.data() + offset,
                      static_cast<int>(std::min(request.size() - offset, static_cast<std::size_t>(2048))));
        REQUIRE(written > 0);
        offset += static_cast<std::size_t>(written);
    }

    std::string response;
    std::array<char, 2048> buffer{};
    for (;;) {
        const int bytes = SSL_read(ssl, buffer.data(), static_cast<int>(buffer.size()));
        if (bytes <= 0) {
            break;
        }
        response.append(buffer.data(), static_cast<std::size_t>(bytes));
    }

    SSL_shutdown(ssl);
    SSL_free(ssl);
    SSL_CTX_free(ctx);
    close(fd);
    return response;
}

struct ChimeHttps {
    chime::webd::ConfigStore store;
    oc::wifi::WifiScanner scanner;
    chime::webd::ApplyManager apply;
    chime::webd::AuthStore auth;
    chime::webd::WebApi api;
    oc::http::TlsServer server;

    ChimeHttps(oc::logging::Logger &logger, const ScopedTempDir &tmp, chime::webd::AuthStoreOptions auth_options)
        : store(logger, (tmp.path() / "chime.conf").string(), (tmp.path() / "wpa.conf").string()),
          scanner(logger, "wlan0"), apply(logger, "true", "true"), auth(logger, std::move(auth_options)),
          api(logger, store, scanner, apply, auth, (tmp.path() / "ui").string(), (tmp.path() / "topics.txt").string(),
              (tmp.path() / "sounds").string(), (tmp.path() / "ring.wav").string()),
          server(
              logger, MakeTlsConfig(tmp), [this](const oc::http::HttpRequest &request) { return api.Handle(request); },
              chime::webd::WebApi::OffloadToSlowWorker) {}

    static oc::http::TlsServerConfig MakeTlsConfig(const ScopedTempDir &tmp) {
        oc::http::TlsServerConfig config;
        config.bind_address = "127.0.0.1";
        config.port = 0;
        config.cert_path = (tmp.path() / "cert.pem").string();
        config.key_path = (tmp.path() / "key.pem").string();
        config.cert_common_name = "chime.local";
        config.log_component = "webd";
        return config;
    }
};

} // namespace

TEST_SUITE("web_server_tls") {
    TEST_CASE("starts, serves HTTPS, closes the connection, and stops") {
        const ScopedTempDir tmp;
        tmp.WriteFile("chime.conf", kCoreConfig);
        NullLogger logger;
        chime::webd::AuthStoreOptions auth_options;
        auth_options.auth_dir = (tmp.path() / "auth").string();
        auth_options.bootstrap_password = "test-password-ok";
        auth_options.pbkdf2_iterations = 2;
        ChimeHttps https(logger, tmp, auth_options);
        auto &server = https.server;

        REQUIRE(server.Start());
        CHECK(server.port() > 0);

        const std::string login =
            TlsExchange("127.0.0.1", server.port(),
                        "POST /api/v1/auth/login HTTP/1.1\r\nHost: 127.0.0.1\r\nContent-Type: application/json\r\n"
                        "Content-Length: 31\r\n\r\n{\"password\":\"test-password-ok\"}");
        CHECK(login.find("HTTP/1.1 200 OK\r\n") == 0);
        CHECK(login.find("Set-Cookie: chime_session=") != std::string::npos);
        const auto session_pos = login.find("Set-Cookie: chime_session=");
        REQUIRE(session_pos != std::string::npos);
        const auto session_end = login.find(';', session_pos);
        REQUIRE(session_end != std::string::npos);
        const std::string session_cookie =
            login.substr(session_pos + std::string("Set-Cookie: ").size(),
                         session_end - (session_pos + std::string("Set-Cookie: ").size()));

        const std::string ok = TlsExchange(
            "127.0.0.1", server.port(),
            "GET /api/v1/config/core HTTP/1.1\r\nHost: 127.0.0.1\r\nCookie: " + session_cookie + "\r\n\r\n");
        CHECK(ok.find("HTTP/1.1 200 OK\r\n") == 0);
        CHECK(ok.find("Connection: close\r\n") != std::string::npos);
        const auto parsed = oc::json::ParseJson(ok.substr(ok.find("\r\n\r\n") + 4));
        REQUIRE(parsed.success);
        CHECK(oc::json::GetObjectField(parsed.value, "mqtt_host").has_value());
        CHECK_FALSE(oc::json::GetObjectField(parsed.value, "mqtt_password").has_value());

        const std::string unauthenticated =
            TlsExchange("127.0.0.1", server.port(), "GET /api/v1/config/core HTTP/1.1\r\nHost: 127.0.0.1\r\n\r\n");
        CHECK(unauthenticated.find("HTTP/1.1 401 Unauthorized\r\n") == 0);

        const std::string bad = TlsExchange("127.0.0.1", server.port(), "GET / HTTP/2.0\r\n\r\n");
        CHECK(bad.find("HTTP/1.1 400 Bad Request\r\n") == 0);
        CHECK(bad.find("Connection: close\r\n") != std::string::npos);

        server.Stop();
        server.Stop();
    }

    TEST_CASE("version stays available while two slow login hashes run") {
        const ScopedTempDir tmp;
        tmp.WriteFile("chime.conf", kCoreConfig);
        NullLogger logger;
        chime::webd::AuthStoreOptions auth_options;
        auth_options.auth_dir = (tmp.path() / "auth").string();
        auth_options.bootstrap_password = "test-password-ok";
        auth_options.pbkdf2_iterations = 2000000;
        ChimeHttps https(logger, tmp, auth_options);
        auto &server = https.server;

        REQUIRE(server.Start());

        const auto login_request =
            "POST /api/v1/auth/login HTTP/1.1\r\nHost: 127.0.0.1\r\nContent-Type: application/json\r\n"
            "Content-Length: 31\r\n\r\n{\"password\":\"test-password-ok\"}";
        std::string login_one;
        std::string login_two;
        std::chrono::milliseconds login_one_ms{0};
        std::chrono::milliseconds login_two_ms{0};
        const auto run_login = [&](std::string *response, std::chrono::milliseconds *elapsed) {
            const auto started = std::chrono::steady_clock::now();
            *response = TlsExchange("127.0.0.1", server.port(), login_request, std::chrono::seconds(30));
            *elapsed =
                std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - started);
        };
        std::thread first([&]() { run_login(&login_one, &login_one_ms); });
        std::thread second([&]() { run_login(&login_two, &login_two_ms); });

        std::this_thread::sleep_for(std::chrono::milliseconds(80));
        const auto version_started = std::chrono::steady_clock::now();
        const std::string version =
            TlsExchange("127.0.0.1", server.port(), "GET /api/v1/system/version HTTP/1.1\r\nHost: 127.0.0.1\r\n\r\n");
        const auto version_ms =
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - version_started);

        first.join();
        second.join();
        CHECK(login_one.find("HTTP/1.1 200 OK\r\n") == 0);
        CHECK(login_two.find("HTTP/1.1 200 OK\r\n") == 0);
        CHECK(version.find("HTTP/1.1 200 OK\r\n") == 0);
        const auto faster_login = std::min(login_one_ms, login_two_ms);
        CHECK(faster_login > std::chrono::milliseconds(200));
        CHECK(version_ms * 3 < faster_login);

        server.Stop();
    }

    TEST_CASE("Stop returns promptly when a client stalls in the TLS handshake") {
        const ScopedTempDir tmp;
        tmp.WriteFile("chime.conf", kCoreConfig);
        NullLogger logger;
        chime::webd::AuthStoreOptions auth_options;
        auth_options.auth_dir = (tmp.path() / "auth").string();
        auth_options.bootstrap_password = "test-password-ok";
        auth_options.pbkdf2_iterations = 2;
        ChimeHttps https(logger, tmp, auth_options);
        auto &server = https.server;

        REQUIRE(server.Start());

        int fd = socket(AF_INET, SOCK_STREAM, 0);
        REQUIRE(fd >= 0);
        struct sockaddr_in address {};
        address.sin_family = AF_INET;
        address.sin_port = htons(static_cast<uint16_t>(server.port()));
        REQUIRE(inet_pton(AF_INET, "127.0.0.1", &address.sin_addr) == 1);
        REQUIRE(connect(fd, reinterpret_cast<struct sockaddr *>(&address), sizeof(address)) == 0);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        const auto started = std::chrono::steady_clock::now();
        server.Stop();
        const auto elapsed = std::chrono::steady_clock::now() - started;
        CHECK(elapsed < std::chrono::seconds(1));
        close(fd);
    }
}
