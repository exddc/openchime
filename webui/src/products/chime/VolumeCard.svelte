<script lang="ts">
  export let volumeBell: number;
  export let volumeNotifications: number;
  export let volumeOther: number;
</script>

<section class="card">
  <h2>Volume</h2>
  <div class="row">
    <div>
      <label for="volume_bell">Bell (%)</label>
      <input id="volume_bell" type="number" min="0" max="100" step="1" bind:value={volumeBell} />
    </div>
    <div>
      <label for="volume_notifications">Notifications (%)</label>
      <input
        id="volume_notifications"
        type="number"
        min="0"
        max="100"
        step="1"
        bind:value={volumeNotifications}
      />
    </div>
  </div>
  <div class="row">
    <div>
      <label for="volume_other">Other (%)</label>
      <input id="volume_other" type="number" min="0" max="100" step="1" bind:value={volumeOther} />
    </div>
    <div></div>
  </div>
  <p class="hint">These are software volume levels (0-100) applied before playback.</p>
</section>
