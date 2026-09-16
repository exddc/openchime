import { describe, expect, test } from "bun:test";
import { resolveProduct, screenFromAuthStatus } from "./shell";

describe("screenFromAuthStatus", () => {
  test("maps pairing and session state", () => {
    expect(screenFromAuthStatus({ paired: false })).toBe("pair");
    expect(screenFromAuthStatus({ paired: true, authenticated: false })).toBe(
      "login",
    );
    expect(screenFromAuthStatus({ paired: true, authenticated: true })).toBe(
      "app",
    );
  });
});

describe("resolveProduct", () => {
  test("selects a second product without replacing the first", () => {
    const pages = [
      { id: "chime", title: "Chime Web Console" },
      { id: "ring", title: "Ring Web Console" },
    ];
    expect(resolveProduct(pages, "ring").title).toBe("Ring Web Console");
    expect(resolveProduct(pages, "chime").title).toBe("Chime Web Console");
  });

  test("rejects an unknown product id", () => {
    expect(() => resolveProduct([{ id: "chime" }], "bell")).toThrow(
      "Unknown product page: bell",
    );
  });
});
