#ifndef CHIME_WEBD_SOUND_NAME_H
#define CHIME_WEBD_SOUND_NAME_H

#include <cctype>
#include <string>

#include "oc/util/strings.h"

namespace chime::webd {

inline bool IsSafeSoundName(const std::string &file_name) {
    if (file_name.empty() || file_name.size() > 128) {
        return false;
    }
    if (file_name.find('/') != std::string::npos || file_name.find('\\') != std::string::npos) {
        return false;
    }
    if (file_name.find("..") != std::string::npos) {
        return false;
    }
    const std::string lowered = oc::util::ToLower(file_name);
    const bool has_prefix = lowered.size() >= 5 && lowered.compare(0, 5, "ring-") == 0;
    if (!has_prefix || lowered.rfind(".wav") != lowered.size() - 4) {
        return false;
    }
    for (const char c : file_name) {
        if (!(std::isalnum(static_cast<unsigned char>(c)) || c == '.' || c == '-' || c == '_')) {
            return false;
        }
    }
    return true;
}

} // namespace chime::webd

#endif
