import { MAX_RING_SOUND_BYTES, isWavFile } from "../../lib/ring-sound";

export function wifiPasswordHint(wifiPasswordSet: boolean | undefined): string {
  return wifiPasswordSet
    ? "Password is set. Leave blank to keep it unchanged."
    : "No saved password yet. Enter one before saving.";
}

export function isSaveDisabled(
  isSaving: boolean,
  configHydrated: boolean,
): boolean {
  return isSaving || !configHydrated;
}

export function requireUploadableWav(file: File | null): File {
  if (!file) {
    throw new Error("Choose a .wav file to upload.");
  }
  if (!isWavFile(file)) {
    throw new Error("Please select a .wav file.");
  }
  if (file.size > MAX_RING_SOUND_BYTES) {
    throw new Error("File must be <= 2MB.");
  }
  return file;
}
