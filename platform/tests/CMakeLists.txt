add_executable(oc_process_helper process_helper.cpp)
target_compile_options(oc_process_helper PRIVATE -Wall -Wextra -Wpedantic)

add_executable(
  oc_platform_tests
  test_main.cpp
  mqtt_topic_test.cpp
  kv_config_test.cpp
  atomic_write_test.cpp
  apply_job_test.cpp
  process_runner_test.cpp)

target_include_directories(oc_platform_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_options(oc_platform_tests PRIVATE -Wall -Wextra -Wpedantic)
target_compile_definitions(oc_platform_tests PRIVATE "OC_PROCESS_HELPER=\"$<TARGET_FILE:oc_process_helper>\"")
target_link_libraries(oc_platform_tests PRIVATE oc_platform doctest::doctest)
add_dependencies(oc_platform_tests oc_process_helper)

include("${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake")
doctest_discover_tests(oc_platform_tests PROPERTIES TIMEOUT 30)

if(OC_BUILD_HTTP)
  add_executable(
    oc_platform_http_tests
    test_main.cpp
    json_test.cpp
    http_parse_test.cpp
    http_router_test.cpp
    product_route_test.cpp
    static_files_test.cpp
    tls_server_test.cpp)

  target_include_directories(oc_platform_http_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
  target_compile_options(oc_platform_http_tests PRIVATE -Wall -Wextra -Wpedantic)
  target_link_libraries(oc_platform_http_tests PRIVATE oc_platform_http doctest::doctest)

  doctest_discover_tests(oc_platform_http_tests PROPERTIES TIMEOUT 30)
endif()
