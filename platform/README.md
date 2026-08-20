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

- `oc_platform` — logging, MQTT transport, signal handling, filesystem/environment/time, key/value schema primitives (`kv_config`, `kv_document`, atomic writes), apply-job infrastructure.
- `oc_platform_http` — HTTP parser/router, JSON adapter, TLS server, static UI files, Wi-Fi scan, mDNS. Linked by configuration daemons only, so the ring binary does not take OpenSSL.
- CMake fails configure if any file under `platform/include`, `platform/src`, or `platform/tests` includes `chime/`, or if `oc_platform` / `oc_platform_http` link a `chime*` target.
- Product binaries stay `chime` (ring) and `chime-webd` (configuration). They remain separate processes.
- A platform-only graph is `cmake -DOC_BUILD_CHIME=OFF`. Native CI runs `./scripts/platform_only_ci.sh`, which configures, builds, and tests a staged tree that does not contain `chime/`. `./scripts/test_platform_link_guard.sh` checks that a Chime target added after `platform/` still fails configure.

Board-specific work (kernel, device tree, rootfs overlay, Pi quirks) stays in `buildroot/` until TW-356 documents that boundary. Do not add Bell here.

## Product hooks

Route registration: implement `oc::http::ProductRoutes` and call `Register(router)`. The platform router is a method/path table. Adding a route does not edit platform internals.

Apply: implement `oc::apply::ProductApply` (or pass `std::vector<oc::apply::Step>` to `oc::apply::JobRunner`). Platform owns job lifecycle; the product chooses the steps.

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
