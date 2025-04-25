<script setup lang="ts">
import { ref, computed, defineProps, defineEmits, onMounted } from 'vue';
import type { Widget } from '@/core/widget';
import type { Metric } from '@/core/metric';
import type { MetricOption } from '@/core/metric-option';
import type { Stat } from '@/core/stat';
import LoadoutLine from '@/components/LoadoutLine.vue';
import { Loadout } from '@/store/loadout';
import type { WidgetMetric } from '@/core/widget-metric';

defineOptions({name: 'LoadoutRow'})

defineProps({
  widgetIdx: { required: true, type: Number },
  widgets: { required: true, type: Array as () => Widget[] },
  metrics: { required: true, type: Array as () => Metric[] },
  stats: { required: true, type: Array as () => Stat[] },
  metricOptions: { required: true, type: Array as () => MetricOption[] },
  locked: { default: false, type: Boolean },
  adapterId: { required: true, type: Number || null },
});

defineEmits(['delete']);

const lineKeys = ref<number[]>([]);
const nextKey = ref(0);

onMounted(() => {
  lineKeys.value = Array(defineProps().metrics.length).fill(0).map((n, i) => i);
  nextKey.value = defineProps().metrics.length;
});

const deleteLine = (lineIdx: number) => {
  Loadout.removeWidgetMetric({ index: defineProps().widgetIdx, metricIdIdx: lineIdx });
  lineKeys.value.splice(lineIdx, 1);
};

const handleDelete = (lineIdx: number) => {
  if (lineIdx === 0) {
    defineEmits().delete(defineProps().widgetIdx);
  } else {
    deleteLine(lineIdx);
  }
};

const handleAdd = () => {
  try {
    Loadout.addWidgetMetric({ index: defineProps().widgetIdx, metric: null });
    lineKeys.value.push(nextKey.value++);
  } catch (e) {}
};

const handleClearMulti = () => {
  while (lineKeys.value.length > 1) {
    deleteLine(1);
  }
};

const widget = computed(() => defineProps().widgets[defineProps().widgetIdx]);
const widgetMetrics = computed(() => widget.value.metrics);
</script>

<template>
<div class="widget-row sortable-row">
  <div class="grip-wrap">
    <v-icon v-show="!locked" class="sortable-handle">mdi-drag-horizontal-variant</v-icon>
  </div>
  <div class="line-wrap">
    <loadout-line
      v-for="(oneBased, i) in widgetMetrics.length" :key="lineKeys[i]"
      :widgetIdx="widgetIdx" :lineIdx="i" :widgets="widgets" :stats="stats"
      :metrics="metrics" :metricOptions="metricOptions" :locked="locked" :adapterId="adapterId"
      @delete="handleDelete" @add="handleAdd" @clearMulti="handleClearMulti"
    ></loadout-line>
  </div>
</div>
</template>

<style scoped lang="scss">
</style>