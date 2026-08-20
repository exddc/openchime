import type { AuthStatusResponse } from "./api";

export type AppScreen = "loading" | "pair" | "login" | "app";

export function screenFromAuthStatus(
  data: AuthStatusResponse,
): Exclude<AppScreen, "loading"> {
  if (!data.paired) {
    return "pair";
  }
  if (!data.authenticated) {
    return "login";
  }
  return "app";
}

export function screenAfterProductLoadFailure(
  _error: unknown,
  current: AppScreen,
): AppScreen {
  return current;
}

export function resolveProduct<T extends { id: string }>(
  products: T[],
  id: string,
): T {
  const selected = products.find((product) => product.id === id);
  if (!selected) {
    throw new Error(`Unknown product page: ${id}`);
  }
  return selected;
}
