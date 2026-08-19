#ifndef CHIME_WEBD_API_H
#define CHIME_WEBD_API_H

#include <cstddef>
#include <string>

#include "chime/webd_http.h"
#include "chime/webd_router.h"

namespace oc::logging {
class Logger;
}

namespace chime::webd {

constexpr std::size_t kMaxJsonBodyBytes = 64 * 1024;

class ApplyManager;
class ConfigStore;
class WifiScanner;

class WebApi {
  public:
    WebApi(oc::logging::Logger &logger, ConfigStore &config_store, WifiScanner &wifi_scanner,
           ApplyManager &apply_manager, std::string ui_dist_dir, std::string observed_topics_path,
           std::string ring_sounds_dir, std::string active_ring_sound_path);

    HttpRouter &router() { return router_; }
    const HttpRouter &router() const { return router_; }

    HttpResponse Handle(const HttpRequest &request);

  private:
    void RegisterRoutes();

    HttpResponse HandleGetCoreConfig();
    HttpResponse HandlePostCoreConfig(const HttpRequest &request);
    HttpResponse HandleWifiScan();
    HttpResponse HandleGetSystemVersion();
    HttpResponse HandleGetObservedTopics();
    HttpResponse HandleGetRingSounds();
    HttpResponse HandleUploadRingSound(const HttpRequest &request);
    HttpResponse HandleSelectRingSound(const HttpRequest &request);
    HttpResponse ReservedNotImplemented(const HttpRequest &request) const;
    HttpResponse HandleFallback(const HttpRequest &request) const;

    oc::logging::Logger &logger_;
    ConfigStore &config_store_;
    WifiScanner &wifi_scanner_;
    ApplyManager &apply_manager_;
    std::string ui_dist_dir_;
    std::string observed_topics_path_;
    std::string ring_sounds_dir_;
    std::string active_ring_sound_path_;
    HttpRouter router_;
};

} // namespace chime::webd

#endif
