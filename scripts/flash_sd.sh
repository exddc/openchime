#!/usr/bin/env bash
# Usage: ./scripts/flash_sd.sh [--product chime|ring] [--board rpi0w|rpi02w] [/dev/diskN]
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

log() { echo "[flash] $*"; }
error() { echo "[flash] ERROR: $*" >&2; exit 1; }

PRODUCT="chime"
BOARD=""
DISK=""
while [ $# -gt 0 ]; do
    case "$1" in
        --board)
            [ $# -ge 2 ] || error "--board requires a value"
            BOARD="$2"
            shift 2
            ;;
        --product)
            [ $# -ge 2 ] || error "--product requires a value"
            PRODUCT="$2"
            shift 2
            ;;
        *)
            [ -z "$DISK" ] || error "Unexpected argument: $1"
            DISK="$1"
            shift
            ;;
    esac
done

case "$PRODUCT" in
    chime)
        BOARD="${BOARD:-rpi0w}"
        [ "$BOARD" = "rpi0w" ] || error "Chime supports only --board rpi0w"
        IMAGE_PATH="$PROJECT_DIR/buildroot/output/sdcard.img"
        BOARD_NAME="Raspberry Pi Zero W"
        ;;
    ring)
        BOARD="${BOARD:-rpi02w}"
        case "$BOARD" in
            rpi0w)
                IMAGE_PATH="$PROJECT_DIR/buildroot/output/ring-rpi0w/sdcard.img"
                BOARD_NAME="Raspberry Pi Zero W"
                ;;
            rpi02w)
                IMAGE_PATH="$PROJECT_DIR/buildroot/output/ring/sdcard.img"
                BOARD_NAME="Raspberry Pi Zero 2 W"
                ;;
            *) error "Unknown Ring board '$BOARD' (expected rpi0w or rpi02w)" ;;
        esac
        ;;
    *)
        error "Unknown product '$PRODUCT' (expected chime or ring)"
        ;;
esac

# Check image exists
if [ ! -f "$IMAGE_PATH" ]; then
    error "Image not found at: $IMAGE_PATH
Run ./scripts/docker_build.sh --product $PRODUCT --board $BOARD first to build the image."
fi

IMAGE_SIZE=$(ls -lh "$IMAGE_PATH" | awk '{print $5}')
log "Image: $IMAGE_PATH ($IMAGE_SIZE)"

if [ -z "$DISK" ]; then
    echo ""
    echo "Available disks:"
    echo "================"
    diskutil list physical
    echo ""
    echo "Enter the disk device (e.g., /dev/disk4):"
    read -r DISK
fi

if [ -z "$DISK" ]; then
    error "No disk specified"
fi

if [[ ! "$DISK" =~ ^/dev/disk[0-9]+$ ]]; then
    error "Invalid disk format: $DISK (expected /dev/diskN)"
fi

DISK_INFO="$(diskutil info -plist "$DISK")" || error "Cannot inspect $DISK"
plist_value() { plutil -extract "$1" raw -o - -; }
[ "$(plist_value WholeDisk <<< "$DISK_INFO")" = true ] || error "Target must be a whole disk"
[ "$(plist_value RemovableMediaOrExternalDevice <<< "$DISK_INFO")" = true ] || \
    error "Refusing to flash a non-removable internal disk"
[ "$(plist_value WritableMedia <<< "$DISK_INFO")" = true ] || error "Target is read-only"
IMAGE_BYTES="$(wc -c < "$IMAGE_PATH" | tr -d '[:space:]')"
DISK_BYTES="$(plist_value TotalSize <<< "$DISK_INFO")"
[ "$DISK_BYTES" -ge "$IMAGE_BYTES" ] || error "Image is larger than $DISK"

BOOT_INFO="$(diskutil info -plist /)" || error "Cannot inspect the boot disk"
BOOT_DISK="$(plist_value ParentWholeDisk <<< "$BOOT_INFO")"
[ "$DISK" != "/dev/$BOOT_DISK" ] || error "Refusing to flash the boot disk!"
STORE_INDEX=0
while BOOT_STORE="$(plist_value "APFSPhysicalStores.$STORE_INDEX.APFSPhysicalStore" <<< "$BOOT_INFO" 2>/dev/null)"; do
    case "$BOOT_STORE" in
        "${DISK#/dev/}"s[0-9]*) error "Refusing to flash the boot disk!" ;;
    esac
    STORE_INDEX=$((STORE_INDEX + 1))
done

# Show disk info
echo ""
log "Target disk: $DISK"
diskutil list "$DISK"
echo ""

# Get disk size for display
log "Disk size: $DISK_BYTES bytes"

# Confirm
echo ""
echo "WARNING: This will ERASE ALL DATA on $DISK"
echo ""
read -p "Type 'yes' to continue: " CONFIRM

if [ "$CONFIRM" != "yes" ]; then
    log "Aborted."
    exit 1
fi

# Unmount all partitions
log "Unmounting $DISK..."
diskutil unmountDisk "$DISK" || error "Cannot unmount $DISK"

# Flash
RAW_DISK="${DISK/disk/rdisk}"
log "Flashing to $RAW_DISK (this takes ~1-2 minutes)..."

sudo dd if="$IMAGE_PATH" of="$RAW_DISK" bs=4m status=progress conv=fsync || error "Failed to write image to $DISK"

# Sync
log "Syncing..."
sync

diskutil unmountDisk "$DISK" || error "Cannot unmount $DISK before verification"
log "Verifying $IMAGE_BYTES bytes from $RAW_DISK..."
VERIFY_STARTED_AT="$(date +%s)"
sudo head -c "$IMAGE_BYTES" "$RAW_DISK" | cmp "$IMAGE_PATH" - || error "Read-back verification failed for $DISK"
log "Read-back verification passed ($(( $(date +%s) - VERIFY_STARTED_AT ))s)"

# Eject
log "Ejecting..."
diskutil eject "$DISK"

echo ""
log "✓ Flash complete!"
echo ""
echo "Next steps:"
echo "  1. Insert SD card into the $BOARD_NAME"
echo "  2. Connect FTDI for serial console (optional): screen /dev/tty.usbserial* 115200"
echo "  3. Power on the Pi"
echo "  4. Wait ~30 seconds for boot + WiFi connection"
echo "  5. Find the Pi: ping $PRODUCT.local (or check your router's DHCP leases)"
echo "  6. SSH in: ssh root@<ip-address>"
