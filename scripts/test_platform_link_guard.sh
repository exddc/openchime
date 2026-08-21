#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
# shellcheck source=cmake_host_args.sh
. "$SCRIPT_DIR/cmake_host_args.sh"

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

expect_configure_rejected() {
  local name="$1"
  local needle="$2"
  local chime_cmake="$3"
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
    error "$name: configure succeeded despite a product boundary violation"
  fi
  if ! grep -q "$needle" "$log_file"; then
    echo "----- $name cmake log -----" >&2
    cat "$log_file" >&2
    rm -rf "$stage"
    error "$name: configure failed without diagnostic '$needle'"
  fi
  rm -rf "$stage"
  log "$name rejected"
}

require_tool cmake
require_tool ninja
require_tool rsync
require_tool grep
discover_cmake_host_args

expect_configure_rejected "post-platform PUBLIC link" "must not link product target" "$(cat <<'EOF'
add_library(chime_core INTERFACE)
target_link_libraries(oc_platform PUBLIC chime_core)
EOF
)"

expect_configure_rejected "post-platform LINK_ONLY genex" "must not link product target" "$(cat <<'EOF'
add_library(chime_core INTERFACE)
target_link_libraries(oc_platform PUBLIC "$<LINK_ONLY:chime_core>")
EOF
)"

expect_configure_rejected "post-platform IF genex" "must not link product target" "$(cat <<'EOF'
add_library(chime_core INTERFACE)
target_link_libraries(oc_platform PUBLIC "$<IF:1,chime_core,unused>")
EOF
)"

expect_configure_rejected "post-platform alias" "must not link product target" "$(cat <<'EOF'
add_library(chime_core INTERFACE)
add_library(product_alias ALIAS chime_core)
target_link_libraries(oc_platform PUBLIC product_alias)
EOF
)"

expect_configure_rejected "conditional CHIME compile definition" "must not compile with product definition" "$(cat <<'EOF'
target_compile_definitions(oc_platform PRIVATE "$<$<CONFIG:Debug>:CHIME_LEAK=1>")
EOF
)"

expect_configure_rejected "conditional oc_build_version genex" "must not depend on oc_build_version" "$(cat <<'EOF'
target_link_libraries(oc_platform PUBLIC "$<IF:1,oc_build_version,unused>")
EOF
)"

log "Product-link guard rejected late, genex, definition, and alias edges"
