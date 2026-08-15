#include "chime/chime_config.h"
#include "chime/chime_service.h"
#include "doctest.h"
#include "oc/mqtt/client.h"
#include "test_support.h"

TEST_SUITE("chime_service_ring") {
    TEST_CASE("matching ring topic invokes the audio seam") {
        const ScopedTempDir tmp;
        chime::ChimeConfig config;
        config.audio_enabled = true;
        config.ring_topic = "doorbell/+/ring";
        config.sound_path = "/usr/local/share/chime/ring.wav";
        config.volume_bell = 80;

        NullLogger logger;
        RecordingAudioPlayer audio;
        NullWifiMonitor wifi;
        chime::ChimeService service(config, logger, audio, wifi, (tmp.path() / "observed_topics.txt").string());

        oc::mqtt::Message matching;
        matching.topic = "doorbell/2OG/ring";
        matching.payload = "ding";
        service.OnMessage(matching);

        REQUIRE(audio.calls().size() == 1);
        CHECK(audio.calls()[0].path == config.sound_path);
        CHECK(audio.calls()[0].volume_percent == 80);

        oc::mqtt::Message non_matching;
        non_matching.topic = "doorbell/status";
        service.OnMessage(non_matching);
        CHECK(audio.calls().size() == 1);
    }

    TEST_CASE("disabled audio does not play on a matching topic") {
        const ScopedTempDir tmp;
        chime::ChimeConfig config;
        config.audio_enabled = false;
        config.ring_topic = "doorbell/ring";

        NullLogger logger;
        RecordingAudioPlayer audio;
        NullWifiMonitor wifi;
        chime::ChimeService service(config, logger, audio, wifi, (tmp.path() / "observed_topics.txt").string());

        oc::mqtt::Message matching;
        matching.topic = "doorbell/ring";
        service.OnMessage(matching);
        CHECK(audio.calls().empty());
    }
}
