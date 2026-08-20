#include "chime/config_migrate.h"

#include <fstream>
#include <filesystem>
#include <iterator>
#include <string>
#include <sys/stat.h>
#include <utility>

#include "chime/generated/config_types.h"
#include "oc/config/kv_config.h"
#include "oc/config/kv_document.h"
#include "oc/util/filesystem.h"

namespace chime {
namespace {

constexpr mode_t kChimeConfigMode = 0600;

bool ReadFile(const std::string &path, std::string *content, std::string *error) {
    if (content == nullptr || error == nullptr) {
        return false;
    }
    std::ifstream file(path);
    if (!file.is_open()) {
        *error = "failed to open config '" + path + "'";
        return false;
    }
    std::string text((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    if (file.bad()) {
        *error = "failed to read config '" + path + "'";
        return false;
    }
    *content = std::move(text);
    return true;
}

bool ParseSchemaVersion(const std::vector<oc::config::KvEntry> &document, int *version, std::string *error) {
    if (version == nullptr || error == nullptr) {
        return false;
    }
    if (!oc::config::KvDocumentHasKey(document, "schema_version")) {
        *version = kLegacyUnversionedSchema;
        return true;
    }
    const std::string raw = oc::config::KvDocumentValue(document, "schema_version");
    if (!oc::config::parse_int_value(raw, 1, 1000000, version)) {
        *error = "malformed schema_version";
        return false;
    }
    return true;
}

bool FieldValueValid(const ConfigFieldSpec &spec, std::string_view value) {
    if (spec.type == ConfigValueType::kInt) {
        int parsed = 0;
        return oc::config::parse_int_value(value, spec.min_value, spec.max_value, &parsed);
    }
    if (spec.type == ConfigValueType::kBool) {
        bool parsed = false;
        return oc::config::parse_bool_value(value, &parsed);
    }
    if (spec.type == ConfigValueType::kCsv) {
        if (spec.file_required) {
            return !oc::config::split_csv(value).empty();
        }
        return true;
    }
    return true;
}

void ApplyCurrentSchema(std::vector<oc::config::KvEntry> *document) {
    for (const char *key : kRemovedConfigKeys) {
        oc::config::KvDocumentRemoveKey(*document, key);
    }
    for (const auto &spec : kAllConfigFields) {
        if (spec.persist != ConfigPersist::kFile) {
            continue;
        }
        if (!oc::config::KvDocumentHasKey(*document, spec.key)) {
            oc::config::KvDocumentSetValue(*document, spec.key, spec.repair_text);
            continue;
        }
        const std::string current = oc::config::KvDocumentValue(*document, spec.key);
        if (!FieldValueValid(spec, current)) {
            oc::config::KvDocumentSetValue(*document, spec.key, spec.repair_text);
        }
    }
    oc::config::KvDocumentSetValue(*document, "schema_version", std::to_string(kConfigSchemaVersion));
}

std::string PersistentBackupPath(const std::string &path) {
    const std::filesystem::path twin = std::filesystem::path("/data/etc") / std::filesystem::path(path).filename();
    struct stat target_stat;
    struct stat twin_stat;
    if (stat(path.c_str(), &target_stat) == 0 && stat(twin.c_str(), &twin_stat) == 0 &&
        target_stat.st_dev == twin_stat.st_dev && target_stat.st_ino == twin_stat.st_ino) {
        return twin.string() + ".bak";
    }
    return path + ".bak";
}

} // namespace

MigrateResult MigratePersistedConfig(const std::string &path) {
    MigrateResult result;
    result.to_version = kConfigSchemaVersion;

    std::string original;
    if (!ReadFile(path, &original, &result.error)) {
        return result;
    }

    auto document = oc::config::ParseKvDocument(original);
    if (!ParseSchemaVersion(document, &result.from_version, &result.error)) {
        result.malformed_version = true;
        return result;
    }
    if (result.from_version > kConfigSchemaVersion) {
        result.unsupported_version = true;
        result.error = "config schema_version is newer than this software";
        return result;
    }

    ApplyCurrentSchema(&document);
    const std::string migrated = oc::config::RenderKvDocument(document);
    if (migrated == original) {
        result.success = true;
        result.rewritten = false;
        result.from_version = kConfigSchemaVersion;
        return result;
    }

    const std::string backup_path = PersistentBackupPath(path);
    if (!oc::util::AtomicWriteFile(backup_path, original, kChimeConfigMode, &result.error)) {
        return result;
    }
    if (!oc::util::AtomicWriteFile(path, migrated, kChimeConfigMode, &result.error)) {
        return result;
    }

    result.success = true;
    result.rewritten = true;
    result.to_version = kConfigSchemaVersion;
    return result;
}

} // namespace chime
