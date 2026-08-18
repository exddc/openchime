# Chime Service

`chime` is a minimal MQTT-to-audio daemon for the Raspberry Pi Zero W.
`chime-webd` is a lightweight HTTPS control-plane daemon for web configuration APIs/UI.

Operational runbook: [docs/reliability-runbook.md](../docs/reliability-runbook.md).

## Local CI Checks

Run the same checks as GitHub chime CI from the repo root:

```bash
./scripts/chime_ci.sh
```

This configures the native CMake build with `-DOC_BUILD_TESTS=ON`, builds, and runs `ctest --output-on-failure`. A plain `cmake -S .` leaves tests off, so production and offline configures do not download doctest.

CMake is the single build graph for `oc_common`, `chime_core`, `chime_webd_core`, `chime`, and `chime-webd`. Native CI, `scripts/local_chime.sh`, and the Buildroot `chime` package all consume that graph—add a source to a CMake target and it is included everywhere.

Useful options:
- `./scripts/chime_ci.sh --fix-format` to apply `clang-format` to the checked files.
- `./scripts/chime_ci.sh --skip-build` to run CTest against an existing build directory.
- `./scripts/chime_ci.sh --skip-tests` to skip CTest.
- `CHIME_CI_SCOPE=changed CHIME_CI_BASE_REF=origin/main ./scripts/chime_ci.sh` to lint/format only files changed from a base ref.

Native C++ tests can also be run directly:

```bash
cmake -S . -B chime/build-ci -G Ninja -DCMAKE_BUILD_TYPE=Release -DOC_BUILD_TESTS=ON
cmake --build chime/build-ci
ctest --test-dir chime/build-ci --output-on-failure --no-tests=error
```

For repository-wide formatting/lint checks (C/C++ + webui Biome), run:

```bash
./scripts/lint_format_ci.sh
```

## Runtime Behavior

1. Loads config from `/etc/chime.conf` (or `$CHIME_CONFIG`).
2. If `mqtt_host` is empty, waits without connecting and logs that MQTT is not configured. `chime-webd` remains available for setup.
3. Otherwise connects to the MQTT broker and subscribes to configured topics.
4. When a message arrives on `ring_topic`, plays `sound_path` using `aplay`.
5. Publishes `heartbeat_topic` every `heartbeat_interval` seconds.
6. Automatically reconnects to MQTT after disconnect or loop errors.

## Web Platform (`chime-webd`)

- Serves HTTPS UI/API on port `8443`.
- Hosts current v1 endpoints:
  - `GET /`
  - `GET /api/v1/config/core`
  - `POST /api/v1/config/core`
  - `GET /api/v1/wifi/scan`
  - `GET /api/v1/mqtt/topics` (observed MQTT topics for ring-topic suggestions)
- Reserves `/api/v1/system/*`, `/api/v1/device/*`, and `/api/v1/diagnostics/*` for future API expansion (`501` responses in v1).
- Uses self-signed TLS cert/key at:
  - `/etc/chime-web/tls/cert.pem`
  - `/etc/chime-web/tls/key.pem`
- Optional static UI override:
  - Set `CHIME_WEBD_UI_DIST_DIR` to serve built web assets (for example Svelte
    `dist/`) instead of the embedded fallback UI.
- Runs as a separate process from `chime` for ring-path reliability isolation.

## Reliability Logging

All ring-service logs go to `/var/log/chime.log` through the init supervisor (`S99chime`). `chime-webd` logs to `/var/log/chime-web.log`. Command examples are in the [reliability runbook](../docs/reliability-runbook.md).

The daemon logs:
- Service lifecycle (`service starting`, config loaded, shutdown reason, `service stopped`)
- Unconfigured MQTT (`mqtt_host` empty: wait, no broker connection)
- MQTT lifecycle (connect attempts, successful connection, subscribe results, disconnects, loop errors, reconnect attempts, heartbeat publish success/fail)
- Message traffic (topic, qos, retain, payload length and sanitized payload)
- Ring handling (`ring received`, audio playback start, playback completion/failure, dedup when already playing)
- WiFi state (`operstate` and `carrier`) and changes/dropouts for the configured interface
- Periodic health summary every 60 seconds (message counters, reconnect counters, connection state)

## Config Keys

See `/etc/chime.conf` for image defaults. Empty `mqtt_host` means the broker is not configured.

Daemon keys:
- `mqtt_host`, `mqtt_port`, `mqtt_client_id`
- `mqtt_username`, `mqtt_password` (optional broker auth)
- `mqtt_tls_enabled`, `mqtt_tls_validate_certificate`
- `mqtt_tls_ca_file`, `mqtt_tls_cert_file`, `mqtt_tls_key_file`
- `mqtt_topics` (comma-separated)
- `mqtt_subscribe_qos` (0-2)
- `heartbeat_interval` (0 disables)
- `heartbeat_topic`
- `ring_topic`
  - Supports MQTT topic filters (`+` and `#`) for matching incoming message topics
- `sound_path`
- `volume_bell` (0-100, bell/ring events)
- `volume_notifications` (0-100, startup/notification category)
- `volume_other` (0-100, fallback category)
- `audio_enabled`
- `wifi_interface`
- `wifi_check_interval` (0 disables WiFi state checks)

Init-service keys (used by `S41timesync` and `S99chime`):
- `ntp_servers` (comma-separated)
- `time_http_urls` (comma-separated HTTP URLs used as fallback Date source)
- `time_sync_retries`, `time_sync_retry_delay`, `time_sync_interval`
- `log_max_bytes`, `log_rotate_keep`, `log_rotate_check_interval`
