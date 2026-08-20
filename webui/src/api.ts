export type AuthStatusResponse = {
  paired?: boolean;
  authenticated?: boolean;
  error?: string;
  message?: string;
};

export type SessionLoss = "pair" | "login";

type AuthErrorHandler = (kind: SessionLoss) => void;

let authErrorHandler: AuthErrorHandler | undefined;

export function setAuthErrorHandler(
  handler: AuthErrorHandler | undefined,
): void {
  authErrorHandler = handler;
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

export function readCookie(name: string): string {
  const parts = document.cookie.split(";");
  for (const part of parts) {
    const trimmed = part.trim();
    if (trimmed.startsWith(`${name}=`)) {
      return decodeURIComponent(trimmed.slice(name.length + 1));
    }
  }
  return "";
}

export async function apiFetch(
  path: string,
  init: RequestInit = {},
): Promise<Response> {
  const headers = new Headers(init.headers);
  const method = (init.method ?? "GET").toUpperCase();
  if (method !== "GET" && method !== "HEAD") {
    const csrf = readCookie("chime_csrf");
    if (csrf && !headers.has("X-CSRF-Token")) {
      headers.set("X-CSRF-Token", csrf);
    }
  }
  const response = await fetch(path, {
    ...init,
    headers,
    credentials: "same-origin",
  });
  if (!response.ok) {
    try {
      const data = (await response.clone().json()) as { error?: string };
      notifyAuthError(data.error);
    } catch {
      // Non-JSON error bodies are not session-loss signals.
    }
  }
  return response;
}
