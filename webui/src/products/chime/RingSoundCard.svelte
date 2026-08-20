<script lang="ts">
  import { buildUploadSoundName } from "../../lib/ring-sound";

  export let ringSounds: string[];
  export let selectedRingSound: string;
  export let isUploadingRingSound: boolean;
  export let onUpload: (file: File) => Promise<void>;
  export let onActivate: () => Promise<void>;
  export let onRefresh: () => Promise<void>;

  let ringSoundUpload: File | null = null;
  let preparedUploadName = "";
</script>

<section class="card">
  <h2>Ring Sound</h2>
  <div class="row">
    <div>
      <label for="ring_sound_upload">Upload WAV</label>
      <input
        id="ring_sound_upload"
        type="file"
        accept=".wav,audio/wav"
        on:change={(event) => {
          const target = event.currentTarget as HTMLInputElement;
          ringSoundUpload = target.files && target.files.length > 0 ? target.files[0] : null;
          preparedUploadName = ringSoundUpload ? buildUploadSoundName(ringSoundUpload.name) : "";
        }}
      />
    </div>
    <div>
      <label for="ring_sound_select">Available Sounds</label>
      <select id="ring_sound_select" bind:value={selectedRingSound}>
        <option value="">Select sound</option>
        {#each ringSounds as sound}
          <option value={sound}>{sound}</option>
        {/each}
      </select>
    </div>
  </div>
  {#if preparedUploadName}
    <p class="hint">Upload filename: <code>{preparedUploadName}</code></p>
  {/if}
  <div class="button-row">
    <button
      class="secondary"
      type="button"
      disabled={isUploadingRingSound || !ringSoundUpload}
      on:click={async () => {
        if (ringSoundUpload) {
          await onUpload(ringSoundUpload);
        }
      }}
    >
      Upload Sound
    </button>
    <button
      type="button"
      disabled={isUploadingRingSound || !selectedRingSound}
      on:click={onActivate}
    >
      Activate Selected Sound
    </button>
    <button class="secondary" type="button" on:click={onRefresh}>
      Refresh Sounds
    </button>
  </div>
  <p class="hint">Upload a file and activate it. The chime daemon will use it for new rings without a restart.</p>
</section>
