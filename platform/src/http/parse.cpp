#include "oc/http/http.h"

#include <algorithm>
#include <array>
#include <limits>
#include <map>
#include <sstream>
#include <utility>

#include "oc/config/kv_config.h"
#include "oc/util/strings.h"

namespace oc::http {
namespace {

bool IsAlphaUpper(char c) {
    return c >= 'A' && c <= 'Z';
}

bool IsDigit(char c) {
    return c >= '0' && c <= '9';
}

bool ContainsNul(std::string_view value) {
    return value.find('\0') != std::string_view::npos;
}

bool HasUnsafeEncodedPath(const std::string &path) {
    const std::string lowered = oc::util::ToLower(path);
    return lowered.find("%2e") != std::string::npos || lowered.find("%2f") != std::string::npos ||
           lowered.find("%5c") != std::string::npos || lowered.find("%00") != std::string::npos;
}

bool HasDotDotSegment(const std::string &path) {
    std::size_t start = 0;
    while (start <= path.size()) {
        const std::size_t slash = path.find('/', start);
        const std::size_t end = slash == std::string::npos ? path.size() : slash;
        if (end - start == 2 && path[start] == '.' && path[start + 1] == '.') {
            return true;
        }
        if (slash == std::string::npos) {
            break;
        }
        start = slash + 1;
    }
    return false;
}

bool IsSafeRequestPath(const std::string &path, std::string *error) {
    if (path.empty() || path[0] != '/') {
        if (error != nullptr) {
            *error = "invalid path";
        }
        return false;
    }
    if (ContainsNul(path) || HasUnsafeEncodedPath(path) || HasDotDotSegment(path)) {
        if (error != nullptr) {
            *error = "invalid path";
        }
        return false;
    }
    for (const char c : path) {
        if (static_cast<unsigned char>(c) < 0x20 || c == '\\') {
            if (error != nullptr) {
                *error = "invalid path";
            }
            return false;
        }
    }
    return true;
}

bool ParseContentLengthValue(const std::string &text, std::size_t *value, std::string *error) {
    if (value == nullptr || error == nullptr) {
        return false;
    }
    if (text.empty()) {
        *error = "invalid Content-Length";
        return false;
    }
    unsigned long long parsed = 0;
    for (const char c : text) {
        if (!IsDigit(c)) {
            *error = "invalid Content-Length";
            return false;
        }
        const unsigned int digit = static_cast<unsigned int>(c - '0');
        if (parsed > (std::numeric_limits<unsigned long long>::max() - digit) / 10ULL) {
            *error = "request body too large";
            return false;
        }
        parsed = parsed * 10ULL + digit;
    }
    if (parsed > kMaxBodyBytes) {
        *error = "request body too large";
        return false;
    }
    *value = static_cast<std::size_t>(parsed);
    return true;
}

std::string StatusText(int code) {
    switch (code) {
    case 200:
        return "OK";
    case 400:
        return "Bad Request";
    case 401:
        return "Unauthorized";
    case 403:
        return "Forbidden";
    case 404:
        return "Not Found";
    case 405:
        return "Method Not Allowed";
    case 415:
        return "Unsupported Media Type";
    case 429:
        return "Too Many Requests";
    case 500:
        return "Internal Server Error";
    case 501:
        return "Not Implemented";
    case 503:
        return "Service Unavailable";
    default:
        return "Error";
    }
}

HttpParseResult ParseHeaderBlock(std::string_view header_blob) {
    HttpParseResult result;
    if (header_blob.size() > kMaxHeaderBytes) {
        result.error = "request too large";
        return result;
    }

    std::istringstream header_stream{std::string(header_blob)};
    std::string request_line;
    if (!std::getline(header_stream, request_line)) {
        result.error = "missing request line";
        return result;
    }
    if (!request_line.empty() && request_line.back() == '\r') {
        request_line.pop_back();
    }
    if (request_line.size() > kMaxRequestLineBytes) {
        result.error = "request line too long";
        return result;
    }
    if (ContainsNul(request_line)) {
        result.error = "invalid request line";
        return result;
    }

    const std::size_t first_space = request_line.find(' ');
    const std::size_t second_space =
        first_space == std::string::npos ? std::string::npos : request_line.find(' ', first_space + 1);
    if (first_space == std::string::npos || second_space == std::string::npos ||
        request_line.find(' ', second_space + 1) != std::string::npos) {
        result.error = "invalid request line";
        return result;
    }

    const std::string method = request_line.substr(0, first_space);
    std::string path = request_line.substr(first_space + 1, second_space - first_space - 1);
    const std::string version = request_line.substr(second_space + 1);
    if (method.empty() || method.size() > 16) {
        result.error = "invalid request line";
        return result;
    }
    for (const char c : method) {
        if (!IsAlphaUpper(c)) {
            result.error = "invalid request line";
            return result;
        }
    }
    if (version != "HTTP/1.0" && version != "HTTP/1.1") {
        result.error = "unsupported HTTP version";
        return result;
    }

    const auto query = path.find('?');
    if (query != std::string::npos) {
        path = path.substr(0, query);
    }
    if (!IsSafeRequestPath(path, &result.error)) {
        return result;
    }

    std::map<std::string, std::string> headers;
    std::string header_line;
    std::size_t header_count = 0;
    while (std::getline(header_stream, header_line)) {
        if (!header_line.empty() && header_line.back() == '\r') {
            header_line.pop_back();
        }
        if (header_line.empty()) {
            result.error = "invalid header";
            return result;
        }
        if (header_line.size() > kMaxHeaderLineBytes) {
            result.error = "header line too long";
            return result;
        }
        if (!header_line.empty() && (header_line[0] == ' ' || header_line[0] == '\t')) {
            result.error = "obsolete header folding is not supported";
            return result;
        }
        const auto sep = header_line.find(':');
        if (sep == std::string::npos || sep == 0) {
            result.error = "invalid header";
            return result;
        }
        ++header_count;
        if (header_count > kMaxHeaderCount) {
            result.error = "too many headers";
            return result;
        }

        const std::string key = oc::util::ToLower(oc::config::trim(header_line.substr(0, sep)));
        const std::string value = oc::config::trim(header_line.substr(sep + 1));
        if (key.empty()) {
            result.error = "invalid header";
            return result;
        }
        if (headers.find(key) != headers.end() && (key == "content-length" || key == "transfer-encoding")) {
            result.error = "duplicate " + key + " header";
            return result;
        }
        headers[key] = value;
    }

    const auto transfer_it = headers.find("transfer-encoding");
    if (transfer_it != headers.end()) {
        result.error = "unsupported transfer encoding";
        return result;
    }

    std::size_t content_length = 0;
    const auto content_length_it = headers.find("content-length");
    if (content_length_it != headers.end()) {
        if (!ParseContentLengthValue(content_length_it->second, &content_length, &result.error)) {
            return result;
        }
    }

    result.request.method = method;
    result.request.path = std::move(path);
    result.content_length = content_length;
    const auto content_type_it = headers.find("content-type");
    result.request.has_content_type = content_type_it != headers.end();
    result.request.content_type = content_type_it != headers.end() ? content_type_it->second : "";
    result.request.headers = std::move(headers);
    result.success = true;
    return result;
}

} // namespace

bool IsSupportedHttpMethod(std::string_view method) {
    return method == "GET" || method == "POST" || method == "PUT";
}

HttpParseResult ParseHttpRequest(std::string_view raw) {
    HttpParseResult result;
    if (ContainsNul(raw)) {
        result.error = "invalid request";
        return result;
    }

    const auto headers_end = raw.find("\r\n\r\n");
    if (headers_end == std::string_view::npos) {
        if (raw.size() >= kMaxHeaderBytes) {
            result.error = "request too large";
        } else {
            result.error = "incomplete request headers";
        }
        return result;
    }
    if (headers_end > kMaxHeaderBytes) {
        result.error = "request too large";
        return result;
    }

    result = ParseHeaderBlock(raw.substr(0, headers_end));
    if (!result.success) {
        return result;
    }

    const std::size_t expected = result.content_length;
    const std::string_view body = raw.substr(headers_end + 4);
    if (body.size() < expected) {
        result.success = false;
        result.error = "incomplete request body";
        result.request = HttpRequest();
        return result;
    }
    result.request.body.assign(body.data(), expected);
    return result;
}

bool ReadHttpRequest(const HttpReadFn &read, HttpRequest *request, std::string *error) {
    if (!read || request == nullptr || error == nullptr) {
        if (error != nullptr) {
            *error = "invalid reader";
        }
        return false;
    }

    std::string data;
    data.reserve(2048);
    std::size_t headers_end = std::string::npos;
    while (headers_end == std::string::npos) {
        if (data.size() >= kMaxHeaderBytes) {
            *error = "request too large";
            return false;
        }

        std::array<char, 2048> buffer{};
        const std::size_t remaining = kMaxHeaderBytes - data.size();
        const std::size_t to_read = std::min(buffer.size(), remaining);
        const int bytes = read(buffer.data(), to_read);
        if (bytes <= 0) {
            *error = "failed to read request";
            return false;
        }
        data.append(buffer.data(), static_cast<std::size_t>(bytes));
        headers_end = data.find("\r\n\r\n");
    }

    HttpParseResult parsed = ParseHeaderBlock(std::string_view(data).substr(0, headers_end));
    if (!parsed.success) {
        *error = parsed.error;
        return false;
    }

    std::string body = data.substr(headers_end + 4);
    const std::size_t content_length = parsed.content_length;
    while (body.size() < content_length) {
        std::array<char, 2048> buffer{};
        const std::size_t remaining = content_length - body.size();
        const int bytes = read(buffer.data(), std::min(buffer.size(), remaining));
        if (bytes <= 0) {
            *error = "failed to read request body";
            return false;
        }
        body.append(buffer.data(), static_cast<std::size_t>(bytes));
    }
    if (body.size() > content_length) {
        body.resize(content_length);
    }

    parsed.request.body = std::move(body);
    *request = std::move(parsed.request);
    return true;
}

std::string FormatHttpResponse(const HttpResponse &response) {
    std::string raw;
    raw += "HTTP/1.1 " + std::to_string(response.status) + " " + StatusText(response.status) + "\r\n";
    raw += "Content-Type: " + response.content_type + "\r\n";
    raw += "Content-Length: " + std::to_string(response.body.size()) + "\r\n";
    raw += "Cache-Control: " + response.cache_control + "\r\n";
    raw += "Connection: close\r\n";
    for (const auto &cookie : response.set_cookies) {
        raw += "Set-Cookie: " + cookie + "\r\n";
    }
    raw += "\r\n";
    raw += response.body;
    return raw;
}

std::string RequestHeader(const HttpRequest &request, std::string_view name) {
    const std::string key = oc::util::ToLower(std::string(name));
    const auto it = request.headers.find(key);
    if (it == request.headers.end()) {
        return "";
    }
    return it->second;
}

} // namespace oc::http
