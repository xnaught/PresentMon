<template>
  <v-app>
    <v-main>
      <v-container class="py-10">
        <v-card class="pa-6" elevation="4">
          <v-card-title>Dashboard View</v-card-title>

          <v-card-text>
            <v-row>
              <v-col cols="12" md="6">
                <v-text-field
                  label="Search"
                  prepend-inner-icon="mdi-magnify"
                  variant="outlined"
                ></v-text-field>
              </v-col>
              <v-col cols="12" md="6">
                <v-btn color="primary" class="me-2">Submit</v-btn>
                <v-btn color="secondary" variant="outlined">Reset</v-btn>
              </v-col>
            </v-row>

            <v-divider class="my-6" />

            <v-row>
              <v-col cols="12" md="4">
                <v-switch label="Enable option" color="primary" v-model="enabled" />
              </v-col>
              <v-col cols="12" md="4">
                <v-slider v-model="value" :max="100" label="Volume" />
              </v-col>
              <v-col cols="12" md="4">
                <v-progress-circular
                  :model-value="value"
                  :size="50"
                  :width="6"
                  color="primary"
                  indeterminate
                ></v-progress-circular>
              </v-col>
            </v-row>
          </v-card-text>
        </v-card>
      </v-container>
    </v-main>
  </v-app>
</template>

<script setup lang="ts">
import { ref } from 'vue'

const enabled = ref(true)
const value = ref(40)
</script>