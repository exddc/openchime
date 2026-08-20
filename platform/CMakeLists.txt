if(NOT DEFINED OC_BUILD_HTTP)
  set(OC_BUILD_HTTP ON)
endif()

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

file(GLOB_RECURSE OC_PLATFORM_TREE
  "${CMAKE_CURRENT_SOURCE_DIR}/include/*"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/*"
  "${CMAKE_CURRENT_SOURCE_DIR}/tests/*")
foreach(_oc_platform_file IN LISTS OC_PLATFORM_TREE)
  file(STRINGS "${_oc_platform_file}" _oc_chime_hits REGEX "#include[ \t]*[\"<]chime/")
  if(_oc_chime_hits)
    message(FATAL_ERROR
      "platform must not include chime headers: ${_oc_platform_file}")
  endif()
endforeach()

# Core platform: logging, MQTT, signals, filesystem/env/time, schema/kv primitives,
# apply-job infrastructure. No OpenSSL so the ring binary stays free of TLS.
add_library(
  oc_platform STATIC
  src/apply/job_runner.cpp
  src/config/kv_document.cpp
  src/logging/logger.cpp
  ${OC_MQTT_CLIENT_SOURCE}
  src/runtime/signal_handler.cpp
  src/util/environment.cpp
  src/util/filesystem.cpp
  src/util/platform.cpp
  src/util/strings.cpp
  src/util/time.cpp)
target_include_directories(oc_platform PUBLIC include)
target_compile_options(oc_platform PRIVATE -Wall -Wextra -Wpedantic)
if(OC_MQTT_TARGET)
  target_link_libraries(oc_platform PUBLIC ${OC_MQTT_TARGET})
endif()

if(OC_BUILD_HTTP)
  find_package(OpenSSL REQUIRED)
  find_package(Threads REQUIRED)

  add_library(oc_cjson STATIC third_party/cJSON/cJSON.c)
  # cJSON 1.7.19 (MIT). Keep this include private and SYSTEM: a public -I on this
  # directory shadows the C++20 <version> header on case-insensitive filesystems,
  # and clang-tidy should not apply project checks to the vendored header.
  target_include_directories(oc_cjson SYSTEM PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/third_party/cJSON")
  set_target_properties(oc_cjson PROPERTIES C_STANDARD 99 C_STANDARD_REQUIRED ON)
  target_compile_options(oc_cjson PRIVATE -Wno-unused-parameter)

  # HTTPS/JSON/discovery. Linked by configuration daemons, not the ring service.
  add_library(
    oc_platform_http STATIC
    src/http/json_http.cpp
    src/http/parse.cpp
    src/http/router.cpp
    src/http/static_files.cpp
    src/http/tls_server.cpp
    src/json/json.cpp
    src/json/validate.cpp
    src/mdns/responder.cpp
    src/wifi/scan.cpp)
  target_include_directories(oc_platform_http PUBLIC include)
  target_include_directories(oc_platform_http SYSTEM PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/third_party/cJSON")
  target_compile_options(oc_platform_http PRIVATE -Wall -Wextra -Wpedantic)
  target_link_libraries(
    oc_platform_http
    PUBLIC oc_platform oc_cjson OpenSSL::SSL OpenSSL::Crypto Threads::Threads)
endif()

if(OC_BUILD_TESTS)
  add_subdirectory(tests)
endif()
