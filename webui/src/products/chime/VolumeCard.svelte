<script lang="ts">
  import { CORE_CONFIG_INT_BOUNDS } from "../../generated/config_schema";

  export let volumeBell: number;
  export let volumeNotifications: number;
</script>

<section class="card">
  <h2>Volume</h2>
  <div class="row">
    <div>
      <label for="volume_bell">Bell (%)</label>
      <input
        id="volume_bell"
        type="number"
        min={CORE_CONFIG_INT_BOUNDS.volume_bell.min}
        max={CORE_CONFIG_INT_BOUNDS.volume_bell.max}
        step="1"
        bind:value={volumeBell}
      />
    </div>
    <div>
      <label for="volume_notifications">Notifications (%)</label>
      <input
        id="volume_notifications"
        type="number"
        min={CORE_CONFIG_INT_BOUNDS.volume_notifications.min}
        max={CORE_CONFIG_INT_BOUNDS.volume_notifications.max}
        step="1"
        bind:value={volumeNotifications}
      />
    </div>
  </div>
  <p class="hint">These are software volume levels (0-100) applied before playback.</p>
</section>
