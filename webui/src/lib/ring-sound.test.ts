import { describe, expect, test } from "bun:test";
import {
  MAX_RING_SOUND_BYTES,
  buildUploadSoundName,
  isWavFile,
} from "./ring-sound";

describe("buildUploadSoundName", () => {
  test("prefixes a cleaned wav name with ring-", () => {
    expect(buildUploadSoundName("Doorbell Tone.WAV")).toBe(
      "ring-doorbell-tone.wav",
    );
  });

  test("does not double the ring- prefix", () => {
    expect(buildUploadSoundName("ring-front-door.wav")).toBe(
      "ring-front-door.wav",
    );
  });

  test("turns punctuation-only names into a wav filename", () => {
    expect(buildUploadSoundName("!!!")).toBe("ring-wav.wav");
  });
});

describe("isWavFile", () => {
  test("accepts wav extension or mime type", () => {
    expect(isWavFile(new File([""], "tone.wav", { type: "text/plain" }))).toBe(
      true,
    );
    expect(isWavFile(new File([""], "tone.bin", { type: "audio/wav" }))).toBe(
      true,
    );
    expect(isWavFile(new File([""], "tone.bin", { type: "text/plain" }))).toBe(
      false,
    );
  });

  test("keeps the 2MB upload cap", () => {
    expect(MAX_RING_SOUND_BYTES).toBe(2 * 1024 * 1024);
  });
});
