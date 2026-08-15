#include <cmath>
#include <string>

#include "chime/webd_json.h"
#include "doctest.h"

TEST_SUITE("json") {
    TEST_CASE("parses objects, nested values, and field lookup") {
        const auto result = chime::webd::ParseJson(R"({"a":1,"b":{"c":"x"},"d":[true,false,null]})");
        REQUIRE(result.success);
        CHECK(result.value.type() == chime::webd::JsonValue::Type::kObject);

        std::string nested;
        const auto b = chime::webd::GetObjectField(result.value, "b");
        REQUIRE(b.has_value());
        const auto c = chime::webd::GetObjectField(*b, "c");
        REQUIRE(c.has_value());
        REQUIRE(c->AsString(&nested));
        CHECK(nested == "x");

        double a = 0;
        const auto a_field = chime::webd::GetObjectField(result.value, "a");
        REQUIRE(a_field.has_value());
        REQUIRE(a_field->AsNumber(&a));
        CHECK(a == 1.0);
    }

    TEST_CASE("parses strings and escape sequences") {
        const auto result = chime::webd::ParseJson(R"("a\"b\\c\/d\n\t")");
        REQUIRE(result.success);
        std::string value;
        REQUIRE(result.value.AsString(&value));
        CHECK(value == "a\"b\\c/d\n\t");
    }

    TEST_CASE("parses unicode escapes using current ASCII-or-placeholder behavior") {
        const auto ascii = chime::webd::ParseJson(R"("\u0041")");
        REQUIRE(ascii.success);
        std::string ascii_value;
        REQUIRE(ascii.value.AsString(&ascii_value));
        CHECK(ascii_value == "A");

        const auto non_ascii = chime::webd::ParseJson(R"("\u00e9")");
        REQUIRE(non_ascii.success);
        std::string non_ascii_value;
        REQUIRE(non_ascii.value.AsString(&non_ascii_value));
        CHECK(non_ascii_value == "?");
    }

    TEST_CASE("parses numbers, booleans, and null") {
        const auto number = chime::webd::ParseJson("3.14");
        REQUIRE(number.success);
        double parsed = 0;
        REQUIRE(number.value.AsNumber(&parsed));
        CHECK(std::fabs(parsed - 3.14) < 0.0001);

        const auto exp = chime::webd::ParseJson("1e2");
        REQUIRE(exp.success);
        REQUIRE(exp.value.AsNumber(&parsed));
        CHECK(parsed == 100.0);

        const auto negative = chime::webd::ParseJson("-12");
        REQUIRE(negative.success);
        REQUIRE(negative.value.AsNumber(&parsed));
        CHECK(parsed == -12.0);

        bool flag = false;
        const auto true_value = chime::webd::ParseJson("true");
        REQUIRE(true_value.success);
        REQUIRE(true_value.value.AsBool(&flag));
        CHECK(flag);

        const auto false_value = chime::webd::ParseJson("false");
        REQUIRE(false_value.success);
        REQUIRE(false_value.value.AsBool(&flag));
        CHECK_FALSE(flag);

        const auto null_value = chime::webd::ParseJson("null");
        REQUIRE(null_value.success);
        CHECK(null_value.value.type() == chime::webd::JsonValue::Type::kNull);
    }

    TEST_CASE("rejects malformed input") {
        const char *malformed[] = {
            "",          "{",           "[",      R"({"a"})", R"({a:1})",
            R"({"a":})", R"({"a":1,})", "[1,2,]", "'hello'",  R"("unterminated)",
            R"("\q")",   "01",          "1.",     "-",        "tru",
            "nul",
        };

        for (const char *input : malformed) {
            CAPTURE(input);
            const auto result = chime::webd::ParseJson(input);
            CHECK_FALSE(result.success);
            CHECK_FALSE(result.error.empty());
        }
    }

    TEST_CASE("rejects trailing junk after a valid value") {
        const auto result = chime::webd::ParseJson("{} true");
        CHECK_FALSE(result.success);
        CHECK(result.error == "unexpected trailing characters");

        const auto number_junk = chime::webd::ParseJson("123abc");
        CHECK_FALSE(number_junk.success);
    }

    TEST_CASE("allows surrounding whitespace") {
        const auto result = chime::webd::ParseJson(" \n\t{ \"k\" : true }\r\n");
        REQUIRE(result.success);
        bool flag = false;
        const auto field = chime::webd::GetObjectField(result.value, "k");
        REQUIRE(field.has_value());
        REQUIRE(field->AsBool(&flag));
        CHECK(flag);
    }
}
