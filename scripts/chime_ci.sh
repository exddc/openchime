#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

BUILD_DIR="${BUILD_DIR:-$PROJECT_DIR/chime/build-ci}"
BUILD_TYPE="${BUILD_TYPE:-Release}"
SCOPE="${CHIME_CI_SCOPE:-all}"
BASE_REF="${CHIME_CI_BASE_REF:-}"
FIX_FORMAT=0
SKIP_FORMAT=0
SKIP_TIDY=0
SKIP_BUILD=0
SKIP_TESTS=0
CLANG_FORMAT_TOOL="${CLANG_FORMAT_TOOL:-clang-format-18}"
CLANG_TIDY_TOOL="${CLANG_TIDY_TOOL:-clang-tidy-18}"

log() {
  echo "[chime-ci] $*"
}

error() {
  echo "[chime-ci] ERROR: $*" >&2
  exit 1
}

usage() {
  cat <<USAGE
Usage: $(basename "$0") [options]

Runs local checks equivalent to the chime CI pipeline.

Options:
  --scope <all|changed>     File scope for format/tidy checks (default: all)
  --base-ref <git-ref>      Base ref for --scope changed (or CHIME_CI_BASE_REF)
  --build-dir <path>        Build directory (default: chime/build-ci)
  --build-type <type>       CMake build type (default: Release)
  --fix-format              Apply clang-format in place instead of check-only
  --skip-format             Skip clang-format
  --skip-tidy               Skip clang-tidy
  --skip-build              Skip compile (CTest still runs unless --skip-tests)
  --skip-tests              Skip CTest
  -h, --help                Show this help text

Examples:
  ./scripts/chime_ci.sh
  ./scripts/chime_ci.sh --fix-format
  CHIME_CI_SCOPE=changed CHIME_CI_BASE_REF=origin/main ./scripts/chime_ci.sh
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
      --build-dir)
        [ $# -ge 2 ] || error "--build-dir requires a value"
        BUILD_DIR="$2"
        shift 2
        ;;
      --build-type)
        [ $# -ge 2 ] || error "--build-type requires a value"
        BUILD_TYPE="$2"
        shift 2
        ;;
      --fix-format)
        FIX_FORMAT=1
        shift
        ;;
      --skip-format)
        SKIP_FORMAT=1
        shift
        ;;
      --skip-tidy)
        SKIP_TIDY=1
        shift
        ;;
      --skip-build)
        SKIP_BUILD=1
        shift
        ;;
      --skip-tests)
        SKIP_TESTS=1
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

