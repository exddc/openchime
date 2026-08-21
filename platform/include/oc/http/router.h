#ifndef OC_HTTP_ROUTER_H
#define OC_HTTP_ROUTER_H

#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "oc/http/http.h"

namespace oc::http {

using HttpHandler = std::function<HttpResponse(const HttpRequest &)>;

class HttpRouter {
  public:
    HttpRouter();

    void Add(std::string method, std::string path, HttpHandler handler);
    void AddPrefix(std::string method, std::string prefix, HttpHandler handler);
    void AddAny(std::string path, HttpHandler handler);
    void AddAnyPrefix(std::string prefix, HttpHandler handler);
    void SetFallback(HttpHandler handler);
    void SetMethodNotAllowed(HttpResponse response);
    void SetNotFound(HttpResponse response);

    HttpResponse Dispatch(const HttpRequest &request) const;

  private:
    struct Route {
        std::optional<std::string> method;
        std::string path;
        bool prefix = false;
        HttpHandler handler;
    };

    std::vector<Route> routes_;
    HttpHandler fallback_;
    HttpResponse method_not_allowed_;
    HttpResponse not_found_;
};

} // namespace oc::http

#endif
