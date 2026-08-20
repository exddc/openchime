add_executable(
  oc_platform_tests
  test_main.cpp
  mqtt_topic_test.cpp
  kv_config_test.cpp
  json_test.cpp
  http_parse_test.cpp
  http_router_test.cpp
  product_route_test.cpp
  static_files_test.cpp
  atomic_write_test.cpp
  apply_job_test.cpp
  tls_server_test.cpp)

target_include_directories(oc_platform_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_options(oc_platform_tests PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(oc_platform_tests PRIVATE oc_platform oc_platform_http doctest::doctest)

include("${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake")
doctest_discover_tests(oc_platform_tests PROPERTIES TIMEOUT 30)
