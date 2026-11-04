#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
INIT_SCRIPT="$SCRIPT_DIR/../buildroot/board/common/rootfs_overlay/etc/init.d/S41timesync"
source /dev/stdin <<< "$(sed '/^case "\$1" in/,$d' "$INIT_SCRIPT")"
CALLS_FILE="$(mktemp)"
trap 'rm "$CALLS_FILE"' EXIT

timeout() {
    [ "$1 $2 $3" = '-s KILL 15' ] || return 1
    shift 3
    if [ "$NTP_TIMEOUT" -eq 1 ]; then
        printf 'timeout\n' >> "$CALLS_FILE"
        return 124
    fi
    "$@"
}

ntpd() {
    if [ "$1" = --version ]; then
        if [ "$NTP_KIND" = full ]; then
            echo 'ntpd 4.2.8p17'
            return 0
        fi
        return 1
    fi
    printf 'ntpd %s\n' "$*" >> "$CALLS_FILE"
    return "$NTP_RESULT"
}

busybox() {
    printf 'busybox %s\n' "$*" >> "$CALLS_FILE"
    return "$BUSYBOX_RESULT"
}

check_sync() {
    local expected_status="$1"
    shift
    : > "$CALLS_FILE"
    local status=0
    sync_with_server clock.example.test || status=$?
    [ "$status" -eq "$expected_status" ]
    [ "$(cat "$CALLS_FILE")" = "$(printf '%s\n' "$@")" ]
}

NTP_KIND=full
NTP_RESULT=0
BUSYBOX_RESULT=1
NTP_TIMEOUT=0
check_sync 0 'ntpd -n -gq -c /proc/self/fd/0 -- clock.example.test'

NTP_RESULT=1
check_sync 1 'ntpd -n -gq -c /proc/self/fd/0 -- clock.example.test' \
    'busybox ntpd -n -q -p clock.example.test'

BUSYBOX_RESULT=0
check_sync 0 'ntpd -n -gq -c /proc/self/fd/0 -- clock.example.test' \
    'busybox ntpd -n -q -p clock.example.test'

NTP_KIND=busybox
NTP_RESULT=0
BUSYBOX_RESULT=1
check_sync 0 'ntpd -n -q -p clock.example.test'

NTP_KIND=full
NTP_TIMEOUT=1
check_sync 1 'timeout' 'busybox ntpd -n -q -p clock.example.test'

for config in openchime_rpi0w_defconfig openchime_ring_rpi0w_defconfig openchime_ring_rpi02w_defconfig; do
    grep -qx 'BR2_PACKAGE_BUSYBOX_CONFIG_FRAGMENT_FILES="$(BR2_EXTERNAL_OPENCHIME_PATH)/board/common/busybox.fragment"' \
        "$SCRIPT_DIR/../buildroot/configs/$config"
done
grep -qx 'CONFIG_TIMEOUT=y' "$SCRIPT_DIR/../buildroot/board/common/busybox.fragment"

echo 'PASS: configured NTP servers and BusyBox fallback'
