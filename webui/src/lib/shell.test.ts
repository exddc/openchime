import { describe, expect, test } from "bun:test";
import { ApiError } from "./api";
import {
  resolveProduct,
  screenAfterProductLoadFailure,
  screenFromAuthStatus,
} from "./shell";

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

describe("screenAfterProductLoadFailure", () => {
  test("keeps the authenticated product screen for data failures", () => {
    expect(
      screenAfterProductLoadFailure(new Error("topics failed"), "app"),
    ).toBe("app");
    expect(
      screenAfterProductLoadFailure(new ApiError("Scan failed", 500), "app"),
    ).toBe("app");
  });
});

describe("resolveProduct", () => {
  test("selects a second product without replacing the first", () => {
    const pages = [
      { id: "chime", title: "Chime Web Console" },
      { id: "bell", title: "Bell Web Console" },
    ];
    expect(resolveProduct(pages, "bell").title).toBe("Bell Web Console");
    expect(resolveProduct(pages, "chime").title).toBe("Chime Web Console");
  });

  test("rejects an unknown product id", () => {
    expect(() => resolveProduct([{ id: "chime" }], "bell")).toThrow(
      "Unknown product page: bell",
    );
  });
});
