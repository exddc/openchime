#ifndef CHIME_WEBD_WEB_SERVER_H
#define CHIME_WEBD_WEB_SERVER_H

#include <atomic>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <vector>

#include "chime/webd_api.h"
#include "chime/webd_http.h"

namespace oc::logging {
class Logger;
}

namespace chime::webd {

class ApplyManager;
class AuthStore;
class ConfigStore;
class WifiScanner;

class WebServer {
  public:
    WebServer(oc::logging::Logger &logger, ConfigStore &config_store, WifiScanner &wifi_scanner,
              ApplyManager &apply_manager, AuthStore &auth_store, std::string bind_address, int port,
              std::string cert_path, std::string key_path, std::string ui_dist_dir, std::string observed_topics_path,
              std::string ring_sounds_dir, std::string active_ring_sound_path);
    ~WebServer();

    WebServer(const WebServer &) = delete;
    WebServer &operator=(const WebServer &) = delete;

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

    oc::logging::Logger &logger_;
    std::string bind_address_;
    int port_ = 8443;
    std::string cert_path_;
    std::string key_path_;
    WebApi api_;

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

} // namespace chime::webd

#endif
