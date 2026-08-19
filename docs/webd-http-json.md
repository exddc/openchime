# HTTP and JSON in chime-webd

For contributors who add administration endpoints or change `chime-webd` dependencies. TW-351 required a maintained JSON parser and either a maintained HTTP library or a split of the in-tree stack behind tested interfaces.

## Decision

`chime-webd` now parses and emits JSON through cJSON 1.7.19 (MIT), vendored at `chime/third_party/cJSON/` and compiled by the same CMake graph used for native CI and Buildroot. `chime/src/webd/json.cpp` is a cJSON adapter only. It no longer contains a project-owned JSON grammar, and handlers no longer concatenate JSON strings. HTTP JSON response helpers live in `json_http.cpp`. Product field readers live in `json_validate.cpp`.

The HTTP server stays in-tree, split into parser, router, static-file serving, and product handlers. TLS accept, certificate generation, and `SSL_shutdown` on connection close remain in `web_server.cpp`. That split is the fallback the ticket allowed. The measurements below are why a third-party HTTP library was not a fit.

## Measurements

Host toolchain: macOS arm64, AppleClang, CMake `Release`, stripped with `strip(1)`. These numbers are a host proxy for the Pi Zero W image, not an ARMv6 Buildroot binary. The configured rootfs budget is `BR2_TARGET_ROOTFS_EXT2_SIZE=256M` in `buildroot/configs/openchime_rpi0w_defconfig`.

Stripped `chime-webd`:

| Build | Bytes |
| --- | ---: |
| Baseline (origin/main, handmade JSON + monolithic HTTP) | 372400 |
| This change (cJSON adapter + split HTTP) | 427344 |
| Delta | +54944 |

A 54 KiB increase is about 0.02% of the 256 MiB rootfs. This session did not rebuild the full SD image, so rootfs usage is inferred from that binary delta. Confirm `chime-webd` still starts after flashing.

Library probes, same host flags, overhead versus a stripped empty `main`:

| Probe | Stripped bytes | Overhead vs empty |
| --- | ---: | ---: |
| empty `main` | 16824 | 0 |
| cJSON 1.7.19 parse + print | 68824 | 52000 |
| nlohmann/json 3.11.3 parse + dump | 121312 | 104488 |
| cpp-httplib 0.53.1 `SSLServer` stub (OpenSSL, no `listen`) | 370160 | 353336 |

cJSON's overhead matches the `chime-webd` delta. nlohmann/json was about 2x cJSON on this host. cpp-httplib's TLS server templates alone were 353 KiB, nearly the size of the previous `chime-webd` binary.

Runtime memory for cpp-httplib was not sampled with `rss`. The header default thread pool is `max(8, hardware_concurrency())`. glibc's default pthread stack is commonly 8 MiB, so eight idle workers can reserve on the order of 64 MiB of virtual memory on a 512 MiB Pi Zero W. The current daemon accepts connections one at a time on the listener thread.

## Why not Mongoose or cpp-httplib

Mongoose 7.18 is dual-licensed GPL-2.0 or commercial ([mongoose.ws/licensing](https://mongoose.ws/licensing/)). Open Chime is MIT. Shipping Mongoose would force a GPL relicensing or a commercial license, so it is out.

cpp-httplib is MIT and talks to OpenSSL, which matches the ticket's evaluation request. Two properties made it a poor replacement here:

1. Size. A stub that only constructed `httplib::SSLServer` and registered three routes added 353 KiB. OpenSSL certificate generation would still live in-tree.
2. Concurrency. The library's default pool is eight worker threads. `chime-webd` is a control-plane daemon on a single-core 512 MiB board and already isolates ring playback in a separate process. A one-thread compile-time override would shrink the RAM concern and would not shrink the binary.

The in-tree parser is now small enough to test without opening sockets, which is what the malformed-request cases need.

## JSON

Handlers build `JsonValue` trees. `DumpJson` returns a success/error result instead of substituting JSON `null` on failure; `JsonHttpBody` maps a serialization failure to HTTP 500. `webui/` field names, status codes, and password redaction are unchanged. GET and POST `/api/v1/config/core` still return `wifi_password_set` and `mqtt_password_set`, never `wifi_password` or `mqtt_password`.

cJSON 1.7.19 is MIT and lives in two vendored files, so native CMake and the Buildroot `chime` package both compile it offline. The upstream archive is `https://github.com/DaveGamble/cJSON/archive/refs/tags/v1.7.19.tar.gz` (SHA-256 `7fa616e3046edfa7a28a32d5f9eacfd23f92900fe1f8ccd988c1662f30454562`). Keep `third_party/cJSON` off public include paths. On a case-insensitive filesystem that directory shadows the C++20 `<version>` header.

Number spellings are slightly looser than RFC 8259: `01` and `1.` parse as `1`. The previous hand parser rejected those. Config payloads from `webui/` use `JSON.stringify` / `Number()`, so they do not emit those forms. Trailing junk, truncated objects, and unknown escapes still fail. Outside strings, only space, tab, CR, and LF are whitespace. Unescaped bytes below `0x20` are rejected, including inside strings.

`\u00e9` now decodes to UTF-8 `é` instead of `?`.

## HTTP layout

| Piece | Role |
| --- | --- |
| `http_parse.cpp` | Request-line, headers, `Content-Length`, body limits |
| `http_router.cpp` | Method/path table used by TW-350-style registration |
| `json.cpp` | cJSON adapter (`ParseJson` / `DumpJson` / object field lookup) |
| `json_http.cpp` | `JsonHttpBody` / `JsonHttpError` |
| `json_validate.cpp` | Required/optional field readers for product payloads |
| `static_files.cpp` | UI dist serving with path containment |
| `api_handlers.cpp` | Product endpoints |
| `web_server.cpp` | TLS listen, accept, `SSL_shutdown`, process shutdown |

Supported methods are GET, POST, and PUT. Anything else returns 405. Request-line limit is 8192 bytes, header block 64 KiB, header count 100, body 2 MiB. JSON POST bodies on `/api/v1/config/core` and `/api/v1/ring/sounds/select` are rejected above 64 KiB; WAV upload still uses the 2 MiB HTTP cap. Any `Transfer-Encoding` header is rejected; this server does not decode transfer codings and will not accept `Transfer-Encoding` together with `Content-Length`. Paths with `..` or encoded `/`, `\`, or NUL are rejected at parse time. Static files also resolve `weakly_canonical` under the UI root, including `index.html` for `/` and SPA fallback.

Exact routes beat prefixes. `POST /api/v1/system/version` is 405 because a GET exact route exists; `GET /api/v1/system/reboot` is 501 from the reserved prefix. The router matches exact path, then longest prefix, then fallback. It does not emit JSON; `WebApi` injects JSON 404/405 bodies.

Register new product routes with `HttpRouter::Add` / `AddPrefix` (or `WebApi::router()` after construction). Do not add a method/path `if` chain in `WebServer`.

## Tests

CTest covers malformed request lines, invalid `Content-Length`, oversized bodies, blank header lines, and unsupported methods. It also covers path traversal, malformed JSON, a large-array JSON conversion, static-file containment (SPA fallback, `/assets/` 404, outbound symlink), a config GET/POST round trip that asserts password redaction, ring-sound upload/select, and a TLS start/stop smoke test (ephemeral port, self-signed cert, `Connection: close`, `WebServer::Stop()`). Run them with `./scripts/chime_ci.sh` or `ctest --test-dir chime/build-ci --output-on-failure --no-tests=error`.
