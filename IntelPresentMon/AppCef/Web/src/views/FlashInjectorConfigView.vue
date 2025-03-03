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
        Enable Flash Injection
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

    <v-row class="mt-8">
      <v-col cols="3">
        Enable Background
        <p class="text--secondary text-sm-caption mb-0">Renders a static rectangle every frame. When a click occurs the "flash" rectangle is rendered on top of the background.</p>
      </v-col>
      <v-col cols="9">
        <v-row>
            <v-col cols="6">
                <v-switch v-model="flashInjectionBackgroundEnable" label="Enable"></v-switch>
            </v-col>
        </v-row>
      </v-col>
    </v-row>

    <v-row class="mt-4">
    <v-col>
        <h3>Notes</h3>
        <p class="text--secondary text-sm-caption">To perform injection, target an app with the overlay and then restart that same application. Also, any changes to the above settings will only take effect after the target app has been restarted.</p>
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
      set(val: boolean) {
        Preferences.writeAttribute({ attr: 'enableFlashInjection', val });
      },
    },
    flashInjectionBackgroundEnable: {
      get(): boolean { return Preferences.preferences.flashInjectionBackgroundEnable; },
      set(val: boolean) {
        Preferences.writeAttribute({ attr: 'flashInjectionBackgroundEnable', val });
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
