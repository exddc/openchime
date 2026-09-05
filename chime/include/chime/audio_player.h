#ifndef CHIME_AUDIO_PLAYER_H
#define CHIME_AUDIO_PLAYER_H

#include <atomic>
#include <mutex>
#include <string>
#include <thread>

namespace oc::logging {
class Logger;
}

namespace oc::process {
class Runner;
}

namespace chime {

class AudioPlayer {
  public:
    virtual ~AudioPlayer() = default;
    virtual void Play(const std::string &path, int volume_percent = 100) = 0;
    virtual bool IsPlaying() const = 0;
};

class AplayAudioPlayer final : public AudioPlayer {
  public:
    AplayAudioPlayer(oc::logging::Logger &logger, oc::process::Runner &runner, bool execute_commands);
    ~AplayAudioPlayer() override;

    AplayAudioPlayer(const AplayAudioPlayer &) = delete;
    AplayAudioPlayer &operator=(const AplayAudioPlayer &) = delete;
    AplayAudioPlayer(AplayAudioPlayer &&) = delete;
    AplayAudioPlayer &operator=(AplayAudioPlayer &&) = delete;

    void Play(const std::string &path, int volume_percent = 100) override;
    bool IsPlaying() const override;

  private:
    oc::logging::Logger &logger_;
    oc::process::Runner &runner_;
    bool execute_commands_;
    std::atomic<bool> playing_{false};
    std::mutex playback_thread_mutex_;
    std::jthread playback_thread_;
};

} // namespace chime

#endif
