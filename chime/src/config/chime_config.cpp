#include "chime/chime_config.h"

namespace chime {
namespace {
constexpr oc::config::Field<ChimeConfig> kConfigFields[] = {
    {"mqtt_host", oc::config::parse_string<ChimeConfig, &ChimeConfig::host>, true},
    {"mqtt_port", oc::config::parse_int<ChimeConfig, &ChimeConfig::port>, true},
    {"mqtt_client_id", oc::config::parse_string<ChimeConfig, &ChimeConfig::client_id>, false},
    {"mqtt_username", oc::config::parse_string<ChimeConfig, &ChimeConfig::mqtt_username>, false},
    {"mqtt_password", oc::config::parse_string<ChimeConfig, &ChimeConfig::mqtt_password>, false},
    {"mqtt_tls_enabled", oc::config::parse_bool<ChimeConfig, &ChimeConfig::mqtt_tls_enabled>, false},
    {"mqtt_tls_validate_certificate", oc::config::parse_bool<ChimeConfig, &ChimeConfig::mqtt_tls_validate_certificate>,
     false},
    {"mqtt_tls_ca_file", oc::config::parse_string<ChimeConfig, &ChimeConfig::mqtt_tls_ca_file>, false},
    {"mqtt_tls_cert_file", oc::config::parse_string<ChimeConfig, &ChimeConfig::mqtt_tls_cert_file>, false},
    {"mqtt_tls_key_file", oc::config::parse_string<ChimeConfig, &ChimeConfig::mqtt_tls_key_file>, false},
    {"mqtt_topics", oc::config::parse_csv<ChimeConfig, &ChimeConfig::topics>, true},
    {"mqtt_subscribe_qos", oc::config::parse_int<ChimeConfig, &ChimeConfig::mqtt_subscribe_qos, 0, 2>, false},
    {"heartbeat_interval", oc::config::parse_int<ChimeConfig, &ChimeConfig::heartbeat_interval, 0, 3600>, false},
    {"heartbeat_topic", oc::config::parse_string<ChimeConfig, &ChimeConfig::heartbeat_topic>, false},
    {"ring_topic", oc::config::parse_string<ChimeConfig, &ChimeConfig::ring_topic>, false},
    {"sound_path", oc::config::parse_string<ChimeConfig, &ChimeConfig::sound_path>, false},
    {"notification_success_sound_path",
     oc::config::parse_string<ChimeConfig, &ChimeConfig::notification_success_sound_path>, false},
    {"notification_failure_sound_path",
     oc::config::parse_string<ChimeConfig, &ChimeConfig::notification_failure_sound_path>, false},
    {"volume_bell", oc::config::parse_int<ChimeConfig, &ChimeConfig::volume_bell, 0, 100>, false},
    {"volume_notifications", oc::config::parse_int<ChimeConfig, &ChimeConfig::volume_notifications, 0, 100>, false},
    {"volume_other", oc::config::parse_int<ChimeConfig, &ChimeConfig::volume_other, 0, 100>, false},
    {"audio_enabled", oc::config::parse_bool<ChimeConfig, &ChimeConfig::audio_enabled>, false},
    {"wifi_interface", oc::config::parse_string<ChimeConfig, &ChimeConfig::wifi_interface>, false},
    {"wifi_check_interval", oc::config::parse_int<ChimeConfig, &ChimeConfig::wifi_check_interval, 0, 3600>, false},
};
} // namespace

oc::config::LoadResult<ChimeConfig> LoadConfig(const std::string &path) {
    return oc::config::load(path, ChimeConfig{}, kConfigFields);
}

} // namespace chime
