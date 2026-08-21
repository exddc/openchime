#ifndef OC_HTTP_STATIC_FILES_H
#define OC_HTTP_STATIC_FILES_H

#include <filesystem>
#include <optional>
#include <string>

#include "oc/http/http.h"

namespace oc::http {

bool IsContainedRelativePath(const std::filesystem::path &path);
bool ResolveContainedPath(const std::filesystem::path &root, const std::string &request_path,
                          std::filesystem::path *resolved, std::string *error);

std::optional<HttpResponse> ServeStaticUi(const std::string &root_dir, const HttpRequest &request);

} // namespace oc::http

#endif
