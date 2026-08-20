#include "chime/webd_api.h"

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <map>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <unistd.h>
#include <utility>
#include <vector>

#include "chime/generated/config_json.h"
#include "chime/webd_apply_manager.h"
#include "chime/webd_auth.h"
#include "chime/webd_config_store.h"
#include "chime/webd_json.h"
#include "chime/webd_json_http.h"
#include "chime/webd_json_validate.h"
#include "chime/webd_sound_name.h"
#include "chime/webd_static_files.h"
#include "chime/webd_string_utils.h"
#include "chime/webd_ui_assets.h"
#include "chime/webd_wifi_scan.h"
#include "oc/config/kv_config.h"
#include "oc/logging/logger.h"

namespace chime::webd {
namespace {

constexpr const char *kDefaultRingSoundName = "ring-default.wav";
constexpr const char *kReleaseInfoPath = "/etc/openchime-release";
constexpr const char *kAppVersionPath = "/etc/chime-app-version";

std::string MimeTypeOnly(const std::string &content_type) {
    const std::size_t semicolon = content_type.find(';');
    const std::string raw = semicolon == std::string::npos ? content_type : content_type.substr(0, semicolon);
    return ToLower(oc::config::trim(raw));
}

bool EnsureDirectoryExists(const std::string &path, std::string *error) {
    std::error_code ec;
    const bool exists = std::filesystem::exists(path, ec);
    if (ec) {
        if (error != nullptr) {
            *error = "exists(" + path + ") failed: " + ec.message();
        }
        return false;
    }

    if (exists) {
        const bool is_directory = std::filesystem::is_directory(path, ec);
        if (ec) {
            if (error != nullptr) {
                *error = "is_directory(" + path + ") failed: " + ec.message();
            }
            return false;
        }
        if (!is_directory) {
            if (error != nullptr) {
                *error = "path exists but is not a directory: " + path;
            }
            return false;
        }
        return true;
    }

    std::filesystem::create_directories(path, ec);
    if (ec) {
        if (error != nullptr) {
            *error = "create_directories(" + path + ") failed: " + ec.message();
        }
        return false;
    }
    return true;
}

void TrySeedDefaultRingSound(const std::string &ring_sounds_dir, const std::string &active_ring_sound_path,
                             oc::logging::Logger &logger) {
    std::error_code ec;
    const std::filesystem::path source_path(active_ring_sound_path);
    const std::filesystem::path target_path = std::filesystem::path(ring_sounds_dir) / kDefaultRingSoundName;
    const std::filesystem::path temp_path = target_path.string() + ".tmp";

    if (!std::filesystem::exists(source_path, ec) || !std::filesystem::is_regular_file(source_path, ec)) {
        return;
    }
    if (std::filesystem::exists(target_path, ec)) {
        return;
    }

    std::filesystem::copy_file(source_path, temp_path, std::filesystem::copy_options::overwrite_existing, ec);
    if (ec) {
        logger.Warn("webd", "failed to seed default ring sound: copy " + source_path.string() + " -> " +
                                temp_path.string() + " error=" + ec.message());
        return;
    }

    std::filesystem::rename(temp_path, target_path, ec);
    if (ec) {
        std::error_code remove_ec;
        std::filesystem::remove(temp_path, remove_ec);
        logger.Warn("webd", "failed to seed default ring sound: rename " + temp_path.string() + " -> " +
                                target_path.string() + " error=" + ec.message());
    }
}

std::vector<std::string> ReadObservedTopicsFromFile(const std::string &path, std::string *error) {
    std::vector<std::string> topics;
    std::ifstream file(path);
    if (!file.is_open()) {
        if (std::filesystem::exists(path) && error != nullptr) {
            *error = "failed to open observed topics file";
        }
        return topics;
    }

    std::set<std::string> seen;
    std::string line;
    while (std::getline(file, line)) {
        const std::string topic = oc::config::trim(line);
        if (topic.empty()) {
            continue;
        }
        if (seen.insert(topic).second) {
            topics.push_back(topic);
        }
    }
    return topics;
}

std::string ReadTrimmedFirstLine(const std::string &path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return "";
    }
    std::string line;
    if (!std::getline(file, line)) {
        return "";
    }
    return oc::config::trim(line);
}

std::map<std::string, std::string> ReadKeyValueFile(const std::string &path) {
    std::map<std::string, std::string> values;
    std::ifstream file(path);
    if (!file.is_open()) {
        return values;
    }

    std::string line;
    while (std::getline(file, line)) {
        const std::string trimmed = oc::config::trim(line);
        if (trimmed.empty() || trimmed[0] == '#') {
            continue;
        }

        const std::size_t separator = trimmed.find('=');
        if (separator == std::string::npos) {
            continue;
        }

        const std::string key = oc::config::trim(trimmed.substr(0, separator));
        const std::string value = oc::config::trim(trimmed.substr(separator + 1));
        if (!key.empty()) {
            values[key] = value;
        }
    }

    return values;
}

std::string ReadValueOrDefault(const std::map<std::string, std::string> &values, const std::string &key,
                               const std::string &fallback) {
    const auto it = values.find(key);
    if (it == values.end() || it->second.empty()) {
        return fallback;
    }
    return it->second;
}

JsonValue SerializeValidationErrors(const std::vector<ValidationError> &validation_errors) {
    std::vector<JsonValue> items;
    items.reserve(validation_errors.size());
    for (const auto &entry : validation_errors) {
        items.push_back(JsonValue::Object({
            {"field", JsonValue::String(entry.field)},
            {"message", JsonValue::String(entry.message)},
        }));
    }
    return JsonValue::Array(std::move(items));
}

JsonValue SerializeTopics(const std::vector<std::string> &topics) {
    std::vector<JsonValue> items;
    items.reserve(topics.size());
    for (const auto &topic : topics) {
        items.push_back(JsonValue::String(topic));
    }
    return JsonValue::Array(std::move(items));
}

JsonValue SerializeApplyStatus(const ApplyStatus &status) {
    return JsonValue::Object({
        {"job_id", JsonValue::Number(static_cast<double>(status.job_id))},
        {"state", JsonValue::String(status.state)},
        {"started_at_utc", JsonValue::String(status.started_at_utc)},
        {"finished_at_utc", JsonValue::String(status.finished_at_utc)},
        {"error", JsonValue::String(status.error)},
    });
}

JsonValue SerializeCoreConfig(const CoreConfigSnapshot &snapshot, const ApplyStatus &apply) {
    auto fields = generated_config_json::CoreConfigFieldsToJson(snapshot.config);
    fields["wifi_password_set"] = JsonValue::Bool(snapshot.wifi_password_set);
    fields["mqtt_password_set"] = JsonValue::Bool(snapshot.mqtt_password_set);
    fields["apply"] = SerializeApplyStatus(apply);
    return JsonValue::Object(std::move(fields));
}

bool IsApiPath(const std::string &path) {
    return path == "/api" || path.rfind("/api/", 0) == 0;
}

bool IsPublicApi(const HttpRequest &request) {
    if (request.method == "GET" && request.path == "/api/v1/system/version") {
        return true;
    }
    if (request.method == "GET" && request.path == "/api/v1/auth/status") {
        return true;
    }
    if (request.method == "POST" && request.path == "/api/v1/auth/pair") {
        return true;
    }
    if (request.method == "POST" && request.path == "/api/v1/auth/login") {
        return true;
    }
    return false;
}

bool IsMutatingMethod(const std::string &method) {
    return method == "POST" || method == "PUT";
}

} // namespace

