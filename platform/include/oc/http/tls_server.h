#ifndef OC_HTTP_TLS_SERVER_H
#define OC_HTTP_TLS_SERVER_H

#include <atomic>
#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <vector>

#include "oc/http/http.h"

namespace oc::logging {
class Logger;
}

namespace oc::http {

using RequestHandler = std::function<HttpResponse(const HttpRequest &)>;
using RequestPredicate = std::function<bool(const HttpRequest &)>;

struct TlsServerConfig {
    std::string bind_address = "0.0.0.0";
    int port = 8443;
    std::string cert_path;
    std::string key_path;
    std::string cert_organization;
    std::string cert_common_name = "localhost";
    std::string log_component = "http";
    std::function<void()> stop_work;
};

class TlsServer {
  public:
    TlsServer(oc::logging::Logger &logger, TlsServerConfig config, RequestHandler handler,
              RequestPredicate slow_request = {});
    ~TlsServer();

    TlsServer(const TlsServer &) = delete;
    TlsServer &operator=(const TlsServer &) = delete;

    bool Start();
    void Stop();
    int port() const;

  private:
    struct HashJob {
        int client_fd = -1;
        void *ssl = nullptr;
        HttpRequest request;
    };

    void AcceptLoop();
    void WorkerLoop();
    void HashLoop();
    void EnqueueClient(int client_fd);
    bool EnqueueHashJob(int client_fd, void *ssl, HttpRequest request);
    void HandleConnection(int client_fd);
    void CompleteConnection(void *ssl, int client_fd, const HttpResponse &response);
    void ReleaseConnection(void *ssl, int client_fd);
    void UnregisterClientFd(int client_fd);
    void InterruptClients();
    bool EnsureTlsMaterial(std::string *error) const;
    bool IsSlowRequest(const HttpRequest &request) const;
    HttpResponse InvokeHandler(const HttpRequest &request) const;

    oc::logging::Logger &logger_;
    TlsServerConfig config_;
    RequestHandler handler_;
    RequestPredicate slow_request_;
    int port_ = 8443;

    std::atomic<bool> running_{false};
    int listen_fd_ = -1;
    void *ssl_ctx_ = nullptr;
    std::thread accept_thread_;
    std::thread hash_thread_;
    std::vector<std::thread> workers_;
    std::mutex connection_mutex_;
    std::condition_variable connection_cv_;
    std::deque<int> pending_clients_;
    std::set<int> active_client_fds_;
    std::mutex hash_mutex_;
    std::condition_variable hash_cv_;
    std::deque<HashJob> hash_jobs_;
};

} // namespace oc::http

#endif
