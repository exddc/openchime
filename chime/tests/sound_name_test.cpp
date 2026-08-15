#include <string>

#include "chime/webd_sound_name.h"
#include "doctest.h"

TEST_SUITE("sound_name") {
    TEST_CASE("accepts ring-*.wav names") {
        CHECK(chime::webd::IsSafeSoundName("ring-default.wav"));
        CHECK(chime::webd::IsSafeSoundName("ring-foo_bar.wav"));
        CHECK(chime::webd::IsSafeSoundName("ring-1.wav"));
        CHECK(chime::webd::IsSafeSoundName("RING-Default.WAV"));
    }

    TEST_CASE("rejects empty, oversized, path, traversal, and type-unsafe names") {
        CHECK_FALSE(chime::webd::IsSafeSoundName(""));
        CHECK_FALSE(chime::webd::IsSafeSoundName(std::string(129, 'a')));
        CHECK_FALSE(chime::webd::IsSafeSoundName("/tmp/ring-x.wav"));
        CHECK_FALSE(chime::webd::IsSafeSoundName("ring-foo/bar.wav"));
        CHECK_FALSE(chime::webd::IsSafeSoundName("ring-foo\\bar.wav"));
        CHECK_FALSE(chime::webd::IsSafeSoundName("ring-../x.wav"));
        CHECK_FALSE(chime::webd::IsSafeSoundName("ring-..wav"));
        CHECK_FALSE(chime::webd::IsSafeSoundName("ring-foo bar.wav"));
        CHECK_FALSE(chime::webd::IsSafeSoundName("ring-foo$.wav"));
        CHECK_FALSE(chime::webd::IsSafeSoundName("bell-default.wav"));
        CHECK_FALSE(chime::webd::IsSafeSoundName("ring-default.mp3"));
        CHECK_FALSE(chime::webd::IsSafeSoundName("ring-default.wav.bak"));
        CHECK_FALSE(chime::webd::IsSafeSoundName("default.wav"));
    }
}
