#include "doctest.h"
#include "oc/config/kv_config.h"
#include "test_support.h"

namespace {

struct SampleConfig {
    std::string name;
    int port = 1;
    bool enabled = false;
    std::vector<std::string> items;
};

constexpr oc::config::Field<SampleConfig> kFields[] = {
    {"name", oc::config::parse_string<SampleConfig, &SampleConfig::name>, true},
    {"port", oc::config::parse_int<SampleConfig, &SampleConfig::port, 1, 65535>, true},
    {"enabled", oc::config::parse_bool<SampleConfig, &SampleConfig::enabled>, false},
    {"items", oc::config::parse_csv<SampleConfig, &SampleConfig::items>, false},
};

} // namespace

TEST_SUITE("kv_config") {
    TEST_CASE("loads required keys, optional values, and whitespace") {
        const ScopedTempDir tmp;
        const auto path = tmp.WriteFile("ok.conf", R"(
# comment
 name = door-bell 
port = 1883
  enabled = YES
items = a, b,c
)");

        const auto result = oc::config::load(path.string(), SampleConfig{}, kFields);
        REQUIRE(result);
        CHECK(result.config.name == "door-bell");
        CHECK(result.config.port == 1883);
        CHECK(result.config.enabled == true);
        REQUIRE(result.config.items.size() == 3);
        CHECK(result.config.items[0] == "a");
        CHECK(result.config.items[1] == "b");
        CHECK(result.config.items[2] == "c");
    }

    TEST_CASE("reports missing required keys") {
        const ScopedTempDir tmp;
        const auto path = tmp.WriteFile("missing.conf", "port=1883\n");
        const auto result = oc::config::load(path.string(), SampleConfig{}, kFields);
        CHECK_FALSE(result);
        CHECK(result.error == "Missing required config key: name");
    }

    TEST_CASE("treats invalid required integers as missing") {
        const ScopedTempDir tmp;
        const auto path = tmp.WriteFile("bad-int.conf", "name=x\nport=abc\n");
        const auto result = oc::config::load(path.string(), SampleConfig{}, kFields);
        CHECK_FALSE(result);
        CHECK(result.error == "Missing required config key: port");
    }

    TEST_CASE("rejects out-of-bounds integers") {
        const ScopedTempDir tmp;
        const auto path = tmp.WriteFile("bounds.conf", "name=x\nport=65536\n");
        const auto result = oc::config::load(path.string(), SampleConfig{}, kFields);
        CHECK_FALSE(result);
        CHECK(result.error == "Missing required config key: port");
    }

    TEST_CASE("rejects zero below the default integer minimum") {
        const ScopedTempDir tmp;
        const auto path = tmp.WriteFile("zero.conf", "name=x\nport=0\n");
        const auto result = oc::config::load(path.string(), SampleConfig{}, kFields);
        CHECK_FALSE(result);
    }

    TEST_CASE("parses booleans and ignores invalid optional bools") {
        const ScopedTempDir tmp;
        SampleConfig defaults;
        defaults.enabled = true;

        const auto valid = tmp.WriteFile("bool.conf", "name=x\nport=1\nenabled=off\n");
        const auto valid_result = oc::config::load(valid.string(), defaults, kFields);
        REQUIRE(valid_result);
        CHECK(valid_result.config.enabled == false);

        const auto invalid = tmp.WriteFile("bad-bool.conf", "name=x\nport=1\nenabled=maybe\n");
        const auto invalid_result = oc::config::load(invalid.string(), defaults, kFields);
        REQUIRE(invalid_result);
        CHECK(invalid_result.config.enabled == true);
    }

    TEST_CASE("duplicate keys keep the last successful value") {
        const ScopedTempDir tmp;
        const auto path = tmp.WriteFile("dup.conf", "name=first\nport=1\nname=second\nport=not-a-number\nport=9\n");
        const auto result = oc::config::load(path.string(), SampleConfig{}, kFields);
        REQUIRE(result);
        CHECK(result.config.name == "second");
        CHECK(result.config.port == 9);
    }

    TEST_CASE("skips comments, blank lines, and lines without equals") {
        const ScopedTempDir tmp;
        const auto path = tmp.WriteFile("skip.conf", "# only a comment\n\nnot-a-pair\nname=ok\nport=2\n");
        const auto result = oc::config::load(path.string(), SampleConfig{}, kFields);
        REQUIRE(result);
        CHECK(result.config.name == "ok");
        CHECK(result.config.port == 2);
    }

    TEST_CASE("reports a missing file") {
        const auto result = oc::config::load("/no/such/openchime-kv-config.conf", SampleConfig{}, kFields);
        CHECK_FALSE(result);
        CHECK(result.error.find("Failed to open config") != std::string::npos);
    }
}
