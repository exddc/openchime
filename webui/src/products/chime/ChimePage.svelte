<script lang="ts">
  import { onMount } from "svelte";
  import { CORE_CONFIG_DEFAULTS } from "../../generated/config_schema";
  import {
    errorMessage,
    getCoreConfig,
    getObservedTopics,
    getRingSounds,
    parseTopicList,
    saveCoreConfig,
    scanWifi,
    selectRingSound,
    sleep,
    uploadRingSound,
    waitForApplyCompletion,
    type WifiNetwork,
  } from "../../lib/api";
  import { buildUploadSoundName } from "../../lib/ring-sound";
  import { isSaveDisabled, requireUploadableWav, wifiPasswordHint } from "./actions";
  import MqttCard from "./MqttCard.svelte";
  import NotificationsCard from "./NotificationsCard.svelte";
  import RingSoundCard from "./RingSoundCard.svelte";
  import VolumeCard from "./VolumeCard.svelte";
  import WifiCard from "./WifiCard.svelte";

  export let messageText: string;
  export let messageIsError: boolean;
  export let setMessage: (text: string, isError?: boolean) => void;
  export let onLoadFailed: (error: unknown) => void;
  export let loadRetryDelayMs = 800;

  let wifiSsid = CORE_CONFIG_DEFAULTS.wifi_ssid;
  let wifiPassword = "";
  let mqttHost = CORE_CONFIG_DEFAULTS.mqtt_host;
  let mqttPort = CORE_CONFIG_DEFAULTS.mqtt_port;
  let mqttClientId = CORE_CONFIG_DEFAULTS.mqtt_client_id;
  let mqttUsername = CORE_CONFIG_DEFAULTS.mqtt_username;
  let mqttPassword = "";
  let mqttPasswordSet = false;
  let mqttTlsEnabled = CORE_CONFIG_DEFAULTS.mqtt_tls_enabled;
  let mqttTlsValidateCertificate = CORE_CONFIG_DEFAULTS.mqtt_tls_validate_certificate;
  let mqttTlsCaFile = CORE_CONFIG_DEFAULTS.mqtt_tls_ca_file;
  let mqttTlsCertFile = CORE_CONFIG_DEFAULTS.mqtt_tls_cert_file;
  let mqttTlsKeyFile = CORE_CONFIG_DEFAULTS.mqtt_tls_key_file;
  let ringTopic = CORE_CONFIG_DEFAULTS.ring_topic;
  let notificationSuccessSoundPath =
    CORE_CONFIG_DEFAULTS.notification_success_sound_path ?? "";
  let notificationFailureSoundPath =
    CORE_CONFIG_DEFAULTS.notification_failure_sound_path ?? "";
  let volumeBell = CORE_CONFIG_DEFAULTS.volume_bell;
  let volumeNotifications = CORE_CONFIG_DEFAULTS.volume_notifications;
  let mqttTopics = "";
  let observedTopics: string[] = [];
  let ringSounds: string[] = [];
  let selectedRingSound = "";
  let scanResults: WifiNetwork[] = [];
  let isSaving = false;
  let isUploadingRingSound = false;
  let configHydrated = false;
  let loadFailed = false;
  let applyAbort: AbortController | undefined;
  let retryDelayAbort: AbortController | undefined;

  function requestInit(): RequestInit {
    return { signal: applyAbort?.signal };
  }

  function clampVolumeValue(value: unknown, fallback: number): number {
    if (value == null) {
      return fallback;
    }

    if (typeof value === "string" && value.trim().length === 0) {
      return fallback;
    }

    const parsed = typeof value === "number" ? value : Number(value);
    if (!Number.isFinite(parsed)) {
      return fallback;
    }

    return Math.min(100, Math.max(0, Math.round(parsed)));
  }

  async function runAction(action: () => Promise<void>): Promise<void> {
    try {
      await action();
    } catch (error) {
      if (error instanceof Error && error.name === "AbortError") {
        return;
      }
      setMessage(errorMessage(error), true);
    }
  }

  async function loadConfig(): Promise<void> {
    const data = await getCoreConfig(requestInit());

    wifiSsid = data.wifi_ssid ?? CORE_CONFIG_DEFAULTS.wifi_ssid;
    mqttHost = data.mqtt_host ?? CORE_CONFIG_DEFAULTS.mqtt_host;
    mqttPort = data.mqtt_port ?? CORE_CONFIG_DEFAULTS.mqtt_port;
    mqttClientId = data.mqtt_client_id ?? CORE_CONFIG_DEFAULTS.mqtt_client_id;
    mqttUsername = data.mqtt_username ?? CORE_CONFIG_DEFAULTS.mqtt_username;
    mqttPasswordSet = data.mqtt_password_set ?? false;
    mqttTlsEnabled = data.mqtt_tls_enabled ?? CORE_CONFIG_DEFAULTS.mqtt_tls_enabled;
    mqttTlsValidateCertificate =
      data.mqtt_tls_validate_certificate ??
      CORE_CONFIG_DEFAULTS.mqtt_tls_validate_certificate;
    mqttTlsCaFile = data.mqtt_tls_ca_file ?? CORE_CONFIG_DEFAULTS.mqtt_tls_ca_file;
    mqttTlsCertFile = data.mqtt_tls_cert_file ?? CORE_CONFIG_DEFAULTS.mqtt_tls_cert_file;
    mqttTlsKeyFile = data.mqtt_tls_key_file ?? CORE_CONFIG_DEFAULTS.mqtt_tls_key_file;
    ringTopic = data.ring_topic ?? CORE_CONFIG_DEFAULTS.ring_topic;
    notificationSuccessSoundPath =
      data.notification_success_sound_path ??
      CORE_CONFIG_DEFAULTS.notification_success_sound_path ??
      "";
    notificationFailureSoundPath =
      data.notification_failure_sound_path ??
      CORE_CONFIG_DEFAULTS.notification_failure_sound_path ??
      "";
    volumeBell = data.volume_bell ?? CORE_CONFIG_DEFAULTS.volume_bell;
    volumeNotifications =
      data.volume_notifications ?? CORE_CONFIG_DEFAULTS.volume_notifications;
    mqttTopics = (data.mqtt_topics ?? []).join(",");
    setMessage(wifiPasswordHint(data.wifi_password_set), false);
  }

  async function loadRingSoundState(): Promise<void> {
    const data = await getRingSounds(requestInit());
    ringSounds = data.sounds ?? [];
    selectedRingSound = data.selected_sound ?? "";
  }

  async function loadObservedTopicState(): Promise<void> {
    observedTopics = await getObservedTopics(requestInit());
  }

  async function loadConsole(): Promise<void> {
    await loadConfig();
    configHydrated = true;
    loadFailed = false;
    await Promise.all([loadObservedTopicState(), loadRingSoundState()]);
  }

  function retryLoad(): void {
    retryDelayAbort?.abort();
  }

  async function waitForRetryDelay(): Promise<void> {
    retryDelayAbort = new AbortController();
    try {
      await sleep(loadRetryDelayMs, retryDelayAbort.signal);
    } catch (error) {
      if (error instanceof Error && error.name === "AbortError") {
        if (applyAbort?.signal.aborted) {
          throw error;
        }
        return;
      }
      throw error;
    }
  }

  async function loadConsoleUntilHydrated(): Promise<void> {
    while (!configHydrated) {
      if (applyAbort?.signal.aborted) {
        return;
      }
      try {
        loadFailed = false;
        await loadConsole();
      } catch (error) {
        if (error instanceof Error && error.name === "AbortError") {
          return;
        }
        onLoadFailed(error);
        if (configHydrated) {
          return;
        }
        loadFailed = true;
        await waitForRetryDelay();
      }
    }
  }

  async function scanNetworks(): Promise<void> {
    scanResults = await scanWifi(requestInit());
    if (scanResults.length === 0) {
      setMessage("No networks found.", false);
    }
  }

  async function refreshObservedTopics(): Promise<void> {
    await loadObservedTopicState();
    setMessage("Observed topics refreshed.");
  }

  async function refreshRingSounds(): Promise<void> {
    await loadRingSoundState();
    setMessage("Ring sounds refreshed.", false);
  }

  async function uploadSelectedSound(file: File): Promise<void> {
    const uploadFile = requireUploadableWav(file);
    const uploadName = buildUploadSoundName(uploadFile.name);
    isUploadingRingSound = true;
    try {
      await uploadRingSound(
        uploadName,
        await uploadFile.arrayBuffer(),
        requestInit(),
      );
      await loadRingSoundState();
      selectedRingSound = uploadName;
      setMessage(`Uploaded ${uploadName}. Select it below to activate.`, false);
    } finally {
      isUploadingRingSound = false;
    }
  }

  async function activateSelectedSound(): Promise<void> {
    if (!selectedRingSound) {
      throw new Error("Select a ring sound to activate.");
    }
    const data = await selectRingSound(selectedRingSound, requestInit());
    if (data.selection_persisted === false) {
      setMessage(
        "Ring sound activated, but selected-sound metadata could not be persisted.",
        true,
      );
      return;
    }
    setMessage("Ring sound updated. New rings use this sound immediately.", false);
  }

  async function saveConfig(): Promise<void> {
    isSaving = true;
    setMessage("Saving and applying changes...", false);

    const safeVolumeBell = clampVolumeValue(
      volumeBell,
      CORE_CONFIG_DEFAULTS.volume_bell,
    );
    const safeVolumeNotifications = clampVolumeValue(
      volumeNotifications,
      CORE_CONFIG_DEFAULTS.volume_notifications,
    );

    volumeBell = safeVolumeBell;
    volumeNotifications = safeVolumeNotifications;

    try {
      const data = await saveCoreConfig(
        {
          wifi_ssid: wifiSsid.trim(),
          wifi_password: wifiPassword,
          mqtt_host: mqttHost.trim(),
          mqtt_port: Number(mqttPort),
          mqtt_client_id: mqttClientId.trim(),
          mqtt_username: mqttUsername.trim(),
          mqtt_password: mqttPassword,
          mqtt_tls_enabled: mqttTlsEnabled,
          mqtt_tls_validate_certificate: mqttTlsValidateCertificate,
          mqtt_tls_ca_file: mqttTlsCaFile.trim(),
          mqtt_tls_cert_file: mqttTlsCertFile.trim(),
          mqtt_tls_key_file: mqttTlsKeyFile.trim(),
          mqtt_topics: parseTopicList(mqttTopics),
          ring_topic: ringTopic.trim(),
          notification_success_sound_path: notificationSuccessSoundPath.trim(),
          notification_failure_sound_path: notificationFailureSoundPath.trim(),
          volume_bell: safeVolumeBell,
          volume_notifications: safeVolumeNotifications,
        },
        requestInit(),
      );

      wifiPassword = "";
      mqttPassword = "";
      mqttPasswordSet = data.mqtt_password_set ?? mqttPasswordSet;

      const apply = data.apply;
      if (!apply || apply.state === "succeeded") {
        setMessage("Saved and applied.", false);
        return;
      }

      await waitForApplyCompletion(apply.job_id, { signal: applyAbort?.signal });
      setMessage("Saved and applied.", false);
    } catch (error) {
      if (error instanceof Error && error.name === "AbortError") {
        return;
      }
      throw error;
    } finally {
      isSaving = false;
    }
  }

  onMount(() => {
    applyAbort = new AbortController();
    void loadConsoleUntilHydrated();
    return () => {
      applyAbort?.abort();
      retryDelayAbort?.abort();
    };
  });
