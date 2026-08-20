<script lang="ts">
  export let productLabel: string;
  export let productVersion: string;
  export let osVersion: string;
  export let configVersion: string;
</script>

<section class="version-strip">
  <span>{productLabel} {productVersion}</span>
  <span class="dot">•</span>
  <span>OS {osVersion}</span>
  <span class="dot">•</span>
  <span>Config {configVersion}</span>
</section>
