#include "chime/webd_web_server.h"

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <string>
#include <thread>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#include <openssl/bn.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/x509.h>

#include "chime/webd_http.h"
#include "chime/webd_json_http.h"
#include "oc/logging/logger.h"

namespace chime::webd {
namespace {

bool WriteAllSsl(SSL *ssl, const std::string &data) {
    std::size_t offset = 0;
    while (offset < data.size()) {
        const int written =
            SSL_write(ssl, data.data() + offset, static_cast<int>(std::min<std::size_t>(data.size() - offset, 16384)));
        if (written <= 0) {
            return false;
        }
        offset += static_cast<std::size_t>(written);
    }
    return true;
}

bool GenerateSelfSignedCertificate(const std::string &cert_path, const std::string &key_path, std::string *error) {
    EVP_PKEY *pkey = EVP_PKEY_new();
    if (pkey == nullptr) {
        if (error != nullptr) {
            *error = "EVP_PKEY_new failed";
        }
        return false;
    }

    RSA *rsa = RSA_new();
    BIGNUM *exponent = BN_new();
    X509 *cert = nullptr;
    X509_NAME *name = nullptr;
    FILE *key_file = nullptr;
    FILE *cert_file = nullptr;

    bool success = false;

    if (rsa == nullptr || exponent == nullptr) {
        if (error != nullptr) {
            *error = "RSA allocation failed";
        }
        goto cleanup;
    }

    if (BN_set_word(exponent, RSA_F4) != 1) {
        if (error != nullptr) {
            *error = "BN_set_word failed";
        }
        goto cleanup;
    }

    if (RSA_generate_key_ex(rsa, 2048, exponent, nullptr) != 1) {
        if (error != nullptr) {
            *error = "RSA_generate_key_ex failed";
        }
        goto cleanup;
    }

    if (EVP_PKEY_assign_RSA(pkey, rsa) != 1) {
        if (error != nullptr) {
            *error = "EVP_PKEY_assign_RSA failed";
        }
        goto cleanup;
    }
    rsa = nullptr;

    cert = X509_new();
    if (cert == nullptr) {
        if (error != nullptr) {
            *error = "X509_new failed";
        }
        goto cleanup;
    }

    X509_set_version(cert, 2);
    ASN1_INTEGER_set(X509_get_serialNumber(cert), static_cast<long>(time(nullptr)));
    X509_gmtime_adj(X509_get_notBefore(cert), 0);
    X509_gmtime_adj(X509_get_notAfter(cert), 60L * 60L * 24L * 365L * 10L);
    X509_set_pubkey(cert, pkey);

    name = X509_get_subject_name(cert);
    X509_NAME_add_entry_by_txt(name, "C", MBSTRING_ASC, reinterpret_cast<const unsigned char *>("US"), -1, -1, 0);
    X509_NAME_add_entry_by_txt(name, "O", MBSTRING_ASC, reinterpret_cast<const unsigned char *>("OpenChime"), -1, -1,
                               0);
    X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, reinterpret_cast<const unsigned char *>("chime.local"), -1, -1,
                               0);
    X509_set_issuer_name(cert, name);

    if (X509_sign(cert, pkey, EVP_sha256()) == 0) {
        if (error != nullptr) {
            *error = "X509_sign failed";
        }
        goto cleanup;
    }

    key_file = fopen(key_path.c_str(), "w");
    if (key_file == nullptr) {
        if (error != nullptr) {
            *error = "failed to open key file";
        }
        goto cleanup;
    }
    if (PEM_write_PrivateKey(key_file, pkey, nullptr, nullptr, 0, nullptr, nullptr) != 1) {
        if (error != nullptr) {
            *error = "PEM_write_PrivateKey failed";
        }
        goto cleanup;
    }

    cert_file = fopen(cert_path.c_str(), "w");
    if (cert_file == nullptr) {
        if (error != nullptr) {
            *error = "failed to open cert file";
        }
        goto cleanup;
    }
    if (PEM_write_X509(cert_file, cert) != 1) {
        if (error != nullptr) {
            *error = "PEM_write_X509 failed";
        }
        goto cleanup;
    }

    chmod(key_path.c_str(), 0600);
    chmod(cert_path.c_str(), 0644);
    success = true;

cleanup:
    if (key_file != nullptr) {
        fclose(key_file);
    }
    if (cert_file != nullptr) {
        fclose(cert_file);
    }
    if (rsa != nullptr) {
        RSA_free(rsa);
    }
    if (exponent != nullptr) {
        BN_free(exponent);
    }
    if (cert != nullptr) {
        X509_free(cert);
    }
    EVP_PKEY_free(pkey);

    return success;
}

} // namespace

WebServer::WebServer(oc::logging::Logger &logger, ConfigStore &config_store, WifiScanner &wifi_scanner,
                     ApplyManager &apply_manager, std::string bind_address, int port, std::string cert_path,
                     std::string key_path, std::string ui_dist_dir, std::string observed_topics_path,
                     std::string ring_sounds_dir, std::string active_ring_sound_path)
    : logger_(logger), bind_address_(std::move(bind_address)), port_(port), cert_path_(std::move(cert_path)),
      key_path_(std::move(key_path)),
      api_(logger, config_store, wifi_scanner, apply_manager, std::move(ui_dist_dir), std::move(observed_topics_path),
           std::move(ring_sounds_dir), std::move(active_ring_sound_path)) {}

WebServer::~WebServer() {
    Stop();
}

