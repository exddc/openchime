# Open Chime platform

`platform/` is the product-agnostic C++ layer. The tree uses this directory instead of expanding `common/`: headers already live under `oc/`, and after HTTP/TLS/JSON/Wi-Fi/mDNS/apply moved here the name matches the dependency (platform, then product, then board).

Public types stay in namespace `oc::`. Do not introduce a second generic namespace.

## Dependency direction

```
board (Buildroot image, overlays, init)     # not a C++ target
    ^
chime (product policy, routes, schema)      # chime, chime-webd, chime-migrate
    ^
platform (oc_platform, oc_platform_http)    # no chime/ includes or links
```

- `oc_platform` — logging, MQTT transport, signal handling, filesystem/environment/time, key/value schema primitives (`kv_config`, `kv_document`, atomic writes), process runner (`oc::process::Runner`), apply-job infrastructure. Does not link OpenSSL or Chime release macros.
- `oc_platform_http` — HTTP parser/router, JSON adapter, TLS server, static UI files, Wi-Fi scan, mDNS. Optional (`OC_BUILD_HTTP`, default ON). Linked by configuration daemons only, so the ring binary does not take OpenSSL.
- CMake fails configure if any file under `platform/include`, `platform/src`, or `platform/tests` includes `chime/`, if `oc_platform` / `oc_platform_http` link a `chime*` target, or if `CHIME_*` / `OPENCHIME_*` compile definitions reach platform targets.
- Product binaries stay `chime` (ring) and `chime-webd` (configuration). They remain separate processes.
- A platform-only graph is `cmake -DOC_BUILD_CHIME=OFF`. A TLS-free core is `cmake -DOC_BUILD_CHIME=OFF -DOC_BUILD_HTTP=OFF`. Native CI runs `./scripts/platform_only_ci.sh`, which configures, builds, and tests a staged tree that does not contain `chime/`, then repeats that for a core-only OpenSSL-free configure. `./scripts/test_platform_link_guard.sh` checks that a Chime target added after `platform/` still fails configure.

Board-specific work (kernel, device tree, rootfs overlay, Pi quirks) stays in `buildroot/` until TW-356 documents that boundary. Do not add Bell here.

## Product hooks

Route registration: implement `oc::http::ProductRoutes` and call `Register(router)`. The platform router is a method/path table. Adding a route does not edit platform internals.

Apply: implement `oc::apply::ProductApply` (or pass `std::vector<oc::apply::Step>` to `oc::apply::JobRunner`). Platform owns job lifecycle; the product chooses the steps. Restart commands go through `oc::apply::ArgvCommand` and `oc::process::Runner` as argv arrays, never a shell. Concurrent apply requests coalesce onto the current job ID while a job is pending or running; there is no queue. `JobRunner::Stop()` refuses new work, cancels the worker, and joins it. Each apply step times out after `oc::process::kDefaultTimeout` (30s).

Process: `oc::process::Runner` runs an executable plus argv, with a 30s default timeout, bounded output, and stop-token cancellation. `PosixRunner` is the production implementation; tests inject `FakeRunner`. `Request.timeout` must be positive. `Command.arguments` excludes argv[0], which the runner supplies from `Command.executable`. Arguments pass literally, including spaces and shell punctuation. The legacy restart environment variables accept whitespace-separated words; shell quoting, redirection, and operators are rejected at startup.

Capture is optional and limited to 64 KiB per stream by default. Excess bytes are drained and discarded, with a truncation flag for each stream. Uncaptured streams and stdin use `/dev/null`. A pre-cancelled request does not spawn a child. Children run in separate process groups; cancellation and timeout send SIGTERM to the group, then SIGKILL after the configured grace period (2s by default, 0 for immediate escalation). Normal completion preserves descendants created by service init scripts.

Chime wires apply cancellation into `TlsServer::Stop()`. Shutdown closes the listener, refuses further apply jobs, requests cancellation, and joins the apply worker before returning. Cancellation is checked at intervals of at most 20ms; SIGKILL follows within the 2s grace period plus polling and scheduling delay. The runner reaps the direct child before returning. This is a bound on termination escalation, not an absolute wall-clock guarantee if the kernel stalls child exit. Custom apply callbacks and injected runners must cooperate with the stop token; arbitrary blocking callbacks cannot provide this bound. Cancelled jobs have state `failed`, a cancellation error, and a finish timestamp.

Audio uses a 3s timeout per mixer attempt and a 30s playback timeout. Destruction cancels and joins playback. Mixer failures retain the existing software-volume fallback; playback failures and timeouts are logged and clear the playing flag.

### Device smoke check

After deploying the binaries to a test device, save network configuration in the authenticated UI and inspect apply status. Confirm the network restart finishes before the Chime restart and the job succeeds. Trigger a ring at two volumes and confirm audible playback and volume changes. During a running apply, stop `S45webd` and confirm it exits after cancellation, without leaving the restart command running. Native tests use an injected fake for apply and audio; these device checks require the target hardware.

Config: product schema and defaults stay in `chime/`. Persistence uses `oc::config` and `oc::util::AtomicWriteFile`.

## Example: add `GET /api/v1/ping`

```cpp
class PingRoutes final : public oc::http::ProductRoutes {
  public:
    void Register(oc::http::HttpRouter &router) override {
        router.Add("GET", "/api/v1/ping", [](const oc::http::HttpRequest &) {
            return oc::http::JsonHttpBody(200, oc::json::JsonValue::Object({
                                                   {"ok", oc::json::JsonValue::Bool(true)},
                                               }));
        });
    }
};

oc::http::HttpRouter router;
PingRoutes ping;
ping.Register(router);
```

The same pattern is covered by `platform/tests/product_route_test.cpp`. Chime registers its API the same way in `chime::webd::WebApi::Register`.
