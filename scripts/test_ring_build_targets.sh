#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TMP_DIR="$(mktemp -d)"
trap 'rm -rf "$TMP_DIR"' EXIT
PROJECT="$TMP_DIR/project"
mkdir -p "$PROJECT/scripts" "$PROJECT/buildroot/board/common/rootfs_overlay/etc/wpa_supplicant" \
    "$PROJECT/buildroot/board/common/rootfs_overlay/root/.ssh" "$PROJECT/chime" "$TMP_DIR/bin"
cp "$SCRIPT_DIR/docker_build.sh" "$SCRIPT_DIR/flash_sd.sh" "$SCRIPT_DIR/write_build_meta.sh" "$PROJECT/scripts/"
printf 'OPENCHIME_OS_VERSION=1.0.0\nCHIME_CONFIG_VERSION=1\n' > "$PROJECT/buildroot/version.env"
printf '1.0.0\n' > "$PROJECT/chime/VERSION"
: > "$PROJECT/buildroot/board/common/rootfs_overlay/etc/wpa_supplicant/wpa_supplicant.conf"
: > "$PROJECT/buildroot/board/common/rootfs_overlay/root/.ssh/authorized_keys"

cat > "$TMP_DIR/bin/docker" <<'MOCK'
#!/usr/bin/env bash
set -euo pipefail
printf '%s\n' "$@" >> "$DOCKER_LOG"
if [ "$1" != run ]; then exit 0; fi
for arg in "$@"; do
    case "$arg" in
        *'MemTotal'*) echo '8 8388608 0'; exit 0 ;;
        *:/out) : > "${arg%:/out}/sdcard.img" ;;
    esac
done
MOCK
chmod +x "$TMP_DIR/bin/docker"
export PATH="$TMP_DIR/bin:$PATH"
export DOCKER_LOG="$TMP_DIR/docker.log"

check_target() {
    local product="$1" board="$2" target="$3" defconfig="$4" output
    local args=(--product "$product")
    if [ -n "$board" ]; then args+=(--board "$board"); fi
    : > "$DOCKER_LOG"
    SKIP_IMAGE_BUILD=1 bash "$PROJECT/scripts/docker_build.sh" "${args[@]}" > "$TMP_DIR/build.log"
    grep -qx "DEFCONFIG=$defconfig" "$DOCKER_LOG"
    grep -qx "BUILD_TARGET=$target" "$DOCKER_LOG"
    if [ "$product" = chime ]; then
        output="$PROJECT/buildroot/output"
        grep -qx 'SRC=/work' "$DOCKER_LOG"
    else
        output="$PROJECT/buildroot/output/$target"
        grep -qx "SRC=/work/$target" "$DOCKER_LOG"
    fi
    [ -f "$output/sdcard.img" ]
    rm "$output/sdcard.img"
    if bash "$PROJECT/scripts/flash_sd.sh" "${args[@]}" /dev/unused > "$TMP_DIR/flash.log" 2>&1; then
        echo 'ERROR: flash accepted a missing image' >&2
        exit 1
    fi
    grep -q "Image not found at: $output/sdcard.img" "$TMP_DIR/flash.log"
}

check_target chime "" chime openchime_rpi0w_defconfig
check_target ring "" ring openchime_ring_rpi02w_defconfig
check_target chime rpi0w chime openchime_rpi0w_defconfig
check_target ring rpi02w ring openchime_ring_rpi02w_defconfig
check_target ring rpi0w ring-rpi0w openchime_ring_rpi0w_defconfig
for script in docker_build.sh flash_sd.sh; do
    for args in '--product ring --board invalid' '--product chime --board rpi02w' '--board'; do
        read -r -a invalid_args <<< "$args"
        if bash "$PROJECT/scripts/$script" "${invalid_args[@]}" > "$TMP_DIR/error.log" 2>&1; then
            echo "ERROR: $script accepted $args" >&2
            exit 1
        fi
    done
done

echo '[test-ring-build-targets] passed'
