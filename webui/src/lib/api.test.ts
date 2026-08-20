import { afterEach, describe, expect, mock, test } from "bun:test";
import {
  ApiError,
  type CoreConfigWrite,
  apiUrl,
  errorMessage,
  getAuthStatus,
  getCoreConfig,
  login,
  parseTopicList,
  saveCoreConfig,
  setAuthErrorHandler,
  uploadRingSound,
  waitForApplyCompletion,
} from "./api";

const originalFetch = globalThis.fetch;
const originalDocument = (globalThis as { document?: Document }).document;

function jsonResponse(body: unknown, status = 200): Response {
  return new Response(JSON.stringify(body), {
    status,
    headers: { "Content-Type": "application/json" },
  });
}

function installCookies(initial: Record<string, string> = {}): void {
  const jar = new Map(Object.entries(initial));
  Object.defineProperty(globalThis, "document", {
    configurable: true,
    value: {
      get cookie() {
        return [...jar.entries()]
          .map(([name, value]) => `${name}=${encodeURIComponent(value)}`)
          .join("; ");
      },
      set cookie(entry: string) {
        const pair = entry.split(";")[0] ?? "";
        const eq = pair.indexOf("=");
        if (eq < 0) {
          return;
        }
        jar.set(
          pair.slice(0, eq).trim(),
          decodeURIComponent(pair.slice(eq + 1).trim()),
        );
      },
    },
  });
}

function toRequest(input: RequestInfo | URL, init?: RequestInit): Request {
  if (input instanceof Request) {
    return input;
  }
  const url = String(input);
  const absolute = /^https?:\/\//i.test(url)
    ? url
    : new URL(url, "http://127.0.0.1").toString();
  return new Request(absolute, init);
}

type FetchCall = {
  request: Request;
  init?: RequestInit;
};

function mockFetch(
  handler: (request: Request) => Response | Promise<Response>,
): FetchCall[] {
  const calls: FetchCall[] = [];
  globalThis.fetch = mock(
    async (input: RequestInfo | URL, init?: RequestInit) => {
      const request = toRequest(input, init);
      calls.push({ request, init });
      return handler(request);
    },
  ) as unknown as typeof fetch;
  return calls;
}

function sampleWrite(
  overrides: Partial<CoreConfigWrite> = {},
): CoreConfigWrite {
  return {
    wifi_ssid: "lab",
    mqtt_host: "broker.local",
    mqtt_port: 1883,
    mqtt_client_id: "chime",
    mqtt_username: "",
    mqtt_tls_enabled: false,
    mqtt_tls_validate_certificate: true,
    mqtt_tls_ca_file: "",
    mqtt_tls_cert_file: "",
    mqtt_tls_key_file: "",
    mqtt_topics: ["doorbell/ring"],
    ring_topic: "doorbell/ring",
    notification_success_sound_path: "/usr/local/share/chime/test.wav",
    notification_failure_sound_path: "/usr/local/share/chime/ring.wav",
    volume_bell: 80,
    volume_notifications: 70,
    volume_other: 70,
    ...overrides,
  };
}

afterEach(() => {
  globalThis.fetch = originalFetch;
  setAuthErrorHandler(undefined);
  if (originalDocument) {
    Object.defineProperty(globalThis, "document", {
      configurable: true,
      value: originalDocument,
    });
  } else {
    Reflect.deleteProperty(globalThis, "document");
  }
});

describe("apiUrl", () => {
  test("keeps same-origin API paths", () => {
    expect(apiUrl("/api/v1/config/core")).toBe("/api/v1/config/core");
  });

  test("passes through absolute URLs", () => {
    expect(apiUrl("https://chime.local/api/v1/config/core")).toBe(
      "https://chime.local/api/v1/config/core",
    );
  });
});

describe("parseTopicList", () => {
  test("trims and drops empty entries", () => {
    expect(parseTopicList(" doorbell/ring , , doorbell/status ")).toEqual([
      "doorbell/ring",
      "doorbell/status",
    ]);
  });
});

