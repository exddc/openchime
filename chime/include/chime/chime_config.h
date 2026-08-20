#ifndef CHIME_CHIME_CONFIG_H
#define CHIME_CHIME_CONFIG_H

#include <string>

#include "chime/generated/config_types.h"
#include "oc/config/kv_config.h"

namespace chime {

oc::config::LoadResult<ChimeConfig> LoadConfig(const std::string &path);

inline bool MqttBrokerConfigured(const ChimeConfig &config) {
    return !config.mqtt_host.empty();
}

} // namespace chime

#endif
