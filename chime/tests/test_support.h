#ifndef OC_TEST_SUPPORT_H
#define OC_TEST_SUPPORT_H

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>
#include <unistd.h>
#include <vector>

#include "chime/audio_player.h"
#include "chime/wifi_monitor.h"
#include "doctest.h"
#include "oc/logging/logger.h"

class ScopedTempDir {
  public:
    ScopedTempDir() {
        const auto parent = std::filesystem::temp_directory_path() / "openchime-tests";
        std::filesystem::create_directories(parent);
        std::string tmpl = (parent / "XXXXXX").string();
        std::vector<char> buf(tmpl.begin(), tmpl.end());
        buf.push_back('\0');
        REQUIRE(mkdtemp(buf.data()) != nullptr);
        path_ = buf.data();
    }

    ~ScopedTempDir() {
        std::error_code ec;
        std::filesystem::remove_all(path_, ec);
    }

    ScopedTempDir(const ScopedTempDir &) = delete;
    ScopedTempDir &operator=(const ScopedTempDir &) = delete;

    const std::filesystem::path &path() const { return path_; }

    std::filesystem::path WriteFile(const std::string &name, std::string_view contents) const {
        const auto file_path = path_ / name;
        std::ofstream out(file_path);
        REQUIRE(out.is_open());
        out << contents;
        REQUIRE(out.good());
        return file_path;
    }

  private:
    std::filesystem::path path_;
};

class NullLogger final : public oc::logging::Logger {
  public:
    void Log(oc::logging::Level, std::string_view, std::string_view) override {}
};

class RecordingAudioPlayer final : public chime::AudioPlayer {
  public:
    struct PlayCall {
        std::string path;
        int volume_percent = 0;
    };

    void Play(const std::string &path, int volume_percent = 100) override { calls_.push_back({path, volume_percent}); }

    bool IsPlaying() const override { return false; }

    const std::vector<PlayCall> &calls() const { return calls_; }

  private:
    std::vector<PlayCall> calls_;
};

class NullWifiMonitor final : public chime::WifiMonitor {
  public:
    std::optional<chime::WifiState> ReadState(const std::string &) const override { return std::nullopt; }
};

#endif
