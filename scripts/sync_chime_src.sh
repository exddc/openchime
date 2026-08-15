#!/usr/bin/env bash
set -euo pipefail

if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <repo_root> <dest_chime_src>" >&2
    exit 1
fi

REPO_ROOT="$1"
DEST="$2"

[ -f "$REPO_ROOT/CMakeLists.txt" ] || {
    echo "ERROR: missing $REPO_ROOT/CMakeLists.txt" >&2
    exit 1
}
[ -d "$REPO_ROOT/chime" ] || {
    echo "ERROR: missing $REPO_ROOT/chime" >&2
    exit 1
}
[ -d "$REPO_ROOT/common" ] || {
    echo "ERROR: missing $REPO_ROOT/common" >&2
    exit 1
}

mkdir -p "$DEST"

rsync -a --delete \
    --exclude 'build/' \
    --exclude 'build-ci/' \
    --exclude 'build-local/' \
    --exclude 'cmake-build-*/' \
    "$REPO_ROOT/chime/" "$DEST/chime/"
rsync -a --delete "$REPO_ROOT/common/" "$DEST/common/"
cp "$REPO_ROOT/CMakeLists.txt" "$DEST/CMakeLists.txt"

if [ -d "$REPO_ROOT/cmake" ]; then
    mkdir -p "$DEST/cmake"
    rsync -a --delete "$REPO_ROOT/cmake/" "$DEST/cmake/"
fi

keep_dest_entry() {
    case "$1" in
        CMakeLists.txt|chime|common)
            return 0
            ;;
        cmake)
            if [ -d "$REPO_ROOT/cmake" ]; then
                return 0
            fi
            return 1
            ;;
        *)
            return 1
            ;;
    esac
}

for dest_entry in "$DEST"/*; do
    [ -e "$dest_entry" ] || continue
    name="${dest_entry##*/}"
    if keep_dest_entry "$name"; then
        continue
    fi
    rm -rf "$dest_entry"
done
