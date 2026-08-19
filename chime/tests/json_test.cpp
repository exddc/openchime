#include <cmath>
#include <string>

#include "chime/webd_json.h"
#include "doctest.h"

#if defined(CHIME_WEBD_HTTP_H)
#error "json tests must not depend on HTTP types"
#endif
#if defined(CHIME_WEBD_TYPES_H)
#error "json tests must not depend on product validation types"
#endif

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

    TEST_CASE("parses unicode escapes as UTF-8") {
        const auto ascii = chime::webd::ParseJson(R"("\u0041")");
        REQUIRE(ascii.success);
        std::string ascii_value;
        REQUIRE(ascii.value.AsString(&ascii_value));
        CHECK(ascii_value == "A");

        const auto non_ascii = chime::webd::ParseJson(R"("\u00e9")");
        REQUIRE(non_ascii.success);
        std::string non_ascii_value;
        REQUIRE(non_ascii.value.AsString(&non_ascii_value));
        CHECK(non_ascii_value == "é");
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
            R"("\q")",   "-",           "tru",    "nul",
        };

        for (const char *input : malformed) {
            CAPTURE(input);
            const auto result = chime::webd::ParseJson(input);
            CHECK_FALSE(result.success);
            CHECK_FALSE(result.error.empty());
        }
    }

    TEST_CASE("cJSON accepts some non-RFC number spellings") {
        const auto leading_zero = chime::webd::ParseJson("01");
        REQUIRE(leading_zero.success);
        double parsed = 0;
        REQUIRE(leading_zero.value.AsNumber(&parsed));
        CHECK(parsed == 1.0);

        const auto trailing_dot = chime::webd::ParseJson("1.");
        REQUIRE(trailing_dot.success);
        REQUIRE(trailing_dot.value.AsNumber(&parsed));
        CHECK(parsed == 1.0);
    }

    TEST_CASE("rejects trailing junk after a valid value") {
        const auto result = chime::webd::ParseJson("{} true");
        CHECK_FALSE(result.success);
        CHECK_FALSE(result.error.empty());

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

    TEST_CASE("dumps objects without concatenating raw JSON") {
        const auto dumped = chime::webd::DumpJson(chime::webd::JsonValue::Object({
            {"error", chime::webd::JsonValue::String("not_found")},
            {"message", chime::webd::JsonValue::String("a\"b")},
        }));
        REQUIRE(dumped.success);
        const auto parsed = chime::webd::ParseJson(dumped.text);
        REQUIRE(parsed.success);
        const auto error_field = chime::webd::GetObjectField(parsed.value, "error");
        REQUIRE(error_field.has_value());
        std::string error;
        REQUIRE(error_field->AsString(&error));
        CHECK(error == "not_found");
        const auto message_field = chime::webd::GetObjectField(parsed.value, "message");
        REQUIRE(message_field.has_value());
        std::string message;
        REQUIRE(message_field->AsString(&message));
        CHECK(message == "a\"b");
    }

    TEST_CASE("converts large arrays in linear time" * doctest::timeout(2.0)) {
        constexpr int kCount = 80000;
        std::string json;
        json.reserve(static_cast<std::size_t>(kCount) * 2 + 2);
        json.push_back('[');
        for (int i = 0; i < kCount; ++i) {
            if (i != 0) {
                json.push_back(',');
            }
            json.push_back('1');
        }
        json.push_back(']');

        const auto parsed = chime::webd::ParseJson(json);
        REQUIRE(parsed.success);
        REQUIRE(parsed.value.type() == chime::webd::JsonValue::Type::kArray);
        CHECK(parsed.value.array_items().size() == static_cast<std::size_t>(kCount));

        const auto dumped = chime::webd::DumpJson(parsed.value);
        REQUIRE(dumped.success);
        const auto round_trip = chime::webd::ParseJson(dumped.text);
        REQUIRE(round_trip.success);
        CHECK(round_trip.value.array_items().size() == static_cast<std::size_t>(kCount));
    }

    TEST_CASE("rejects raw and escaped NUL rather than truncating") {
        CHECK_FALSE(chime::webd::ParseJson(R"("\u0000")").success);
        CHECK_FALSE(chime::webd::ParseJson(R"("a\u0000b")").success);
        CHECK_FALSE(chime::webd::ParseJson(R"({"a\u0000b":1})").success);

        std::string raw_nul = "\"a";
        raw_nul.push_back('\0');
        raw_nul += "b\"";
        CHECK_FALSE(chime::webd::ParseJson(raw_nul).success);

        const auto escaped_backslash = chime::webd::ParseJson(R"("\\u0000")");
        REQUIRE(escaped_backslash.success);
        std::string escaped_value;
        REQUIRE(escaped_backslash.value.AsString(&escaped_value));
        CHECK(escaped_value == "\\u0000");

        const auto null_dump = chime::webd::DumpJson(chime::webd::JsonValue::Null());
        REQUIRE(null_dump.success);
        CHECK(null_dump.text == "null");

        std::string with_nul("a");
        with_nul.push_back('\0');
        with_nul.push_back('b');
        const auto dumped = chime::webd::DumpJson(chime::webd::JsonValue::String(with_nul));
        CHECK_FALSE(dumped.success);
        CHECK_FALSE(dumped.error.empty());
        CHECK(dumped.text != "null");
        CHECK(dumped.text.find('"') == std::string::npos);
    }

    TEST_CASE("rejects unescaped control bytes outside RFC whitespace") {
        constexpr char kControl = '\x01';

        std::string leading;
        leading.push_back(kControl);
        leading += "true";
        CHECK_FALSE(chime::webd::ParseJson(leading).success);

        std::string trailing = "true";
        trailing.push_back(kControl);
        CHECK_FALSE(chime::webd::ParseJson(trailing).success);

        std::string inter_token = "[1,";
        inter_token.push_back(kControl);
        inter_token += "2]";
        CHECK_FALSE(chime::webd::ParseJson(inter_token).success);

        std::string in_string = "\"a";
        in_string.push_back(kControl);
        in_string += "b\"";
        CHECK_FALSE(chime::webd::ParseJson(in_string).success);

        std::string in_string_newline = "\"a";
        in_string_newline.push_back('\n');
        in_string_newline += "b\"";
        CHECK_FALSE(chime::webd::ParseJson(in_string_newline).success);

        CHECK(chime::webd::ParseJson(" \t\r\ntrue\n").success);
        CHECK(chime::webd::ParseJson(R"("a\nb")").success);
        CHECK(chime::webd::ParseJson(R"("\u0001")").success);
    }
}
