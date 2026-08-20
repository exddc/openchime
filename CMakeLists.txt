cmake_minimum_required(VERSION 3.27)
project(openchime LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_FIND_PACKAGE_TARGETS_GLOBAL ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

option(OC_BUILD_TESTS "Build native C++ tests (CTest)" OFF)
option(OC_BUILD_CHIME "Build Chime product binaries and tests" ON)
option(OC_PRODUCTION_BUILD "Require release metadata and libmosquitto" OFF)
option(OC_REQUIRE_MQTT "Require libmosquitto instead of the stub client" OFF)

set(CHIME_APP_VERSION "" CACHE STRING "Chime application SemVer")
set(OPENCHIME_OS_VERSION "" CACHE STRING "Open Chime OS version")
set(CHIME_CONFIG_VERSION "" CACHE STRING "Chime config schema version")
set(CHIME_BUILD_ID "" CACHE STRING "Chime build identifier")

if(CMAKE_CROSSCOMPILING AND OC_BUILD_TESTS)
  message(FATAL_ERROR
    "OC_BUILD_TESTS is native-only and cannot be enabled when cross-compiling")
endif()

if(OC_PRODUCTION_BUILD OR CMAKE_CROSSCOMPILING)
  set(OC_REQUIRE_MQTT ON)
endif()

function(oc_read_first_line path out_var)
  set(_value "")
  if(EXISTS "${path}")
    file(STRINGS "${path}" _lines LIMIT_COUNT 1)
    if(_lines)
      list(GET _lines 0 _value)
      string(STRIP "${_value}" _value)
    endif()
  endif()
  set(${out_var} "${_value}" PARENT_SCOPE)
endfunction()

if(OC_BUILD_CHIME AND CHIME_APP_VERSION STREQUAL "" AND NOT OC_PRODUCTION_BUILD)
  oc_read_first_line("${CMAKE_CURRENT_SOURCE_DIR}/chime/VERSION" _chime_file_version)
  if(NOT _chime_file_version STREQUAL "")
    set(CHIME_APP_VERSION "${_chime_file_version}")
  endif()
endif()

if(OC_PRODUCTION_BUILD)
  foreach(_oc_ver_var IN ITEMS CHIME_APP_VERSION OPENCHIME_OS_VERSION
                        CHIME_CONFIG_VERSION CHIME_BUILD_ID)
    if(${_oc_ver_var} STREQUAL "")
      message(FATAL_ERROR
        "OC_PRODUCTION_BUILD requires ${_oc_ver_var} "
        "(pass -D${_oc_ver_var}=...)")
    endif()
  endforeach()
else()
  if(CHIME_APP_VERSION STREQUAL "")
    set(CHIME_APP_VERSION "dev")
  endif()
  if(OPENCHIME_OS_VERSION STREQUAL "")
    set(OPENCHIME_OS_VERSION "dev")
  endif()
  if(CHIME_CONFIG_VERSION STREQUAL "")
    set(CHIME_CONFIG_VERSION "dev")
  endif()
  if(CHIME_BUILD_ID STREQUAL "")
    set(CHIME_BUILD_ID "unknown")
  endif()
endif()

add_library(oc_build_version INTERFACE)
target_compile_definitions(
  oc_build_version
  INTERFACE
    CHIME_APP_VERSION="${CHIME_APP_VERSION}"
    OPENCHIME_OS_VERSION="${OPENCHIME_OS_VERSION}"
    CHIME_CONFIG_VERSION="${CHIME_CONFIG_VERSION}"
    CHIME_BUILD_ID="${CHIME_BUILD_ID}")

message(STATUS "CHIME_APP_VERSION=${CHIME_APP_VERSION}")
message(STATUS "OPENCHIME_OS_VERSION=${OPENCHIME_OS_VERSION}")
message(STATUS "CHIME_CONFIG_VERSION=${CHIME_CONFIG_VERSION}")
message(STATUS "CHIME_BUILD_ID=${CHIME_BUILD_ID}")

if(OC_BUILD_TESTS)
  include(CTest)
  include(FetchContent)
  FetchContent_Declare(
    doctest
    URL https://github.com/doctest/doctest/archive/refs/tags/v2.5.3.tar.gz
    URL_HASH SHA256=174ebc4e769928959614789c5b4e9c3d0a0f81a62bb608756b127bfebfb21331
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
  set(DOCTEST_WITH_TESTS OFF CACHE BOOL "" FORCE)
  set(DOCTEST_NO_INSTALL ON CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(doctest)
endif()

function(oc_assert_no_product_link target)
  if(NOT TARGET "${target}")
    message(FATAL_ERROR "missing target ${target}")
  endif()
  set(_stack "${target}")
  set(_seen "")
  while(_stack)
    list(POP_FRONT _stack _cur)
    if("${_cur}" IN_LIST _seen)
      continue()
    endif()
    list(APPEND _seen "${_cur}")
    if(_cur MATCHES "^chime")
      message(FATAL_ERROR
        "${target} must not link product target ${_cur}")
    endif()
    if(NOT TARGET "${_cur}")
      continue()
    endif()
    get_target_property(_type "${_cur}" TYPE)
    set(_libs "")
    if(_type STREQUAL "INTERFACE_LIBRARY")
      get_target_property(_libs "${_cur}" INTERFACE_LINK_LIBRARIES)
    else()
      get_target_property(_direct "${_cur}" LINK_LIBRARIES)
      get_target_property(_iface "${_cur}" INTERFACE_LINK_LIBRARIES)
      if(_direct)
        list(APPEND _libs ${_direct})
      endif()
      if(_iface)
        list(APPEND _libs ${_iface})
      endif()
    endif()
    foreach(_lib IN LISTS _libs)
      if(NOT _lib)
        continue()
      endif()
      while(_lib MATCHES "^\\$<[A-Za-z0-9_]+:(.+)>$")
        set(_lib "${CMAKE_MATCH_1}")
      endwhile()
      if(_lib MATCHES "\\$<")
        if(_lib MATCHES "(^|[^A-Za-z0-9_])chime([A-Za-z0-9_]|$)")
          message(FATAL_ERROR
            "${target} must not link product target via ${_lib}")
        endif()
        continue()
      endif()
      list(APPEND _stack "${_lib}")
    endforeach()
  endwhile()
endfunction()

add_subdirectory(platform)

if(OC_BUILD_CHIME)
  if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/chime/CMakeLists.txt")
    message(FATAL_ERROR
      "chime/ is missing; configure with -DOC_BUILD_CHIME=OFF for a platform-only build")
  endif()
  add_subdirectory(chime)
endif()

oc_assert_no_product_link(oc_platform)
oc_assert_no_product_link(oc_platform_http)