WebApi::WebApi(oc::logging::Logger &logger, ConfigStore &config_store, WifiScanner &wifi_scanner,
               ApplyManager &apply_manager, AuthStore &auth_store, std::string ui_dist_dir,
               std::string observed_topics_path, std::string ring_sounds_dir, std::string active_ring_sound_path)
    : logger_(logger), config_store_(config_store), wifi_scanner_(wifi_scanner), apply_manager_(apply_manager),
      auth_store_(auth_store), ui_dist_dir_(std::move(ui_dist_dir)),
      observed_topics_path_(std::move(observed_topics_path)), ring_sounds_dir_(std::move(ring_sounds_dir)),
      active_ring_sound_path_(std::move(active_ring_sound_path)) {
    RegisterRoutes();
}

void WebApi::RegisterRoutes() {
    router_.SetMethodNotAllowed(JsonHttpError(405, "method_not_allowed"));
    router_.SetNotFound(JsonHttpError(404, "not_found"));
    router_.Add("GET", "/api/v1/auth/status",
                [this](const HttpRequest &request) { return auth_store_.HandleStatus(request); });
    router_.Add("POST", "/api/v1/auth/pair",
                [this](const HttpRequest &request) { return auth_store_.HandlePair(request); });
    router_.Add("POST", "/api/v1/auth/login",
                [this](const HttpRequest &request) { return auth_store_.HandleLogin(request); });
    router_.Add("POST", "/api/v1/auth/logout",
                [this](const HttpRequest &request) { return auth_store_.HandleLogout(request); });
    router_.Add("GET", "/api/v1/config/core", [this](const HttpRequest &) { return HandleGetCoreConfig(); });
    router_.Add("POST", "/api/v1/config/core",
                [this](const HttpRequest &request) { return HandlePostCoreConfig(request); });
    router_.Add("GET", "/api/v1/wifi/scan", [this](const HttpRequest &) { return HandleWifiScan(); });
    router_.Add("GET", "/api/v1/system/version", [this](const HttpRequest &) { return HandleGetSystemVersion(); });
    router_.Add("GET", "/api/v1/mqtt/topics", [this](const HttpRequest &) { return HandleGetObservedTopics(); });
    router_.Add("GET", "/api/v1/ring/sounds", [this](const HttpRequest &) { return HandleGetRingSounds(); });
    router_.Add("POST", "/api/v1/ring/sounds/select",
                [this](const HttpRequest &request) { return HandleSelectRingSound(request); });
    router_.AddPrefix("PUT", "/api/v1/ring/sounds/",
                      [this](const HttpRequest &request) { return HandleUploadRingSound(request); });

    const auto reserved = [this](const HttpRequest &request) { return ReservedNotImplemented(request); };
    router_.AddAny("/api/v1/system", reserved);
    router_.AddAnyPrefix("/api/v1/system/", reserved);
    router_.AddAny("/api/v1/device", reserved);
    router_.AddAnyPrefix("/api/v1/device/", reserved);
    router_.AddAny("/api/v1/diagnostics", reserved);
    router_.AddAnyPrefix("/api/v1/diagnostics/", reserved);

    router_.SetFallback([this](const HttpRequest &request) { return HandleFallback(request); });
}

