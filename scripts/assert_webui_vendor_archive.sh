#!/usr/bin/env bash
set -euo pipefail

usage() {
    echo "Usage: $0 <node_modules.tar.gz>" >&2
}

if [ "${1:-}" = "-h" ] || [ "${1:-}" = "--help" ]; then
    usage
    exit 0
fi

if [ "$#" -ne 1 ]; then
    usage
    exit 1
fi

ARCHIVE="$1"

error() {
    echo "ERROR: $*" >&2
    exit 1
}

[ -f "$ARCHIVE" ] || error "missing vendor archive: $ARCHIVE"

LIST="$(tar -tzf "$ARCHIVE")"
[ -n "$LIST" ] || error "vendor archive is empty: $ARCHIVE"

require_member() {
    local path="$1"
    grep -Fxq -- "$path" <<< "$LIST" || \
        error "vendor archive is missing $path (Linux builder natives are required)"
}

require_member "node_modules/@esbuild/linux-x64/bin/esbuild"
require_member "node_modules/@esbuild/linux-arm64/bin/esbuild"
require_member "node_modules/@rollup/rollup-linux-x64-gnu/rollup.linux-x64-gnu.node"
require_member "node_modules/@rollup/rollup-linux-arm64-gnu/rollup.linux-arm64-gnu.node"
require_member "node_modules/@tailwindcss/oxide-linux-x64-gnu/tailwindcss-oxide.linux-x64-gnu.node"
require_member "node_modules/@tailwindcss/oxide-linux-arm64-gnu/tailwindcss-oxide.linux-arm64-gnu.node"
require_member "node_modules/lightningcss-linux-x64-gnu/lightningcss.linux-x64-gnu.node"
require_member "node_modules/lightningcss-linux-arm64-gnu/lightningcss.linux-arm64-gnu.node"
require_member "node_modules/@biomejs/cli-linux-x64/biome"
require_member "node_modules/@biomejs/cli-linux-arm64/biome"

if grep -Eq 'node_modules/.*/(darwin-arm64|darwin-x64)(/|$)' <<< "$LIST"; then
    error "vendor archive contains Darwin natives; regenerate with scripts/vendor_webui_deps.sh"
fi
