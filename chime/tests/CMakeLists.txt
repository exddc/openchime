add_executable(
  chime_tests
  test_main.cpp
  web_api_test.cpp
  web_auth_test.cpp
  web_server_test.cpp
  chime_config_test.cpp
  config_schema_contract_test.cpp
  config_constraints_test.cpp
  config_migrate_test.cpp
  sound_name_test.cpp
  chime_service_ring_test.cpp
  apply_manager_test.cpp
  aplay_audio_player_test.cpp)

target_include_directories(chime_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}"
                                               "${CMAKE_SOURCE_DIR}/platform/tests")
target_compile_options(chime_tests PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(chime_tests PRIVATE chime_core chime_webd_core doctest::doctest)

include("${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake")
doctest_discover_tests(chime_tests PROPERTIES TIMEOUT 30)
