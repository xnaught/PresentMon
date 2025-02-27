<!-- Copyright (C) 2022 Intel Corporation -->
<!-- SPDX-License-Identifier: MIT -->

<template>
  <div class="page-wrap">
    
  <h2 class="mt-5 ml-5 link-head">
      Flash Injection
  </h2>

  <v-card class="page-card">

    <v-row class="mt-8">
      <v-col cols="3">
        Enable Injection
        <p class="text--secondary text-sm-caption mb-0">Enable the injector child process.</p>
      </v-col>
      <v-col cols="9">
        <v-row>
            <v-col cols="6">
                <v-switch v-model="enableFlashInjection" label="Enable"></v-switch>
            </v-col>
        </v-row>
      </v-col>
    </v-row>

    <v-row class="mt-4">
    <v-col>
        <h3>Notes</h3>
        <p class="text--secondary">Use the following procedure to initiate injected flash on a target application:</p>
        <ol class="text--secondary text-sm-caption">
            <li>Track the app in PresentMon</li>
            <li>Stop tracking the app</li>
            <li>Close the app</li>
            <li>
                Re-launch the application. The application should now have the flash rendering logic injected
            </li>
            <li>
                Mouse clicks will trigger a white rectangle (flash) to be drawn in the target app
            </li>
        </ol>
        <p class="text--secondary mt-4">Tracking must be toggled on/off and the target app must be restarted for changes to the above settings to take effect.</p>
    </v-col>
    </v-row>

  </v-card>

  </div>
</template>

<script lang="ts">
import Vue from 'vue'
import { Preferences } from '@/store/preferences'
import { Hotkey } from '@/store/hotkey'

export default Vue.extend({
  name: 'FlashInjectorConfig',

  components: {
  },
  data: () => ({
    dialog: false,
  }),
  methods: {
    async reset(): Promise<void> {
      Preferences.resetPreferences();
      await Hotkey.bindDefaults();
      this.dialog = false;
    },
  },
  computed: {
    enableFlashInjection: {
      get(): boolean { return Preferences.preferences.enableFlashInjection; },
      set(ena: boolean) {
        Preferences.writeAttribute({ attr: 'enableFlashInjection', val: ena });
      },
    },
  },
  watch: {
  }
});
</script>

<style scoped>
.top-label {
    margin: 0;
    padding: 0;
    height: auto;
}
.link-head {
  color: white;
  user-select: none;
}
.page-card {
  margin: 15px 0;
  padding: 0 15px 15px;
}
.page-wrap {
  max-width: 750px;
  flex-grow: 1;
}
</style>
