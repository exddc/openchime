#include "oc/json/json.h"

#include <memory>
#include <string_view>
#include <utility>

#include "cJSON.h"

namespace oc::json {
namespace {

struct CJsonDelete {
    void operator()(cJSON *node) const noexcept { cJSON_Delete(node); }
};

struct CJsonFree {
    void operator()(char *text) const noexcept { cJSON_free(text); }
};

using CJsonPtr = std::unique_ptr<cJSON, CJsonDelete>;
using CJsonPrinted = std::unique_ptr<char, CJsonFree>;

bool ContainsNul(std::string_view value) {
    return value.find('\0') != std::string_view::npos;
}

bool IsRfcJsonWhitespace(unsigned char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

bool JsonTextHasDisallowedControl(std::string_view json) {
    bool in_string = false;
    bool escape = false;
    for (std::size_t i = 0; i < json.size(); ++i) {
        const unsigned char c = static_cast<unsigned char>(json[i]);
        if (!in_string) {
            if (c == '"') {
                in_string = true;
                continue;
            }
            if (c < 0x20U && !IsRfcJsonWhitespace(c)) {
                return true;
            }
            continue;
        }
        if (escape) {
            escape = false;
            if (c == 'u' && i + 4 < json.size() && json[i + 1] == '0' && json[i + 2] == '0' && json[i + 3] == '0' &&
                json[i + 4] == '0') {
                return true;
            }
            continue;
        }
        if (c == '\\') {
            escape = true;
            continue;
        }
        if (c == '"') {
            in_string = false;
            continue;
        }
        if (c < 0x20U) {
            return true;
        }
    }
    return false;
}

bool JsonValueContainsNul(const JsonValue &value) {
    switch (value.type()) {
    case JsonValue::Type::kString: {
        std::string text;
        value.AsString(&text);
        return ContainsNul(text);
    }
    case JsonValue::Type::kArray:
        for (const auto &item : value.array_items()) {
            if (JsonValueContainsNul(item)) {
                return true;
            }
        }
        return false;
    case JsonValue::Type::kObject:
        for (const auto &entry : value.object_items()) {
            if (ContainsNul(entry.first) || JsonValueContainsNul(entry.second)) {
                return true;
            }
        }
        return false;
    default:
        return false;
    }
}

bool RemainderIsJsonWhitespace(const char *begin, const char *end) {
    for (const auto *cursor = begin; cursor != end; ++cursor) {
        if (!IsRfcJsonWhitespace(static_cast<unsigned char>(*cursor))) {
            return false;
        }
    }
    return true;
}

JsonValue FromCJson(const cJSON *node) {
    if (node == nullptr || cJSON_IsInvalid(node) || cJSON_IsNull(node)) {
        return JsonValue::Null();
    }
    if (cJSON_IsBool(node)) {
        return JsonValue::Bool(cJSON_IsTrue(node) != 0);
    }
    if (cJSON_IsNumber(node)) {
        return JsonValue::Number(node->valuedouble);
    }
    if (cJSON_IsString(node)) {
        if (node->valuestring == nullptr) {
            return JsonValue::String("");
        }
        return JsonValue::String(node->valuestring);
    }
    if (cJSON_IsArray(node)) {
        std::vector<JsonValue> items;
        const int size = cJSON_GetArraySize(node);
        if (size > 0) {
            items.reserve(static_cast<std::size_t>(size));
        }
        for (const cJSON *child = node->child; child != nullptr; child = child->next) {
            items.push_back(FromCJson(child));
        }
        return JsonValue::Array(std::move(items));
    }
    if (cJSON_IsObject(node)) {
        std::map<std::string, JsonValue> object;
        for (const cJSON *child = node->child; child != nullptr; child = child->next) {
            if (child->string == nullptr) {
                continue;
            }
            object[child->string] = FromCJson(child);
        }
        return JsonValue::Object(std::move(object));
    }
    return JsonValue::Null();
}

cJSON *ToCJson(const JsonValue &value) {
    switch (value.type()) {
    case JsonValue::Type::kNull:
        return cJSON_CreateNull();
    case JsonValue::Type::kBool: {
        bool flag = false;
        value.AsBool(&flag);
        return cJSON_CreateBool(flag ? 1 : 0);
    }
    case JsonValue::Type::kNumber: {
        double number = 0.0;
        value.AsNumber(&number);
        return cJSON_CreateNumber(number);
    }
    case JsonValue::Type::kString: {
        std::string text;
        value.AsString(&text);
        if (ContainsNul(text)) {
            return nullptr;
        }
        return cJSON_CreateString(text.c_str());
    }
    case JsonValue::Type::kArray: {
        CJsonPtr array(cJSON_CreateArray());
        if (!array) {
            return nullptr;
        }
        for (const auto &item : value.array_items()) {
            CJsonPtr child(ToCJson(item));
            if (!child) {
                return nullptr;
            }
            if (cJSON_AddItemToArray(array.get(), child.get()) == 0) {
                return nullptr;
            }
            child.release();
        }
        return array.release();
    }
    case JsonValue::Type::kObject: {
        CJsonPtr object(cJSON_CreateObject());
        if (!object) {
            return nullptr;
        }
        for (const auto &entry : value.object_items()) {
            if (ContainsNul(entry.first)) {
                return nullptr;
            }
            CJsonPtr child(ToCJson(entry.second));
            if (!child) {
                return nullptr;
            }
            if (cJSON_AddItemToObject(object.get(), entry.first.c_str(), child.get()) == 0) {
                return nullptr;
            }
            child.release();
        }
        return object.release();
    }
    }
    return cJSON_CreateNull();
}

} // namespace

JsonValue::JsonValue() = default;

JsonValue JsonValue::Null() {
    return JsonValue();
}

JsonValue JsonValue::Bool(bool value) {
    JsonValue result;
    result.type_ = Type::kBool;
    result.bool_value_ = value;
    return result;
}

JsonValue JsonValue::Number(double value) {
    JsonValue result;
    result.type_ = Type::kNumber;
    result.number_value_ = value;
    return result;
}

JsonValue JsonValue::String(std::string value) {
    JsonValue result;
    result.type_ = Type::kString;
    result.string_value_ = std::move(value);
    return result;
}

JsonValue JsonValue::Array(std::vector<JsonValue> value) {
    JsonValue result;
    result.type_ = Type::kArray;
    result.array_value_ = std::move(value);
    return result;
}

JsonValue JsonValue::Object(std::map<std::string, JsonValue> value) {
    JsonValue result;
    result.type_ = Type::kObject;
    result.object_value_ = std::move(value);
    return result;
}

JsonValue::Type JsonValue::type() const {
    return type_;
}

bool JsonValue::AsBool(bool *value) const {
    if (type_ != Type::kBool || value == nullptr) {
        return false;
    }
    *value = bool_value_;
    return true;
}

bool JsonValue::AsNumber(double *value) const {
    if (type_ != Type::kNumber || value == nullptr) {
        return false;
    }
    *value = number_value_;
    return true;
}

bool JsonValue::AsString(std::string *value) const {
    if (type_ != Type::kString || value == nullptr) {
        return false;
    }
    *value = string_value_;
    return true;
}

bool JsonValue::AsArray(std::vector<JsonValue> *value) const {
    if (type_ != Type::kArray || value == nullptr) {
        return false;
    }
    *value = array_value_;
    return true;
}

bool JsonValue::AsObject(std::map<std::string, JsonValue> *value) const {
    if (type_ != Type::kObject || value == nullptr) {
        return false;
    }
    *value = object_value_;
    return true;
}

const std::vector<JsonValue> &JsonValue::array_items() const {
    return array_value_;
}

const std::map<std::string, JsonValue> &JsonValue::object_items() const {
    return object_value_;
}

JsonParseResult ParseJson(std::string_view input) {
    JsonParseResult result;
    if (input.empty() || input.data() == nullptr || JsonTextHasDisallowedControl(input)) {
        result.error = "invalid json";
        return result;
    }

    const char *parse_end = nullptr;
    CJsonPtr root(cJSON_ParseWithLengthOpts(input.data(), input.size(), &parse_end, 0));
    if (!root || parse_end == nullptr) {
        result.error = "invalid json";
        return result;
    }
    if (!RemainderIsJsonWhitespace(parse_end, input.data() + input.size())) {
        result.error = "invalid json";
        return result;
    }

    result.value = FromCJson(root.get());
    result.success = true;
    return result;
}

JsonDumpResult DumpJson(const JsonValue &value) {
    JsonDumpResult result;
    if (JsonValueContainsNul(value)) {
        result.error = "serialize_failed";
        return result;
    }
    CJsonPtr root(ToCJson(value));
    if (!root) {
        result.error = "serialize_failed";
        return result;
    }
    CJsonPrinted printed(cJSON_PrintUnformatted(root.get()));
    if (!printed) {
        result.error = "serialize_failed";
        return result;
    }
    result.text = printed.get();
    result.success = true;
    return result;
}

std::optional<JsonValue> GetObjectField(const JsonValue &value, const std::string &key) {
    if (value.type() != JsonValue::Type::kObject) {
        return std::nullopt;
    }
    const auto &object = value.object_items();
    const auto it = object.find(key);
    if (it == object.end()) {
        return std::nullopt;
    }
    return it->second;
}

} // namespace oc::json
