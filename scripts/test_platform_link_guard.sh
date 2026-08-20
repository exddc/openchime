#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

log() {
  echo "[platform-link-guard] $*"
}

error() {
  echo "[platform-link-guard] ERROR: $*" >&2
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

expect_product_link_rejected() {
  local name="$1"
  local chime_cmake="$2"
  local stage log_file

  stage="$(mktemp -d)"
  log_file="$stage/cmake.log"
  cp "$PROJECT_DIR/CMakeLists.txt" "$stage/CMakeLists.txt"
  rsync -a --delete \
    --exclude 'build/' \
    --exclude 'build-*/' \
    --exclude 'cmake-build-*/' \
    "$PROJECT_DIR/platform/" "$stage/platform/"
  mkdir -p "$stage/chime"
  printf '%s\n' "$chime_cmake" > "$stage/chime/CMakeLists.txt"

  set +e
  cmake -S "$stage" -B "$stage/build" \
    -G Ninja \
    -DOC_BUILD_TESTS=OFF \
    -DOC_BUILD_CHIME=ON \
    ${CMAKE_HOST_ARGS[@]+"${CMAKE_HOST_ARGS[@]}"} \
    >"$log_file" 2>&1
  local rc=$?
  set -e

  if [ "$rc" -eq 0 ]; then
    echo "----- $name cmake log -----" >&2
    cat "$log_file" >&2
    rm -rf "$stage"
    error "$name: configure succeeded despite a product link edge"
  fi
  if ! grep -q 'must not link product target' "$log_file"; then
    echo "----- $name cmake log -----" >&2
    cat "$log_file" >&2
    rm -rf "$stage"
    error "$name: configure failed without the product-link diagnostic"
  fi
  rm -rf "$stage"
  log "$name rejected"
}

require_tool cmake
require_tool ninja
require_tool rsync
require_tool grep
discover_cmake_host_args

expect_product_link_rejected "post-platform PUBLIC link" "$(cat <<'EOF'
add_library(chime_core INTERFACE)
target_link_libraries(oc_platform PUBLIC chime_core)
EOF
)"

expect_product_link_rejected "post-platform LINK_ONLY genex" "$(cat <<'EOF'
add_library(chime_core INTERFACE)
target_link_libraries(oc_platform PUBLIC "$<LINK_ONLY:chime_core>")
EOF
)"

expect_product_link_rejected "post-platform alias" "$(cat <<'EOF'
add_library(chime_core INTERFACE)
add_library(product_alias ALIAS chime_core)
target_link_libraries(oc_platform PUBLIC product_alias)
EOF
)"

log "Product-link guard rejected late, genex, and alias edges"
