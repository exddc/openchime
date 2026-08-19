#ifndef CHIME_WEBD_WEB_SERVER_H
#define CHIME_WEBD_WEB_SERVER_H

#include <atomic>
#include <string>
#include <thread>

#include "chime/webd_api.h"

namespace oc::logging {
class Logger;
}

namespace chime::webd {

class ApplyManager;
class ConfigStore;
class WifiScanner;

class WebServer {
  public:
    WebServer(oc::logging::Logger &logger, ConfigStore &config_store, WifiScanner &wifi_scanner,
              ApplyManager &apply_manager, std::string bind_address, int port, std::string cert_path,
              std::string key_path, std::string ui_dist_dir, std::string observed_topics_path,
              std::string ring_sounds_dir, std::string active_ring_sound_path);
    ~WebServer();

    WebServer(const WebServer &) = delete;
    WebServer &operator=(const WebServer &) = delete;

    bool Start();
    void Stop();
    int port() const;

  private:
    void AcceptLoop();
    void HandleConnection(int client_fd);
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
};

} // namespace chime::webd

#endif