HttpResponse WebApi::Handle(const HttpRequest &request) {
    if (!IsApiPath(request.path) || IsPublicApi(request)) {
        return router_.Dispatch(request);
    }
    const std::lock_guard<std::mutex> lock(product_mutex_);
    return AuthorizeAndDispatch(request);
}

HttpResponse WebApi::AuthorizeAndDispatch(const HttpRequest &request) {
    if (!auth_store_.Ready()) {
        return JsonHttpError(503, "auth_unavailable");
    }
    if (!auth_store_.IsPaired()) {
        return JsonHttpError(401, "unpaired");
    }
    const SessionView session = auth_store_.Inspect(request);
    if (!session.authenticated) {
        return JsonHttpError(401, "unauthorized");
    }
    if (IsMutatingMethod(request.method) && !session.csrf_valid) {
        return JsonHttpError(403, "csrf_failed");
    }
    return router_.Dispatch(request);
}

HttpResponse WebApi::HandleGetCoreConfig() {
    const SaveResult loaded = config_store_.LoadCoreConfig();
    if (!loaded.success) {
        return JsonHttpError(500, "load_failed", loaded.error);
    }
    return JsonHttpBody(200, SerializeCoreConfig(loaded.snapshot, apply_manager_.CurrentStatus()));
}

HttpResponse WebApi::HandlePostCoreConfig(const HttpRequest &request) {
    if (request.body.size() > kMaxJsonBodyBytes) {
        return JsonHttpError(400, "payload_too_large");
    }
    const JsonParseResult parsed = ParseJson(request.body);
    if (!parsed.success) {
        return JsonHttpError(400, "invalid_json", parsed.error);
    }
    if (parsed.value.type() != JsonValue::Type::kObject) {
        return JsonHttpError(400, "invalid_payload", "payload must be an object");
    }

    std::vector<ValidationError> parse_errors;
    SaveRequest save_request;
    generated_config_json::ReadSaveRequestFromJson(parsed.value, &save_request, &parse_errors);

    if (!parse_errors.empty()) {
        return JsonHttpBody(400, JsonValue::Object({
                                     {"error", JsonValue::String("validation_failed")},
                                     {"validation_errors", SerializeValidationErrors(parse_errors)},
                                 }));
    }

    const bool missing_success_sound = !GetObjectField(parsed.value, "notification_success_sound_path").has_value();
    const bool missing_failure_sound = !GetObjectField(parsed.value, "notification_failure_sound_path").has_value();
    if (missing_success_sound || missing_failure_sound) {
        const SaveResult loaded = config_store_.LoadCoreConfig();
        if (!loaded.success) {
            return JsonHttpError(500, "save_failed", loaded.error);
        }
        if (missing_success_sound) {
            save_request.config.notification_success_sound_path =
                loaded.snapshot.config.notification_success_sound_path;
        }
        if (missing_failure_sound) {
            save_request.config.notification_failure_sound_path =
                loaded.snapshot.config.notification_failure_sound_path;
        }
    }

    const SaveResult saved = config_store_.SaveCoreConfig(save_request);
    if (!saved.validation_errors.empty()) {
        return JsonHttpBody(400, JsonValue::Object({
                                     {"error", JsonValue::String("validation_failed")},
                                     {"validation_errors", SerializeValidationErrors(saved.validation_errors)},
                                 }));
    }
    if (!saved.success) {
        return JsonHttpError(500, "save_failed", saved.error);
    }

    const ApplyStatus apply = apply_manager_.StartApply();
    return JsonHttpBody(200, SerializeCoreConfig(saved.snapshot, apply));
}

