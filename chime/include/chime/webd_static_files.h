#ifndef CHIME_WEBD_STATIC_FILES_H
#define CHIME_WEBD_STATIC_FILES_H

#include <filesystem>
#include <optional>
#include <string>

#include "chime/webd_http.h"

namespace chime::webd {

bool IsContainedRelativePath(const std::filesystem::path &path);
bool ResolveContainedPath(const std::filesystem::path &root, const std::string &request_path,
                          std::filesystem::path *resolved, std::string *error);

std::optional<HttpResponse> ServeStaticUi(const std::string &root_dir, const HttpRequest &request);

} // namespace chime::webd

#endif
