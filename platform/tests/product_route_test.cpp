#include "doctest.h"
#include "oc/http/http.h"
#include "oc/http/json_http.h"
#include "oc/http/product_routes.h"
#include "oc/json/json.h"

namespace {

class PingRoutes final : public oc::http::ProductRoutes {
  public:
    void Register(oc::http::HttpRouter &router) override {
        router.Add("GET", "/api/v1/ping", [](const oc::http::HttpRequest &) {
            return oc::http::JsonHttpBody(200, oc::json::JsonValue::Object({
                                                   {"ok", oc::json::JsonValue::Bool(true)},
                                               }));
        });
    }
};

} // namespace

TEST_SUITE("product_routes") {
    TEST_CASE("a product can register GET /api/v1/ping without editing platform internals") {
        oc::http::HttpRouter router;
        PingRoutes ping;
        ping.Register(router);

        oc::http::HttpRequest request;
        request.method = "GET";
        request.path = "/api/v1/ping";
        const oc::http::HttpResponse response = router.Dispatch(request);
        CHECK(response.status == 200);
        const auto parsed = oc::json::ParseJson(response.body);
        REQUIRE(parsed.success);
        bool ok = false;
        const auto field = oc::json::GetObjectField(parsed.value, "ok");
        REQUIRE(field.has_value());
        REQUIRE(field->AsBool(&ok));
        CHECK(ok);
    }
}
