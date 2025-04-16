<script setup lang="ts">
import { ref } from 'vue'

const inSettings = ref(true)
</script>

<template>
  <v-app>
    <v-navigation-drawer
      v-if="inSettings"
      permanent
      :width="180"
      color="#030308"
      class="pt-3"
      app
    >
      <router-link :to="{name: 'simple'}" class="nav-back"><v-icon class="nav-back-arrow">mdi-arrow-left</v-icon> Top</router-link>
      <v-list nav>
        <v-list-item color="primary" :to="{name: 'overlay-config'}">
          <v-list-item-content>
            <v-list-item-title>Overlay</v-list-item-title>
          </v-list-item-content>
        </v-list-item>
        <v-list-item color="primary" :to="{name: 'metric-processing'}">
          <v-list-item-content>
            <v-list-item-title>Data Processing</v-list-item-title>
          </v-list-item-content>
        </v-list-item>
        <v-list-item color="primary" :to="{name: 'capture-config'}">
          <v-list-item-content>
            <v-list-item-title>Capture</v-list-item-title>
          </v-list-item-content>
        </v-list-item>
        <v-list-item color="primary" :to="{name: 'flash-config'}">
          <v-list-item-content>
            <v-list-item-title>Flash Injection</v-list-item-title>
          </v-list-item-content>
        </v-list-item>
        <v-list-item color="primary" :to="{name: 'other-config'}">
          <v-list-item-content>
            <v-list-item-title>Other</v-list-item-title>
          </v-list-item-content>
        </v-list-item>
        <v-list-item v-if="true" color="primary">
          <v-list-item-content>
            <v-btn @click="">#Updt. Prest.#</v-btn>
          </v-list-item-content>
        </v-list-item>
      </v-list>
    </v-navigation-drawer>

    <v-main>
      <div class="d-flex justify-center"> 
        <router-view></router-view>
      </div>
    </v-main>

    <v-footer
      color="primary darken-3"
      height="22"
      class="status-bar"
      app
    >
      <div class="sta-region">
        <div class="pl-2">{{ "Heaven.exe" }}</div>
        <div><v-icon v-show="true" small color="red darken-1">mdi-camera-control</v-icon></div>
      </div>
      <div class="sta-region">
        <div>{{ "Hidden" }}</div>
        <div>{{ 60 }}Hz</div>
        <div>{{ 15 }}fps</div>
      </div>
    </v-footer>
  </v-app>
</template>

<style scoped>
* {
  user-select: none; 
}
.status-bar {
  padding: 0;
  font-size: 12px;
  font-weight: 300;
  justify-content: space-between;
  user-select: none;
}
.sta-region {
  display: flex;
}
.sta-region > div {
  display: flex;
  align-items: center;
  padding: 0 4px;
}
.menu-item {
  border-radius: 0;
}
.nav-back {
  text-decoration: none;
  color: whitesmoke;
  margin-left: 10px;
  text-transform: uppercase;
  font-size: 18px;
}
</style>