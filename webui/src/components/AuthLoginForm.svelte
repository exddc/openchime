<script lang="ts">
  import MessageBanner from "./MessageBanner.svelte";

  export let loginPassword: string;
  export let authBusy: boolean;
  export let messageText: string;
  export let messageIsError: boolean;
  export let onSubmit: () => Promise<void>;
</script>

<section class="card">
  <h1>Sign in</h1>
  <p>This administration API requires the device password.</p>
  <form
    on:submit|preventDefault={async () => {
      await onSubmit();
    }}
  >
    <label for="login_password">Admin password</label>
    <input
      id="login_password"
      type="password"
      bind:value={loginPassword}
      autocomplete="current-password"
      required
    />
    <div class="button-row">
      <button type="submit" disabled={authBusy}>Sign in</button>
    </div>
  </form>
  <MessageBanner text={messageText} isError={messageIsError} />
</section>