describe("request helpers", () => {
  test("GET does not send a CSRF header", async () => {
    installCookies({ chime_csrf: "token" });
    const calls = mockFetch(() =>
      jsonResponse({ paired: true, authenticated: true }),
    );
    await getAuthStatus();
    expect(calls[0]?.request.headers.get("X-CSRF-Token")).toBeNull();
    expect(calls[0]?.init?.credentials).toBe("same-origin");
  });

  test("POST sends CSRF from the cookie", async () => {
    installCookies({ chime_csrf: "csrf-token" });
    const calls = mockFetch(() => jsonResponse({ authenticated: true }));
    await login("secret");
    expect(calls[0]?.request.method).toBe("POST");
    expect(calls[0]?.request.headers.get("X-CSRF-Token")).toBe("csrf-token");
    expect(calls[0]?.request.headers.get("Content-Type")).toBe(
      "application/json",
    );
    expect(await calls[0]?.request.json()).toEqual({ password: "secret" });
  });

  test("PUT upload sends CSRF and raw body", async () => {
    installCookies({ chime_csrf: "csrf-token" });
    const payload = new Uint8Array([1, 2, 3]).buffer;
    const calls = mockFetch(() => jsonResponse({ message: "ok" }));
    await uploadRingSound("ring-custom.wav", payload);
    expect(calls[0]?.request.method).toBe("PUT");
    expect(calls[0]?.request.url).toContain(
      "/api/v1/ring/sounds/ring-custom.wav",
    );
    expect(calls[0]?.request.headers.get("X-CSRF-Token")).toBe("csrf-token");
    expect(await calls[0]?.request.arrayBuffer()).toEqual(payload);
  });

  test("maps unauthorized JSON to login session loss", async () => {
    const kinds: string[] = [];
    setAuthErrorHandler((kind) => kinds.push(kind));
    mockFetch(() => jsonResponse({ error: "unauthorized" }, 401));
    await expect(getCoreConfig()).rejects.toMatchObject({
      name: "ApiError",
      status: 401,
      code: "unauthorized",
      message: "unauthorized",
    });
    expect(kinds).toEqual(["login"]);
  });

  test("maps unpaired JSON to pair session loss", async () => {
    const kinds: string[] = [];
    setAuthErrorHandler((kind) => kinds.push(kind));
    mockFetch(() => jsonResponse({ error: "unpaired" }, 401));
    await expect(getCoreConfig()).rejects.toBeInstanceOf(ApiError);
    expect(kinds).toEqual(["pair"]);
  });

  test("maps csrf_failed JSON to login session loss", async () => {
    const kinds: string[] = [];
    setAuthErrorHandler((kind) => kinds.push(kind));
    mockFetch(() =>
      jsonResponse({ error: "csrf_failed", message: "bad csrf" }, 403),
    );
    await expect(login("x")).rejects.toMatchObject({
      message: "bad csrf",
      status: 403,
    });
    expect(kinds).toEqual(["login"]);
  });

  test("does not treat invalid JSON as session loss", async () => {
    const kinds: string[] = [];
    setAuthErrorHandler((kind) => kinds.push(kind));
    mockFetch(() => new Response("not-json", { status: 500 }));
    await expect(getAuthStatus()).rejects.toMatchObject({
      message: "Invalid JSON response",
      status: 500,
    });
    expect(kinds).toEqual([]);
  });

  test("normalizes an empty error body with the endpoint fallback", async () => {
    mockFetch(() => new Response("", { status: 500 }));
    await expect(getAuthStatus()).rejects.toMatchObject({
      message: "Failed to read authentication status",
      status: 500,
    });
  });

  test("normalizes network failures", async () => {
    mockFetch(() => {
      throw new TypeError("Failed to fetch");
    });
    await expect(getAuthStatus()).rejects.toMatchObject({
      name: "ApiError",
      status: 0,
      message: "Failed to fetch",
    });
  });

  test("joins validation errors from save", async () => {
    mockFetch(() =>
      jsonResponse(
        {
          error: "validation_failed",
          validation_errors: [
            { field: "mqtt_port", message: "must be 1-65535" },
            { field: "wifi_ssid", message: "required" },
          ],
        },
        400,
      ),
    );
    await expect(saveCoreConfig(sampleWrite())).rejects.toMatchObject({
      message: "mqtt_port: must be 1-65535\nwifi_ssid: required",
      status: 400,
    });
  });
});

