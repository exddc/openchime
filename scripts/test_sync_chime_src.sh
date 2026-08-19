#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SYNC_SCRIPT="$SCRIPT_DIR/sync_chime_src.sh"

fail() {
    echo "[test-sync-chime-src] ERROR: $*" >&2
    exit 1
}

expect_unsafe_destination() {
    local source="$1"
    local destination="$2"
    local sentinel="$3"

    if bash "$SYNC_SCRIPT" "$source" "$destination"; then
        fail "unsafe destination was accepted: $destination"
    fi
    [ -e "$sentinel" ] || fail "unsafe destination was modified: $destination"
}

main() {
    local tmp source destination
    tmp="$(mktemp -d)"
    trap 'rm -rf "${tmp:-}"' EXIT
    source="$tmp/repo"
    destination="$tmp/chime-src"

    mkdir -p "$source/chime" "$source/common" "$source/cmake" "$destination/cmake" "$destination/stale-root"
    printf 'cmake_minimum_required(VERSION 3.27)\nproject(sync_test)\n' > "$source/CMakeLists.txt"
    printf 'keep\n' > "$source/cmake/keep.cmake"
    printf 'gone\n' > "$destination/cmake/deleted.cmake"
    printf 'keep\n' > "$destination/cmake/keep.cmake"
    printf 'stale\n' > "$destination/stale-root/leftover"
    printf 'openchime-sync-root\n' > "$destination/.openchime-sync-root"

    bash "$SYNC_SCRIPT" "$source" "$destination"
    [ -f "$destination/cmake/keep.cmake" ] || fail "sync dropped cmake/keep.cmake"
    [ ! -e "$destination/cmake/deleted.cmake" ] || fail "sync retained deleted cmake/deleted.cmake"
    [ ! -e "$destination/stale-root" ] || fail "sync retained obsolete root entry"

    mkdir -p "$source/chime/build-tw351-baseline"
    printf 'cache\n' > "$source/chime/build-tw351-baseline/CMakeCache.txt"
    bash "$SYNC_SCRIPT" "$source" "$destination"
    [ ! -e "$destination/chime/build-tw351-baseline" ] || fail "sync copied host CMake output"

    rm -rf "$source/cmake"
    bash "$SYNC_SCRIPT" "$source" "$destination"
    [ ! -e "$destination/cmake" ] || fail "sync retained removed cmake/"

    local unmanaged="$tmp/unmanaged"
    mkdir -p "$unmanaged"
    printf 'keep\n' > "$unmanaged/sentinel"
    if bash "$SYNC_SCRIPT" "$source" "$unmanaged"; then
        fail "unmanaged destination was accepted"
    fi
    [ -e "$unmanaged/sentinel" ] || fail "unmanaged destination was modified"

    mkdir -p "$source/unsafe-destination"
    printf 'keep\n' > "$source/unsafe-destination/sentinel"
    expect_unsafe_destination "$source" "$source/unsafe-destination" "$source/unsafe-destination/sentinel"
    if bash "$SYNC_SCRIPT" "$source" "$source/new-unsafe-destination"; then
        fail "unsafe new destination was accepted"
    fi
    [ ! -e "$source/new-unsafe-destination" ] || fail "unsafe new destination was created"
    expect_unsafe_destination "$source" "$tmp" "$source/CMakeLists.txt"
    expect_unsafe_destination "$source" "$source" "$source/CMakeLists.txt"
    expect_unsafe_destination "$source" / "$source/CMakeLists.txt"

    echo "[test-sync-chime-src] passed"
}

main
