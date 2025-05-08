<!-- Copyright (C) 2022 Intel Corporation -->
<!-- SPDX-License-Identifier: MIT -->

<script lang="ts" setup>
import { ref, computed } from 'vue';
import { OverlayPosition } from '@/core/overlay-position';

interface Props {
  label: string;
  value: number;
}
const props = defineProps<Props>();
const emit = defineEmits<{
  (e: 'update:modelValue', val: OverlayPosition): void
}>();

const positionKeys = computed(() => Object.freeze({
  tl: OverlayPosition.TopLeft,
  tr: OverlayPosition.TopRight,
  bl: OverlayPosition.BottomLeft,
  br: OverlayPosition.BottomRight,
}));
</script>

<template>
  <div class="position-wrapper">
    <label v-if="props.label.length > 0" class="v-label mb-2">{{ props.label }}</label>
    <div class="position-container">
      <div v-for="(key, name) in positionKeys" :key="name"
        @click="emit('update:modelValue', key)"
        class="position-panel"
        :class="{'position-panel-selected': props.value === key}"
        v-ripple>
      </div>
    </div>
  </div>
</template>

<style scoped>
.position-wrapper {
  display: flex;
  flex-direction: column;
  align-items: center;
  width: 150px;
}
.position-container {
  display: flex;
  flex-direction: row;
  flex-flow: wrap;
  align-self: stretch;
}
.position-panel {
  width: 73px;
  height: 73px;
  margin: 1px;
  background-color: grey;
  cursor: pointer;
  transition: filter .5s;
  filter: brightness(100%);
}
.position-panel-selected {
  background-color: lightskyblue;
}
.position-panel:hover {
  filter: brightness(120%);
}
</style>