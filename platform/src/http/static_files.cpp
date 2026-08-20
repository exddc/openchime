#include "oc/http/static_files.h"

#include <fstream>
#include <sstream>
#include <utility>

#include "oc/http/json_http.h"
#include "oc/util/strings.h"

namespace oc::http {
namespace {

std::string ContentTypeForPath(const std::filesystem::path &path) {
    const std::string extension = oc::util::ToLower(path.extension().string());
    if (extension == ".html") {
        return "text/html; charset=utf-8";
    }
    if (extension == ".css") {
        return "text/css; charset=utf-8";
    }
    if (extension == ".js") {
        return "application/javascript; charset=utf-8";
    }
    if (extension == ".json") {
        return "application/json; charset=utf-8";
    }
    if (extension == ".svg") {
        return "image/svg+xml";
    }
    if (extension == ".ico") {
        return "image/x-icon";
    }
    if (extension == ".png") {
        return "image/png";
    }
    if (extension == ".jpg" || extension == ".jpeg") {
        return "image/jpeg";
    }
    if (extension == ".webp") {
        return "image/webp";
    }
    if (extension == ".woff2") {
        return "font/woff2";
    }
    if (extension == ".woff") {
        return "font/woff";
    }
    return "application/octet-stream";
}

std::string CacheControlForPath(const std::string &request_path, const std::filesystem::path &path) {
    if (oc::util::ToLower(path.extension().string()) == ".html") {
        return "no-cache";
    }
    if (request_path.starts_with("/assets/")) {
        return "public, max-age=31536000, immutable";
    }
    return "public, max-age=3600";
}

bool ReadFile(const std::filesystem::path &path, std::string *body) {
    if (body == nullptr) {
        return false;
    }
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    std::ostringstream stream;
    stream << file.rdbuf();
    *body = stream.str();
    return true;
}

bool PathIsUnderRoot(const std::filesystem::path &root, const std::filesystem::path &candidate) {
    const std::string root_text = root.lexically_normal().string();
    const std::string candidate_text = candidate.lexically_normal().string();
    if (candidate_text.size() < root_text.size()) {
        return false;
    }
    if (candidate_text.compare(0, root_text.size(), root_text) != 0) {
        return false;
    }
    return candidate_text.size() == root_text.size() || candidate_text[root_text.size()] == '/';
}

} // namespace

bool IsContainedRelativePath(const std::filesystem::path &path) {
    if (path.empty() || path.is_absolute()) {
        return false;
    }
    const auto normalized = path.lexically_normal();
    if (normalized.empty() || normalized.is_absolute()) {
        return false;
    }
    for (const auto &part : normalized) {
        if (part == "..") {
            return false;
        }
    }
    return true;
}

bool ResolveContainedPath(const std::filesystem::path &root, const std::string &request_path,
                          std::filesystem::path *resolved, std::string *error) {
    if (resolved == nullptr) {
        return false;
    }
    if (request_path.empty() || request_path[0] != '/') {
        if (error != nullptr) {
            *error = "invalid path";
        }
        return false;
    }

    const std::filesystem::path relative_path = std::filesystem::path(request_path).relative_path();
    if (!IsContainedRelativePath(relative_path)) {
        if (error != nullptr) {
            *error = "path escapes static root";
        }
        return false;
    }

    std::error_code ec;
    const auto joined = (root / relative_path).lexically_normal();
    if (!PathIsUnderRoot(root, joined)) {
        if (error != nullptr) {
            *error = "path escapes static root";
        }
        return false;
    }

    const auto canon_root = std::filesystem::weakly_canonical(root, ec);
    if (ec) {
        if (error != nullptr) {
            *error = "failed to resolve static root";
        }
        return false;
    }
    const auto canon_file = std::filesystem::weakly_canonical(joined, ec);
    if (ec) {
        if (error != nullptr) {
            *error = "failed to resolve static path";
        }
        return false;
    }
    if (!PathIsUnderRoot(canon_root, canon_file)) {
        if (error != nullptr) {
            *error = "path escapes static root";
        }
        return false;
    }

    *resolved = canon_file;
    return true;
}

std::optional<HttpResponse> ServeStaticUi(const std::string &root_dir, const HttpRequest &request) {
    if (root_dir.empty() || request.method != "GET") {
        return std::nullopt;
    }
    if (request.path.empty() || request.path[0] != '/' || request.path.starts_with("/api/")) {
        return std::nullopt;
    }

    const std::filesystem::path root(root_dir);
    std::error_code ec;
    if (!std::filesystem::exists(root, ec) || !std::filesystem::is_directory(root, ec)) {
        return std::nullopt;
    }

    auto response_from_file = [&](const std::filesystem::path &file_path,
                                  const std::string &request_path) -> std::optional<HttpResponse> {
        if (!std::filesystem::exists(file_path, ec) || !std::filesystem::is_regular_file(file_path, ec)) {
            return std::nullopt;
        }
        std::string body;
        if (!ReadFile(file_path, &body)) {
            return JsonHttpError(500, "ui_read_failed");
        }
        HttpResponse response;
        response.status = 200;
        response.content_type = ContentTypeForPath(file_path);
        response.cache_control = CacheControlForPath(request_path, file_path);
        response.body = std::move(body);
        return response;
    };

    auto contained_index = [&]() -> std::optional<HttpResponse> {
        std::filesystem::path index_path;
        std::string resolve_error;
        if (!ResolveContainedPath(root, "/index.html", &index_path, &resolve_error)) {
            return JsonHttpError(404, "not_found");
        }
        return response_from_file(index_path, "/");
    };

    if (request.path == "/") {
        return contained_index();
    }

    std::filesystem::path resolved;
    std::string resolve_error;
    if (!ResolveContainedPath(root, request.path, &resolved, &resolve_error)) {
        return JsonHttpError(404, "not_found");
    }

    if (const auto file_response = response_from_file(resolved, request.path); file_response.has_value()) {
        return file_response;
    }

    const std::filesystem::path relative_path = std::filesystem::path(request.path).relative_path();
    if (!request.path.starts_with("/assets/") && relative_path.extension().empty()) {
        if (const auto fallback_index = contained_index(); fallback_index.has_value()) {
            return fallback_index;
        }
    }

    return JsonHttpError(404, "not_found");
}

} // namespace oc::http
