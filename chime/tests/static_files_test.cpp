#include <fstream>

#include "chime/webd_http.h"
#include "chime/webd_static_files.h"
#include "doctest.h"
#include "test_support.h"

TEST_SUITE("static_files") {
    TEST_CASE("rejects relative paths that escape the root") {
        CHECK_FALSE(chime::webd::IsContainedRelativePath(".."));
        CHECK_FALSE(chime::webd::IsContainedRelativePath("../secret"));
        CHECK_FALSE(chime::webd::IsContainedRelativePath("assets/../../etc/passwd"));
        CHECK_FALSE(chime::webd::IsContainedRelativePath("/etc/passwd"));
        CHECK(chime::webd::IsContainedRelativePath("index.html"));
        CHECK(chime::webd::IsContainedRelativePath("assets/app.js"));
    }

    TEST_CASE("serves files inside the UI root and blocks traversal") {
        const ScopedTempDir tmp;
        const auto root = tmp.path() / "ui";
        std::filesystem::create_directories(root / "assets");
        {
            std::ofstream index(root / "index.html");
            index << "<html>ok</html>";
        }
        {
            std::ofstream asset(root / "assets" / "app.js");
            asset << "console.log(1)";
        }
        const auto secret = tmp.WriteFile("secret.txt", "nope");

        chime::webd::HttpRequest index_request;
        index_request.method = "GET";
        index_request.path = "/";
        const auto index_response = chime::webd::ServeStaticUi(root.string(), index_request);
        REQUIRE(index_response.has_value());
        CHECK(index_response->status == 200);
        CHECK(index_response->body.find("ok") != std::string::npos);

        chime::webd::HttpRequest asset_request;
        asset_request.method = "GET";
        asset_request.path = "/assets/app.js";
        const auto asset_response = chime::webd::ServeStaticUi(root.string(), asset_request);
        REQUIRE(asset_response.has_value());
        CHECK(asset_response->status == 200);
        CHECK(asset_response->body == "console.log(1)");

        std::filesystem::path resolved;
        std::string error;
        CHECK_FALSE(chime::webd::ResolveContainedPath(root, "/../secret.txt", &resolved, &error));
        CHECK_FALSE(chime::webd::ResolveContainedPath(root, "/assets/../../secret.txt", &resolved, &error));

        chime::webd::HttpRequest traversal;
        traversal.method = "GET";
        traversal.path = "/assets/../../secret.txt";
        const auto traversal_response = chime::webd::ServeStaticUi(root.string(), traversal);
        REQUIRE(traversal_response.has_value());
        CHECK(traversal_response->status == 404);
        CHECK(traversal_response->body.find("secret") == std::string::npos);
        CHECK(traversal_response->body.find("nope") == std::string::npos);
        (void)secret;
    }

    TEST_CASE("falls back to index.html for extensionless paths and not for missing assets") {
        const ScopedTempDir tmp;
        const auto root = tmp.path() / "ui";
        std::filesystem::create_directories(root / "assets");
        {
            std::ofstream index(root / "index.html");
            index << "<html>spa</html>";
        }

        chime::webd::HttpRequest spa;
        spa.method = "GET";
        spa.path = "/settings";
        const auto spa_response = chime::webd::ServeStaticUi(root.string(), spa);
        REQUIRE(spa_response.has_value());
        CHECK(spa_response->status == 200);
        CHECK(spa_response->body == "<html>spa</html>");
        CHECK(spa_response->content_type.find("text/html") == 0);

        chime::webd::HttpRequest missing_asset;
        missing_asset.method = "GET";
        missing_asset.path = "/assets/missing.js";
        const auto missing_response = chime::webd::ServeStaticUi(root.string(), missing_asset);
        REQUIRE(missing_response.has_value());
        CHECK(missing_response->status == 404);
        CHECK(missing_response->body.find("spa") == std::string::npos);
    }

    TEST_CASE("does not serve a symlink that points outside the UI root") {
        const ScopedTempDir tmp;
        const auto root = tmp.path() / "ui";
        std::filesystem::create_directories(root);
        {
            std::ofstream index(root / "index.html");
            index << "<html>ok</html>";
        }
        const auto secret = tmp.WriteFile("secret.txt", "nope");
        std::filesystem::create_symlink(secret, root / "leak.txt");

        chime::webd::HttpRequest leak;
        leak.method = "GET";
        leak.path = "/leak.txt";
        const auto leak_response = chime::webd::ServeStaticUi(root.string(), leak);
        REQUIRE(leak_response.has_value());
        CHECK(leak_response->status == 404);
        CHECK(leak_response->body.find("nope") == std::string::npos);
    }

    TEST_CASE("does not serve an outbound index.html symlink for / or SPA fallback") {
        const ScopedTempDir tmp;
        const auto root = tmp.path() / "ui";
        std::filesystem::create_directories(root);
        const auto secret = tmp.WriteFile("secret.txt", "nope");
        std::filesystem::create_symlink(secret, root / "index.html");

        chime::webd::HttpRequest root_request;
        root_request.method = "GET";
        root_request.path = "/";
        const auto root_response = chime::webd::ServeStaticUi(root.string(), root_request);
        REQUIRE(root_response.has_value());
        CHECK(root_response->status == 404);
        CHECK(root_response->body.find("nope") == std::string::npos);

        chime::webd::HttpRequest spa;
        spa.method = "GET";
        spa.path = "/settings";
        const auto spa_response = chime::webd::ServeStaticUi(root.string(), spa);
        REQUIRE(spa_response.has_value());
        CHECK(spa_response->status == 404);
        CHECK(spa_response->body.find("nope") == std::string::npos);
    }
}
