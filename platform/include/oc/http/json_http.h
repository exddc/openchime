#ifndef OC_HTTP_JSON_HTTP_H
#define OC_HTTP_JSON_HTTP_H

#include <string>

#include "oc/http/http.h"
#include "oc/json/json.h"

namespace oc::http {

HttpResponse JsonHttpError(int status, const std::string &error, const std::string &message = "");
HttpResponse JsonHttpBody(int status, const oc::json::JsonValue &body);

} // namespace oc::http

#endif
