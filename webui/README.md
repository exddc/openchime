# Web UI (Svelte)

This directory contains the Svelte + Vite frontend for `chime-webd`.

## Tooling

This project uses **Bun as the default package manager/runtime** for local frontend tasks.

Install dependencies:

```bash
cd webui
bun install
```

Run linting (Biome):

```bash
bun run lint
```

## Local Development

From the repository root:

```bash
./scripts/local_chime.sh webui-dev
```

In a second terminal, run the backend on port 8443 (or set `CHIME_WEBD_PORT`):

```bash
./scripts/local_chime.sh run-webd
```

Vite proxies `/api/*` to `https://127.0.0.1:8443` (TLS verification disabled for local self-signed certs).

## Production Build

Locally:

```bash
./scripts/local_chime.sh webui-build
```

This writes static assets to `webui/dist/`. That directory is gitignored and is **not** consumed by the OS image build.

When `CHIME_WEBD_UI_DIST_DIR` points at that directory, a locally run `chime-webd` serves the built UI instead of the embedded fallback HTML.

## OS image (baked assets)

A clean `./scripts/docker_build.sh` produces an image whose rootfs already contains the production bundle. No `webui/dist` on the host and no post-flash deploy step are required.

| Location | Path |
|----------|------|
| Repository sources | `webui/` (`package.json`, `bun.lock`, `src/`, …) |
| Image build staging | `/home/builder/webui-src` inside the builder (no `dist/` or `node_modules/`) |
| On-device install | `/usr/local/share/chime-web-ui/dist` |
| Served URL | `https://<device>:8443/` (`GET /`) |

The Buildroot package `chime-web-ui` installs the bundle with `bun install --frozen-lockfile` and `bun run build`. If `index.html` is missing or the dist is empty, the image build fails.

The embedded “Web UI Unavailable” page is only a runtime diagnostic. To update UI assets on a device that is already flashed, without rebuilding the OS image:

```bash
./scripts/local_chime.sh webui-build
./scripts/deploy.sh chime <pi-ip> --with-webd
```

## Styling

Tailwind CSS is configured via the Vite plugin and imported in `src/app.css`.
