<script setup lang="ts">
import { useCounterStore } from '@/stores/counter'; 
const counter = useCounterStore()
</script>

<template>
  <div>
    <v-btn @click="counter.click">Click Me</v-btn>
  </div>
</template>
