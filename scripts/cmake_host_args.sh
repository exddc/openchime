# Shared host-prefix discovery for native CMake configures.
#
# Sourced by:
#   scripts/chime_ci.sh
#   scripts/platform_only_ci.sh
#   scripts/test_platform_link_guard.sh
#
# Sets CMAKE_HOST_ARGS (OpenSSL root + CMAKE_PREFIX_PATH) and extends
# PKG_CONFIG_PATH for OpenSSL and Mosquitto when Homebrew is present.

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
