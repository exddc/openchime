cmake_minimum_required(VERSION 3.27)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  message(FATAL_ERROR
    "Configure Open Chime from the repository root, for example:\n"
    "  cmake -S <repo> -B <build-dir>")
endif()

find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)
include(GNUInstallDirs)

add_library(
  chime_core STATIC
  src/audio/aplay_audio_player.cpp
  src/config/chime_config.cpp
  src/network/linux_wifi_monitor.cpp
  src/service/chime_service.cpp)
target_include_directories(chime_core PUBLIC include)
target_compile_options(chime_core PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(chime_core PUBLIC oc_common oc_build_version)

add_library(
  chime_webd_core STATIC
  src/webd/apply_manager.cpp
  src/webd/config_store.cpp
  src/webd/json.cpp
  src/webd/mdns.cpp
  src/webd/string_utils.cpp
  src/webd/ui_assets.cpp
  src/webd/web_server.cpp
  src/webd/wifi_scan.cpp)
target_include_directories(chime_webd_core PUBLIC include)
target_compile_options(chime_webd_core PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(
  chime_webd_core
  PUBLIC oc_common oc_build_version OpenSSL::SSL OpenSSL::Crypto
         Threads::Threads)

add_executable(chime src/main.cpp)
target_include_directories(chime PRIVATE include)
target_compile_options(chime PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(chime PRIVATE chime_core oc_common oc_build_version)

add_executable(chime-webd src/webd/main.cpp)
target_include_directories(chime-webd PRIVATE include)
target_compile_options(chime-webd PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(
  chime-webd
  PRIVATE chime_webd_core oc_common oc_build_version OpenSSL::SSL
          OpenSSL::Crypto Threads::Threads)

install(TARGETS chime chime-webd RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

if(OC_BUILD_TESTS)
  add_subdirectory(tests)
endif()
