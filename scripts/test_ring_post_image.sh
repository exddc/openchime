#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
POST_IMAGE="$SCRIPT_DIR/../buildroot/board/raspberrypizero2w/post_image.sh"
TMP_DIR="$(mktemp -d)"
trap 'rm -rf "$TMP_DIR"' EXIT
mkdir -p "$TMP_DIR/host/bin" "$TMP_DIR/support/scripts"
cat > "$TMP_DIR/host/bin/dtc" <<'MOCK'
#!/usr/bin/env bash
set -eu
while [ "$1" != -o ]; do shift; done
: > "$2"
MOCK
cat > "$TMP_DIR/host/bin/mkfs.ext4" <<'MOCK'
#!/usr/bin/env bash
exit 0
MOCK
cat > "$TMP_DIR/support/scripts/genimage.sh" <<'MOCK'
#!/usr/bin/env bash
set -eu
grep -q "\"$EXPECTED_DTB\"" "$2"
[ -f "$BINARIES_DIR/$EXPECTED_DTB" ]
[ -f "$BINARIES_DIR/rpi-firmware/overlays/openchime-ring-audio.dtbo" ]
[ -f "$BINARIES_DIR/data.ext4" ]
MOCK
chmod +x "$TMP_DIR/host/bin/"* "$TMP_DIR/support/scripts/genimage.sh"
export HOST_DIR="$TMP_DIR/host"
export PATH="$HOST_DIR/bin:$PATH"
export DATA_IMAGE_SIZE_MIB=1

check_image() {
    local board="$1"
    export EXPECTED_DTB="$2"
    export BINARIES_DIR="$TMP_DIR/$board"
    mkdir -p "$BINARIES_DIR/rpi-firmware/overlays"
    : > "$BINARIES_DIR/rpi-firmware/bootcode.bin"
    : > "$BINARIES_DIR/$EXPECTED_DTB"
    bash "$POST_IMAGE" "$BINARIES_DIR" "$board"
    grep -qx 'kernel=zImage' "$BINARIES_DIR/config.txt"
    grep -qx 'dtparam=krnbt=on' "$BINARIES_DIR/config.txt"
    grep -q 'root=/dev/mmcblk0p2' "$BINARIES_DIR/cmdline.txt"
    rm "$BINARIES_DIR/rpi-firmware/bootcode.bin"
    if bash "$POST_IMAGE" "$BINARIES_DIR" "$board" > "$TMP_DIR/error.log" 2>&1; then
        echo 'ERROR: post-image accepted missing bootcode.bin' >&2
        exit 1
    fi
}

cd "$TMP_DIR"
check_image rpi0w bcm2708-rpi-zero-w.dtb
check_image rpi02w bcm2710-rpi-zero-2-w.dtb
echo '[test-ring-post-image] passed'
