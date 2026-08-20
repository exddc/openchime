#include "oc/http/json_http.h"

#include <map>
#include <utility>

namespace oc::http {

HttpResponse JsonHttpBody(int status, const oc::json::JsonValue &body) {
    HttpResponse response;
    const auto dumped = oc::json::DumpJson(body);
    if (!dumped.success) {
        response.status = 500;
        response.body = R"({"error":"serialize_failed"})";
        return response;
    }
    response.status = status;
    response.body = dumped.text;
    return response;
}

HttpResponse JsonHttpError(int status, const std::string &error, const std::string &message) {
    std::map<std::string, oc::json::JsonValue> object;
    object.emplace("error", oc::json::JsonValue::String(error));
    if (!message.empty()) {
        object.emplace("message", oc::json::JsonValue::String(message));
    }
    return JsonHttpBody(status, oc::json::JsonValue::Object(std::move(object)));
}

} // namespace oc::http
