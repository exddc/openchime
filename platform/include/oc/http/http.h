#ifndef OC_HTTP_HTTP_H
#define OC_HTTP_HTTP_H

#include <cstddef>
#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace oc::http {

constexpr std::size_t kMaxRequestLineBytes = 8192;
constexpr std::size_t kMaxHeaderBytes = 65536;
constexpr std::size_t kMaxHeaderLineBytes = 8192;
constexpr std::size_t kMaxHeaderCount = 100;
constexpr std::size_t kMaxBodyBytes = 2 * 1024 * 1024;

struct HttpRequest {
    std::string method;
    std::string path;
    std::string body;
    std::string content_type;
    bool has_content_type = false;
    std::map<std::string, std::string> headers;
    std::string peer_address;
};

struct HttpResponse {
    int status = 500;
    std::string content_type = "application/json; charset=utf-8";
    std::string cache_control = "no-store";
    std::string body = "{\"error\":\"internal\"}";
    std::vector<std::string> set_cookies;
};

struct HttpParseResult {
    bool success = false;
    std::string error;
    HttpRequest request;
    std::size_t content_length = 0;
};

using HttpReadFn = std::function<int(char *buffer, std::size_t length)>;

bool IsSupportedHttpMethod(std::string_view method);
HttpParseResult ParseHttpRequest(std::string_view raw);
bool ReadHttpRequest(const HttpReadFn &read, HttpRequest *request, std::string *error);
std::string FormatHttpResponse(const HttpResponse &response);
std::string RequestHeader(const HttpRequest &request, std::string_view name);

} // namespace oc::http

#endif
