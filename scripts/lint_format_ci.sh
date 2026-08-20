#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

SCOPE="${CHIME_CI_SCOPE:-all}"
BASE_REF="${CHIME_CI_BASE_REF:-}"
FIX_FORMAT=0
SKIP_CXX=0
SKIP_WEBUI=0
SKIP_SHELL=0
CLANG_FORMAT_TOOL="${CLANG_FORMAT_TOOL:-clang-format-18}"

log() {
  echo "[lint-format-ci] $*"
}

error() {
  echo "[lint-format-ci] ERROR: $*" >&2
  exit 1
}

usage() {
  cat <<USAGE
Usage: $(basename "$0") [options]

Runs formatting and linting checks used by CI.

Options:
  --scope <all|changed>     File scope (default: all)
  --base-ref <git-ref>      Base ref for --scope changed (or CHIME_CI_BASE_REF)
  --fix-format              Apply formatters in place instead of check-only
  --skip-cxx                Skip C/C++ clang-format checks
  --skip-webui              Skip webui biome checks
  --skip-shell              Skip ShellCheck
  -h, --help                Show this help text
USAGE
}

parse_args() {
  while [ $# -gt 0 ]; do
    case "$1" in
      --scope)
        [ $# -ge 2 ] || error "--scope requires a value"
        SCOPE="$2"
        shift 2
        ;;
      --base-ref)
        [ $# -ge 2 ] || error "--base-ref requires a value"
        BASE_REF="$2"
        shift 2
        ;;
      --fix-format)
        FIX_FORMAT=1
        shift
        ;;
      --skip-cxx)
        SKIP_CXX=1
        shift
        ;;
      --skip-webui)
        SKIP_WEBUI=1
        shift
        ;;
      --skip-shell)
        SKIP_SHELL=1
        shift
        ;;
      -h|--help)
        usage
        exit 0
        ;;
      *)
        error "Unknown argument: $1"
        ;;
    esac
  done

  case "$SCOPE" in
    all|changed) ;;
    *) error "Invalid scope '$SCOPE' (expected all or changed)" ;;
  esac

  if [ "$SCOPE" = "changed" ] && [ -z "$BASE_REF" ]; then
    error "changed scope requires --base-ref or CHIME_CI_BASE_REF"
  fi
}

require_tool() {
  local tool="$1"
  command -v "$tool" >/dev/null 2>&1 || error "Required tool not found: $tool"
}

