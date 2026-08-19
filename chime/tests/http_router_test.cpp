#include "chime/webd_http.h"
#include "chime/webd_router.h"
#include "doctest.h"

namespace {

chime::webd::HttpResponse Status(int status) {
    chime::webd::HttpResponse response;
    response.status = status;
    response.content_type = "text/plain; charset=utf-8";
    response.body.clear();
    return response;
}

} // namespace

TEST_SUITE("http_router") {
    TEST_CASE("dispatches exact routes and returns 405 for unsupported methods") {
        chime::webd::HttpRouter router;
        router.Add("GET", "/api/v1/config/core", [](const chime::webd::HttpRequest &) { return Status(200); });
        router.Add("POST", "/api/v1/config/core", [](const chime::webd::HttpRequest &) { return Status(201); });

        chime::webd::HttpRequest get;
        get.method = "GET";
        get.path = "/api/v1/config/core";
        CHECK(router.Dispatch(get).status == 200);

        chime::webd::HttpRequest post;
        post.method = "POST";
        post.path = "/api/v1/config/core";
        CHECK(router.Dispatch(post).status == 201);

        chime::webd::HttpRequest del;
        del.method = "DELETE";
        del.path = "/api/v1/config/core";
        CHECK(router.Dispatch(del).status == 405);

        chime::webd::HttpRequest patch;
        patch.method = "PATCH";
        patch.path = "/nope";
        CHECK(router.Dispatch(patch).status == 405);
    }

    TEST_CASE("prefers exact routes over prefixes so product code can register without a monolith") {
        chime::webd::HttpRouter router;
        router.Add("GET", "/api/v1/system/version", [](const chime::webd::HttpRequest &) { return Status(200); });
        router.AddAnyPrefix("/api/v1/system/", [](const chime::webd::HttpRequest &) { return Status(501); });
        router.AddPrefix("PUT", "/api/v1/ring/sounds/", [](const chime::webd::HttpRequest &) { return Status(200); });

        chime::webd::HttpRequest version;
        version.method = "GET";
        version.path = "/api/v1/system/version";
        CHECK(router.Dispatch(version).status == 200);

        chime::webd::HttpRequest version_post;
        version_post.method = "POST";
        version_post.path = "/api/v1/system/version";
        CHECK(router.Dispatch(version_post).status == 405);

        chime::webd::HttpRequest reserved;
        reserved.method = "GET";
        reserved.path = "/api/v1/system/reboot";
        CHECK(router.Dispatch(reserved).status == 501);

        chime::webd::HttpRequest upload;
        upload.method = "PUT";
        upload.path = "/api/v1/ring/sounds/ring-default.wav";
        CHECK(router.Dispatch(upload).status == 200);

        chime::webd::HttpRequest wrong_method;
        wrong_method.method = "GET";
        wrong_method.path = "/api/v1/ring/sounds/ring-default.wav";
        CHECK(router.Dispatch(wrong_method).status == 405);
    }

    TEST_CASE("uses fallback for unknown paths and 404 when none is set") {
        chime::webd::HttpRouter router;
        router.SetFallback([](const chime::webd::HttpRequest &) { return Status(418); });

        chime::webd::HttpRequest request;
        request.method = "GET";
        request.path = "/index.html";
        CHECK(router.Dispatch(request).status == 418);

        chime::webd::HttpRouter bare;
        CHECK(bare.Dispatch(request).status == 404);
        CHECK(bare.Dispatch(request).content_type.find("text/plain") == 0);
    }
}
