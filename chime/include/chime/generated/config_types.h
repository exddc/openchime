#ifndef CHIME_GENERATED_CONFIG_TYPES_H
#define CHIME_GENERATED_CONFIG_TYPES_H

#include <map>
#include <string>
#include <string_view>
#include <vector>

#include "oc/config/kv_config.h"

namespace chime {

constexpr int kConfigSchemaVersion = 5;
constexpr int kLegacyUnversionedSchema = 4;

enum class ConfigValueType { kString, kInt, kBool, kCsv };
enum class ConfigPersist { kNone, kFile, kWpa };

struct ConfigFieldSpec {
    const char *key;
    ConfigValueType type;
    const char *default_text;
    const char *repair_text;
    ConfigPersist persist;
    bool runtime;
    bool api;
    bool ui;
    bool init_only;
    bool secret;
    bool file_required;
    bool api_required;
    bool api_empty_ok;
    bool forbid_whitespace;
    bool forbid_newline;
    int min_value;
    int max_value;
    int min_len;
    int max_len;
};

constexpr ConfigFieldSpec kAllConfigFields[] = {
    {"schema_version", ConfigValueType::kInt, "5", "5", ConfigPersist::kFile, false, false, false, false, false, false, false, false, false, false, 1, 1000000, 0, 0},
    {"mqtt_host", ConfigValueType::kString, "", "", ConfigPersist::kFile, true, true, true, false, false, true, true, false, true, false, 0, 0, 0, 256},
    {"mqtt_port", ConfigValueType::kInt, "1883", "1883", ConfigPersist::kFile, true, true, true, false, false, true, true, false, false, false, 1, 65535, 0, 0},
    {"mqtt_client_id", ConfigValueType::kString, "chime", "chime", ConfigPersist::kFile, true, true, true, false, false, false, true, false, false, false, 0, 0, 0, 128},
    {"mqtt_username", ConfigValueType::kString, "", "", ConfigPersist::kFile, true, true, true, false, false, false, true, true, false, false, 0, 0, 0, 128},
    {"mqtt_password", ConfigValueType::kString, "", "", ConfigPersist::kFile, true, true, true, false, true, false, false, false, false, false, 0, 0, 0, 256},
    {"mqtt_tls_enabled", ConfigValueType::kBool, "false", "false", ConfigPersist::kFile, true, true, true, false, false, false, true, false, false, false, 0, 0, 0, 0},
    {"mqtt_tls_validate_certificate", ConfigValueType::kBool, "true", "true", ConfigPersist::kFile, true, true, true, false, false, false, true, false, false, false, 0, 0, 0, 0},
    {"mqtt_tls_ca_file", ConfigValueType::kString, "", "", ConfigPersist::kFile, true, true, true, false, false, false, true, true, false, false, 0, 0, 0, 256},
    {"mqtt_tls_cert_file", ConfigValueType::kString, "", "", ConfigPersist::kFile, true, true, true, false, false, false, true, true, false, false, 0, 0, 0, 256},
    {"mqtt_tls_key_file", ConfigValueType::kString, "", "", ConfigPersist::kFile, true, true, true, false, false, false, true, true, false, false, 0, 0, 0, 256},
    {"mqtt_topics", ConfigValueType::kCsv, "", "doorbell/ring,doorbell/status", ConfigPersist::kFile, true, true, true, false, false, true, true, false, true, false, 0, 0, 0, 0},
    {"mqtt_subscribe_qos", ConfigValueType::kInt, "0", "0", ConfigPersist::kFile, true, false, false, false, false, false, false, false, false, false, 0, 2, 0, 0},
    {"heartbeat_interval", ConfigValueType::kInt, "60", "60", ConfigPersist::kFile, true, false, false, false, false, false, false, false, false, false, 0, 3600, 0, 0},
    {"heartbeat_topic", ConfigValueType::kString, "chime/heartbeat", "chime/heartbeat", ConfigPersist::kFile, true, false, false, false, false, false, false, false, false, false, 0, 0, 0, 256},
    {"ntp_servers", ConfigValueType::kCsv, "time.cloudflare.com,time.google.com,pool.ntp.org", "time.cloudflare.com,time.google.com,pool.ntp.org", ConfigPersist::kFile, false, false, false, true, false, false, false, false, false, false, 0, 0, 0, 0},
    {"time_http_urls", ConfigValueType::kCsv, "http://connectivitycheck.gstatic.com/generate_204,http://detectportal.firefox.com/success.txt,http://example.com/", "http://connectivitycheck.gstatic.com/generate_204,http://detectportal.firefox.com/success.txt,http://example.com/", ConfigPersist::kFile, false, false, false, true, false, false, false, false, false, false, 0, 0, 0, 0},
    {"time_sync_retries", ConfigValueType::kInt, "6", "6", ConfigPersist::kFile, false, false, false, true, false, false, false, false, false, false, 1, 100, 0, 0},
    {"time_sync_retry_delay", ConfigValueType::kInt, "5", "5", ConfigPersist::kFile, false, false, false, true, false, false, false, false, false, false, 1, 3600, 0, 0},
    {"time_sync_interval", ConfigValueType::kInt, "3600", "3600", ConfigPersist::kFile, false, false, false, true, false, false, false, false, false, false, 0, 86400, 0, 0},
    {"ring_topic", ConfigValueType::kString, "doorbell/ring", "doorbell/ring", ConfigPersist::kFile, true, true, true, false, false, false, true, false, true, false, 0, 0, 0, 256},
    {"sound_path", ConfigValueType::kString, "/usr/local/share/chime/ring.wav", "/usr/local/share/chime/ring.wav", ConfigPersist::kFile, true, false, false, false, false, false, false, false, false, false, 0, 0, 0, 256},
    {"notification_success_sound_path", ConfigValueType::kString, "/usr/local/share/chime/test.wav", "/usr/local/share/chime/test.wav", ConfigPersist::kFile, true, true, true, false, false, false, false, false, false, true, 0, 0, 1, 256},
    {"notification_failure_sound_path", ConfigValueType::kString, "/usr/local/share/chime/ring.wav", "/usr/local/share/chime/ring.wav", ConfigPersist::kFile, true, true, true, false, false, false, false, false, false, true, 0, 0, 1, 256},
    {"volume_bell", ConfigValueType::kInt, "80", "80", ConfigPersist::kFile, true, true, true, false, false, false, true, false, false, false, 0, 100, 0, 0},
    {"volume_notifications", ConfigValueType::kInt, "70", "70", ConfigPersist::kFile, true, true, true, false, false, false, true, false, false, false, 0, 100, 0, 0},
    {"audio_enabled", ConfigValueType::kBool, "true", "true", ConfigPersist::kFile, true, false, false, false, false, false, false, false, false, false, 0, 0, 0, 0},
    {"wifi_interface", ConfigValueType::kString, "wlan0", "wlan0", ConfigPersist::kFile, true, false, false, false, false, false, false, false, false, false, 0, 0, 0, 32},
    {"wifi_check_interval", ConfigValueType::kInt, "5", "5", ConfigPersist::kFile, true, false, false, false, false, false, false, false, false, false, 0, 3600, 0, 0},
    {"log_max_bytes", ConfigValueType::kInt, "262144", "262144", ConfigPersist::kFile, false, false, false, true, false, false, false, false, false, false, 1024, 104857600, 0, 0},
    {"log_rotate_keep", ConfigValueType::kInt, "5", "5", ConfigPersist::kFile, false, false, false, true, false, false, false, false, false, false, 1, 100, 0, 0},
    {"log_rotate_check_interval", ConfigValueType::kInt, "30", "30", ConfigPersist::kFile, false, false, false, true, false, false, false, false, false, false, 1, 3600, 0, 0},
    {"wifi_ssid", ConfigValueType::kString, "", "", ConfigPersist::kWpa, false, true, true, false, false, false, true, false, false, false, 0, 0, 0, 32},
    {"wifi_password", ConfigValueType::kString, "", "", ConfigPersist::kWpa, false, true, true, false, true, false, false, false, false, false, 0, 0, 8, 63}
};

inline constexpr const char *kRemovedConfigKeys[] = {"volume_other"};

inline const ConfigFieldSpec *FindConfigField(std::string_view key) {
    for (const auto &field : kAllConfigFields) {
        if (key == field.key) {
            return &field;
        }
    }
    return nullptr;
}

struct ChimeConfig {
    std::string mqtt_host{};
    int mqtt_port = 1883;
    std::string mqtt_client_id = "chime";
    std::string mqtt_username{};
    std::string mqtt_password{};
    bool mqtt_tls_enabled = false;
    bool mqtt_tls_validate_certificate = true;
    std::string mqtt_tls_ca_file{};
    std::string mqtt_tls_cert_file{};
    std::string mqtt_tls_key_file{};
    std::vector<std::string> mqtt_topics{};
    int mqtt_subscribe_qos = 0;
    int heartbeat_interval = 60;
    std::string heartbeat_topic = "chime/heartbeat";
    std::string ring_topic = "doorbell/ring";
    std::string sound_path = "/usr/local/share/chime/ring.wav";
    std::string notification_success_sound_path = "/usr/local/share/chime/test.wav";
    std::string notification_failure_sound_path = "/usr/local/share/chime/ring.wav";
    int volume_bell = 80;
    int volume_notifications = 70;
    bool audio_enabled = true;
    std::string wifi_interface = "wlan0";
    int wifi_check_interval = 5;
};

inline const oc::config::Field<ChimeConfig> kChimeConfigFields[] = {
    {"mqtt_host", oc::config::parse_string<ChimeConfig, &ChimeConfig::mqtt_host>, true},
    {"mqtt_port", oc::config::parse_int<ChimeConfig, &ChimeConfig::mqtt_port, 1, 65535>, true},
    {"mqtt_client_id", oc::config::parse_string<ChimeConfig, &ChimeConfig::mqtt_client_id>, false},
    {"mqtt_username", oc::config::parse_string<ChimeConfig, &ChimeConfig::mqtt_username>, false},
    {"mqtt_password", oc::config::parse_string<ChimeConfig, &ChimeConfig::mqtt_password>, false},
    {"mqtt_tls_enabled", oc::config::parse_bool<ChimeConfig, &ChimeConfig::mqtt_tls_enabled>, false},
    {"mqtt_tls_validate_certificate", oc::config::parse_bool<ChimeConfig, &ChimeConfig::mqtt_tls_validate_certificate>, false},
    {"mqtt_tls_ca_file", oc::config::parse_string<ChimeConfig, &ChimeConfig::mqtt_tls_ca_file>, false},
    {"mqtt_tls_cert_file", oc::config::parse_string<ChimeConfig, &ChimeConfig::mqtt_tls_cert_file>, false},
    {"mqtt_tls_key_file", oc::config::parse_string<ChimeConfig, &ChimeConfig::mqtt_tls_key_file>, false},
    {"mqtt_topics", oc::config::parse_csv<ChimeConfig, &ChimeConfig::mqtt_topics>, true},
    {"mqtt_subscribe_qos", oc::config::parse_int<ChimeConfig, &ChimeConfig::mqtt_subscribe_qos, 0, 2>, false},
    {"heartbeat_interval", oc::config::parse_int<ChimeConfig, &ChimeConfig::heartbeat_interval, 0, 3600>, false},
    {"heartbeat_topic", oc::config::parse_string<ChimeConfig, &ChimeConfig::heartbeat_topic>, false},
    {"ring_topic", oc::config::parse_string<ChimeConfig, &ChimeConfig::ring_topic>, false},
    {"sound_path", oc::config::parse_string<ChimeConfig, &ChimeConfig::sound_path>, false},
    {"notification_success_sound_path", oc::config::parse_string<ChimeConfig, &ChimeConfig::notification_success_sound_path>, false},
    {"notification_failure_sound_path", oc::config::parse_string<ChimeConfig, &ChimeConfig::notification_failure_sound_path>, false},
    {"volume_bell", oc::config::parse_int<ChimeConfig, &ChimeConfig::volume_bell, 0, 100>, false},
    {"volume_notifications", oc::config::parse_int<ChimeConfig, &ChimeConfig::volume_notifications, 0, 100>, false},
    {"audio_enabled", oc::config::parse_bool<ChimeConfig, &ChimeConfig::audio_enabled>, false},
    {"wifi_interface", oc::config::parse_string<ChimeConfig, &ChimeConfig::wifi_interface>, false},
    {"wifi_check_interval", oc::config::parse_int<ChimeConfig, &ChimeConfig::wifi_check_interval, 0, 3600>, false},
};

struct FileConfig {
    int schema_version = 5;
    std::string mqtt_host{};
    int mqtt_port = 1883;
    std::string mqtt_client_id = "chime";
    std::string mqtt_username{};
    std::string mqtt_password{};
    bool mqtt_tls_enabled = false;
    bool mqtt_tls_validate_certificate = true;
    std::string mqtt_tls_ca_file{};
    std::string mqtt_tls_cert_file{};
    std::string mqtt_tls_key_file{};
    std::vector<std::string> mqtt_topics{};
    int mqtt_subscribe_qos = 0;
    int heartbeat_interval = 60;
    std::string heartbeat_topic = "chime/heartbeat";
    std::vector<std::string> ntp_servers{"time.cloudflare.com", "time.google.com", "pool.ntp.org"};
    std::vector<std::string> time_http_urls{"http://connectivitycheck.gstatic.com/generate_204", "http://detectportal.firefox.com/success.txt", "http://example.com/"};
    int time_sync_retries = 6;
    int time_sync_retry_delay = 5;
    int time_sync_interval = 3600;
    std::string ring_topic = "doorbell/ring";
    std::string sound_path = "/usr/local/share/chime/ring.wav";
    std::string notification_success_sound_path = "/usr/local/share/chime/test.wav";
    std::string notification_failure_sound_path = "/usr/local/share/chime/ring.wav";
    int volume_bell = 80;
    int volume_notifications = 70;
    bool audio_enabled = true;
    std::string wifi_interface = "wlan0";
    int wifi_check_interval = 5;
    int log_max_bytes = 262144;
    int log_rotate_keep = 5;
    int log_rotate_check_interval = 30;
};

inline const oc::config::Field<FileConfig> kFileConfigFields[] = {
    {"schema_version", oc::config::parse_int<FileConfig, &FileConfig::schema_version, 1, 1000000>, false},
    {"mqtt_host", oc::config::parse_string<FileConfig, &FileConfig::mqtt_host>, false},
    {"mqtt_port", oc::config::parse_int<FileConfig, &FileConfig::mqtt_port, 1, 65535>, false},
    {"mqtt_client_id", oc::config::parse_string<FileConfig, &FileConfig::mqtt_client_id>, false},
    {"mqtt_username", oc::config::parse_string<FileConfig, &FileConfig::mqtt_username>, false},
    {"mqtt_password", oc::config::parse_string<FileConfig, &FileConfig::mqtt_password>, false},
    {"mqtt_tls_enabled", oc::config::parse_bool<FileConfig, &FileConfig::mqtt_tls_enabled>, false},
    {"mqtt_tls_validate_certificate", oc::config::parse_bool<FileConfig, &FileConfig::mqtt_tls_validate_certificate>, false},
    {"mqtt_tls_ca_file", oc::config::parse_string<FileConfig, &FileConfig::mqtt_tls_ca_file>, false},
    {"mqtt_tls_cert_file", oc::config::parse_string<FileConfig, &FileConfig::mqtt_tls_cert_file>, false},
    {"mqtt_tls_key_file", oc::config::parse_string<FileConfig, &FileConfig::mqtt_tls_key_file>, false},
    {"mqtt_topics", oc::config::parse_csv<FileConfig, &FileConfig::mqtt_topics>, false},
    {"mqtt_subscribe_qos", oc::config::parse_int<FileConfig, &FileConfig::mqtt_subscribe_qos, 0, 2>, false},
    {"heartbeat_interval", oc::config::parse_int<FileConfig, &FileConfig::heartbeat_interval, 0, 3600>, false},
    {"heartbeat_topic", oc::config::parse_string<FileConfig, &FileConfig::heartbeat_topic>, false},
    {"ntp_servers", oc::config::parse_csv<FileConfig, &FileConfig::ntp_servers>, false},
    {"time_http_urls", oc::config::parse_csv<FileConfig, &FileConfig::time_http_urls>, false},
    {"time_sync_retries", oc::config::parse_int<FileConfig, &FileConfig::time_sync_retries, 1, 100>, false},
    {"time_sync_retry_delay", oc::config::parse_int<FileConfig, &FileConfig::time_sync_retry_delay, 1, 3600>, false},
    {"time_sync_interval", oc::config::parse_int<FileConfig, &FileConfig::time_sync_interval, 0, 86400>, false},
    {"ring_topic", oc::config::parse_string<FileConfig, &FileConfig::ring_topic>, false},
    {"sound_path", oc::config::parse_string<FileConfig, &FileConfig::sound_path>, false},
    {"notification_success_sound_path", oc::config::parse_string<FileConfig, &FileConfig::notification_success_sound_path>, false},
    {"notification_failure_sound_path", oc::config::parse_string<FileConfig, &FileConfig::notification_failure_sound_path>, false},
    {"volume_bell", oc::config::parse_int<FileConfig, &FileConfig::volume_bell, 0, 100>, false},
    {"volume_notifications", oc::config::parse_int<FileConfig, &FileConfig::volume_notifications, 0, 100>, false},
    {"audio_enabled", oc::config::parse_bool<FileConfig, &FileConfig::audio_enabled>, false},
    {"wifi_interface", oc::config::parse_string<FileConfig, &FileConfig::wifi_interface>, false},
    {"wifi_check_interval", oc::config::parse_int<FileConfig, &FileConfig::wifi_check_interval, 0, 3600>, false},
    {"log_max_bytes", oc::config::parse_int<FileConfig, &FileConfig::log_max_bytes, 1024, 104857600>, false},
    {"log_rotate_keep", oc::config::parse_int<FileConfig, &FileConfig::log_rotate_keep, 1, 100>, false},
    {"log_rotate_check_interval", oc::config::parse_int<FileConfig, &FileConfig::log_rotate_check_interval, 1, 3600>, false},
};

namespace webd {

struct CoreConfig {
    std::string mqtt_host{};
    int mqtt_port = 1883;
    std::string mqtt_client_id = "chime";
    std::string mqtt_username{};
    std::string mqtt_password{};
    bool mqtt_tls_enabled = false;
    bool mqtt_tls_validate_certificate = true;
    std::string mqtt_tls_ca_file{};
    std::string mqtt_tls_cert_file{};
    std::string mqtt_tls_key_file{};
    std::vector<std::string> mqtt_topics{};
    std::string ring_topic = "doorbell/ring";
    std::string notification_success_sound_path = "/usr/local/share/chime/test.wav";
    std::string notification_failure_sound_path = "/usr/local/share/chime/ring.wav";
    int volume_bell = 80;
    int volume_notifications = 70;
    std::string wifi_ssid{};
};

} // namespace webd

inline ChimeConfig RuntimeConfigFromFile(const FileConfig &file) {
    ChimeConfig out;
    out.mqtt_host = file.mqtt_host;
    out.mqtt_port = file.mqtt_port;
    out.mqtt_client_id = file.mqtt_client_id;
    out.mqtt_username = file.mqtt_username;
    out.mqtt_password = file.mqtt_password;
    out.mqtt_tls_enabled = file.mqtt_tls_enabled;
    out.mqtt_tls_validate_certificate = file.mqtt_tls_validate_certificate;
    out.mqtt_tls_ca_file = file.mqtt_tls_ca_file;
    out.mqtt_tls_cert_file = file.mqtt_tls_cert_file;
    out.mqtt_tls_key_file = file.mqtt_tls_key_file;
    out.mqtt_topics = file.mqtt_topics;
    out.mqtt_subscribe_qos = file.mqtt_subscribe_qos;
    out.heartbeat_interval = file.heartbeat_interval;
    out.heartbeat_topic = file.heartbeat_topic;
    out.ring_topic = file.ring_topic;
    out.sound_path = file.sound_path;
    out.notification_success_sound_path = file.notification_success_sound_path;
    out.notification_failure_sound_path = file.notification_failure_sound_path;
    out.volume_bell = file.volume_bell;
    out.volume_notifications = file.volume_notifications;
    out.audio_enabled = file.audio_enabled;
    out.wifi_interface = file.wifi_interface;
    out.wifi_check_interval = file.wifi_check_interval;
    return out;
}

inline webd::CoreConfig CoreConfigFromFile(const FileConfig &file) {
    webd::CoreConfig out;
    out.mqtt_host = file.mqtt_host;
    out.mqtt_port = file.mqtt_port;
    out.mqtt_client_id = file.mqtt_client_id;
    out.mqtt_username = file.mqtt_username;
    out.mqtt_password = file.mqtt_password;
    out.mqtt_tls_enabled = file.mqtt_tls_enabled;
    out.mqtt_tls_validate_certificate = file.mqtt_tls_validate_certificate;
    out.mqtt_tls_ca_file = file.mqtt_tls_ca_file;
    out.mqtt_tls_cert_file = file.mqtt_tls_cert_file;
    out.mqtt_tls_key_file = file.mqtt_tls_key_file;
    out.mqtt_topics = file.mqtt_topics;
    out.ring_topic = file.ring_topic;
    out.notification_success_sound_path = file.notification_success_sound_path;
    out.notification_failure_sound_path = file.notification_failure_sound_path;
    out.volume_bell = file.volume_bell;
    out.volume_notifications = file.volume_notifications;
    return out;
}

inline void ApplyCoreConfigToFile(const webd::CoreConfig &core, FileConfig *file) {
    if (file == nullptr) {
        return;
    }
    file->mqtt_host = core.mqtt_host;
    file->mqtt_port = core.mqtt_port;
    file->mqtt_client_id = core.mqtt_client_id;
    file->mqtt_username = core.mqtt_username;
    file->mqtt_password = core.mqtt_password;
    file->mqtt_tls_enabled = core.mqtt_tls_enabled;
    file->mqtt_tls_validate_certificate = core.mqtt_tls_validate_certificate;
    file->mqtt_tls_ca_file = core.mqtt_tls_ca_file;
    file->mqtt_tls_cert_file = core.mqtt_tls_cert_file;
    file->mqtt_tls_key_file = core.mqtt_tls_key_file;
    file->mqtt_topics = core.mqtt_topics;
    file->ring_topic = core.ring_topic;
    file->notification_success_sound_path = core.notification_success_sound_path;
    file->notification_failure_sound_path = core.notification_failure_sound_path;
    file->volume_bell = core.volume_bell;
    file->volume_notifications = core.volume_notifications;
}

inline std::map<std::string, std::string> CoreConfigFileReplacements(const webd::CoreConfig &config,
                                                                    const std::string &mqtt_password) {
    return {
        {"schema_version", std::to_string(kConfigSchemaVersion)},
        {"mqtt_host", config.mqtt_host},
        {"mqtt_port", std::to_string(config.mqtt_port)},
        {"mqtt_client_id", config.mqtt_client_id},
        {"mqtt_username", config.mqtt_username},
        {"mqtt_password", mqtt_password},
        {"mqtt_tls_enabled", oc::config::bool_to_text(config.mqtt_tls_enabled)},
        {"mqtt_tls_validate_certificate", oc::config::bool_to_text(config.mqtt_tls_validate_certificate)},
        {"mqtt_tls_ca_file", config.mqtt_tls_ca_file},
        {"mqtt_tls_cert_file", config.mqtt_tls_cert_file},
        {"mqtt_tls_key_file", config.mqtt_tls_key_file},
        {"mqtt_topics", oc::config::join_csv(config.mqtt_topics)},
        {"ring_topic", config.ring_topic},
        {"notification_success_sound_path", config.notification_success_sound_path},
        {"notification_failure_sound_path", config.notification_failure_sound_path},
        {"volume_bell", std::to_string(config.volume_bell)},
        {"volume_notifications", std::to_string(config.volume_notifications)},
    };
}

} // namespace chime

#endif
