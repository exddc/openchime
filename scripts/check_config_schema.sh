#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

cd "$PROJECT_DIR"
python3 "$PROJECT_DIR/scripts/gen_chime_config_schema.py" --check
echo "[config-schema] generated files, inventory, chime.conf, and init defaults match"