is_chime_format_file() {
  local path="$1"
  case "$path" in
    chime/third_party/*) return 1 ;;
    chime/include/chime/generated/*) return 1 ;;
    chime/tests/config_schema_contract_test.cpp) return 1 ;;
    chime/*|common/*) ;;
    *) return 1 ;;
  esac
  case "$path" in
    *.c|*.cc|*.cpp|*.h|*.hpp) return 0 ;;
    *) return 1 ;;
  esac
}

is_chime_tidy_file() {
  local path="$1"
  case "$path" in
    chime/src/*|common/src/*) ;;
    *) return 1 ;;
  esac
  case "$path" in
    *.cc|*.cpp) return 0 ;;
    *) return 1 ;;
  esac
}

collect_candidates() {
  if [ "$SCOPE" = "all" ]; then
    git ls-files -z -- chime common
    return
  fi

  git rev-parse --verify "${BASE_REF}^{commit}" >/dev/null 2>&1 || \
    error "Base ref does not resolve to a commit: $BASE_REF"

  local merge_base
  merge_base="$(git merge-base "$BASE_REF" HEAD)"
  [ -n "$merge_base" ] || error "Failed to find merge-base for $BASE_REF and HEAD"

  log "Using merge-base $merge_base (base ref: $BASE_REF)" >&2
  if ! git diff --quiet "$merge_base" HEAD -- .clang-format .clang-tidy; then
    log "Clang configuration changed; checking all C/C++ files" >&2
    git ls-files -z -- chime common
    return
  fi
  git diff --name-only --diff-filter=ACMR -z "$merge_base" HEAD -- chime common
}

collect_format_files() {
  local file
  collect_candidates | while IFS= read -r -d '' file; do
    if is_chime_format_file "$file" && [ -f "$PROJECT_DIR/$file" ]; then
      printf '%s\0' "$file"
    fi
  done
}

collect_tidy_files() {
  local file
  collect_candidates | while IFS= read -r -d '' file; do
    if is_chime_tidy_file "$file" && [ -f "$PROJECT_DIR/$file" ]; then
      printf '%s\0' "$file"
    fi
  done
}

run_clang_format() {
  [ "$SKIP_FORMAT" = "1" ] && {
    log "Skipping clang-format"
    return
  }

  require_tool "$CLANG_FORMAT_TOOL"

  local files=()
  local file_list
  file_list="$(mktemp)"
  if ! collect_format_files > "$file_list"; then
    rm -f "$file_list"
    return 1
  fi
  while IFS= read -r -d '' file; do
    files+=("$file")
  done < "$file_list"
  rm -f "$file_list"

  if [ "${#files[@]}" -eq 0 ]; then
    log "No chime/common C/C++ files found for clang-format"
    return
  fi

  "$CLANG_FORMAT_TOOL" --version
  log "clang-format files: ${#files[@]}"

  if [ "$FIX_FORMAT" = "1" ]; then
    "$CLANG_FORMAT_TOOL" -i "${files[@]}"
    log "Applied clang-format to ${#files[@]} files"
  else
    "$CLANG_FORMAT_TOOL" -n -Werror "${files[@]}"
    log "clang-format check passed"
  fi
}

discover_cmake_host_args() {
  CMAKE_HOST_ARGS=()
  if ! command -v brew >/dev/null 2>&1; then
    return
  fi

  local openssl_prefix mosq_prefix prefixes=""
  openssl_prefix="$(brew --prefix openssl@3 2>/dev/null || brew --prefix openssl 2>/dev/null || true)"
  mosq_prefix="$(brew --prefix mosquitto 2>/dev/null || true)"
  if [ -n "$openssl_prefix" ] && [ -d "$openssl_prefix" ]; then
    CMAKE_HOST_ARGS+=("-DOPENSSL_ROOT_DIR=$openssl_prefix")
    prefixes="$openssl_prefix"
    if [ -d "$openssl_prefix/lib/pkgconfig" ]; then
      export PKG_CONFIG_PATH="$openssl_prefix/lib/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"
    fi
  fi
  if [ -n "$mosq_prefix" ] && [ -d "$mosq_prefix" ]; then
    if [ -n "$prefixes" ]; then
      prefixes="$prefixes;$mosq_prefix"
    else
      prefixes="$mosq_prefix"
    fi
    if [ -d "$mosq_prefix/lib/pkgconfig" ]; then
      export PKG_CONFIG_PATH="$mosq_prefix/lib/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"
    fi
  fi
  if [ -n "$prefixes" ]; then
    CMAKE_HOST_ARGS+=("-DCMAKE_PREFIX_PATH=$prefixes")
  fi
}

configure_cmake() {
  require_tool cmake
  require_tool ninja

  local enable_tests=ON
  if [ "$SKIP_TESTS" = "1" ]; then
    enable_tests=OFF
  fi

  discover_cmake_host_args
  cmake --version
  cmake -S "$PROJECT_DIR" -B "$BUILD_DIR" \
    -G Ninja \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    -DOC_BUILD_TESTS="$enable_tests" \
    ${CMAKE_HOST_ARGS[@]+"${CMAKE_HOST_ARGS[@]}"}
}

run_clang_tidy() {
  [ "$SKIP_TIDY" = "1" ] && {
    log "Skipping clang-tidy"
    return
  }

  require_tool "$CLANG_TIDY_TOOL"

  local files=()
  local file_list
  file_list="$(mktemp)"
  if ! collect_tidy_files > "$file_list"; then
    rm -f "$file_list"
    return 1
  fi
  while IFS= read -r -d '' file; do
    files+=("$file")
  done < "$file_list"
  rm -f "$file_list"

  if [ "${#files[@]}" -eq 0 ]; then
    log "No chime/common C++ sources found for clang-tidy"
    return
  fi

  "$CLANG_TIDY_TOOL" --version
  log "clang-tidy files: ${#files[@]}"
  "$CLANG_TIDY_TOOL" -p "$BUILD_DIR" "${files[@]}"
  log "clang-tidy passed"
}

run_build() {
  [ "$SKIP_BUILD" = "1" ] && {
    log "Skipping build"
    return
  }

  cmake --build "$BUILD_DIR" --config "$BUILD_TYPE"
  log "Build passed"
}

run_tests() {
  [ "$SKIP_TESTS" = "1" ] && {
    log "Skipping tests"
    return
  }

  require_tool ctest
  [ -d "$BUILD_DIR" ] || error "Build directory not found: $BUILD_DIR (build once before --skip-build)"
  ctest --test-dir "$BUILD_DIR" --output-on-failure --no-tests=error
  bash "$PROJECT_DIR/scripts/test_chime_config_fatal.sh" "$BUILD_DIR/bin/chime-migrate"
  log "Tests passed"
}

main() {
  parse_args "$@"

  git -C "$PROJECT_DIR" rev-parse --is-inside-work-tree >/dev/null 2>&1 || \
    error "Must run inside the repository"
  cd "$PROJECT_DIR"

  run_clang_format
  if [ "$SKIP_TIDY" = "1" ] && [ "$SKIP_BUILD" = "1" ]; then
    log "Skipping CMake configure (no tidy/build requested)"
  else
    configure_cmake
  fi
  run_clang_tidy
  run_build
  run_tests

  log "All requested checks passed"
}

main "$@"
