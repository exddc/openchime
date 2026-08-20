import { afterEach, describe, expect, test } from "bun:test";
import {
  flushSync,
  mount,
  unmount,
} from "../../../node_modules/svelte/src/index-client.js";
import ChimePage from "./ChimePage.svelte";

const originalFetch = globalThis.fetch;

function jsonResponse(body: unknown, status = 200): Response {
  return new Response(JSON.stringify(body), {
    status,
    headers: { "Content-Type": "application/json" },
  });
}

function buttonByLabel(
  root: ParentNode,
  label: string,
): HTMLButtonElement | undefined {
  return [...root.querySelectorAll("button")].find(
    (button) => button.textContent?.replace(/\s+/g, " ").trim() === label,
  ) as HTMLButtonElement | undefined;
}

async function waitFor<T>(
  lookup: () => T | null | undefined | false,
  timeoutMs = 2000,
): Promise<T> {
  const started = Date.now();
  while (Date.now() - started < timeoutMs) {
    const value = lookup();
    if (value) {
      return value;
    }
    await Bun.sleep(10);
  }
  throw new Error("Timed out waiting for condition");
}

afterEach(() => {
  globalThis.fetch = originalFetch;
  document.body.replaceChildren();
});

describe("ChimePage", () => {
  test("retries a failed config load and enables Save without reloading", async () => {
    let coreCalls = 0;
    globalThis.fetch = async (input: RequestInfo | URL) => {
      const url = String(input instanceof Request ? input.url : input);
      if (url.includes("/api/v1/config/core")) {
        coreCalls += 1;
        if (coreCalls === 1) {
          return jsonResponse({ error: "temporarily unavailable" }, 500);
        }
        return jsonResponse({
          wifi_ssid: "labnet",
          mqtt_host: "broker.local",
        });
      }
      if (url.includes("/api/v1/mqtt/topics")) {
        return jsonResponse({ topics: [] });
      }
      if (url.includes("/api/v1/ring/sounds")) {
        return jsonResponse({ sounds: [], selected_sound: "" });
      }
      return jsonResponse({ error: `unexpected ${url}` }, 404);
    };

    const target = document.createElement("div");
    document.body.append(target);
    const app = mount(ChimePage, {
      target,
      intro: false,
      props: {
        messageText: "",
        messageIsError: false,
        setMessage: () => {},
        onLoadFailed: () => {},
        loadRetryDelayMs: 30_000,
      },
    });
    flushSync();

    const retry = await waitFor(() => buttonByLabel(target, "Retry"));
    const saveBefore = buttonByLabel(target, "Save & Apply");
    const ssidBefore = target.querySelector(
      "#wifi_ssid",
    ) as HTMLInputElement | null;
    expect(saveBefore?.disabled).toBe(true);
    expect(ssidBefore?.value).toBe("");
    expect(coreCalls).toBe(1);

    retry.click();
    flushSync();

    const hydrated = await waitFor(() => {
      const ssid = target.querySelector(
        "#wifi_ssid",
      ) as HTMLInputElement | null;
      const save = buttonByLabel(target, "Save & Apply");
      if (ssid?.value === "labnet" && save && !save.disabled) {
        return { ssid, save };
      }
      return null;
    });

    expect(hydrated.ssid.value).toBe("labnet");
    expect(hydrated.save.disabled).toBe(false);
    expect(buttonByLabel(target, "Retry")).toBeUndefined();
    expect(coreCalls).toBe(2);
    expect(document.body.contains(target)).toBe(true);

    unmount(app);
  });
});