is_cxx_file() {
  local path="$1"
  case "$path" in
    chime/third_party/*) return 1 ;;
    platform/third_party/*) return 1 ;;
    chime/include/chime/generated/*) return 1 ;;
    chime/tests/config_schema_contract_test.cpp) return 1 ;;
    chime/*|platform/*) ;;
    *) return 1 ;;
  esac

  case "$path" in
    *.c|*.cc|*.cpp|*.h|*.hpp) return 0 ;;
    *) return 1 ;;
  esac
}

is_webui_file() {
  local path="$1"
  case "$path" in
    webui/*) ;;
    *) return 1 ;;
  esac

  case "$path" in
    webui/node_modules/*|webui/dist/*|webui/src/generated/*) return 1 ;;
  esac

  case "$path" in
    *.js|*.jsx|*.mjs|*.cjs|*.ts|*.tsx|*.json|*.jsonc|*.css|*.svelte) return 0 ;;
    *) return 1 ;;
  esac
}

is_shell_file() {
  local path="$1"
  local first_line

  case "$path" in
    scripts/*.sh) return 0 ;;
    buildroot/board/*) ;;
    *) return 1 ;;
  esac

  IFS= read -r first_line < "$PROJECT_DIR/$path" || true
  case "$first_line" in
    '#!'*sh*) return 0 ;;
    *) return 1 ;;
  esac
}

collect_candidates() {
  if [ "$SCOPE" = "all" ]; then
    git ls-files -z -- chime platform webui scripts buildroot/board
    return 0
  fi

  git rev-parse --verify "${BASE_REF}^{commit}" >/dev/null 2>&1 || \
    error "Base ref does not resolve to a commit: $BASE_REF"

  local merge_base
  merge_base="$(git merge-base "$BASE_REF" HEAD)"
  [ -n "$merge_base" ] || error "Failed to find merge-base for $BASE_REF and HEAD"

  log "Using merge-base $merge_base (base ref: $BASE_REF)" >&2
  git diff --name-only --diff-filter=ACMR -z "$merge_base" HEAD -- chime platform webui scripts buildroot/board
  return 0
}

cxx_format_configuration_changed() {
  [ "$SCOPE" = "changed" ] || return 1

  git rev-parse --verify "${BASE_REF}^{commit}" >/dev/null 2>&1 || \
    error "Base ref does not resolve to a commit: $BASE_REF"

  local merge_base
  merge_base="$(git merge-base "$BASE_REF" HEAD)"
  [ -n "$merge_base" ] || error "Failed to find merge-base for $BASE_REF and HEAD"

  ! git diff --quiet "$merge_base" HEAD -- .clang-format
}

emit_files() {
  local mode="$1"
  local file

  while IFS= read -r -d '' file; do
    [ -f "$PROJECT_DIR/$file" ] || continue

    case "$mode" in
      cxx)
        is_cxx_file "$file" && printf '%s\0' "$file"
        ;;
      webui)
        is_webui_file "$file" && printf '%s\0' "$file"
        ;;
      shell)
        is_shell_file "$file" && printf '%s\0' "$file"
        ;;
      *)
        error "Unknown collect mode: $mode"
        ;;
    esac
  done

  return 0
}

collect_files() {
  local mode="$1"

  if [ "$mode" = "cxx" ] && cxx_format_configuration_changed; then
    log ".clang-format changed; checking all C/C++ files" >&2
    emit_files "$mode" < <(git ls-files -z -- chime platform)
    return 0
  fi

  emit_files "$mode" < <(collect_candidates)

  return 0
}

load_file_array() {
  local mode="$1"
  local -n out_ref="$2"
  local tmp_file
  tmp_file="$(mktemp)"

  collect_files "$mode" > "$tmp_file"

  while IFS= read -r -d '' file; do
    out_ref+=("$file")
  done < "$tmp_file"

  rm -f "$tmp_file"
}

run_clang_format() {
  [ "$SKIP_CXX" = "1" ] && {
    log "Skipping C/C++ checks"
    return
  }

  require_tool "$CLANG_FORMAT_TOOL"

  local files=()
  load_file_array cxx files

  if [ "${#files[@]}" -eq 0 ]; then
    log "No C/C++ files selected for clang-format"
    return
  fi

  "$CLANG_FORMAT_TOOL" --version
  log "clang-format files: ${#files[@]}"

  if [ "$FIX_FORMAT" = "1" ]; then
    "$CLANG_FORMAT_TOOL" -i "${files[@]}"
    log "Applied clang-format"
  else
    "$CLANG_FORMAT_TOOL" -n -Werror "${files[@]}"
    log "clang-format check passed"
  fi
}

run_biome() {
  [ "$SKIP_WEBUI" = "1" ] && {
    log "Skipping webui checks"
    return
  }

  require_tool bun

  local files=()
  load_file_array webui files

  if [ "${#files[@]}" -eq 0 ]; then
    log "No webui files selected for biome"
    return
  fi

  log "biome files: ${#files[@]}"

  pushd "$PROJECT_DIR/webui" >/dev/null
  if [ "$FIX_FORMAT" = "1" ]; then
    bunx --bun @biomejs/biome check --write --no-errors-on-unmatched "${files[@]/#webui\//}"
    log "Applied biome formatting/lint fixes"
  else
    bunx --bun @biomejs/biome check --no-errors-on-unmatched "${files[@]/#webui\//}"
    log "biome check passed"
  fi
  popd >/dev/null
}

run_shellcheck() {
  [ "$SKIP_SHELL" = "1" ] && {
    log "Skipping ShellCheck"
    return
  }

  require_tool shellcheck

  local files=()
  load_file_array shell files

  if [ "${#files[@]}" -eq 0 ]; then
    log "No shell scripts selected for ShellCheck"
    return
  fi

  shellcheck --version
  log "ShellCheck files: ${#files[@]}"
  shellcheck --severity=warning "${files[@]}"
  log "ShellCheck passed"
}

main() {
  parse_args "$@"

  git -C "$PROJECT_DIR" rev-parse --is-inside-work-tree >/dev/null 2>&1 || \
    error "Must run inside the repository"
  cd "$PROJECT_DIR"

  run_clang_format
  run_biome
  run_shellcheck

  require_tool python3
  bash "$PROJECT_DIR/scripts/check_config_schema.sh"

  log "All requested checks passed"
}

main "$@"