bool WebServer::Start() {
    if (running_.load()) {
        return true;
    }

    std::string error;
    if (!EnsureTlsMaterial(&error)) {
        logger_.Error("webd", "TLS setup failed: " + error);
        return false;
    }

    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();

    SSL_CTX *ctx = SSL_CTX_new(TLS_server_method());
    if (ctx == nullptr) {
        logger_.Error("webd", "SSL_CTX_new failed");
        return false;
    }

    SSL_CTX_set_min_proto_version(ctx, TLS1_2_VERSION);

    if (SSL_CTX_use_certificate_file(ctx, cert_path_.c_str(), SSL_FILETYPE_PEM) != 1) {
        logger_.Error("webd", "failed to load TLS certificate from " + cert_path_);
        SSL_CTX_free(ctx);
        return false;
    }

    if (SSL_CTX_use_PrivateKey_file(ctx, key_path_.c_str(), SSL_FILETYPE_PEM) != 1) {
        logger_.Error("webd", "failed to load TLS private key from " + key_path_);
        SSL_CTX_free(ctx);
        return false;
    }

    listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd_ < 0) {
        logger_.Error("webd", "socket() failed");
        SSL_CTX_free(ctx);
        return false;
    }

    const int reuse = 1;
    setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    struct sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(static_cast<uint16_t>(port_));
    if (inet_pton(AF_INET, bind_address_.c_str(), &address.sin_addr) != 1) {
        logger_.Error("webd", "invalid bind address: " + bind_address_);
        close(listen_fd_);
        listen_fd_ = -1;
        SSL_CTX_free(ctx);
        return false;
    }

    if (bind(listen_fd_, reinterpret_cast<struct sockaddr *>(&address), sizeof(address)) != 0) {
        logger_.Error("webd", "bind() failed on " + bind_address_ + ":" + std::to_string(port_));
        close(listen_fd_);
        listen_fd_ = -1;
        SSL_CTX_free(ctx);
        return false;
    }

    if (port_ == 0) {
        struct sockaddr_in actual{};
        socklen_t actual_len = sizeof(actual);
        if (getsockname(listen_fd_, reinterpret_cast<struct sockaddr *>(&actual), &actual_len) != 0) {
            logger_.Error("webd", "getsockname() failed after bind");
            close(listen_fd_);
            listen_fd_ = -1;
            SSL_CTX_free(ctx);
            return false;
        }
        port_ = static_cast<int>(ntohs(actual.sin_port));
    }

    if (listen(listen_fd_, 16) != 0) {
        logger_.Error("webd", "listen() failed");
        close(listen_fd_);
        listen_fd_ = -1;
        SSL_CTX_free(ctx);
        return false;
    }

    ssl_ctx_ = ctx;
    running_.store(true);
    accept_thread_ = std::thread([this]() { AcceptLoop(); });

    logger_.Info("webd", "https server listening on " + bind_address_ + ":" + std::to_string(port_));
    return true;
}

int WebServer::port() const {
    return port_;
}

void WebServer::Stop() {
    if (!running_.exchange(false)) {
        return;
    }

    if (listen_fd_ >= 0) {
        shutdown(listen_fd_, SHUT_RDWR);
        close(listen_fd_);
        listen_fd_ = -1;
    }

    if (accept_thread_.joinable()) {
        accept_thread_.join();
    }

    if (ssl_ctx_ != nullptr) {
        SSL_CTX_free(static_cast<SSL_CTX *>(ssl_ctx_));
        ssl_ctx_ = nullptr;
    }

    EVP_cleanup();
}

void WebServer::AcceptLoop() {
    while (running_.load()) {
        struct sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        const int client_fd = accept(listen_fd_, reinterpret_cast<struct sockaddr *>(&client_addr), &client_len);

        if (client_fd < 0) {
            if (errno == EINTR) {
                continue;
            }
            if (!running_.load()) {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }

        HandleConnection(client_fd);
    }
}

void WebServer::HandleConnection(int client_fd) {
    SSL *ssl = SSL_new(static_cast<SSL_CTX *>(ssl_ctx_));
    if (ssl == nullptr) {
        close(client_fd);
        return;
    }

    SSL_set_fd(ssl, client_fd);
    if (SSL_accept(ssl) <= 0) {
        SSL_shutdown(ssl);
        SSL_free(ssl);
        close(client_fd);
        return;
    }

    HttpRequest request;
    std::string read_error;
    HttpResponse response;
    const HttpReadFn reader = [ssl](char *buffer, std::size_t length) {
        const int to_read = static_cast<int>(std::min(length, static_cast<std::size_t>(16384)));
        return SSL_read(ssl, buffer, to_read);
    };
    if (!ReadHttpRequest(reader, &request, &read_error)) {
        response = JsonHttpError(400, "bad_request", read_error);
    } else {
        response = api_.Handle(request);
    }

    WriteAllSsl(ssl, FormatHttpResponse(response));

    SSL_shutdown(ssl);
    SSL_free(ssl);
    close(client_fd);
}

bool WebServer::EnsureTlsMaterial(std::string *error) const {
    const bool cert_exists = std::filesystem::exists(cert_path_);
    const bool key_exists = std::filesystem::exists(key_path_);
    if (cert_exists && key_exists) {
        return true;
    }

    std::error_code ec;
    std::filesystem::create_directories(std::filesystem::path(cert_path_).parent_path(), ec);
    if (ec) {
        if (error != nullptr) {
            *error = "failed to create cert directory: " + ec.message();
        }
        return false;
    }

    std::filesystem::create_directories(std::filesystem::path(key_path_).parent_path(), ec);
    if (ec) {
        if (error != nullptr) {
            *error = "failed to create key directory: " + ec.message();
        }
        return false;
    }

    return GenerateSelfSignedCertificate(cert_path_, key_path_, error);
}

} // namespace chime::webd
