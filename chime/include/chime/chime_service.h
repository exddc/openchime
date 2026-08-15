#ifndef CHIME_CHIME_SERVICE_H
#define CHIME_CHIME_SERVICE_H

#include <atomic>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

#include "chime/audio_player.h"
#include "chime/chime_config.h"
#include "chime/wifi_monitor.h"
#include "oc/mqtt/client.h"

namespace oc::logging {
class Logger;
}

namespace oc::runtime {
class SignalHandler;
}

namespace chime {

class ChimeService final : public oc::mqtt::EventHandler {
  public:
    ChimeService(const ChimeConfig &config, oc::logging::Logger &logger, AudioPlayer &audio_player,
                 const WifiMonitor &wifi_monitor,
                 std::string observed_topics_path = "/var/lib/chime/observed_topics.txt");

    int Run(oc::runtime::SignalHandler &signal_handler);

    void OnConnect(int rc) override;
    void OnDisconnect(int rc) override;
    void OnMessage(const oc::mqtt::Message &message) override;

  private:
    enum class NotificationSoundType { kSuccess, kFailure };

    void LogWifiState(const WifiState &state) const;
    void LogHealth(bool clock_sane);
    bool RingTopicMatches(const std::string &message_topic) const;
    bool WifiStateIsConnected(const std::optional<WifiState> &state) const;
    void PlayNotification(NotificationSoundType type);
    void RecordObservedTopic(const std::string &topic);
    void LoadObservedTopics();
    bool PersistObservedTopics(std::string *error) const;

    const ChimeConfig &config_;
    oc::logging::Logger &logger_;
    oc::mqtt::Client mqtt_client_;
    AudioPlayer &audio_player_;
    const WifiMonitor &wifi_monitor_;
    std::string observed_topics_path_;

    std::atomic<bool> mqtt_connected_{false};
    std::atomic<unsigned long long> messages_received_{0};
    std::atomic<unsigned long long> ring_messages_received_{0};
    std::atomic<unsigned long long> loop_errors_{0};
    std::atomic<unsigned long long> reconnect_attempts_{0};
    std::atomic<unsigned long long> heartbeats_sent_{0};

    bool clock_was_unsynced_ = false;
    std::vector<std::string> observed_topics_;
    std::unordered_set<std::string> observed_topics_set_;
    bool observed_topics_loaded_ = false;
};

} // namespace chime

#endif
