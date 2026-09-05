#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include "chime/audio_player.h"
#include "doctest.h"
#include "fake_process_runner.h"
#include "oc/logging/logger.h"
#include "test_support.h"

namespace {

class RecordingLogger final : public oc::logging::Logger {
  public:
    struct Entry {
        oc::logging::Level level = oc::logging::Level::kInfo;
        std::string component;
        std::string message;
    };

    void Log(oc::logging::Level level, std::string_view component, std::string_view message) override {
        const std::lock_guard<std::mutex> lock(mutex_);
        entries_.push_back(Entry{level, std::string(component), std::string(message)});
    }

    std::vector<Entry> Entries() const {
        const std::lock_guard<std::mutex> lock(mutex_);
        return entries_;
    }

  private:
    mutable std::mutex mutex_;
    std::vector<Entry> entries_;
};

bool WaitIdle(const chime::AplayAudioPlayer &player) {
    for (int i = 0; i < 100; ++i) {
        if (!player.IsPlaying()) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return !player.IsPlaying();
}

bool HasMessage(const std::vector<RecordingLogger::Entry> &entries, oc::logging::Level level,
                const std::string &needle) {
    for (const auto &entry : entries) {
        if (entry.level == level && entry.message.find(needle) != std::string::npos) {
            return true;
        }
    }
    return false;
}

} // namespace

TEST_SUITE("aplay_audio_player") {
    TEST_CASE("playback requests are serialized and cancellation reaches aplay") {
        const ScopedTempDir tmp;
        const auto wav = tmp.WriteFile("ring ; literal.wav", "RIFF");
        RecordingLogger logger;
        oc::process::FakeRunner processes;
        std::atomic<bool> entered{false};
        processes.SetHandler([&](const oc::process::Request &request) {
            if (request.command.executable == "aplay") {
                entered.store(true);
                while (!request.stop.stop_requested()) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
            }
            return oc::process::Exited(0);
        });
        {
            chime::AplayAudioPlayer player(logger, processes, true);
            player.Play(wav.string(), 150);
            for (int i = 0; i < 100 && !entered.load(); ++i) {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
            REQUIRE(entered.load());
            player.Play(wav.string(), 20);
            CHECK(processes.Calls().size() == 2);
        }
        const auto calls = processes.Calls();
        REQUIRE(calls.size() == 2);
        CHECK(calls[0].command.arguments.back() == "100%");
        CHECK(calls[0].timeout == std::chrono::seconds(3));
        CHECK(calls[1].command.arguments.back() == wav.string());
        CHECK(calls[1].timeout == std::chrono::seconds(30));
        CHECK(HasMessage(logger.Entries(), oc::logging::Level::kWarn, "already playing"));
        CHECK(HasMessage(logger.Entries(), oc::logging::Level::kInfo, "playback cancelled"));
    }

    TEST_CASE("mixer timeouts retain software volume scaling and remove the temporary wav") {
        const ScopedTempDir tmp;
        std::string wav("RIFF", 4);
        wav.append("\x26\0\0\0WAVEfmt ", 12);
        wav.append("\x10\0\0\0\x01\0\x01\0\x40\x1f\0\0\x80\x3e\0\0\x02\0\x10\0", 20);
        wav.append("data\x02\0\0\0\xe8\x03", 10);
        const auto path = tmp.WriteFile("ring.wav", wav);
        RecordingLogger logger;
        oc::process::FakeRunner processes;
        std::string scaled_path;
        std::string scaled;
        processes.SetHandler([&](const oc::process::Request &request) {
            if (request.command.executable == "amixer") {
                return oc::process::TimedOut();
            }
            scaled_path = request.command.arguments.back();
            std::ifstream file(scaled_path, std::ios::binary);
            scaled.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
            return oc::process::Exited(0);
        });
        chime::AplayAudioPlayer player(logger, processes, true);
        player.Play(path.string(), 50);
        REQUIRE(WaitIdle(player));
        REQUIRE(scaled.size() == 46);
        CHECK(static_cast<unsigned char>(scaled[44]) == 0xf4);
        CHECK(static_cast<unsigned char>(scaled[45]) == 0x01);
        CHECK(scaled_path != path.string());
        CHECK_FALSE(std::filesystem::exists(scaled_path));
        CHECK(processes.Calls().size() == 10);
    }

    TEST_CASE("amixer and aplay are invoked with argv arrays") {
        const ScopedTempDir tmp;
        const auto wav = tmp.WriteFile("ring.wav", "RIFF");
        RecordingLogger logger;
        oc::process::FakeRunner processes;
        chime::AplayAudioPlayer player(logger, processes, true);

        player.Play(wav.string(), 80);
        REQUIRE(WaitIdle(player));

        const auto calls = processes.Calls();
        REQUIRE(calls.size() >= 2);
        CHECK(calls[0].command.executable == "amixer");
        REQUIRE(calls[0].command.arguments.size() == 4);
        CHECK(calls[0].command.arguments[0] == "-q");
        CHECK(calls[0].command.arguments[1] == "sset");
        CHECK(calls[0].command.arguments[2] == "PCM");
        CHECK(calls[0].command.arguments[3] == "80%");

        CHECK(calls.back().command.executable == "aplay");
        REQUIRE(calls.back().command.arguments.size() == 2);
        CHECK(calls.back().command.arguments[0] == "-q");
        CHECK(calls.back().command.arguments[1] == wav.string());
        CHECK(HasMessage(logger.Entries(), oc::logging::Level::kInfo, "playback complete"));
    }

    TEST_CASE("aplay failure is logged and clears the playing flag") {
        const ScopedTempDir tmp;
        const auto wav = tmp.WriteFile("ring.wav", "RIFF");
        RecordingLogger logger;
        oc::process::FakeRunner processes;
        processes.SetHandler([](const oc::process::Request &request) {
            if (request.command.executable == "aplay") {
                return oc::process::Exited(1);
            }
            return oc::process::Exited(0);
        });
        chime::AplayAudioPlayer player(logger, processes, true);

        player.Play(wav.string(), 40);
        REQUIRE(WaitIdle(player));
        CHECK_FALSE(player.IsPlaying());
        CHECK(HasMessage(logger.Entries(), oc::logging::Level::kError, "aplay failed: exit code 1"));
    }

    TEST_CASE("aplay timeout is logged and clears the playing flag") {
        const ScopedTempDir tmp;
        const auto wav = tmp.WriteFile("ring.wav", "RIFF");
        RecordingLogger logger;
        oc::process::FakeRunner processes;
        processes.SetHandler([](const oc::process::Request &request) {
            if (request.command.executable == "aplay") {
                return oc::process::TimedOut();
            }
            return oc::process::Exited(0);
        });
        chime::AplayAudioPlayer player(logger, processes, true);

        player.Play(wav.string(), 100);
        REQUIRE(WaitIdle(player));
        CHECK_FALSE(player.IsPlaying());
        CHECK(HasMessage(logger.Entries(), oc::logging::Level::kError, "aplay failed: timed out"));
    }

    TEST_CASE("destroying the player cancels in-flight playback") {
        const ScopedTempDir tmp;
        const auto wav = tmp.WriteFile("ring.wav", "RIFF");
        RecordingLogger logger;
        oc::process::FakeRunner processes;
        std::atomic<bool> entered{false};
        std::atomic<bool> saw_stop{false};
        processes.SetHandler([&](const oc::process::Request &request) {
            entered.store(true);
            while (!request.stop.stop_requested()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
            saw_stop.store(true);
            oc::process::Result result;
            result.outcome = oc::process::Outcome::Cancelled;
            result.error = "cancelled";
            return result;
        });

        {
            chime::AplayAudioPlayer player(logger, processes, true);
            player.Play(wav.string(), 80);
            for (int i = 0; i < 50 && !entered.load(); ++i) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
            REQUIRE(entered.load());
        }
        CHECK(saw_stop.load());
    }
}
