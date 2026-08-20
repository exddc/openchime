<script lang="ts">
  import { onMount } from "svelte";
  import AuthLoginForm from "./components/AuthLoginForm.svelte";
  import AuthPairForm from "./components/AuthPairForm.svelte";
  import VersionFooter from "./components/VersionFooter.svelte";
  import {
    errorMessage,
    getAuthStatus,
    login,
    logout,
    pairDevice,
    setAuthErrorHandler,
  } from "./lib/api";
  import { screenFromAuthStatus, type AppScreen } from "./lib/shell";
  import { getActiveProduct } from "./products";

  const activeProduct = getActiveProduct();
  const ProductPage = activeProduct.page;

  let messageText = "";
  let messageIsError = false;
  let screen: AppScreen = "loading";
  let pairingCode = "";
  let setupPassword = "";
  let setupPasswordConfirm = "";
  let loginPassword = "";
  let authBusy = false;
  let productVersion = "unknown";
  let osVersion = "unknown";
  let configVersion = "unknown";

  setAuthErrorHandler((kind) => {
    if (kind === "pair") {
      screen = "pair";
      return;
    }
    screen = "login";
    loginPassword = "";
  });

  function setMessage(text: string, isError = false): void {
    messageText = text;
    messageIsError = isError;
  }

  function clearVersion(): void {
    productVersion = "unknown";
    osVersion = "unknown";
    configVersion = "unknown";
  }

  async function loadSystemVersion(): Promise<void> {
    try {
      const data = await activeProduct.loadVersion();
      productVersion = data.product;
      osVersion = data.os;
      configVersion = data.config;
    } catch (error: unknown) {
      console.warn("loadSystemVersion failed", error);
      clearVersion();
    }
  }

  async function refreshAuth(): Promise<void> {
    screen = screenFromAuthStatus(await getAuthStatus());
  }

  async function submitPairing(): Promise<void> {
    if (setupPassword !== setupPasswordConfirm) {
      throw new Error("Passwords do not match.");
    }
    authBusy = true;
    try {
      await pairDevice(pairingCode.trim(), setupPassword);
      pairingCode = "";
      setupPassword = "";
      setupPasswordConfirm = "";
      screen = "app";
      setMessage("Device paired. You are signed in.", false);
    } finally {
      authBusy = false;
    }
  }

  async function submitLogin(): Promise<void> {
    authBusy = true;
    try {
      await login(loginPassword);
      loginPassword = "";
      screen = "app";
      setMessage("", false);
    } finally {
      authBusy = false;
    }
  }

  async function submitLogout(): Promise<void> {
    authBusy = true;
    try {
      await logout();
      loginPassword = "";
      screen = "login";
      setMessage("Signed out.", false);
    } finally {
      authBusy = false;
    }
  }

  function onProductLoadFailed(error: unknown): void {
    setMessage(errorMessage(error), true);
  }

  onMount(() => {
    refreshAuth()
      .then(async () => {
        await loadSystemVersion();
      })
      .catch((error: unknown) => {
        setMessage(errorMessage(error), true);
        screen = "login";
      });
  });
</script>

<div class="wrap">
  {#if screen === "loading"}
    <section class="card">
      <h1>{activeProduct.loadingTitle}</h1>
      <p>{activeProduct.loadingMessage}</p>
    </section>
  {:else if screen === "pair"}
    <AuthPairForm
      title={activeProduct.pairTitle}
      description={activeProduct.pairDescription}
      hint={activeProduct.pairHint}
      bind:pairingCode
      bind:setupPassword
      bind:setupPasswordConfirm
      {authBusy}
      {messageText}
      {messageIsError}
      onSubmit={async () => {
        try {
          await submitPairing();
        } catch (error) {
          setMessage(errorMessage(error), true);
        }
      }}
    />
  {:else if screen === "login"}
    <AuthLoginForm
      bind:loginPassword
      {authBusy}
      {messageText}
      {messageIsError}
      onSubmit={async () => {
        try {
          await submitLogin();
        } catch (error) {
          setMessage(errorMessage(error), true);
        }
      }}
    />
  {:else}
    <section class="card">
      <div class="console-header">
        <div>
          <h1>{activeProduct.title}</h1>
          <p>{activeProduct.subtitle}</p>
        </div>
        <button
          class="secondary"
          type="button"
          disabled={authBusy}
          on:click={async () => {
            try {
              await submitLogout();
            } catch (error) {
              setMessage(errorMessage(error), true);
            }
          }}
        >
          Sign out
        </button>
      </div>
    </section>

    <ProductPage {messageText} {messageIsError} {setMessage} onLoadFailed={onProductLoadFailed} />
  {/if}

  <VersionFooter
    productLabel={activeProduct.versionLabel}
    {productVersion}
    {osVersion}
    {configVersion}
  />
</div>
