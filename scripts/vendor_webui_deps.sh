#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
WEBUI_DIR="$PROJECT_DIR/webui"
VENDOR_DIR="$WEBUI_DIR/vendor"
ARCHIVE="$VENDOR_DIR/node_modules.tar.gz"
CHECKSUM="$VENDOR_DIR/node_modules.tar.gz.sha256"

error() { echo "[vendor-webui] ERROR: $*" >&2; exit 1; }

[ -f "$WEBUI_DIR/package.json" ] || error "missing $WEBUI_DIR/package.json"
[ -f "$WEBUI_DIR/bun.lock" ] || error "missing $WEBUI_DIR/bun.lock"
command -v bun >/dev/null 2>&1 || error "bun is required"

mkdir -p "$VENDOR_DIR"

(
    cd "$WEBUI_DIR"
    bun install --frozen-lockfile
    rm -f "$ARCHIVE"
    COPYFILE_DISABLE=1 tar -czf "$ARCHIVE" node_modules
)

if command -v sha256sum >/dev/null 2>&1; then
    (cd "$VENDOR_DIR" && sha256sum "$(basename "$ARCHIVE")" > "$(basename "$CHECKSUM")")
else
    (cd "$VENDOR_DIR" && shasum -a 256 "$(basename "$ARCHIVE")" > "$(basename "$CHECKSUM")")
fi

echo "[vendor-webui] wrote $ARCHIVE"
echo "[vendor-webui] wrote $CHECKSUM"
