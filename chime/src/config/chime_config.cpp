#include "chime/chime_config.h"

namespace chime {

oc::config::LoadResult<ChimeConfig> LoadConfig(const std::string &path) {
    return oc::config::load(path, ChimeConfig{}, kChimeConfigFields);
}

} // namespace chime
