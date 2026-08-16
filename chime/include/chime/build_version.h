#pragma once

#include <iostream>

#ifndef CHIME_APP_VERSION
#define CHIME_APP_VERSION "dev"
#endif

#ifndef OPENCHIME_OS_VERSION
#define OPENCHIME_OS_VERSION "dev"
#endif

#ifndef CHIME_CONFIG_VERSION
#define CHIME_CONFIG_VERSION "dev"
#endif

#ifndef CHIME_BUILD_ID
#define CHIME_BUILD_ID "unknown"
#endif

inline void PrintCompileTimeVersions() {
    std::cout << "CHIME_APP_VERSION=" << CHIME_APP_VERSION << "\n";
    std::cout << "CHIME_BUILD_ID=" << CHIME_BUILD_ID << "\n";
    std::cout << "OPENCHIME_OS_VERSION=" << OPENCHIME_OS_VERSION << "\n";
    std::cout << "CHIME_CONFIG_VERSION=" << CHIME_CONFIG_VERSION << "\n";
}
