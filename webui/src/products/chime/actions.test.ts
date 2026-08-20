import { describe, expect, test } from "bun:test";
import {
  isSaveDisabled,
  requireUploadableWav,
  wifiPasswordHint,
} from "./actions";

describe("isSaveDisabled", () => {
  test("blocks save until core config has loaded", () => {
    expect(isSaveDisabled(false, false)).toBe(true);
    expect(isSaveDisabled(false, true)).toBe(false);
    expect(isSaveDisabled(true, true)).toBe(true);
  });
});

describe("wifiPasswordHint", () => {
  test("does not place a password value into the hint", () => {
    expect(wifiPasswordHint(true)).toBe(
      "Password is set. Leave blank to keep it unchanged.",
    );
    expect(wifiPasswordHint(false)).toBe(
      "No saved password yet. Enter one before saving.",
    );
  });
});

describe("requireUploadableWav", () => {
  test("rejects missing, non-wav, and oversized files", () => {
    expect(() => requireUploadableWav(null)).toThrow(
      "Choose a .wav file to upload.",
    );
    expect(() =>
      requireUploadableWav(new File([""], "tone.bin", { type: "text/plain" })),
    ).toThrow("Please select a .wav file.");
    const oversized = new File([new Uint8Array(2 * 1024 * 1024 + 1)], "a.wav");
    expect(() => requireUploadableWav(oversized)).toThrow(
      "File must be <= 2MB.",
    );
  });

  test("accepts a wav under the upload cap", () => {
    const file = new File([new Uint8Array(16)], "tone.wav", {
      type: "audio/wav",
    });
    expect(requireUploadableWav(file)).toBe(file);
  });
});
