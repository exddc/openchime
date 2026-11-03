#!/bin/bash
# Post-build script for the Ring images
set -euo pipefail

BOARD_DIR="$(dirname "$0")"
TARGET_DIR="$1"
BOARD="${2:-rpi02w}"
CHIME_CONF_TEMPLATE="${BOARD_DIR}/../raspberrypi0w/rootfs_overlay/etc/chime.conf"
FACTORY_DIR="${TARGET_DIR}/usr/local/share/openchime/factory"

# shellcheck source=../common/post_build_lib.sh
. "${BOARD_DIR}/../common/post_build_lib.sh"

oc_assert_web_ui_dist
oc_chmod_binaries chime chime-webd chime-migrate
oc_require_ota_scripts
oc_require_executables \
    etc/init.d/S11modules \
    etc/init.d/S16powerlog \
    etc/init.d/S45webd
oc_common_post_build

FIRMWARE_DIR="${TARGET_DIR}/lib/firmware/brcm"
case "$BOARD" in
    rpi0w) chips="43430"; model="zero-w" ;;
    rpi02w) chips="43436 43436s"; model="zero-2-w" ;;
    *) echo "ERROR: unsupported Ring board: $BOARD" >&2; exit 1 ;;
esac
firmware_found=0
for chip in $chips; do
    if [ -f "${FIRMWARE_DIR}/brcmfmac${chip}-sdio.bin" ]; then
        firmware_found=1
    fi
    for ext in bin txt; do
        if [ -e "${FIRMWARE_DIR}/brcmfmac${chip}-sdio.${ext}" ]; then
            ln -sf "brcmfmac${chip}-sdio.${ext}" \
                "${FIRMWARE_DIR}/brcmfmac${chip}-sdio.raspberrypi,model-${model}.${ext}"
        fi
    done
done

[ "$firmware_found" -eq 1 ] || {
    echo "ERROR: Wi-Fi firmware missing for Ring board $BOARD (chips: $chips)" >&2
    exit 1
}

# Read-only root: mutable files live on /data and are reached through
# /etc/persistent (bind mount of /data/etc, see S31persistent).
mkdir -p "$FACTORY_DIR" \
    "${TARGET_DIR}/etc/persistent" \
    "${TARGET_DIR}/var/lib/ring" \
    "${TARGET_DIR}/var/lib/bluetooth"

[ -f "$CHIME_CONF_TEMPLATE" ] || { echo "ERROR: missing $CHIME_CONF_TEMPLATE" >&2; exit 1; }
{
    printf '%s\n' \
        "# Ring configuration. chime-webd edits this file with the Chime schema" \
        "# until a Ring configuration daemon exists. Only Wi-Fi and MQTT apply."
    sed -e '1,/^schema_version=/{/^#/d;}' \
        -e 's/^mqtt_client_id=chime$/mqtt_client_id=ring/' \
        "$CHIME_CONF_TEMPLATE"
} > "${FACTORY_DIR}/ring.conf"
chmod 600 "${FACTORY_DIR}/ring.conf"
ln -sfn /etc/persistent/ring.conf "${TARGET_DIR}/etc/ring.conf"

WPA_CONF="${TARGET_DIR}/etc/wpa_supplicant/wpa_supplicant.conf"
if [ -f "$WPA_CONF" ] && [ ! -L "$WPA_CONF" ]; then
    mv "$WPA_CONF" "${FACTORY_DIR}/wpa_supplicant.conf"
    chmod 600 "${FACTORY_DIR}/wpa_supplicant.conf"
fi
ln -sfn /etc/persistent/wpa_supplicant.conf "$WPA_CONF"

oc_write_release ring usr/local/share/openchime/factory/ring.conf
printf 'OPENCHIME_BOARD=%s\n' "$BOARD" >> "${TARGET_DIR}/etc/openchime-release"

echo "Post-build complete"