describe("saveCoreConfig secrets", () => {
  test("omits blank wifi and mqtt passwords", async () => {
    const calls = mockFetch(() => jsonResponse({ mqtt_password_set: false }));
    await saveCoreConfig(sampleWrite({ wifi_password: "", mqtt_password: "" }));
    const body = (await calls[0]?.request.json()) as Record<string, unknown>;
    expect(body.wifi_password).toBeUndefined();
    expect(body.mqtt_password).toBeUndefined();
    expect(body.wifi_ssid).toBe("lab");
  });

  test("includes provided secrets without reading them back", async () => {
    const calls = mockFetch(() =>
      jsonResponse({ mqtt_password_set: true, wifi_password_set: true }),
    );
    const result = await saveCoreConfig(
      sampleWrite({
        wifi_password: "wifi-secret",
        mqtt_password: "mqtt-secret",
      }),
    );
    const body = (await calls[0]?.request.json()) as Record<string, unknown>;
    expect(body.wifi_password).toBe("wifi-secret");
    expect(body.mqtt_password).toBe("mqtt-secret");
    expect(result).not.toHaveProperty("wifi_password");
    expect(result).not.toHaveProperty("mqtt_password");
  });
});

describe("waitForApplyCompletion", () => {
  test("returns when the matching job succeeds", async () => {
    let calls = 0;
    mockFetch(() => {
      calls += 1;
      const state = calls === 1 ? "running" : "succeeded";
      return jsonResponse({ apply: { job_id: 9, state } });
    });
    await waitForApplyCompletion(9, {
      pollMs: 0,
      timeoutMs: 1000,
      sleep: async () => {},
    });
    expect(calls).toBe(2);
  });

  test("retries transient failures then throws the last error", async () => {
    let calls = 0;
    mockFetch(() => {
      calls += 1;
      return jsonResponse({ error: "apply_busy" }, 500);
    });
    await expect(
      waitForApplyCompletion(3, {
        pollMs: 0,
        timeoutMs: 1000,
        sleep: async () => {},
      }),
    ).rejects.toMatchObject({ message: "apply_busy" });
    expect(calls).toBe(5);
  });

  test("times out when the job never finishes", async () => {
    let now = 0;
    mockFetch(() => jsonResponse({ apply: { job_id: 1, state: "running" } }));
    await expect(
      waitForApplyCompletion(1, {
        timeoutMs: 10,
        pollMs: 5,
        now: () => now,
        sleep: async () => {
          now += 5;
        },
      }),
    ).rejects.toMatchObject({
      message: "Timed out waiting for apply to complete.",
    });
  });

  test("stops when aborted", async () => {
    const abort = new AbortController();
    mockFetch(() => jsonResponse({ apply: { job_id: 1, state: "running" } }));
    const pending = waitForApplyCompletion(1, {
      timeoutMs: 1000,
      pollMs: 0,
      signal: abort.signal,
      sleep: async () => {
        abort.abort();
      },
    });
    await expect(pending).rejects.toMatchObject({ name: "AbortError" });
  });

  test("aborts the default poll delay", async () => {
    const abort = new AbortController();
    mockFetch(() => jsonResponse({ apply: { job_id: 1, state: "running" } }));
    const pending = waitForApplyCompletion(1, {
      timeoutMs: 5_000,
      pollMs: 50,
      signal: abort.signal,
    });
    abort.abort();
    await expect(pending).rejects.toMatchObject({ name: "AbortError" });
  });
});

describe("errorMessage", () => {
  test("uses Error.message when available", () => {
    expect(errorMessage(new Error("nope"))).toBe("nope");
    expect(errorMessage("raw")).toBe("raw");
  });
});