</script>

<WifiCard
  bind:wifiSsid
  bind:wifiPassword
  {isSaving}
  {scanResults}
  onScan={() => runAction(scanNetworks)}
/>

<RingSoundCard
  bind:ringSounds
  bind:selectedRingSound
  {isUploadingRingSound}
  onUpload={(file) => runAction(() => uploadSelectedSound(file))}
  onActivate={() => runAction(activateSelectedSound)}
  onRefresh={() => runAction(refreshRingSounds)}
/>

<NotificationsCard
  bind:notificationSuccessSoundPath
  bind:notificationFailureSoundPath
/>

<VolumeCard bind:volumeBell bind:volumeNotifications />

<MqttCard
  bind:mqttHost
  bind:mqttPort
  bind:mqttClientId
  bind:mqttUsername
  bind:mqttPassword
  bind:mqttPasswordSet
  bind:mqttTlsEnabled
  bind:mqttTlsValidateCertificate
  bind:mqttTlsCaFile
  bind:mqttTlsCertFile
  bind:mqttTlsKeyFile
  bind:ringTopic
  bind:mqttTopics
  bind:observedTopics
  {isSaving}
  saveDisabled={isSaveDisabled(isSaving, configHydrated)}
  {messageText}
  {messageIsError}
  onRefreshTopics={() => runAction(refreshObservedTopics)}
  onSave={() => runAction(saveConfig)}
  showRetry={loadFailed && !configHydrated}
  onRetry={retryLoad}
/>
