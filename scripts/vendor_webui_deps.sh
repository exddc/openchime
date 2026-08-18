#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
WEBUI_DIR="$PROJECT_DIR/webui"
VENDOR_DIR="$WEBUI_DIR/vendor"
ARCHIVE="$VENDOR_DIR/node_modules.tar.gz"
CHECKSUM="$VENDOR_DIR/node_modules.tar.gz.sha256"
BUN_VERSION_FILE="$PROJECT_DIR/buildroot/bun-version"
ASSERT_SCRIPT="$SCRIPT_DIR/assert_webui_vendor_archive.sh"

error() { echo "[vendor-webui] ERROR: $*" >&2; exit 1; }

[ -f "$WEBUI_DIR/package.json" ] || error "missing $WEBUI_DIR/package.json"
[ -f "$WEBUI_DIR/bun.lock" ] || error "missing $WEBUI_DIR/bun.lock"
[ -f "$BUN_VERSION_FILE" ] || error "missing $BUN_VERSION_FILE"
[ -f "$ASSERT_SCRIPT" ] || error "missing $ASSERT_SCRIPT"
command -v bun >/dev/null 2>&1 || error "bun is required"

expected_bun="$(tr -d '[:space:]' < "$BUN_VERSION_FILE")"
got_bun="$(bun --version)"
[ "$got_bun" = "$expected_bun" ] || \
    error "bun $got_bun does not match $BUN_VERSION_FILE ($expected_bun)"

mkdir -p "$VENDOR_DIR"
tmp="$(mktemp -d)"
trap 'rm -rf "${tmp:-}"' EXIT

cp "$WEBUI_DIR/package.json" "$WEBUI_DIR/bun.lock" "$tmp/"

(
    cd "$tmp"
    bun install --frozen-lockfile --os linux --cpu x64
    bun install --frozen-lockfile --os linux --cpu arm64
    rm -f "$ARCHIVE"
    COPYFILE_DISABLE=1 tar -czf "$ARCHIVE" node_modules
)

bash "$ASSERT_SCRIPT" "$ARCHIVE"

if command -v sha256sum >/dev/null 2>&1; then
    (cd "$VENDOR_DIR" && sha256sum "$(basename "$ARCHIVE")" > "$(basename "$CHECKSUM")")
else
    (cd "$VENDOR_DIR" && shasum -a 256 "$(basename "$ARCHIVE")" > "$(basename "$CHECKSUM")")
fi

echo "[vendor-webui] wrote $ARCHIVE"
echo "[vendor-webui] wrote $CHECKSUM"
