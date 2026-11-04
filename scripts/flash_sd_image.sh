#!/bin/bash
set -euo pipefail

IMAGE_PATH="$1"
RAW_DISK="$2"
IMAGE_BYTES="$3"
DISK="$4"
log() { echo "[flash] $*"; }
error() { echo "[flash] ERROR: $*" >&2; exit 1; }

log "Unmounting $DISK..."
diskutil unmountDisk "$DISK" || error "Cannot unmount $DISK"
log "Flashing to $RAW_DISK (this takes ~1-2 minutes)..."
dd if="$IMAGE_PATH" of="$RAW_DISK" bs=4m status=progress conv=fsync || error "Failed to write image to $DISK"
log "Syncing..."
sync
log "Verifying $IMAGE_BYTES bytes from $RAW_DISK..."
VERIFY_STARTED_AT="$(date +%s)"
head -c "$IMAGE_BYTES" "$RAW_DISK" | cmp "$IMAGE_PATH" - || error "Read-back verification failed for $DISK"
log "Read-back verification passed ($(( $(date +%s) - VERIFY_STARTED_AT ))s)"
log "Ejecting..."
diskutil eject "$DISK"
