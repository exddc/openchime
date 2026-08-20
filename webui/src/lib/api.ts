import type { CoreConfigFields } from "../generated/config_schema";

export type SessionLoss = "pair" | "login";

type AuthErrorHandler = (kind: SessionLoss) => void;

let authErrorHandler: AuthErrorHandler | undefined;

const API_BASE = "";

export class ApiError extends Error {
  readonly status: number;
  readonly code?: string;

  constructor(message: string, status: number, code?: string) {
    super(message);
    this.name = "ApiError";
    this.status = status;
    this.code = code;
  }
}

export type AuthStatusResponse = {
  paired?: boolean;
  authenticated?: boolean;
  error?: string;
  message?: string;
};

export type ValidationError = {
  field: string;
  message: string;
};

export type ApplyStatus = {
  job_id: number;
  state: string;
  started_at_utc?: string;
  finished_at_utc?: string;
  error?: string;
};

export type CoreConfigResponse = Partial<CoreConfigFields> & {
  wifi_password_set?: boolean;
  mqtt_password_set?: boolean;
  apply?: ApplyStatus;
  error?: string;
  message?: string;
  validation_errors?: ValidationError[];
};

export type CoreConfigWrite = CoreConfigFields & {
  wifi_password?: string;
  mqtt_password?: string;
};

export type RingSoundsResponse = {
  sounds?: string[];
  selected_sound?: string;
  error?: string;
  message?: string;
};

export type SelectRingSoundResponse = {
  selected?: string;
  selection_persisted?: boolean;
  error?: string;
  message?: string;
};

export type WifiNetwork = {
  ssid: string;
  signal_dbm: number;
  security: string;
};

export type WifiScanResponse = {
  networks?: WifiNetwork[];
  error?: string;
  message?: string;
};

export type ObservedTopicsResponse = {
  topics?: string[];
  error?: string;
  message?: string;
};

export type SystemVersionResponse = {
  chime_version?: string;
  os_version?: string;
  config_version?: string;
  error?: string;
  message?: string;
};

export type ApplyPollOptions = {
  timeoutMs?: number;
  pollMs?: number;
  signal?: AbortSignal;
  sleep?: (ms: number, signal?: AbortSignal) => Promise<void>;
  now?: () => number;
};

type ErrorBody = {
  error?: string;
  message?: string;
  validation_errors?: ValidationError[];
};

export function setAuthErrorHandler(
  handler: AuthErrorHandler | undefined,
): void {
  authErrorHandler = handler;
}

export function errorMessage(error: unknown): string {
  return error instanceof Error ? error.message : String(error);
}

export function parseTopicList(csv: string): string[] {
  return csv
    .split(",")
    .map((entry) => entry.trim())
    .filter((entry) => entry.length > 0);
}

export function apiUrl(path: string): string {
  if (/^https?:\/\//i.test(path)) {
    return path;
  }
  const base = API_BASE.replace(/\/$/, "");
  const suffix = path.startsWith("/") ? path : `/${path}`;
  return `${base}${suffix}`;
}

export function readCookie(name: string): string {
  if (typeof document === "undefined") {
    return "";
  }
  const parts = document.cookie.split(";");
  for (const part of parts) {
    const trimmed = part.trim();
    if (trimmed.startsWith(`${name}=`)) {
      return decodeURIComponent(trimmed.slice(name.length + 1));
    }
  }
  return "";
}

function notifyAuthError(error: string | undefined): void {
  if (error === "unpaired") {
    authErrorHandler?.("pair");
    return;
  }
  if (error === "unauthorized" || error === "csrf_failed") {
    authErrorHandler?.("login");
  }
}

function isAbortError(error: unknown): boolean {
  return error instanceof Error && error.name === "AbortError";
}

function abortError(signal?: AbortSignal): Error {
  if (signal?.reason instanceof Error) {
    return signal.reason;
  }
  const error = new Error("Request aborted.");
  error.name = "AbortError";
  return error;
}

export function sleep(ms: number, signal?: AbortSignal): Promise<void> {
  return new Promise((resolve, reject) => {
    if (signal?.aborted) {
      reject(abortError(signal));
      return;
    }
    const timer = setTimeout(() => {
      signal?.removeEventListener("abort", onAbort);
      resolve();
    }, ms);
    const onAbort = () => {
      clearTimeout(timer);
      reject(abortError(signal));
    };
    signal?.addEventListener("abort", onAbort, { once: true });
  });
}

function throwIfAborted(signal: AbortSignal | undefined): void {
  if (!signal?.aborted) {
    return;
  }
  throw abortError(signal);
}

async function parseJsonBody<T>(response: Response): Promise<T> {
  const text = await response.text();
  if (!text) {
    return {} as T;
  }
  try {
    return JSON.parse(text) as T;
  } catch {
    throw new ApiError("Invalid JSON response", response.status);
  }
}

function jsonInit(method: string, body: unknown): RequestInit {
  return {
    method,
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(body),
  };
}

function mergeInit(base: RequestInit, extra: RequestInit = {}): RequestInit {
  const headers = new Headers(base.headers);
  new Headers(extra.headers).forEach((value, key) => {
    headers.set(key, value);
  });
  return { ...base, ...extra, headers };
}

