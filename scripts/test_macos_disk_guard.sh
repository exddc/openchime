#!/bin/bash
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
TEST_DIR="$(mktemp -d)"
DISK=""
OTHER_DISK=""
GUARD_PID=""
cleanup() {
    if [ -n "$GUARD_PID" ]; then kill -TERM "$GUARD_PID" 2>/dev/null || true; wait "$GUARD_PID" 2>/dev/null || true; fi
    if [ -n "$OTHER_DISK" ]; then hdiutil detach "$OTHER_DISK" >/dev/null; fi
    if [ -n "$DISK" ]; then hdiutil detach "$DISK" >/dev/null; fi
    rm -r "$TEST_DIR"
}
trap cleanup EXIT
xcrun clang -std=c11 -Wall -Wextra -Werror -O2 "$SCRIPT_DIR/macos_disk_guard.c" \
    -framework DiskArbitration -framework CoreFoundation -o "$TEST_DIR/disk-guard"
hdiutil create -size 32m -fs MS-DOS -layout MBRSPUD -volname FLASH_TEST "$TEST_DIR/card.dmg" >/dev/null
hdiutil attach -nomount -plist "$TEST_DIR/card.dmg" > "$TEST_DIR/attach.plist"
DISK="$(plutil -extract system-entities.0.dev-entry raw -o - "$TEST_DIR/attach.plist")"
DISK="/dev/$(diskutil info -plist "$DISK" | plutil -extract ParentWholeDisk raw -o - -)"
PARTITION="${DISK}s1"
diskutil mount readOnly "$PARTITION" >/dev/null
diskutil unmountDisk "$DISK" >/dev/null
hdiutil create -size 32m -fs MS-DOS -layout MBRSPUD -volname FLASH_OTHER "$TEST_DIR/other.dmg" >/dev/null
hdiutil attach -nomount -plist "$TEST_DIR/other.dmg" > "$TEST_DIR/other.plist"
OTHER_DISK="$(plutil -extract system-entities.0.dev-entry raw -o - "$TEST_DIR/other.plist")"
OTHER_DISK="/dev/$(diskutil info -plist "$OTHER_DISK" | plutil -extract ParentWholeDisk raw -o - -)"
"$TEST_DIR/disk-guard" "$DISK" /bin/bash -c '
    set -e
    if diskutil mount "$1" > "$3" 2>&1; then echo "ERROR: mounted a guarded disk" >&2; exit 1; fi
    grep -Fq "Image write and verification in progress" "$3"
    diskutil mount readOnly "${2}s1" >/dev/null
    diskutil unmountDisk "$2" >/dev/null
' _ "$PARTITION" "$OTHER_DISK" "$TEST_DIR/mount.log"
diskutil mount readOnly "$PARTITION" >/dev/null
diskutil unmountDisk "$DISK" >/dev/null
if "$TEST_DIR/disk-guard" "$DISK" /bin/sh -c 'exit 7'; then
    echo 'ERROR: swallowed child failure' >&2
    exit 1
else
    [ "$?" -eq 7 ]
fi
if "$TEST_DIR/disk-guard" "$DISK" /bin/sh -c 'kill -STOP $$'; then
    echo 'ERROR: stopped child returned success' >&2
    exit 1
fi
"$TEST_DIR/disk-guard" "$DISK" /bin/bash -c 'echo $$ > "$1"; exec sleep 60' _ "$TEST_DIR/child.pid" &
GUARD_PID=$!
for attempt in {1..50}; do
    [ ! -f "$TEST_DIR/child.pid" ] || break
    [ "$attempt" -lt 50 ] || { echo "ERROR: guard child did not start" >&2; exit 1; }
    sleep 0.1
done
[ -f "$TEST_DIR/child.pid" ]
CHILD_PID="$(cat "$TEST_DIR/child.pid")"
kill -TERM "$GUARD_PID"
if wait "$GUARD_PID"; then
    echo 'ERROR: cancellation returned success' >&2
    exit 1
fi
GUARD_PID=""
if kill -0 "$CHILD_PID" 2>/dev/null; then
    echo 'ERROR: child survived cancellation' >&2
    exit 1
fi
diskutil mount readOnly "$PARTITION" >/dev/null
diskutil unmountDisk "$DISK" >/dev/null
echo '[test-macos-disk-guard] passed'