HttpResponse WebApi::HandleWifiScan() {
    const WifiScanResult scan = wifi_scanner_.Scan();
    if (!scan.success) {
        return JsonHttpError(503, "scan_failed", scan.error);
    }

    std::vector<JsonValue> networks;
    networks.reserve(scan.networks.size());
    for (const auto &network : scan.networks) {
        networks.push_back(JsonValue::Object({
            {"ssid", JsonValue::String(network.ssid)},
            {"signal_dbm", JsonValue::Number(static_cast<double>(network.signal_dbm))},
            {"security", JsonValue::String(network.security)},
        }));
    }
    return JsonHttpBody(200, JsonValue::Object({
                                 {"networks", JsonValue::Array(std::move(networks))},
                             }));
}

HttpResponse WebApi::HandleGetSystemVersion() {
    const std::map<std::string, std::string> release_values = ReadKeyValueFile(kReleaseInfoPath);
    const std::string chime_version_fallback = ReadValueOrDefault(
        release_values, "CHIME_APP_VERSION_FULL", ReadValueOrDefault(release_values, "CHIME_APP_VERSION", "unknown"));
    const std::string app_version_file = ReadTrimmedFirstLine(kAppVersionPath);
    const std::string chime_version = app_version_file.empty() ? chime_version_fallback : app_version_file;
    const std::string os_version =
        ReadValueOrDefault(release_values, "OPENCHIME_OS_VERSION_FULL",
                           ReadValueOrDefault(release_values, "OPENCHIME_OS_VERSION", "unknown"));
    const std::string config_version = ReadValueOrDefault(release_values, "CHIME_CONFIG_VERSION", "unknown");

    return JsonHttpBody(200, JsonValue::Object({
                                 {"chime_version", JsonValue::String(chime_version)},
                                 {"os_version", JsonValue::String(os_version)},
                                 {"config_version", JsonValue::String(config_version)},
                             }));
}

HttpResponse WebApi::HandleGetObservedTopics() {
    std::string read_error;
    const std::vector<std::string> topics = ReadObservedTopicsFromFile(observed_topics_path_, &read_error);
    if (!read_error.empty()) {
        logger_.Warn("webd", read_error + " path=" + observed_topics_path_);
    }
    return JsonHttpBody(200, JsonValue::Object({
                                 {"topics", SerializeTopics(topics)},
                             }));
}

