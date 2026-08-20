#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
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

discover_cmake_host_args() {
  CMAKE_HOST_ARGS=()
  if ! command -v brew >/dev/null 2>&1; then
    return
  fi

  local openssl_prefix mosq_prefix prefixes=""
  openssl_prefix="$(brew --prefix openssl@3 2>/dev/null || brew --prefix openssl 2>/dev/null || true)"
  mosq_prefix="$(brew --prefix mosquitto 2>/dev/null || true)"
  if [ -n "$openssl_prefix" ] && [ -d "$openssl_prefix" ]; then
    CMAKE_HOST_ARGS+=("-DOPENSSL_ROOT_DIR=$openssl_prefix")
    prefixes="$openssl_prefix"
    if [ -d "$openssl_prefix/lib/pkgconfig" ]; then
      export PKG_CONFIG_PATH="$openssl_prefix/lib/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"
    fi
  fi
  if [ -n "$mosq_prefix" ] && [ -d "$mosq_prefix" ]; then
    if [ -n "$prefixes" ]; then
      prefixes="$prefixes;$mosq_prefix"
    else
      prefixes="$mosq_prefix"
    fi
    if [ -d "$mosq_prefix/lib/pkgconfig" ]; then
      export PKG_CONFIG_PATH="$mosq_prefix/lib/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"
    fi
  fi
  if [ -n "$prefixes" ]; then
    CMAKE_HOST_ARGS+=("-DCMAKE_PREFIX_PATH=$prefixes")
  fi
}

[ -f "$PROJECT_DIR/CMakeLists.txt" ] || error "missing $PROJECT_DIR/CMakeLists.txt"
[ -d "$PROJECT_DIR/platform" ] || error "missing $PROJECT_DIR/platform"
require_tool cmake
require_tool ninja
require_tool rsync
require_tool ctest

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
  -DOC_BUILD_TESTS=ON \
  -DOC_BUILD_CHIME=OFF \
  ${CMAKE_HOST_ARGS[@]+"${CMAKE_HOST_ARGS[@]}"}

cmake --build "$STAGE/build"
ctest --test-dir "$STAGE/build" --output-on-failure --no-tests=error

[ ! -e "$STAGE/build/bin/chime" ] || error "platform-only build produced chime"
[ ! -e "$STAGE/build/bin/chime-webd" ] || error "platform-only build produced chime-webd"
[ -x "$STAGE/build/bin/oc_platform_tests" ] || error "missing oc_platform_tests"

log "Platform-only configure, build, and tests passed"
