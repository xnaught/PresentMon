<script setup lang="ts">
import { ref } from 'vue'

const inSettings = ref(true)
</script>

<template>
  <v-app>
    <div class="app-layout">
      <div class="content-row">
        <v-navigation-drawer
          v-if="inSettings"
          permanent
          :width="180"
          color="#030308"
          class="custom-drawer"
        >
          <router-link :to="{ name: 'simple' }" class="nav-back">
            <v-icon class="nav-back-arrow">mdi-arrow-left</v-icon> Top
          </router-link>
          <v-list nav>
            <v-list-item color="primary" :to="{ name: 'overlay-config' }">
              <v-list-item-content>
                <v-list-item-title>Overlay</v-list-item-title>
              </v-list-item-content>
            </v-list-item>
            <!-- Add other list items here -->
          </v-list>
        </v-navigation-drawer>

        <v-main class="main-view">
          <router-view />
        </v-main>
      </div>

      <div class="footer-wrap">
        <v-footer class="footer" color="blue-darken-3" height="22">
          <div class="sta-region">
            <div class="pl-2">Heaven.exe</div>
            <v-icon small color="red-darken-1">mdi-camera-control</v-icon>
          </div>
          <div class="sta-region">
            <div>Hidden</div>
            <div>60Hz</div>
            <div>15fps</div>
          </div>
        </v-footer>
      </div>
    </div>
  </v-app>
</template>

<style scoped>
.app-layout {
  display: flex;
  flex-direction: column;
  height: 100vh;
  overflow: hidden;
}

.content-row {
  display: flex;
  flex: 1 1 auto;
  min-height: 0;
  overflow: hidden;
}

.custom-drawer {
  height: calc(100vh - 22px); /* Subtract footer height */
  max-height: calc(100vh - 22px);
  flex-shrink: 0;
  display: flex;
  flex-direction: column;
  overflow-y: auto;
}

.main-view {
  flex: 1;
  overflow-y: auto;
}

.footer-wrap {
  height: 22px;
  flex-shrink: 0;
}

.footer {
  display: flex;
  align-items: center;
  justify-content: space-between;
  font-size: 12px;
  font-weight: 300;
  padding: 0;
  height: 22px !important;
  min-height: 22px !important;
  max-height: 22px !important;
  user-select: none;
  color: white;
}

.sta-region {
  display: flex;
}

.sta-region > div {
  display: flex;
  align-items: center;
  padding: 0 4px;
}

.nav-back {
  text-decoration: none;
  color: whitesmoke;
  margin-left: 10px;
  text-transform: uppercase;
  font-size: 18px;
}
</style>
