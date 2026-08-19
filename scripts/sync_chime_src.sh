#!/usr/bin/env bash
set -euo pipefail

if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <repo_root> <dest_chime_src>" >&2
    exit 1
fi

require_tool() {
    command -v "$1" >/dev/null 2>&1 || {
        echo "ERROR: required tool not found: $1" >&2
        exit 1
    }
}

is_within() {
    local path="$1"
    local parent="$2"
    [ "$path" = "$parent" ] || [[ "$path" == "$parent/"* ]]
}

REPO_ROOT="$(cd "$1" && pwd -P)"
if [ -e "$2" ]; then
    [ -d "$2" ] || {
        echo "ERROR: destination is not a directory: $2" >&2
        exit 1
    }
    DEST="$(cd "$2" && pwd -P)"
else
    DEST_PARENT="$(cd "$(dirname "$2")" && pwd -P)"
    DEST="$DEST_PARENT/$(basename "$2")"
fi
MARKER_FILE="$DEST/.openchime-sync-root"

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

require_tool rsync

if [ "$DEST" = "/" ] || is_within "$DEST" "$REPO_ROOT" || is_within "$REPO_ROOT" "$DEST"; then
    echo "ERROR: unsafe destination: $DEST" >&2
    exit 1
fi

mkdir -p "$DEST"

if [ ! -f "$MARKER_FILE" ] || ! grep -qx 'openchime-sync-root' "$MARKER_FILE"; then
    if [ -n "$(find "$DEST" -mindepth 1 -maxdepth 1 -print -quit)" ]; then
        echo "ERROR: destination is not managed by sync_chime_src.sh: $DEST" >&2
        exit 1
    fi
    printf 'openchime-sync-root\n' > "$MARKER_FILE"
fi

rsync -a --delete \
    --exclude 'build/' \
    --exclude 'build-*/' \
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
        .openchime-sync-root|CMakeLists.txt|chime|common)
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
