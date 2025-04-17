import { ref, computed } from 'vue'
import { defineStore } from 'pinia'

export const useCounterStore = defineStore('counter', () => {
  const isOpen = ref(true)
  function click() {
    isOpen.value = !isOpen.value
  }

  return { isOpen, click }
})
