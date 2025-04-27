<script setup lang="ts">
import { ref, watchEffect } from 'vue';
import { useCounterStore } from '@/stores/counter';
import { type RgbaColor } from '@/core/color';
import ColorPicker from '@/components/ColorPicker.vue';
const counter = useCounterStore()
const color = ref<RgbaColor>({r: 0, g: 0, b: 0, a:0})
watchEffect(() => {
  console.log(JSON.stringify(color.value))
})
</script>

<template>
  <div>
    <v-btn @click="counter.click">Click Me</v-btn>
    <ColorPicker v-model="color"></ColorPicker>
  </div>
</template>
