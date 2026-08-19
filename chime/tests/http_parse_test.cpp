#include <cstring>
#include <memory>
#include <string>

#include "chime/webd_http.h"
#include "doctest.h"

namespace {

chime::webd::HttpReadFn ReaderFromString(const std::string &raw, std::size_t chunk_size = 64) {
    auto offset = std::make_shared<std::size_t>(0);
    return [raw, offset, chunk_size](char *buffer, std::size_t length) -> int {
        if (*offset >= raw.size()) {
            return 0;
        }
        const std::size_t remaining = raw.size() - *offset;
        const std::size_t take = remaining < length ? remaining : length;
        const std::size_t actual = take < chunk_size ? take : chunk_size;
        std::memcpy(buffer, raw.data() + *offset, actual);
        *offset += actual;
        return static_cast<int>(actual);
    };
}

} // namespace

TEST_SUITE("http_parse") {
    TEST_CASE("parses a GET request and strips the query string") {
        const auto parsed =
            chime::webd::ParseHttpRequest("GET /api/v1/config/core?x=1 HTTP/1.1\r\nHost: chime.local\r\n\r\n");
        REQUIRE(parsed.success);
        CHECK(parsed.request.method == "GET");
        CHECK(parsed.request.path == "/api/v1/config/core");
        CHECK(parsed.request.body.empty());
    }

    TEST_CASE("parses POST body using Content-Length") {
        const auto parsed =
            chime::webd::ParseHttpRequest("POST /api/v1/config/core HTTP/1.1\r\nContent-Length: 2\r\n\r\n{}");
        REQUIRE(parsed.success);
        CHECK(parsed.request.method == "POST");
        CHECK(parsed.request.body == "{}");
    }

    TEST_CASE("rejects malformed request lines") {
        CHECK_FALSE(chime::webd::ParseHttpRequest("GET\r\n\r\n").success);
        CHECK_FALSE(chime::webd::ParseHttpRequest("GET / HTTP/1.1 extra\r\n\r\n").success);
        CHECK_FALSE(chime::webd::ParseHttpRequest("GET / HTTP/2.0\r\n\r\n").success);
        CHECK_FALSE(chime::webd::ParseHttpRequest("get / HTTP/1.1\r\n\r\n").success);
    }

    TEST_CASE("rejects invalid Content-Length") {
        CHECK_FALSE(chime::webd::ParseHttpRequest("POST / HTTP/1.1\r\nContent-Length: -1\r\n\r\n").success);
        CHECK_FALSE(chime::webd::ParseHttpRequest("POST / HTTP/1.1\r\nContent-Length: abc\r\n\r\n").success);
        CHECK_FALSE(chime::webd::ParseHttpRequest("POST / HTTP/1.1\r\nContent-Length: 1 2\r\n\r\n").success);
        CHECK_FALSE(chime::webd::ParseHttpRequest("POST / HTTP/1.1\r\nContent-Length: +8\r\n\r\n").success);
        CHECK_FALSE(
            chime::webd::ParseHttpRequest("POST / HTTP/1.1\r\nContent-Length: 4\r\nContent-Length: 4\r\n\r\nxxxx")
                .success);
    }

    TEST_CASE("rejects oversized bodies") {
        const std::string too_big = std::to_string(chime::webd::kMaxBodyBytes + 1);
        const auto parsed =
            chime::webd::ParseHttpRequest("POST /upload HTTP/1.1\r\nContent-Length: " + too_big + "\r\n\r\n");
        CHECK_FALSE(parsed.success);
        CHECK(parsed.error == "request body too large");
    }

    TEST_CASE("rejects incomplete bodies") {
        const auto parsed = chime::webd::ParseHttpRequest("POST / HTTP/1.1\r\nContent-Length: 8\r\n\r\nshort");
        CHECK_FALSE(parsed.success);
        CHECK(parsed.error == "incomplete request body");
    }

    TEST_CASE("rejects every Transfer-Encoding header") {
        CHECK_FALSE(chime::webd::ParseHttpRequest("POST / HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\n").success);
        const auto identity = chime::webd::ParseHttpRequest("POST / HTTP/1.1\r\nTransfer-Encoding: identity\r\n\r\n");
        CHECK_FALSE(identity.success);
        CHECK(identity.error == "unsupported transfer encoding");
    }

    TEST_CASE("rejects path traversal and encoded separators") {
        CHECK_FALSE(chime::webd::ParseHttpRequest("GET /../../etc/passwd HTTP/1.1\r\n\r\n").success);
        CHECK_FALSE(chime::webd::ParseHttpRequest("GET /assets/%2e%2e/secret HTTP/1.1\r\n\r\n").success);
        CHECK_FALSE(chime::webd::ParseHttpRequest("GET /assets/%2fetc/passwd HTTP/1.1\r\n\r\n").success);
        CHECK_FALSE(chime::webd::ParseHttpRequest("GET /assets/%00hidden HTTP/1.1\r\n\r\n").success);
    }

    TEST_CASE("rejects malformed headers") {
        CHECK_FALSE(chime::webd::ParseHttpRequest("GET / HTTP/1.1\r\nNotAHeader\r\n\r\n").success);
        CHECK_FALSE(chime::webd::ParseHttpRequest("GET / HTTP/1.1\r\n : empty-name\r\n\r\n").success);
        CHECK_FALSE(chime::webd::ParseHttpRequest("GET / HTTP/1.1\r\n folded: no\r\n\r\n").success);
    }

    TEST_CASE("rejects a blank line inside the header block") {
        const auto parsed =
            chime::webd::ParseHttpRequest("GET / HTTP/1.1\r\nHost: chime.local\r\n\nAccept: */*\r\n\r\n");
        CHECK_FALSE(parsed.success);
        CHECK(parsed.error == "invalid header");
    }

    TEST_CASE("streaming reader enforces the same limits") {
        chime::webd::HttpRequest request;
        std::string error;
        const std::string raw = "POST /api HTTP/1.1\r\nContent-Type: application/json\r\nContent-Length: 2\r\n\r\n{}";
        REQUIRE(chime::webd::ReadHttpRequest(ReaderFromString(raw, 7), &request, &error));
        CHECK(request.body == "{}");
        CHECK(request.has_content_type);
        CHECK(request.content_type == "application/json");

        CHECK_FALSE(chime::webd::ReadHttpRequest(ReaderFromString("POST / HTTP/1.1\r\nContent-Length: xyz\r\n\r\n"),
                                                 &request, &error));
        CHECK(error == "invalid Content-Length");
    }

    TEST_CASE("streaming reader rejects Transfer-Encoding and TE plus Content-Length") {
        chime::webd::HttpRequest request;
        std::string error;
        CHECK_FALSE(chime::webd::ReadHttpRequest(
            ReaderFromString("POST / HTTP/1.1\r\nTransfer-Encoding: identity\r\n\r\n", 5), &request, &error));
        CHECK(error == "unsupported transfer encoding");

        CHECK_FALSE(chime::webd::ReadHttpRequest(
            ReaderFromString("POST / HTTP/1.1\r\nTransfer-Encoding: identity\r\nContent-Length: 2\r\n\r\n{}", 7),
            &request, &error));
        CHECK(error == "unsupported transfer encoding");
    }

    TEST_CASE("parses Cookie and CSRF headers onto the request") {
        const auto parsed = chime::webd::ParseHttpRequest(
            "POST /api/v1/config/core HTTP/1.1\r\nCookie: chime_session=abc; chime_csrf=def\r\n"
            "X-CSRF-Token: def\r\nContent-Length: 2\r\n\r\n{}");
        REQUIRE(parsed.success);
        CHECK(chime::webd::RequestHeader(parsed.request, "cookie") == "chime_session=abc; chime_csrf=def");
        CHECK(chime::webd::RequestHeader(parsed.request, "X-CSRF-Token") == "def");
        CHECK(parsed.request.body == "{}");
    }

    TEST_CASE("formats Set-Cookie headers and 401 status text") {
        chime::webd::HttpResponse response;
        response.status = 401;
        response.body = "{\"error\":\"unauthorized\"}";
        response.set_cookies.push_back("chime_session=abc; Path=/; HttpOnly; Secure; SameSite=Strict");
        const std::string raw = chime::webd::FormatHttpResponse(response);
        CHECK(raw.find("HTTP/1.1 401 Unauthorized\r\n") == 0);
        CHECK(raw.find("Set-Cookie: chime_session=abc; Path=/; HttpOnly; Secure; SameSite=Strict\r\n") !=
              std::string::npos);
        CHECK(raw.find("Connection: close\r\n") != std::string::npos);
    }

    TEST_CASE("formats responses with connection close") {
        chime::webd::HttpResponse response;
        response.status = 400;
        response.body = "{\"error\":\"bad_request\"}";
        const std::string raw = chime::webd::FormatHttpResponse(response);
        CHECK(raw.find("HTTP/1.1 400 Bad Request\r\n") == 0);
        CHECK(raw.find("Connection: close\r\n") != std::string::npos);
        CHECK(raw.find("Content-Length: " + std::to_string(response.body.size()) + "\r\n") != std::string::npos);
        CHECK(raw.find("\r\n\r\n{\"error\":\"bad_request\"}") != std::string::npos);
    }
}