HttpResponse WebApi::HandleGetRingSounds() {
    std::string ensure_error;
    if (!EnsureDirectoryExists(ring_sounds_dir_, &ensure_error)) {
        return JsonHttpError(500, "ring_sounds_unavailable", ensure_error);
    }

    TrySeedDefaultRingSound(ring_sounds_dir_, active_ring_sound_path_, logger_);

    const std::string selected_path = ring_sounds_dir_ + "/selected.txt";
    std::string selected_name;
    {
        std::ifstream selected_file(selected_path);
        if (selected_file.is_open()) {
            std::getline(selected_file, selected_name);
            selected_name = oc::config::trim(selected_name);
        }
    }

    std::vector<std::string> sounds;
    std::error_code ec;
    for (const auto &entry : std::filesystem::directory_iterator(ring_sounds_dir_, ec)) {
        if (ec) {
            break;
        }
        if (!entry.is_regular_file(ec)) {
            continue;
        }
        const std::string name = entry.path().filename().string();
        if (!IsSafeSoundName(name)) {
            continue;
        }
        sounds.push_back(name);
    }
    if (ec) {
        logger_.Warn("webd", "failed to iterate ring sounds directory: " + ring_sounds_dir_ + " error=" + ec.message());
    }
    std::sort(sounds.begin(), sounds.end());

    bool selected_name_valid = IsSafeSoundName(selected_name);
    if (selected_name_valid) {
        selected_name_valid = std::find(sounds.begin(), sounds.end(), selected_name) != sounds.end();
    }
    if (!selected_name_valid && !selected_name.empty()) {
        selected_name.clear();
        std::ofstream selected_file(selected_path, std::ios::out | std::ios::trunc);
        if (!selected_file.is_open()) {
            logger_.Warn("webd", "failed to clear invalid selected sound file: " + selected_path +
                                     " error=" + std::strerror(errno));
        }
    }

    return JsonHttpBody(200, JsonValue::Object({
                                 {"selected_sound", JsonValue::String(selected_name)},
                                 {"sounds", SerializeTopics(sounds)},
                             }));
}

HttpResponse WebApi::HandleUploadRingSound(const HttpRequest &request) {
    const std::string prefix = "/api/v1/ring/sounds/";
    const std::string sound_name = request.path.substr(prefix.size());
    if (!IsSafeSoundName(sound_name)) {
        return JsonHttpError(400, "invalid_sound_name", "Use ring-*.wav");
    }
    if (request.has_content_type) {
        const std::string mime_type = MimeTypeOnly(request.content_type);
        if (mime_type != "audio/wav" && mime_type != "audio/x-wav") {
            return JsonHttpError(415, "invalid_payload", "payload is not a WAV file");
        }
    }
    if (request.body.size() < 12 || request.body.rfind("RIFF", 0) != 0 || request.body.compare(8, 4, "WAVE") != 0) {
        return JsonHttpError(415, "invalid_payload", "payload is not a WAV file");
    }

    std::string ensure_error;
    if (!EnsureDirectoryExists(ring_sounds_dir_, &ensure_error)) {
        return JsonHttpError(500, "ring_sounds_unavailable", ensure_error);
    }

    const std::filesystem::path sound_path = std::filesystem::path(ring_sounds_dir_) / sound_name;
    const std::filesystem::path temp_path = sound_path.string() + ".tmp";
    {
        std::ofstream output(temp_path, std::ios::binary | std::ios::trunc);
        if (!output.is_open()) {
            return JsonHttpError(500, "save_failed", "failed to open destination");
        }
        output.write(request.body.data(), static_cast<std::streamsize>(request.body.size()));
        output.flush();
        if (!output.good()) {
            output.close();
            std::error_code remove_ec;
            std::filesystem::remove(temp_path, remove_ec);
            return JsonHttpError(500, "save_failed", "failed to write destination");
        }
    }

    std::error_code rename_ec;
    std::filesystem::rename(temp_path, sound_path, rename_ec);
    if (rename_ec) {
        std::error_code remove_ec;
        std::filesystem::remove(temp_path, remove_ec);
        return JsonHttpError(500, "save_failed", "failed to move destination");
    }

    return JsonHttpBody(200, JsonValue::Object({
                                 {"uploaded", JsonValue::String(sound_name)},
                             }));
}

