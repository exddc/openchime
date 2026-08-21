#include "oc/http/router.h"

#include <utility>

namespace oc::http {
namespace {

HttpResponse PlainStatus(int status, std::string body) {
    HttpResponse response;
    response.status = status;
    response.content_type = "text/plain; charset=utf-8";
    response.body = std::move(body);
    return response;
}

} // namespace

HttpRouter::HttpRouter()
    : method_not_allowed_(PlainStatus(405, "Method Not Allowed")), not_found_(PlainStatus(404, "Not Found")) {}

void HttpRouter::Add(std::string method, std::string path, HttpHandler handler) {
    Route route;
    route.method = std::move(method);
    route.path = std::move(path);
    route.prefix = false;
    route.handler = std::move(handler);
    routes_.push_back(std::move(route));
}

void HttpRouter::AddPrefix(std::string method, std::string prefix, HttpHandler handler) {
    Route route;
    route.method = std::move(method);
    route.path = std::move(prefix);
    route.prefix = true;
    route.handler = std::move(handler);
    routes_.push_back(std::move(route));
}

void HttpRouter::AddAny(std::string path, HttpHandler handler) {
    Route route;
    route.path = std::move(path);
    route.prefix = false;
    route.handler = std::move(handler);
    routes_.push_back(std::move(route));
}

void HttpRouter::AddAnyPrefix(std::string prefix, HttpHandler handler) {
    Route route;
    route.path = std::move(prefix);
    route.prefix = true;
    route.handler = std::move(handler);
    routes_.push_back(std::move(route));
}

void HttpRouter::SetFallback(HttpHandler handler) {
    fallback_ = std::move(handler);
}

void HttpRouter::SetMethodNotAllowed(HttpResponse response) {
    method_not_allowed_ = std::move(response);
}

void HttpRouter::SetNotFound(HttpResponse response) {
    not_found_ = std::move(response);
}

HttpResponse HttpRouter::Dispatch(const HttpRequest &request) const {
    if (!IsSupportedHttpMethod(request.method)) {
        return method_not_allowed_;
    }

    const Route *exact = nullptr;
    bool exact_path_matched = false;
    for (const auto &route : routes_) {
        if (route.prefix || route.path != request.path) {
            continue;
        }
        exact_path_matched = true;
        if (route.method.has_value() && *route.method != request.method) {
            continue;
        }
        if (exact == nullptr || (route.method.has_value() && !exact->method.has_value())) {
            exact = &route;
        }
    }
    if (exact_path_matched) {
        if (exact != nullptr) {
            return exact->handler(request);
        }
        return method_not_allowed_;
    }

    const Route *prefix = nullptr;
    bool prefix_path_matched = false;
    for (const auto &route : routes_) {
        if (!route.prefix || !request.path.starts_with(route.path)) {
            continue;
        }
        prefix_path_matched = true;
        if (route.method.has_value() && *route.method != request.method) {
            continue;
        }
        if (prefix == nullptr || route.path.size() > prefix->path.size()) {
            prefix = &route;
        } else if (route.path.size() == prefix->path.size() && route.method.has_value() &&
                   !prefix->method.has_value()) {
            prefix = &route;
        }
    }
    if (prefix != nullptr) {
        return prefix->handler(request);
    }
    if (prefix_path_matched) {
        return method_not_allowed_;
    }
    if (fallback_) {
        return fallback_(request);
    }
    return not_found_;
}

} // namespace oc::http
