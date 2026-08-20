#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <sys/stat.h>
#if defined(__linux__)
#include <sys/mount.h>
#endif

#include "chime/chime_config.h"
#include "chime/config_migrate.h"
#include "chime/generated/config_types.h"
#include "doctest.h"
#include "oc/config/kv_document.h"
#include "test_support.h"

namespace {

constexpr const char *kShippedV4Config = R"(# Chime configuration
# Apply changes by restarting the service:
#   /etc/init.d/S99chime restart

# MQTT broker settings
mqtt_host=
mqtt_port=1883
mqtt_client_id=chime
mqtt_username=
mqtt_password=
mqtt_tls_enabled=false
mqtt_tls_validate_certificate=true
mqtt_tls_ca_file=
mqtt_tls_cert_file=
mqtt_tls_key_file=
mqtt_topics=doorbell/ring,doorbell/status
mqtt_subscribe_qos=0
heartbeat_interval=20
heartbeat_topic=chime/heartbeat
ntp_servers=time.cloudflare.com,time.google.com,pool.ntp.org
time_http_urls=http://connectivitycheck.gstatic.com/generate_204,http://detectportal.firefox.com/success.txt,http://example.com/
time_sync_retries=6
time_sync_retry_delay=5
time_sync_interval=3600
ring_topic=doorbell/ring
sound_path=/usr/local/share/chime/ring.wav
notification_success_sound_path=/usr/local/share/chime/test.wav
notification_failure_sound_path=/usr/local/share/chime/ring.wav
volume_bell=80
volume_notifications=70
volume_other=70
audio_enabled=true
wifi_interface=wlan0
wifi_check_interval=5
log_max_bytes=262144
log_rotate_keep=5
log_rotate_check_interval=30
)";

std::string ReadText(const std::filesystem::path &path) {
    std::ifstream file(path);
    REQUIRE(file.is_open());
    return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

} // namespace

