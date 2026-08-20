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

tar_without_apple_metadata() {
    local extra=()
    if tar --no-xattrs -cf - /dev/null >/dev/null 2>&1; then
        extra+=(--no-xattrs)
    fi
    if tar --no-mac-metadata -cf - /dev/null >/dev/null 2>&1; then
        extra+=(--no-mac-metadata)
    fi
    COPYFILE_DISABLE=1 tar "${extra[@]}" "$@"
}

mkdir -p "$tmp/node_modules/@esbuild/darwin-arm64/bin"
printf 'darwin\n' > "$tmp/node_modules/@esbuild/darwin-arm64/bin/esbuild"
tar_without_apple_metadata -czf "$tmp/darwin-only.tar.gz" -C "$tmp" node_modules
if bash "$ASSERT_SCRIPT" "$tmp/darwin-only.tar.gz" >/dev/null 2>&1; then
    fail "Darwin-only archive was accepted"
fi

python3 - "$tmp/apple-xattrs.tar.gz" <<'PY'
import io
import sys
import tarfile

path = sys.argv[1]
info = tarfile.TarInfo("node_modules/file")
payload = b"x\n"
info.size = len(payload)
info.pax_headers["LIBARCHIVE.xattr.com.apple.provenance"] = "x"
with tarfile.open(path, "w:gz", format=tarfile.PAX_FORMAT) as tar:
    tar.addfile(info, io.BytesIO(payload))
PY
if output="$(bash "$ASSERT_SCRIPT" "$tmp/apple-xattrs.tar.gz" 2>&1)"; then
    fail "Apple xattr archive was accepted"
fi
printf '%s\n' "$output" | grep -q 'Apple xattrs' || \
    fail "Apple xattr archive was rejected for the wrong reason: $output"

echo "[test-vendor-webui-archive] passed"
