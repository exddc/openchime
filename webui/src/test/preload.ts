import { resolve } from "node:path";
import { plugin } from "bun";
import { Window } from "happy-dom";
import { compile } from "svelte/compiler";

const window = new Window({ url: "http://127.0.0.1/" });
const skip = new Set([
  "AbortController",
  "AbortSignal",
  "Blob",
  "File",
  "FormData",
  "Headers",
  "Request",
  "Response",
  "URL",
  "URLSearchParams",
  "fetch",
]);

function copyWindowGlobals(source: object): void {
  for (const key of Object.getOwnPropertyNames(source)) {
    if (skip.has(key) || key in globalThis) {
      continue;
    }
    try {
      Object.defineProperty(globalThis, key, {
        configurable: true,
        writable: true,
        value: (source as Record<string, unknown>)[key],
      });
    } catch {
      // Ignore prototype accessors that cannot be copied.
    }
  }
}

copyWindowGlobals(window);
copyWindowGlobals(Object.getPrototypeOf(window) as object);
(globalThis as { window: Window }).window = window;
(globalThis as { document: Document }).document =
  window.document as unknown as Document;

const svelteClient = resolve(
  import.meta.dir,
  "../../node_modules/svelte/src/index-client.js",
);

plugin({
  name: "svelte",
  setup(build) {
    build.onLoad({ filter: /\.svelte$/ }, async (args) => {
      const source = await Bun.file(args.path).text();
      const compiled = compile(source, {
        filename: args.path,
        css: "injected",
        generate: "client",
        fragments: "tree",
        runes: false,
      });
      return {
        contents: compiled.js.code.replaceAll(
          /from ["']svelte["']/g,
          `from ${JSON.stringify(svelteClient)}`,
        ),
        loader: "js",
      };
    });
  },
});
