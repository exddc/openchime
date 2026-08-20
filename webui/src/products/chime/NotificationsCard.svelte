<script lang="ts">
  export let notificationSuccessSoundPath: string;
  export let notificationFailureSoundPath: string;
</script>

<section class="card">
  <h2>Notifications</h2>
  <div class="row">
    <div>
      <label for="notification_success_sound_path">Success Sound Path</label>
      <input
        id="notification_success_sound_path"
        bind:value={notificationSuccessSoundPath}
        maxlength="256"
        placeholder="/usr/local/share/chime/test.wav"
      />
    </div>
    <div>
      <label for="notification_failure_sound_path">Failure Sound Path</label>
      <input
        id="notification_failure_sound_path"
        bind:value={notificationFailureSoundPath}
        maxlength="256"
        placeholder="/usr/local/share/chime/ring.wav"
      />
    </div>
  </div>
  <p class="hint">Used for startup and other system notification beeps.</p>
</section>