TEST_SUITE("config_migrate") {
    TEST_CASE("migrates shipped v4 files to the current schema") {
        const ScopedTempDir tmp;
        const std::string original = kShippedV4Config;
        REQUIRE(original.find("volume_other=") != std::string::npos);
        REQUIRE(original.find("schema_version=") == std::string::npos);
        const auto path = tmp.WriteFile("chime.conf", original);

        const auto first = chime::MigratePersistedConfig(path.string());
        REQUIRE(first.success);
        CHECK(first.rewritten);
        CHECK(first.from_version == chime::kLegacyUnversionedSchema);
        CHECK(first.to_version == chime::kConfigSchemaVersion);

        const std::string migrated = ReadText(path);
        CHECK(migrated.find("volume_other=") == std::string::npos);
        CHECK(migrated.find("schema_version=" + std::to_string(chime::kConfigSchemaVersion)) != std::string::npos);
        CHECK(migrated.find("mqtt_password=") != std::string::npos);
        CHECK(migrated.find("# MQTT broker settings") != std::string::npos);
        CHECK(std::filesystem::is_regular_file(path.string() + ".bak"));
        CHECK(ReadText(path.string() + ".bak") == original);

        const auto second = chime::MigratePersistedConfig(path.string());
        REQUIRE(second.success);
        CHECK_FALSE(second.rewritten);
        CHECK(ReadText(path) == migrated);
    }

    TEST_CASE("preserves unknown keys and drops removed keys") {
        const ScopedTempDir tmp;
        const auto path = tmp.WriteFile(
            "chime.conf", "mqtt_host=broker\nmqtt_port=1883\nmqtt_topics=a\nvolume_other=12\nlab_flag=keep-me\n");
        const auto result = chime::MigratePersistedConfig(path.string());
        REQUIRE(result.success);
        const std::string migrated = ReadText(path);
        CHECK(migrated.find("lab_flag=keep-me") != std::string::npos);
        CHECK(migrated.find("volume_other=") == std::string::npos);
        CHECK(migrated.find("ntp_servers=") != std::string::npos);
    }

    TEST_CASE("malformed schema_version leaves the original file") {
        const ScopedTempDir tmp;
        const std::string original = "schema_version=not-a-number\nmqtt_host=broker\n";
        const auto path = tmp.WriteFile("chime.conf", original);
        const auto result = chime::MigratePersistedConfig(path.string());
        CHECK_FALSE(result.success);
        CHECK(result.malformed_version);
        CHECK(chime::MigrateFailureBlocksStartup(result));
        CHECK(ReadText(path) == original);
        CHECK_FALSE(std::filesystem::exists(path.string() + ".bak"));
    }

    TEST_CASE("newer schema versions are rejected without rewrite") {
        const ScopedTempDir tmp;
        const std::string original = "schema_version=9999\nmqtt_host=broker\n";
        const auto path = tmp.WriteFile("chime.conf", original);
        const auto result = chime::MigratePersistedConfig(path.string());
        CHECK_FALSE(result.success);
        CHECK(result.unsupported_version);
        CHECK(chime::MigrateFailureBlocksStartup(result));
        CHECK(ReadText(path) == original);
    }

    TEST_CASE("write failure leaves the original config") {
        const ScopedTempDir tmp;
        const std::string original = "mqtt_host=broker\nmqtt_port=1883\nmqtt_topics=a\nvolume_other=1\n";
        const auto path = tmp.WriteFile("chime.conf", original);
        REQUIRE(chmod(tmp.path().c_str(), 0555) == 0);
        const auto result = chime::MigratePersistedConfig(path.string());
        REQUIRE(chmod(tmp.path().c_str(), 0755) == 0);
        CHECK_FALSE(result.success);
        CHECK_FALSE(result.rewritten);
        CHECK_FALSE(chime::MigrateFailureBlocksStartup(result));
        CHECK(ReadText(path) == original);
    }

    TEST_CASE("invalid init-only integers are replaced with schema defaults") {
        const ScopedTempDir tmp;
        const auto path = tmp.WriteFile(
            "chime.conf", "mqtt_host=\nmqtt_port=1883\nmqtt_topics=a\ntime_sync_retries=0\nlog_max_bytes=12\n");
        REQUIRE(chime::MigratePersistedConfig(path.string()).success);
        const auto document = oc::config::ParseKvDocument(ReadText(path));
        CHECK(oc::config::KvDocumentValue(document, "time_sync_retries") == "6");
        CHECK(oc::config::KvDocumentValue(document, "log_max_bytes") == "262144");
    }

    TEST_CASE("missing or empty mqtt_topics are repaired to the shipped list") {
        const ScopedTempDir tmp;
        const auto missing = tmp.WriteFile("missing.conf", "mqtt_host=broker\nmqtt_port=1883\n");
        REQUIRE(chime::MigratePersistedConfig(missing.string()).success);
        const auto missing_doc = oc::config::ParseKvDocument(ReadText(missing));
        CHECK(oc::config::KvDocumentValue(missing_doc, "mqtt_topics") == "doorbell/ring,doorbell/status");
        const auto loaded_missing = chime::LoadConfig(missing.string());
        REQUIRE(loaded_missing);
        REQUIRE(loaded_missing.config.mqtt_topics.size() == 2);
        CHECK(loaded_missing.config.mqtt_topics[0] == "doorbell/ring");
        CHECK(loaded_missing.config.mqtt_topics[1] == "doorbell/status");

        const auto empty = tmp.WriteFile("empty.conf", "mqtt_host=broker\nmqtt_port=1883\nmqtt_topics=\n");
        REQUIRE(chime::MigratePersistedConfig(empty.string()).success);
        const auto empty_doc = oc::config::ParseKvDocument(ReadText(empty));
        CHECK(oc::config::KvDocumentValue(empty_doc, "mqtt_topics") == "doorbell/ring,doorbell/status");
        const auto loaded_empty = chime::LoadConfig(empty.string());
        REQUIRE(loaded_empty);
        REQUIRE(loaded_empty.config.mqtt_topics.size() == 2);
    }

#if defined(__linux__)
    TEST_CASE("migrates a bind-mounted file the way S31persistent installs it") {
        const ScopedTempDir tmp;
        const auto data_dir = tmp.path() / "data";
        const auto etc_dir = tmp.path() / "etc";
        std::filesystem::create_directories(data_dir);
        std::filesystem::create_directories(etc_dir);
        const auto backing = tmp.WriteFile("data/chime.conf", std::string(kShippedV4Config));
        const auto mounted = etc_dir / "chime.conf";
        {
            std::ofstream placeholder(mounted);
            placeholder << "placeholder\n";
        }
        if (mount(backing.c_str(), mounted.c_str(), nullptr, MS_BIND, nullptr) != 0) {
            SKIP("bind mount not permitted");
        }
        struct Unmount {
            std::string path;
            ~Unmount() { umount(path.c_str()); }
        } unmount{mounted.string()};

        const auto result = chime::MigratePersistedConfig(mounted.string());
        REQUIRE(result.success);
        CHECK(result.rewritten);
        const std::string persisted = ReadText(backing);
        const std::string visible = ReadText(mounted);
        CHECK(persisted == visible);
        CHECK(persisted.find("volume_other=") == std::string::npos);
        CHECK(persisted.find("schema_version=" + std::to_string(chime::kConfigSchemaVersion)) != std::string::npos);
        CHECK(std::filesystem::is_regular_file(mounted.string() + ".bak"));
        CHECK(ReadText(mounted.string() + ".bak") == std::string(kShippedV4Config));
    }
#endif
}
