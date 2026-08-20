<script lang="ts">
  export let text: string;
  export let isError = false;
  export let showSpinner = false;
</script>

{#if text}
  <div class:error={isError} class="message">{#if showSpinner}<span class="spinner" aria-hidden="true"></span>{/if}{text}</div>
{/if}