HttpResponse WebApi::HandleSelectRingSound(const HttpRequest &request) {
    if (request.body.size() > kMaxJsonBodyBytes) {
        return JsonHttpError(400, "payload_too_large");
    }
    const JsonParseResult parsed = ParseJson(request.body);
    if (!parsed.success || parsed.value.type() != JsonValue::Type::kObject) {
        return JsonHttpError(400, "invalid_json", "payload must be an object");
    }

    std::vector<ValidationError> parse_errors;
    const auto sound_name = ReadRequiredString(parsed.value, "name", &parse_errors);
    if (!parse_errors.empty() || !sound_name.has_value() || !IsSafeSoundName(*sound_name)) {
        return JsonHttpError(400, "invalid_sound_name", "Use ring-*.wav");
    }

    std::string ensure_error;
    if (!EnsureDirectoryExists(ring_sounds_dir_, &ensure_error)) {
        return JsonHttpError(500, "ring_sounds_unavailable", ensure_error);
    }

    const std::filesystem::path source = std::filesystem::path(ring_sounds_dir_) / *sound_name;
    if (!std::filesystem::exists(source) || !std::filesystem::is_regular_file(source)) {
        return JsonHttpError(404, "not_found", "sound file does not exist");
    }

    const std::filesystem::path target_path(active_ring_sound_path_);
    std::error_code ec;
    std::filesystem::create_directories(target_path.parent_path(), ec);
    if (ec) {
        return JsonHttpError(500, "create_directory_failed", "failed to create parent directory: " + ec.message());
    }

    const std::filesystem::path temp_path = target_path.string() + ".tmp";
    std::filesystem::copy_file(source, temp_path, std::filesystem::copy_options::overwrite_existing, ec);
    if (ec) {
        return JsonHttpError(500, "activate_failed", "failed to copy selected sound");
    }

    std::filesystem::rename(temp_path, target_path, ec);
    if (ec) {
        std::filesystem::remove(temp_path, ec);
        return JsonHttpError(500, "activate_failed", "failed to activate selected sound");
    }

    bool selection_persisted = true;
    {
        const std::string selected_path = ring_sounds_dir_ + "/selected.txt";
        const std::string temp_selected_path = selected_path + ".tmp";
        const std::string selected_value = *sound_name + "\n";

        int fd = open(temp_selected_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) {
            selection_persisted = false;
            logger_.Warn("webd", "failed to open selected file for write: " + temp_selected_path +
                                     " error=" + std::strerror(errno));
        } else {
            bool io_ok = true;
            const ssize_t written = write(fd, selected_value.data(), static_cast<size_t>(selected_value.size()));
            if (written != static_cast<ssize_t>(selected_value.size())) {
                io_ok = false;
            }
            if (io_ok && fsync(fd) != 0) {
                io_ok = false;
            }
            if (close(fd) != 0) {
                io_ok = false;
            }
            if (!io_ok) {
                selection_persisted = false;
                logger_.Warn("webd",
                             "failed to write selected file: " + temp_selected_path + " error=" + std::strerror(errno));
                std::remove(temp_selected_path.c_str());
            } else if (std::rename(temp_selected_path.c_str(), selected_path.c_str()) != 0) {
                selection_persisted = false;
                logger_.Warn("webd",
                             "failed to write selected file: " + selected_path + " error=" + std::strerror(errno));
                std::remove(temp_selected_path.c_str());
            }
        }
    }

    return JsonHttpBody(200, JsonValue::Object({
                                 {"selected", JsonValue::String(*sound_name)},
                                 {"selection_persisted", JsonValue::Bool(selection_persisted)},
                             }));
}

HttpResponse WebApi::ReservedNotImplemented(const HttpRequest &request) const {
    return JsonHttpBody(501, JsonValue::Object({
                                 {"error", JsonValue::String("not_implemented")},
                                 {"message", JsonValue::String("reserved endpoint")},
                                 {"path", JsonValue::String(request.path)},
                             }));
}

HttpResponse WebApi::HandleFallback(const HttpRequest &request) const {
    if (const auto ui_response = ServeStaticUi(ui_dist_dir_, request); ui_response.has_value()) {
        return *ui_response;
    }
    if (request.method == "GET" && request.path == "/") {
        HttpResponse response;
        response.status = 200;
        response.content_type = "text/html; charset=utf-8";
        response.body = MainPageHtml();
        return response;
    }
    return JsonHttpError(404, "not_found");
}

} // namespace chime::webd
