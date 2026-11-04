#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TMP_DIR="$(mktemp -d)"
trap 'rm -rf "$TMP_DIR"' EXIT
mkdir -p "$TMP_DIR/project/scripts" "$TMP_DIR/project/buildroot/output/ring-rpi0w" "$TMP_DIR/bin"
cp "$SCRIPT_DIR/flash_sd.sh" "$TMP_DIR/project/scripts/"
printf 'Ring test image\n' > "$TMP_DIR/project/buildroot/output/ring-rpi0w/sdcard.img"

cat > "$TMP_DIR/bin/mock" <<'PY'
#!/usr/bin/env python3
import os
from pathlib import Path
import plistlib
import shutil
import sys

name = Path(sys.argv[0]).name
args = sys.argv[1:]
mode = os.environ['FLASH_TEST_MODE']
folder = Path(os.environ['FLASH_TEST_DIR'])

def record(value):
    with (folder / 'calls').open('a') as output:
        output.write(value + '\n')

if name == 'plutil':
    try:
        value = plistlib.loads(sys.stdin.buffer.read())
        for key in args[1].split('.'):
            value = value[int(key)] if isinstance(value, list) else value[key]
        print(str(value).lower() if isinstance(value, bool) else value)
    except (KeyError, IndexError):
        sys.exit(1)
elif name == 'diskutil':
    if args[:2] == ['info', '-plist']:
        if args[2] == '/':
            info = {'ParentWholeDisk': 'disk3', 'APFSPhysicalStores': [{'APFSPhysicalStore': 'disk0s2'}]}
        else:
            info = {'WholeDisk': True, 'RemovableMediaOrExternalDevice': mode != 'internal',
                    'WritableMedia': mode != 'readonly', 'TotalSize': 1 if mode == 'small' else 7818182656}
        sys.stdout.buffer.write(plistlib.dumps(info))
    elif args == ['list', 'physical']:
        print('/dev/disk16 (internal, physical): Built In SDXC Reader, 7.8 GB')
    elif args[0] == 'list':
        print('/dev/disk16: Built In SDXC Reader, 7.8 GB')
    elif args[0] == 'unmountDisk':
        record('unmount')
        count = (folder / 'calls').read_text().splitlines().count('unmount')
        failed = mode == 'unmount_fail' or (mode == 'verify_unmount_fail' and count == 2)
        sys.exit(1 if failed else 0)
    elif args[0] == 'eject':
        record('eject')
    else:
        sys.exit(2)
elif name == 'sudo':
    if args[0] == 'dd':
        record('write')
        if mode == 'write_fail':
            sys.exit(1)
        source = next(arg[3:] for arg in args if arg.startswith('if='))
        target = next(arg[3:] for arg in args if arg.startswith('of='))
        assert target == '/dev/rdisk16'
        shutil.copyfile(source, folder / 'card')
        with (folder / 'card').open('ab') as card:
            card.write(b'Unused card space')
        if mode == 'corrupt':
            with (folder / 'card').open('r+b') as card:
                card.write(b'X')
    elif args[0] == 'head':
        assert args[1] == '-c' and args[-1] == '/dev/rdisk16'
        record('verify')
        if mode == 'read_fail':
            sys.exit(1)
        with (folder / 'card').open('rb') as card:
            sys.stdout.buffer.write(card.read(int(args[2])))
    else:
        sys.exit(2)
elif name != 'sync':
    sys.exit(2)
PY
chmod +x "$TMP_DIR/bin/mock"
for tool in diskutil plutil sudo sync; do
    ln -s mock "$TMP_DIR/bin/$tool"
done
export PATH="$TMP_DIR/bin:$PATH"
export FLASH_TEST_DIR="$TMP_DIR"

run_flash() {
    export FLASH_TEST_MODE="$1"
    : > "$TMP_DIR/calls"
    printf '/dev/disk16\nyes\n' | bash "$TMP_DIR/project/scripts/flash_sd.sh" --product ring --board rpi0w > "$TMP_DIR/output" 2>&1
}

run_flash good || { cat "$TMP_DIR/output" >&2; exit 1; }
grep -q 'Built In SDXC Reader' "$TMP_DIR/output"
grep -q 'Read-back verification passed' "$TMP_DIR/output"
printf 'unmount\nwrite\nunmount\nverify\neject\n' > "$TMP_DIR/expected"
cmp "$TMP_DIR/expected" "$TMP_DIR/calls"

for mode in corrupt read_fail unmount_fail verify_unmount_fail write_fail readonly internal small; do
    if run_flash "$mode"; then
        echo "ERROR: accepted $mode" >&2
        exit 1
    fi
    case "$mode" in
        corrupt|read_fail)
            expected='Read-back verification failed'
            grep -qx verify "$TMP_DIR/calls"
            ;;
        unmount_fail) expected='Cannot unmount' ;;
        verify_unmount_fail)
            expected='before verification'
            grep -qx write "$TMP_DIR/calls"
            if grep -qx verify "$TMP_DIR/calls"; then exit 1; fi
            ;;
        write_fail)
            expected='Failed to write image'
            grep -qx write "$TMP_DIR/calls"
            ;;
        readonly) expected='Target is read-only' ;;
        internal) expected='non-removable internal disk' ;;
        small) expected='Image is larger' ;;
    esac
    grep -Fq "$expected" "$TMP_DIR/output"
    if grep -qx eject "$TMP_DIR/calls"; then
        echo "ERROR: ejected after $mode" >&2
        exit 1
    fi
    case "$mode" in
        readonly|internal|small|unmount_fail)
            if grep -qx write "$TMP_DIR/calls"; then
                echo "ERROR: wrote after $mode" >&2
                exit 1
            fi
            ;;
    esac
done

export FLASH_TEST_MODE=good
: > "$TMP_DIR/calls"
if printf 'yes\n' | bash "$TMP_DIR/project/scripts/flash_sd.sh" --product ring --board rpi0w /dev/disk0 > "$TMP_DIR/output" 2>&1; then
    echo 'ERROR: accepted the physical APFS boot disk' >&2
    exit 1
fi
grep -q 'Refusing to flash the boot disk' "$TMP_DIR/output"
[ ! -s "$TMP_DIR/calls" ]
echo '[test-flash-sd] passed'
