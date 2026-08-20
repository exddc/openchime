<script lang="ts">
  import type { WifiNetwork } from "../../lib/api";

  export let wifiSsid: string;
  export let wifiPassword: string;
  export let isSaving: boolean;
  export let scanResults: WifiNetwork[];
  export let onScan: () => Promise<void>;

  let selectedScanSsid = "";

  function onScanSelectionChanged(event: Event): void {
    const target = event.currentTarget as HTMLSelectElement;
    selectedScanSsid = target.value;
    if (selectedScanSsid) {
      wifiSsid = selectedScanSsid;
    }
  }
</script>

<section class="card">
  <h2>Wi-Fi</h2>
  <div class="row">
    <div>
      <label for="wifi_ssid">SSID</label>
      <input id="wifi_ssid" bind:value={wifiSsid} placeholder="Network name" />
    </div>
    <div>
      <label for="wifi_password">Password</label>
      <input
        id="wifi_password"
        type="password"
        bind:value={wifiPassword}
        placeholder="Leave blank to keep current"
      />
    </div>
  </div>

  <div class="button-row">
    <button class="secondary" type="button" disabled={isSaving} on:click={onScan}>
      Scan Networks
    </button>
  </div>

  <label for="scan_results">Scan Results</label>
  <select id="scan_results" bind:value={selectedScanSsid} on:change={onScanSelectionChanged}>
    <option value="">Select SSID</option>
    {#if scanResults.length === 0}
      <option value="" disabled>No scan results yet</option>
    {/if}
    {#each scanResults as network}
      <option value={network.ssid}>
        {network.ssid} ({network.signal_dbm} dBm, {network.security})
      </option>
    {/each}
  </select>
  <p class="hint">Selecting an SSID fills the field above.</p>
</section>
