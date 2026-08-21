#include "doctest.h"
#include "oc/http/http.h"
#include "oc/http/router.h"

namespace {

oc::http::HttpResponse Status(int status) {
    oc::http::HttpResponse response;
    response.status = status;
    response.content_type = "text/plain; charset=utf-8";
    response.body.clear();
    return response;
}

} // namespace

TEST_SUITE("http_router") {
    TEST_CASE("dispatches exact routes and returns 405 for unsupported methods") {
        oc::http::HttpRouter router;
        router.Add("GET", "/api/v1/config/core", [](const oc::http::HttpRequest &) { return Status(200); });
        router.Add("POST", "/api/v1/config/core", [](const oc::http::HttpRequest &) { return Status(201); });

        oc::http::HttpRequest get;
        get.method = "GET";
        get.path = "/api/v1/config/core";
        CHECK(router.Dispatch(get).status == 200);

        oc::http::HttpRequest post;
        post.method = "POST";
        post.path = "/api/v1/config/core";
        CHECK(router.Dispatch(post).status == 201);

        oc::http::HttpRequest del;
        del.method = "DELETE";
        del.path = "/api/v1/config/core";
        CHECK(router.Dispatch(del).status == 405);

        oc::http::HttpRequest patch;
        patch.method = "PATCH";
        patch.path = "/nope";
        CHECK(router.Dispatch(patch).status == 405);
    }

    TEST_CASE("prefers exact routes over prefixes so product code can register without a monolith") {
        oc::http::HttpRouter router;
        router.Add("GET", "/api/v1/system/version", [](const oc::http::HttpRequest &) { return Status(200); });
        router.AddAnyPrefix("/api/v1/system/", [](const oc::http::HttpRequest &) { return Status(501); });
        router.AddPrefix("PUT", "/api/v1/ring/sounds/", [](const oc::http::HttpRequest &) { return Status(200); });

        oc::http::HttpRequest version;
        version.method = "GET";
        version.path = "/api/v1/system/version";
        CHECK(router.Dispatch(version).status == 200);

        oc::http::HttpRequest version_post;
        version_post.method = "POST";
        version_post.path = "/api/v1/system/version";
        CHECK(router.Dispatch(version_post).status == 405);

        oc::http::HttpRequest reserved;
        reserved.method = "GET";
        reserved.path = "/api/v1/system/reboot";
        CHECK(router.Dispatch(reserved).status == 501);

        oc::http::HttpRequest upload;
        upload.method = "PUT";
        upload.path = "/api/v1/ring/sounds/ring-default.wav";
        CHECK(router.Dispatch(upload).status == 200);

        oc::http::HttpRequest wrong_method;
        wrong_method.method = "GET";
        wrong_method.path = "/api/v1/ring/sounds/ring-default.wav";
        CHECK(router.Dispatch(wrong_method).status == 405);
    }

    TEST_CASE("uses fallback for unknown paths and 404 when none is set") {
        oc::http::HttpRouter router;
        router.SetFallback([](const oc::http::HttpRequest &) { return Status(418); });

        oc::http::HttpRequest request;
        request.method = "GET";
        request.path = "/index.html";
        CHECK(router.Dispatch(request).status == 418);

        oc::http::HttpRouter bare;
        CHECK(bare.Dispatch(request).status == 404);
        CHECK(bare.Dispatch(request).content_type.find("text/plain") == 0);
    }
}
