<script lang="ts">
  import MessageBanner from "./MessageBanner.svelte";

  export let title: string;
  export let description: string;
  export let hint: string;
  export let pairingCode: string;
  export let setupPassword: string;
  export let setupPasswordConfirm: string;
  export let authBusy: boolean;
  export let messageText: string;
  export let messageIsError: boolean;
  export let onSubmit: () => Promise<void>;
</script>

<section class="card">
  <h1>{title}</h1>
  <p>{description}</p>
  <form
    on:submit|preventDefault={async () => {
      await onSubmit();
    }}
  >
    <label for="pairing_code">Pairing code</label>
    <input
      id="pairing_code"
      bind:value={pairingCode}
      autocomplete="one-time-code"
      autocapitalize="characters"
      spellcheck="false"
      required
    />
    <label for="setup_password">Admin password</label>
    <input
      id="setup_password"
      type="password"
      bind:value={setupPassword}
      autocomplete="new-password"
      minlength="8"
      maxlength="128"
      required
    />
    <label for="setup_password_confirm">Confirm password</label>
    <input
      id="setup_password_confirm"
      type="password"
      bind:value={setupPasswordConfirm}
      autocomplete="new-password"
      minlength="8"
      maxlength="128"
      required
    />
    <div class="button-row">
      <button type="submit" disabled={authBusy}>Pair device</button>
    </div>
  </form>
  <MessageBanner text={messageText} isError={messageIsError} />
  <p class="hint">{hint}</p>
</section>
