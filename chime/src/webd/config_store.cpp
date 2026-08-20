#include "chime/webd_config_store.h"

#include <cerrno>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>
#include <sys/stat.h>
#include <utility>
#include <vector>

#include "chime/chime_config.h"
#include "chime/generated/config_json.h"
#include "chime/generated/config_types.h"
#include "oc/config/kv_config.h"
#include "oc/config/kv_document.h"
#include "oc/logging/logger.h"
#include "oc/util/filesystem.h"

namespace chime::webd {
namespace {

constexpr mode_t kChimeConfigMode = 0600;
constexpr mode_t kWpaConfigMode = 0600;

bool ReadAllLines(const std::string &path, std::vector<std::string> *lines, std::string *error) {
    if (lines == nullptr || error == nullptr) {
        return false;
    }

    std::ifstream file(path);
    if (!file.is_open()) {
        *error = "failed to open file '" + path + "': " + std::strerror(errno);
        return false;
    }

    std::vector<std::string> output;
    std::string line;
    while (std::getline(file, line)) {
        output.push_back(line);
    }
    lines->swap(output);
    return true;
}

bool ReadAllLinesIfExists(const std::string &path, std::vector<std::string> *lines, std::string *error) {
    if (!std::filesystem::exists(path)) {
        lines->clear();
        return true;
    }
    return ReadAllLines(path, lines, error);
}

std::string JoinLines(const std::vector<std::string> &lines) {
    std::string content;
    for (const auto &line : lines) {
        content += line;
        content.push_back('\n');
    }
    if (lines.empty()) {
        content.push_back('\n');
    }
    return content;
}

std::string StripQuotes(std::string_view value) {
    const std::string trimmed = oc::config::trim(value);
    if (trimmed.size() < 2 || trimmed.front() != '"' || trimmed.back() != '"') {
        return trimmed;
    }

    std::string output;
    output.reserve(trimmed.size() - 2);
    bool escape = false;
    for (std::size_t i = 1; i + 1 < trimmed.size(); ++i) {
        const char c = trimmed[i];
        if (escape) {
            output.push_back(c);
            escape = false;
            continue;
        }
        if (c == '\\') {
            escape = true;
            continue;
        }
        output.push_back(c);
    }
    return output;
}

std::string QuoteForWpa(const std::string &value) {
    std::string out;
    out.push_back('"');
    for (const char c : value) {
        if (c == '\\' || c == '"') {
            out.push_back('\\');
        }
        out.push_back(c);
    }
    out.push_back('"');
    return out;
}

struct WpaData {
    std::vector<std::string> lines;
    std::string ssid;
    std::string psk;
    bool has_network_block = false;
    std::size_t block_start = 0;
    std::size_t block_end = 0;
};

WpaData ParseWpaData(const std::vector<std::string> &lines) {
    WpaData data;
    data.lines = lines;

    bool in_block = false;
    bool block_closed = false;
    for (std::size_t i = 0; i < lines.size(); ++i) {
        const std::string trimmed = oc::config::trim(lines[i]);
        if (!in_block && trimmed == "network={") {
            in_block = true;
            data.has_network_block = true;
            data.block_start = i;
            continue;
        }

        if (!in_block) {
            continue;
        }

        if (trimmed == "}") {
            data.block_end = i;
            block_closed = true;
            break;
        }

        const auto separator = trimmed.find('=');
        if (separator == std::string::npos) {
            continue;
        }

        const std::string key = oc::config::trim(trimmed.substr(0, separator));
        const std::string value = oc::config::trim(trimmed.substr(separator + 1));

        if (key == "ssid") {
            data.ssid = StripQuotes(value);
        } else if (key == "psk") {
            data.psk = StripQuotes(value);
        }
    }

    if (data.has_network_block && !block_closed) {
        data.has_network_block = false;
    }

    return data;
}

} // namespace

ConfigStore::ConfigStore(oc::logging::Logger &logger, std::string chime_config_path, std::string wpa_supplicant_path)
    : logger_(logger), chime_config_path_(std::move(chime_config_path)),
      wpa_supplicant_path_(std::move(wpa_supplicant_path)) {}

SaveResult ConfigStore::LoadCoreConfig() const {
    return LoadCoreConfigInternal();
}

SaveResult ConfigStore::SaveCoreConfig(const SaveRequest &request) {
    SaveResult result;
    result.validation_errors = ValidateRequest(request);
    if (!result.validation_errors.empty()) {
        result.error = "validation_failed";
        return result;
    }

    const SaveResult existing = LoadCoreConfigInternal();
    if (!existing.success) {
        return existing;
    }

    std::string error;
    if (!SaveWpaSupplicant(request, existing.snapshot, &error)) {
        logger_.Error("webd", "failed to save wpa_supplicant.conf: " + error);
        result.error = error;
        return result;
    }

    if (!SaveChimeConfig(request, existing.snapshot, &error)) {
        logger_.Error("webd", "failed to save chime.conf: " + error);
        result.error = error;
        return result;
    }

    SaveResult loaded = LoadCoreConfigInternal();
    if (!loaded.success) {
        return loaded;
    }

    loaded.success = true;
    return loaded;
}

std::vector<ValidationError> ConfigStore::ValidateRequest(const SaveRequest &request) const {
    std::vector<ValidationError> errors;
    generated_config_json::ValidateSaveRequest(request, &errors);

    if (request.config.mqtt_username.empty() && request.mqtt_password.has_value() && !request.mqtt_password->empty()) {
        errors.push_back({"mqtt_password", "mqtt_password requires mqtt_username to be set"});
    }

    const bool tls_cert_set = !request.config.mqtt_tls_cert_file.empty();
    const bool tls_key_set = !request.config.mqtt_tls_key_file.empty();
    if (tls_cert_set != tls_key_set) {
        errors.push_back({"mqtt_tls_cert_file", "mqtt_tls_cert_file and mqtt_tls_key_file must both be set"});
    }

    const auto reject_blank_after_trim = [&errors](const char *key, const std::string &value) {
        if (!value.empty() && oc::config::trim(value).empty()) {
            const auto *spec = FindConfigField(key);
            const int max_len = spec != nullptr && spec->max_len > 0 ? spec->max_len : 256;
            errors.push_back(
                {key, std::string(key) + " must be 1-" + std::to_string(max_len) + " chars after trimming"});
        }
    };
    reject_blank_after_trim("notification_success_sound_path", request.config.notification_success_sound_path);
    reject_blank_after_trim("notification_failure_sound_path", request.config.notification_failure_sound_path);

    return errors;
}

SaveResult ConfigStore::LoadCoreConfigInternal() const {
    SaveResult result;
    result.success = false;

    const auto loaded = oc::config::load(chime_config_path_, FileConfig{}, kFileConfigFields);
    if (!loaded) {
        result.error = loaded.error;
        return result;
    }

    CoreConfig config = CoreConfigFromFile(loaded.config);

    std::vector<std::string> wpa_lines;
    std::string error;
    if (!ReadAllLinesIfExists(wpa_supplicant_path_, &wpa_lines, &error)) {
        result.error = error;
        return result;
    }

    const WpaData wpa_data = ParseWpaData(wpa_lines);
    config.wifi_ssid = wpa_data.ssid;

    result.snapshot.config = std::move(config);
    result.snapshot.wifi_password_set = !wpa_data.psk.empty();
    result.snapshot.mqtt_password_set = !result.snapshot.config.mqtt_password.empty();
    result.success = true;
    return result;
}

bool ConfigStore::SaveChimeConfig(const SaveRequest &request, const CoreConfigSnapshot &existing,
                                  std::string *error) const {
    std::string original;
    {
        std::ifstream file(chime_config_path_);
        if (!file.is_open()) {
            *error = "failed to open file '" + chime_config_path_ + "'";
            return false;
        }
        original.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    }

    auto document = oc::config::ParseKvDocument(original);
    std::string mqtt_password = existing.config.mqtt_password;
    if (request.mqtt_password.has_value()) {
        mqtt_password = *request.mqtt_password;
    }

    const auto replacements = CoreConfigFileReplacements(request.config, mqtt_password);
    for (const auto &[key, value] : replacements) {
        oc::config::KvDocumentSetValue(document, key, value);
    }
    for (const char *removed : kRemovedConfigKeys) {
        oc::config::KvDocumentRemoveKey(document, removed);
    }

    return oc::util::AtomicWriteFile(chime_config_path_, oc::config::RenderKvDocument(document), kChimeConfigMode,
                                     error);
}

bool ConfigStore::SaveWpaSupplicant(const SaveRequest &request, const CoreConfigSnapshot &, std::string *error) const {
    std::vector<std::string> lines;
    if (!ReadAllLinesIfExists(wpa_supplicant_path_, &lines, error)) {
        return false;
    }

    if (lines.empty()) {
        lines.push_back("ctrl_interface=/var/run/wpa_supplicant");
        lines.push_back("update_config=1");
        lines.push_back("country=US");
        lines.push_back("");
    }

    WpaData parsed = ParseWpaData(lines);

    std::string password_value;
    if (request.wifi_password.has_value()) {
        if (request.wifi_password->empty()) {
            password_value = parsed.psk;
            if (password_value.empty()) {
                *error = "wifi_password is blank and no existing password is available";
                return false;
            }
        } else {
            password_value = *request.wifi_password;
        }
    } else {
        password_value = parsed.psk;
        if (password_value.empty()) {
            *error = "wifi_password is missing and no existing password is available";
            return false;
        }
    }

    const std::string ssid_line = "    ssid=" + QuoteForWpa(request.config.wifi_ssid);
    const std::string psk_line = "    psk=" + QuoteForWpa(password_value);

    if (!parsed.has_network_block) {
        if (!lines.empty() && !lines.back().empty()) {
            lines.push_back("");
        }
        lines.push_back("network={");
        lines.push_back(ssid_line);
        lines.push_back(psk_line);
        lines.push_back("}");
    } else {
        bool ssid_written = false;
        bool psk_written = false;

        for (std::size_t i = parsed.block_start + 1; i < parsed.block_end; ++i) {
            const std::string trimmed = oc::config::trim(lines[i]);
            const auto separator = trimmed.find('=');
            if (separator == std::string::npos) {
                continue;
            }

            const std::string key = oc::config::trim(trimmed.substr(0, separator));
            if (key == "ssid") {
                lines[i] = ssid_line;
                ssid_written = true;
            } else if (key == "psk") {
                lines[i] = psk_line;
                psk_written = true;
            }
        }

        std::size_t insert_pos = parsed.block_end;
        if (!ssid_written) {
            lines.insert(lines.begin() + static_cast<std::vector<std::string>::difference_type>(insert_pos), ssid_line);
            ++insert_pos;
            ++parsed.block_end;
        }
        if (!psk_written) {
            lines.insert(lines.begin() + static_cast<std::vector<std::string>::difference_type>(insert_pos), psk_line);
            ++parsed.block_end;
        }
    }

    const std::string content = JoinLines(lines);
    return oc::util::AtomicWriteFile(wpa_supplicant_path_, content, kWpaConfigMode, error);
}

} // namespace chime::webd
