#ifndef CHIME_CONFIG_MIGRATE_H
#define CHIME_CONFIG_MIGRATE_H

#include <string>

namespace chime {

struct MigrateResult {
    bool success = false;
    bool rewritten = false;
    bool unsupported_version = false;
    bool malformed_version = false;
    int from_version = 0;
    int to_version = 0;
    std::string error;
};

MigrateResult MigratePersistedConfig(const std::string &path);

inline bool MigrateFailureBlocksStartup(const MigrateResult &result) {
    return result.malformed_version || result.unsupported_version;
}

} // namespace chime

#endif
