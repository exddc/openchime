#include <algorithm>
#include <array>
#include <cstdint>
#include <string>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include <openssl/ssl.h>

#include "chime/webd_apply_manager.h"
#include "chime/webd_config_store.h"
#include "chime/webd_json.h"
#include "chime/webd_web_server.h"
#include "chime/webd_wifi_scan.h"
#include "doctest.h"
#include "test_support.h"

namespace {

constexpr const char *kCoreConfig = R"(
mqtt_host=broker.local
mqtt_port=1883
mqtt_topics=doorbell/ring
)";

std::string TlsExchange(const std::string &bind_address, int port, const std::string &request) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    REQUIRE(fd >= 0);

    struct timeval timeout{};
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

    struct sockaddr_in address{};
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

} // namespace

TEST_SUITE("web_server_tls") {
    TEST_CASE("starts, serves HTTPS, closes the connection, and stops") {
        const ScopedTempDir tmp;
        const auto conf = tmp.WriteFile("chime.conf", kCoreConfig);
        NullLogger logger;
        chime::webd::ConfigStore store(logger, conf.string(), (tmp.path() / "wpa.conf").string());
        chime::webd::WifiScanner scanner(logger, "wlan0");
        chime::webd::ApplyManager apply(logger, "true", "true");
        chime::webd::WebServer server(logger, store, scanner, apply, "127.0.0.1", 0, (tmp.path() / "cert.pem").string(),
                                      (tmp.path() / "key.pem").string(), "", (tmp.path() / "topics.txt").string(),
                                      (tmp.path() / "sounds").string(), (tmp.path() / "ring.wav").string());

        REQUIRE(server.Start());
        CHECK(server.port() > 0);

        const std::string ok =
            TlsExchange("127.0.0.1", server.port(), "GET /api/v1/config/core HTTP/1.1\r\nHost: 127.0.0.1\r\n\r\n");
        CHECK(ok.find("HTTP/1.1 200 OK\r\n") == 0);
        CHECK(ok.find("Connection: close\r\n") != std::string::npos);
        const auto parsed = chime::webd::ParseJson(ok.substr(ok.find("\r\n\r\n") + 4));
        REQUIRE(parsed.success);
        CHECK(chime::webd::GetObjectField(parsed.value, "mqtt_host").has_value());
        CHECK_FALSE(chime::webd::GetObjectField(parsed.value, "mqtt_password").has_value());

        const std::string bad = TlsExchange("127.0.0.1", server.port(), "GET / HTTP/2.0\r\n\r\n");
        CHECK(bad.find("HTTP/1.1 400 Bad Request\r\n") == 0);
        CHECK(bad.find("Connection: close\r\n") != std::string::npos);

        server.Stop();
        server.Stop();
    }
}
