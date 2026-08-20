#ifndef CHIME_WEBD_API_H
#define CHIME_WEBD_API_H

#include <cstddef>
#include <mutex>
#include <string>

#include "oc/http/http.h"
#include "oc/http/product_routes.h"
#include "oc/http/router.h"
#include "oc/wifi/scan.h"

namespace oc::logging {
class Logger;
}

namespace chime::webd {

using oc::http::HttpRequest;
using oc::http::HttpResponse;
using oc::http::HttpRouter;

constexpr std::size_t kMaxJsonBodyBytes = 64 * 1024;

class ApplyManager;
class AuthStore;
class ConfigStore;

class WebApi : public oc::http::ProductRoutes {
  public:
    WebApi(oc::logging::Logger &logger, ConfigStore &config_store, oc::wifi::WifiScanner &wifi_scanner,
           ApplyManager &apply_manager, AuthStore &auth_store, std::string ui_dist_dir,
           std::string observed_topics_path, std::string ring_sounds_dir, std::string active_ring_sound_path);

    HttpRouter &router() { return router_; }
    const HttpRouter &router() const { return router_; }

    void Register(HttpRouter &router) override;
    HttpResponse Handle(const HttpRequest &request);
    static bool OffloadToSlowWorker(const HttpRequest &request);

  private:
    HttpResponse AuthorizeAndDispatch(const HttpRequest &request);

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
    oc::wifi::WifiScanner &wifi_scanner_;
    ApplyManager &apply_manager_;
    AuthStore &auth_store_;
    std::string ui_dist_dir_;
    std::string observed_topics_path_;
    std::string ring_sounds_dir_;
    std::string active_ring_sound_path_;
    HttpRouter router_;
    std::mutex product_mutex_;
};

} // namespace chime::webd

#endif