async function request<T>(
  path: string,
  fallbackError: string,
  init: RequestInit = {},
): Promise<T> {
  const headers = new Headers(init.headers);
  const method = (init.method ?? "GET").toUpperCase();
  if (method !== "GET" && method !== "HEAD") {
    const csrf = readCookie("chime_csrf");
    if (csrf && !headers.has("X-CSRF-Token")) {
      headers.set("X-CSRF-Token", csrf);
    }
  }

  let response: Response;
  try {
    response = await fetch(apiUrl(path), {
      ...init,
      headers,
      credentials: "same-origin",
    });
  } catch (error) {
    if (isAbortError(error)) {
      throw error;
    }
    throw new ApiError(errorMessage(error), 0);
  }

  const data = await parseJsonBody<T & ErrorBody>(response);

  if (!response.ok) {
    notifyAuthError(data.error);
    if (data.validation_errors && data.validation_errors.length > 0) {
      const details = data.validation_errors
        .map((entry) => `${entry.field}: ${entry.message}`)
        .join("\n");
      throw new ApiError(details, response.status, data.error);
    }
    throw new ApiError(
      data.message ?? data.error ?? fallbackError,
      response.status,
      data.error,
    );
  }

  return data;
}

export async function getAuthStatus(): Promise<AuthStatusResponse> {
  return request("/api/v1/auth/status", "Failed to read authentication status");
}

export async function pairDevice(
  pairingCode: string,
  password: string,
): Promise<AuthStatusResponse> {
  return request(
    "/api/v1/auth/pair",
    "Pairing failed",
    jsonInit("POST", { pairing_code: pairingCode, password }),
  );
}

export async function login(password: string): Promise<AuthStatusResponse> {
  return request(
    "/api/v1/auth/login",
    "Login failed",
    jsonInit("POST", { password }),
  );
}

export async function logout(): Promise<AuthStatusResponse> {
  return request("/api/v1/auth/logout", "Logout failed", { method: "POST" });
}

export async function getCoreConfig(
  init: RequestInit = {},
): Promise<CoreConfigResponse> {
  return request("/api/v1/config/core", "Failed to load config", init);
}

export async function saveCoreConfig(
  input: CoreConfigWrite,
  init: RequestInit = {},
): Promise<CoreConfigResponse> {
  const payload: Record<string, unknown> = {
    wifi_ssid: input.wifi_ssid,
    mqtt_host: input.mqtt_host,
    mqtt_port: input.mqtt_port,
    mqtt_client_id: input.mqtt_client_id,
    mqtt_username: input.mqtt_username,
    mqtt_tls_enabled: input.mqtt_tls_enabled,
    mqtt_tls_validate_certificate: input.mqtt_tls_validate_certificate,
    mqtt_tls_ca_file: input.mqtt_tls_ca_file,
    mqtt_tls_cert_file: input.mqtt_tls_cert_file,
    mqtt_tls_key_file: input.mqtt_tls_key_file,
    mqtt_topics: input.mqtt_topics,
    ring_topic: input.ring_topic,
    notification_success_sound_path: input.notification_success_sound_path,
    notification_failure_sound_path: input.notification_failure_sound_path,
    volume_bell: input.volume_bell,
    volume_notifications: input.volume_notifications,
  };
  if (input.wifi_password != null && input.wifi_password.length > 0) {
    payload.wifi_password = input.wifi_password;
  }
  if (input.mqtt_password != null && input.mqtt_password.length > 0) {
    payload.mqtt_password = input.mqtt_password;
  }

  return request(
    "/api/v1/config/core",
    "Save failed",
    mergeInit(jsonInit("POST", payload), init),
  );
}

export async function waitForApplyCompletion(
  jobId: number,
  options: ApplyPollOptions = {},
): Promise<void> {
  const timeoutMs = options.timeoutMs ?? 90_000;
  const pollMs = options.pollMs ?? 800;
  const sleepFn = options.sleep ?? sleep;
  const now = options.now ?? Date.now;
  const startedAt = now();
  let transientErrors = 0;

  while (now() - startedAt < timeoutMs) {
    throwIfAborted(options.signal);
    let apply: ApplyStatus | undefined;
    try {
      apply = (await getCoreConfig({ signal: options.signal })).apply;
    } catch (error) {
      if (isAbortError(error)) {
        throw error;
      }
      transientErrors += 1;
      if (transientErrors >= 5) {
        throw error;
      }
      await sleepFn(pollMs, options.signal);
      continue;
    }
    if (apply && apply.job_id === jobId) {
      if (apply.state === "succeeded") {
        return;
      }
      if (apply.state === "failed") {
        throw new Error(apply.error || "Apply failed");
      }
    }
    await sleepFn(pollMs, options.signal);
  }

  throw new Error("Timed out waiting for apply to complete.");
}

export async function scanWifi(init: RequestInit = {}): Promise<WifiNetwork[]> {
  const data = await request<WifiScanResponse>(
    "/api/v1/wifi/scan",
    "Scan failed",
    init,
  );
  return data.networks ?? [];
}

export async function getObservedTopics(
  init: RequestInit = {},
): Promise<string[]> {
  const data = await request<ObservedTopicsResponse>(
    "/api/v1/mqtt/topics",
    "Failed to load observed topics",
    init,
  );
  return data.topics ?? [];
}

export async function getSystemVersion(
  init: RequestInit = {},
): Promise<SystemVersionResponse> {
  return request(
    "/api/v1/system/version",
    "Failed to load system version",
    init,
  );
}

export async function getRingSounds(
  init: RequestInit = {},
): Promise<RingSoundsResponse> {
  return request("/api/v1/ring/sounds", "Failed to load ring sounds", init);
}

export async function uploadRingSound(
  name: string,
  body: ArrayBuffer,
  init: RequestInit = {},
): Promise<{ error?: string; message?: string }> {
  return request(
    `/api/v1/ring/sounds/${encodeURIComponent(name)}`,
    "Upload failed",
    mergeInit({ method: "PUT", body }, init),
  );
}

export async function selectRingSound(
  name: string,
  init: RequestInit = {},
): Promise<SelectRingSoundResponse> {
  return request(
    "/api/v1/ring/sounds/select",
    "Failed to activate sound",
    mergeInit(jsonInit("POST", { name }), init),
  );
}
