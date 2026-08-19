#ifndef CHIME_WEBD_JSON_HTTP_H
#define CHIME_WEBD_JSON_HTTP_H

#include <string>

#include "chime/webd_http.h"
#include "chime/webd_json.h"

namespace chime::webd {

HttpResponse JsonHttpError(int status, const std::string &error, const std::string &message = "");
HttpResponse JsonHttpBody(int status, const JsonValue &body);

} // namespace chime::webd

#endif
