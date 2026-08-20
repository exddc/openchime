find_package(PkgConfig)

set(OC_MQTT_CLIENT_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/src/mqtt/client_stub.cpp")
set(OC_MQTT_TARGET "")

if(PkgConfig_FOUND)
  pkg_check_modules(MOSQ QUIET IMPORTED_TARGET libmosquitto)
  if(NOT TARGET PkgConfig::MOSQ)
    pkg_check_modules(MOSQ QUIET IMPORTED_TARGET mosquitto)
  endif()
endif()

if(TARGET PkgConfig::MOSQ)
  set(OC_MQTT_TARGET PkgConfig::MOSQ)
else()
  find_path(MOSQUITTO_INCLUDE_DIR mosquitto.h)
  find_library(MOSQUITTO_LIBRARY mosquitto)
  if(MOSQUITTO_INCLUDE_DIR AND MOSQUITTO_LIBRARY)
    add_library(oc_mosquitto UNKNOWN IMPORTED GLOBAL)
    set_target_properties(
      oc_mosquitto
      PROPERTIES IMPORTED_LOCATION "${MOSQUITTO_LIBRARY}"
                 INTERFACE_INCLUDE_DIRECTORIES "${MOSQUITTO_INCLUDE_DIR}")
    set(OC_MQTT_TARGET oc_mosquitto)
  endif()
endif()

if(OC_MQTT_TARGET)
  set(OC_MQTT_CLIENT_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/src/mqtt/client.cpp")
  message(STATUS "Open Chime: using libmosquitto (${OC_MQTT_TARGET})")
elseif(OC_REQUIRE_MQTT)
  message(FATAL_ERROR
    "libmosquitto is required for this build (pkg-config or find_library)")
else()
  message(WARNING "libmosquitto not found; building with MQTT stub client")
endif()

add_library(
  oc_common STATIC
  src/config/kv_document.cpp
  src/logging/logger.cpp
  ${OC_MQTT_CLIENT_SOURCE}
  src/runtime/signal_handler.cpp
  src/util/environment.cpp
  src/util/filesystem.cpp
  src/util/platform.cpp
  src/util/strings.cpp
  src/util/time.cpp)
target_include_directories(oc_common PUBLIC include)
target_compile_options(oc_common PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(oc_common PUBLIC oc_build_version)
if(OC_MQTT_TARGET)
  target_link_libraries(oc_common PUBLIC ${OC_MQTT_TARGET})
endif()
