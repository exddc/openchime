export const MAX_RING_SOUND_BYTES = 2 * 1024 * 1024;

export function buildUploadSoundName(originalName: string): string {
  const lower = originalName.toLowerCase();
  const normalized = lower
    .replace(/[^a-z0-9_.-]+/g, "-")
    .replace(/[-._]{2,}/g, "-")
    .replace(/^[._-]+/, "")
    .replace(/[._-]+$/, "");

  const withExtension = normalized.endsWith(".wav")
    ? normalized
    : `${normalized}.wav`;

  const withoutPrefix = withExtension.replace(/^ring-/, "");
  const candidate = `ring-${withoutPrefix}`;

  const cleaned = candidate
    .replace(/[^a-z0-9_.-]+/g, "-")
    .replace(/[-._]{2,}/g, "-")
    .replace(/^[-._]+/, "")
    .replace(/[-._]+$/, "");

  if (!cleaned || cleaned === "ring" || cleaned === "ring.wav") {
    return "ring-custom.wav";
  }

  if (!cleaned.endsWith(".wav")) {
    return `${cleaned}.wav`;
  }

  return cleaned;
}

export function isWavFile(file: File): boolean {
  const fileNameLower = file.name.toLowerCase();
  const hasWavExtension = fileNameLower.endsWith(".wav");
  const hasWavMimeType =
    file.type === "audio/wav" || file.type === "audio/x-wav";
  return hasWavExtension || hasWavMimeType;
}
