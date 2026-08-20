<script lang="ts">
  import { CORE_CONFIG_DEFAULTS } from "../../generated/config_schema";

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
        placeholder={CORE_CONFIG_DEFAULTS.notification_success_sound_path}
      />
    </div>
    <div>
      <label for="notification_failure_sound_path">Failure Sound Path</label>
      <input
        id="notification_failure_sound_path"
        bind:value={notificationFailureSoundPath}
        maxlength="256"
        placeholder={CORE_CONFIG_DEFAULTS.notification_failure_sound_path}
      />
    </div>
  </div>
  <p class="hint">Used for startup and other system notification beeps.</p>
</section>
