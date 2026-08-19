#include "chime/webd_json_http.h"

#include <map>
#include <utility>

namespace chime::webd {

HttpResponse JsonHttpBody(int status, const JsonValue &body) {
    HttpResponse response;
    const auto dumped = DumpJson(body);
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
    std::map<std::string, JsonValue> object;
    object.emplace("error", JsonValue::String(error));
    if (!message.empty()) {
        object.emplace("message", JsonValue::String(message));
    }
    return JsonHttpBody(status, JsonValue::Object(std::move(object)));
}

} // namespace chime::webd
