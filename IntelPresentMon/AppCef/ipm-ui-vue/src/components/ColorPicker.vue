<script setup lang="ts">
import { ref, computed, defineProps, defineEmits } from 'vue';
import type { RgbaColor } from '@/core/color';
import { makeCssString } from '@/core/color';

defineOptions({name: 'ColorPicker'})

defineProps({
  label: { type: String, default: '' },
  color: { type: Object as () => RgbaColor },
  minimal: { type: Boolean, default: false },
});

defineEmits(['update']);

const menuActive = ref(false);

const swatchStyle = computed(() => {
  const c = defineProps().color;
  return {
    backgroundColor: makeCssString(c),
  };
});
</script>

<template>
  <v-card color="#242426">
    <v-card-text class="d-flex flex-column align-center" :class="{'pa-0': minimal}">
      <label v-if="label.length > 0" class="v-label pb-3">{{ label }}</label>
      <v-menu v-model="menuActive" top :close-on-content-click="false">
        <template v-slot:activator="{ on }">
          <div class="swatch" :style="swatchStyle" v-on="on"></div>
        </template>
        <v-card>
          <v-card-text class="pa-0">
            <v-color-picker @input="$emit('update', $event)" :value="color" flat></v-color-picker>
          </v-card-text>
        </v-card>
      </v-menu>
    </v-card-text>
  </v-card>
</template>

<style lang="scss" scoped>
.swatch {
  transition: border-color .1s;
  border: 1px solid #333;
  cursor: pointer;
  min-width: 30px;
  min-height: 18px;
  align-self: stretch;
  border-radius: 4px;
}
.swatch:hover {
  border: 1.5px solid #fff;
}
</style>