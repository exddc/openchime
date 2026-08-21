cmake_minimum_required(VERSION 3.27)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  message(FATAL_ERROR
    "Configure Open Chime from the repository root, for example:\n"
    "  cmake -S <repo> -B <build-dir>")
endif()

include(GNUInstallDirs)

add_library(
  chime_config STATIC
  src/config/chime_config.cpp
  src/config/config_migrate.cpp)
target_include_directories(chime_config PUBLIC include)
target_compile_options(chime_config PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(chime_config PUBLIC oc_platform oc_build_version)

add_library(
  chime_core STATIC
  src/audio/aplay_audio_player.cpp
  src/network/linux_wifi_monitor.cpp
  src/service/chime_service.cpp)
target_include_directories(chime_core PUBLIC include)
target_compile_options(chime_core PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(chime_core PUBLIC chime_config)

add_library(
  chime_webd_core STATIC
  src/webd/api_handlers.cpp
  src/webd/auth_store.cpp
  src/webd/apply_manager.cpp
  src/webd/config_store.cpp
  src/webd/ui_assets.cpp)
target_include_directories(chime_webd_core PUBLIC include)
target_compile_options(chime_webd_core PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(chime_webd_core PUBLIC chime_config oc_platform_http)

add_executable(chime src/main.cpp)
target_include_directories(chime PRIVATE include)
target_compile_options(chime PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(chime PRIVATE chime_core)

add_executable(chime-migrate src/config/chime_migrate_main.cpp)
target_include_directories(chime-migrate PRIVATE include)
target_compile_options(chime-migrate PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(chime-migrate PRIVATE chime_config)

add_executable(chime-webd src/webd/main.cpp)
target_include_directories(chime-webd PRIVATE include)
target_compile_options(chime-webd PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(chime-webd PRIVATE chime_webd_core)

install(TARGETS chime chime-webd chime-migrate RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

if(OC_BUILD_TESTS)
  add_subdirectory(tests)
endif()
