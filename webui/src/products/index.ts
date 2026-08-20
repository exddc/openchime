import type { Component } from "svelte";
import { getSystemVersion } from "../lib/api";
import { resolveProduct } from "../lib/shell";
import ChimePage from "./chime/ChimePage.svelte";

export type ProductPageProps = {
  messageText: string;
  messageIsError: boolean;
  setMessage: (text: string, isError?: boolean) => void;
  onLoadFailed: (error: unknown) => void;
};

export type ProductVersion = {
  product: string;
  os: string;
  config: string;
};

export type ProductPage = {
  id: string;
  title: string;
  subtitle: string;
  loadingTitle: string;
  loadingMessage: string;
  pairTitle: string;
  pairDescription: string;
  pairHint: string;
  versionLabel: string;
  loadVersion: () => Promise<ProductVersion>;
  page: Component<ProductPageProps>;
};

async function loadChimeVersion(): Promise<ProductVersion> {
  const data = await getSystemVersion();
  return {
    product: data.chime_version?.trim() || "unknown",
    os: data.os_version?.trim() || "unknown",
    config: data.config_version?.trim() || "unknown",
  };
}

export const productPages: ProductPage[] = [
  {
    id: "chime",
    title: "Chime Web Console",
    subtitle: "Configure Wi-Fi and MQTT. Changes are applied automatically.",
    loadingTitle: "Chime Web Console",
    loadingMessage: "Checking device setup…",
    pairTitle: "Set up this Chime",
    pairDescription:
      "Enter the pairing code from the serial console, then choose an admin password. Pairing closes after this step.",
    pairHint:
      "The pairing code is printed once on the device console while unpaired.",
    versionLabel: "Chime",
    loadVersion: loadChimeVersion,
    page: ChimePage,
  },
];

export const activeProductId = "chime";

export function getActiveProduct(): ProductPage {
  return resolveProduct(productPages, activeProductId);
}
