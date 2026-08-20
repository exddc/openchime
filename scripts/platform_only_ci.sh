#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
# shellcheck source=cmake_host_args.sh
. "$SCRIPT_DIR/cmake_host_args.sh"
BUILD_TYPE="${BUILD_TYPE:-Release}"

log() {
  echo "[platform-only-ci] $*"
}

error() {
  echo "[platform-only-ci] ERROR: $*" >&2
  exit 1
}

require_tool() {
  command -v "$1" >/dev/null 2>&1 || error "Required tool not found: $1"
}

host_args_without_openssl() {
  CORE_HOST_ARGS=()
  local arg
  for arg in "${CMAKE_HOST_ARGS[@]+"${CMAKE_HOST_ARGS[@]}"}"; do
    case "$arg" in
      -DOPENSSL_ROOT_DIR=*) ;;
      *) CORE_HOST_ARGS+=("$arg") ;;
    esac
  done
}

[ -f "$PROJECT_DIR/CMakeLists.txt" ] || error "missing $PROJECT_DIR/CMakeLists.txt"
[ -d "$PROJECT_DIR/platform" ] || error "missing $PROJECT_DIR/platform"
require_tool cmake
require_tool ninja
require_tool rsync
require_tool ctest
require_tool grep

STAGE="$(mktemp -d)"
trap 'rm -rf "${STAGE:-}"' EXIT

log "Staging a tree without chime/ at $STAGE"
cp "$PROJECT_DIR/CMakeLists.txt" "$STAGE/CMakeLists.txt"
rsync -a --delete \
  --exclude 'build/' \
  --exclude 'build-*/' \
  --exclude 'cmake-build-*/' \
  "$PROJECT_DIR/platform/" "$STAGE/platform/"

if [ -d "$PROJECT_DIR/cmake" ]; then
  mkdir -p "$STAGE/cmake"
  rsync -a --delete "$PROJECT_DIR/cmake/" "$STAGE/cmake/"
fi

[ ! -e "$STAGE/chime" ] || error "staged tree must not contain chime/"

discover_cmake_host_args
cmake --version
cmake -S "$STAGE" -B "$STAGE/build" \
  -G Ninja \
  -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  -DOC_BUILD_TESTS=ON \
  -DOC_BUILD_CHIME=OFF \
  ${CMAKE_HOST_ARGS[@]+"${CMAKE_HOST_ARGS[@]}"}

if grep -E -- '-DCHIME_|-DOPENCHIME_' "$STAGE/build/compile_commands.json"; then
  error "platform compilation received CHIME_* or OPENCHIME_* definitions"
fi
if grep -q 'oc_build_version' "$STAGE/build/build.ninja"; then
  error "platform-only ninja graph still references oc_build_version"
fi

cmake --build "$STAGE/build"
ctest --test-dir "$STAGE/build" --output-on-failure --no-tests=error

[ ! -e "$STAGE/build/bin/chime" ] || error "platform-only build produced chime"
[ ! -e "$STAGE/build/bin/chime-webd" ] || error "platform-only build produced chime-webd"
[ -x "$STAGE/build/bin/oc_platform_tests" ] || error "missing oc_platform_tests"
[ -x "$STAGE/build/bin/oc_platform_http_tests" ] || error "missing oc_platform_http_tests"

log "Configuring a core-only graph without OpenSSL"
host_args_without_openssl
cmake -S "$STAGE" -B "$STAGE/core" \
  -G Ninja \
  -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
  -DOC_BUILD_TESTS=ON \
  -DOC_BUILD_CHIME=OFF \
  -DOC_BUILD_HTTP=OFF \
  ${CORE_HOST_ARGS[@]+"${CORE_HOST_ARGS[@]}"}

if [ -e "$STAGE/core/CMakeFiles/oc_platform_http.dir" ]; then
  error "core-only configure created oc_platform_http"
fi
if grep -q 'oc_platform_http\|OpenSSL::SSL' "$STAGE/core/build.ninja"; then
  error "core-only ninja graph still references HTTP or OpenSSL"
fi

cmake --build "$STAGE/core"
ctest --test-dir "$STAGE/core" --output-on-failure --no-tests=error
[ -x "$STAGE/core/bin/oc_platform_tests" ] || error "missing core oc_platform_tests"
[ ! -e "$STAGE/core/bin/oc_platform_http_tests" ] || error "core-only build produced HTTP tests"

log "Configuring production platform-only without Chime metadata"
cmake -S "$STAGE" -B "$STAGE/prod" \
  -G Ninja \
  -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
  -DOC_BUILD_TESTS=OFF \
  -DOC_BUILD_CHIME=OFF \
  -DOC_BUILD_HTTP=OFF \
  -DOC_PRODUCTION_BUILD=ON \
  ${CORE_HOST_ARGS[@]+"${CORE_HOST_ARGS[@]}"}

log "Platform-only configure, build, and tests passed"
