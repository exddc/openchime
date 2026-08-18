#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
ASSERT_SCRIPT="$SCRIPT_DIR/assert_webui_vendor_archive.sh"
ARCHIVE="$PROJECT_DIR/webui/vendor/node_modules.tar.gz"

fail() {
    echo "[test-vendor-webui-archive] ERROR: $*" >&2
    exit 1
}

[ -f "$ARCHIVE" ] || fail "missing $ARCHIVE"
bash "$ASSERT_SCRIPT" "$ARCHIVE" || fail "committed vendor archive failed Linux native checks"

tmp="$(mktemp -d)"
trap 'rm -rf "${tmp:-}"' EXIT
mkdir -p "$tmp/node_modules/@esbuild/darwin-arm64/bin"
printf 'darwin\n' > "$tmp/node_modules/@esbuild/darwin-arm64/bin/esbuild"
COPYFILE_DISABLE=1 tar -czf "$tmp/darwin-only.tar.gz" -C "$tmp" node_modules
if bash "$ASSERT_SCRIPT" "$tmp/darwin-only.tar.gz" >/dev/null 2>&1; then
    fail "Darwin-only archive was accepted"
fi

echo "[test-vendor-webui-archive] passed"
