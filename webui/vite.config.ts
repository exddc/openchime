import { svelte } from "@sveltejs/vite-plugin-svelte";
import tailwindcss from "@tailwindcss/vite";
import { defineConfig } from "vite";

export default defineConfig({
  plugins: [svelte(), tailwindcss()],
  server: {
    proxy: {
      "/api": {
        target: "https://127.0.0.1:8443",
        changeOrigin: true,
        secure: false,
        configure: (proxy) => {
          proxy.on("proxyRes", (proxyRes) => {
            const cookies = proxyRes.headers["set-cookie"];
            if (!cookies) {
              return;
            }
            const list = Array.isArray(cookies) ? cookies : [cookies];
            proxyRes.headers["set-cookie"] = list.map((cookie) =>
              cookie.replace(/;\s*Secure/gi, ""),
            );
          });
        },
      },
    },
  },
  build: {
    target: "es2018",
    sourcemap: false,
    minify: "esbuild",
    cssCodeSplit: false,
  },
});
