include(FetchContent)

FetchContent_Declare(
  doctest
  URL https://github.com/doctest/doctest/archive/refs/tags/v2.5.3.tar.gz
  URL_HASH SHA256=174ebc4e769928959614789c5b4e9c3d0a0f81a62bb608756b127bfebfb21331
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
set(DOCTEST_WITH_TESTS OFF CACHE BOOL "" FORCE)
set(DOCTEST_NO_INSTALL ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(doctest)

add_executable(
  oc_native_tests
  test_main.cpp
  mqtt_topic_test.cpp
  kv_config_test.cpp
  json_test.cpp
  http_parse_test.cpp
  http_router_test.cpp
  static_files_test.cpp
  web_api_test.cpp
  web_server_test.cpp
  chime_config_test.cpp
  sound_name_test.cpp
  chime_service_ring_test.cpp)

target_include_directories(oc_native_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_options(oc_native_tests PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(oc_native_tests PRIVATE chime_core chime_webd_core doctest::doctest)

include("${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake")
doctest_discover_tests(oc_native_tests PROPERTIES TIMEOUT 30)
