<script setup lang="ts">
import { ref, computed, defineProps, defineEmits } from 'vue';
import type { Widget, WidgetType } from '@/core/widget';
import type { Metric } from '@/core/metric';
import type { MetricOption } from '@/core/metric-option';
import type { QualifiedMetric } from '@/core/qualified-metric';
import type { Stat } from '@/core/stat';
import { Loadout } from '@/store/loadout';
import type { AxisAffinity, WidgetMetric } from '@/core/widget-metric';
import ColorPicker from './ColorPicker.vue';
import type { RgbaColor } from '@/core/color';

defineOptions({name: 'LoadoutLine'})

defineProps({
  widgetIdx: { required: true, type: Number },
  lineIdx: { required: true, type: Number },
  widgets: { required: true, type: Array as () => Widget[] },
  metrics: { required: true, type: Array as () => Metric[] },
  stats: { required: true, type: Array as () => Stat[] },
  metricOptions: { required: true, type: Array as () => MetricOption[] },
  locked: { default: false, type: Boolean },
  adapterId: { required: true, type: Number || null },
});

defineEmits(['add', 'delete', 'clearMulti']);

const widget = computed(() => defineProps().widgets[defineProps().widgetIdx]);
const widgetMetric = computed(() => widget.value.metrics[defineProps().lineIdx]);

const widgetType = computed({
  get: () => widget.value.widgetType,
  set: (type: WidgetType) => {
    if (type !== widget.value.widgetType) {
      Loadout.resetWidgetAs({ index: defineProps().widgetIdx, type });
    }
  },
});

const widgetSubtype = computed({
  get: () => (widgetType.value === WidgetType.Graph ? AsGraph(widget.value).graphType.name : ''),
  set: (val: string) => {
    if (widgetType.value === WidgetType.Graph) {
      Loadout.setGraphTypeAttribute({ index: defineProps().widgetIdx, attr: 'name', val });
    }
  },
});

const lineColor = computed({
  get: () => widgetMetric.value.lineColor,
  set: (color: RgbaColor) => {
    const metric = { ...widgetMetric.value, lineColor: color };
    Loadout.setWidgetMetric({ index: defineProps().widgetIdx, metricIdx: defineProps().lineIdx, metric });
  },
});

const fillColor = computed({
  get: () => widgetMetric.value.fillColor,
  set: (color: RgbaColor) => {
    const metric = { ...widgetMetric.value, fillColor: color };
    Loadout.setWidgetMetric({ index: defineProps().widgetIdx, metricIdx: defineProps().lineIdx, metric });
  },
});

const axisAffinityRight = computed({
  get: () => widgetMetric.value.axisAffinity === AxisAffinity.Right,
  set: (affinityRight: boolean) => {
    const axisAffinity = affinityRight ? AxisAffinity.Right : AxisAffinity.Left;
    const metric = { ...widgetMetric.value, axisAffinity };
    Loadout.setWidgetMetric({ index: defineProps().widgetIdx, metricIdx: defineProps().lineIdx, metric });
  },
});

const metricOption = computed({
  get: () => {
    const option = defineProps().metricOptions.find(
      (o) =>
        widgetMetric.value.metric.metricId === o.metricId &&
        widgetMetric.value.metric.arrayIndex === o.arrayIndex
    );
    if (!option) throw new Error('Option not found matching metricId:arrayIndex');
    return option;
  },
  set: (opt: MetricOption) => {
    const currentStatId = widgetMetric.value.metric.statId;
    const newAvailableStats = statsForMetric(opt.metricId);
    let statId = currentStatId;
    if (!newAvailableStats.some((s) => s.id === currentStatId)) {
      statId = newAvailableStats[0].id;
    }
    const newMetric = findMetricById(opt.metricId);
    if (!newMetric.availableDeviceIds.length) {
      throw new Error('Metric in selected metric option is not available for any device');
    }
    if (!newMetric.numeric) {
      widgetType.value = WidgetType.Readout;
    }
    const qualifiedMetric: QualifiedMetric = {
      metricId: opt.metricId,
      arrayIndex: opt.arrayIndex,
      deviceId: 0,
      statId,
      desiredUnitId: newMetric.preferredUnitId,
    };
    const metric = { ...widgetMetric.value, metric: qualifiedMetric };
    Loadout.setWidgetMetric({ index: defineProps().widgetIdx, metricIdx: defineProps().lineIdx, metric });
  },
});

const stat = computed({
  get: () => {
    const stat = defineProps().stats.find((s) => s.id === widgetMetric.value.metric.statId);
    if (!stat) throw new Error(`Stat ID ${widgetMetric.value.metric.statId} not found`);
    return stat;
  },
  set: (stat: Stat) => {
    const qualifiedMetric = { ...widgetMetric.value.metric, statId: stat.id };
    const metric = { ...widgetMetric.value, metric: qualifiedMetric };
    Loadout.setWidgetMetric({ index: defineProps().widgetIdx, metricIdx: defineProps().lineIdx, metric });
  },
});

const statsForMetric = (metricId: number) => {
  return defineProps().stats.filter((s) => findMetricById(metricId).availableStatIds.includes(s.id));
};

const findMetricById = (metricId: number) => {
  const metric = defineProps().metrics.find((m) => m.id === metricId);
  if (!metric) throw new Error(`Metric ID ${metricId} not found`);
  return metric;
};

const widgetTypeToString = (t: WidgetType) => WidgetType[t];

const metricOptionsFiltered = computed(() => {
  return defineProps().lineIdx === 0
    ? defineProps().metricOptions
    : defineProps().metricOptions.filter((o) => findMetricById(o.metricId).numeric);
});

const widgetTypeOptions = computed(() => {
  const opts = [WidgetType.Readout];
  if (findMetricById(widgetMetric.value.metric.metricId).numeric) {
    opts.push(WidgetType.Graph);
  }
  return opts;
});

const widgetSubtypeOptions = computed(() => {
  return widgetType.value === WidgetType.Graph ? ['Line', 'Histogram'] : [];
});

const statOptions = computed(() => statsForMetric(widgetMetric.value.metric.metricId));

const isMaster = computed(() => defineProps().lineIdx === 0);
const isGraphWidget = computed(() => widgetType.value === WidgetType.Graph);
const isLineGraphWidget = computed(() => isGraphWidget.value && AsGraph(widget.value).graphType.name === 'Line');
const isReadoutWidget = computed(() => widgetType.value === WidgetType.Readout);
</script>

<template>
  <div class="widget-line">
    <div class="widget-cell col-metric">
      <v-autocomplete
        v-model="metricOption"
        :items="metricOptionsFiltered"
        item-text="name"
        :disabled="locked"
        return-object
        outlined
        :dense="!isMaster"
        hide-details
      ></v-autocomplete>
    </div>
    <div class="widget-cell col-stat"> 
      <v-select
        v-model="stat"
        :items="statOptions"
        item-text="shortName"
        :disabled="locked || statOptions.length < 2"
        return-object
        outlined
        :dense="!isMaster"
        hide-details
      ></v-select>
    </div>
    <div class="widget-cell col-type">
      <v-select
        v-if="isMaster"
        v-model="widgetType"
        :items="widgetTypeOptions"
        :disabled="locked || widgetTypeOptions.length < 2"
        outlined
        hide-details
      >    
        <template #selection="{ item }">
          {{ widgetTypeToString(item) }}
        </template>
        <template #item="{ item }">
          {{ widgetTypeToString(item) }}
        </template>
      </v-select>
    </div>
    <div class="widget-cell col-subtype">
      <v-select
        v-if="isMaster"
        v-model="widgetSubtype"
        :items="widgetSubtypeOptions"
        :disabled="locked || widgetSubtypeOptions.length < 2"
        outlined
        hide-details
      ></v-select>
      <v-switch v-else v-model="axisAffinityRight" label="Right Axis" hide-details dense class="mt-0"></v-switch>
    </div>
    <div class="widget-cell col-line-color">
      <color-picker
        v-if="isGraphWidget"
        v-model="lineColor"
        :minimal="true"
        class="mb-1"
      ></color-picker>
      <color-picker
        v-if="isGraphWidget"
        v-model="fillColor"
        :minimal="true"
      ></color-picker>
    </div>
    <div class="widget-cell col-controls text-right">
      <v-btn v-if="isMaster && !locked && isGraphWidget" :to="{name: 'graph-config', params: {index: widgetIdx}}" class="widget-btn details-btn" x-large icon>
        <v-icon>mdi-cog</v-icon>
      </v-btn>
      <v-btn v-if="!locked && isReadoutWidget" :to="{name: 'readout-config', params: {index: widgetIdx}}" class="widget-btn details-btn" x-large icon>
        <v-icon>mdi-cog</v-icon>
      </v-btn>
      <v-btn v-if="isMaster && !locked && isLineGraphWidget" @click="$emit('add')" class="widget-btn add-line-btn" x-large icon>
        <v-icon>mdi-plus</v-icon>
      </v-btn>
      <v-btn v-if="isMaster && !locked" @click="$emit('delete', lineIdx)" class="widget-btn remove-btn" x-large icon>
        <v-icon>mdi-close</v-icon>
      </v-btn>
      <v-btn v-if="!isMaster && !locked" @click="$emit('delete', lineIdx)" class="widget-line-btn line-btn mr-2" icon>
        <v-icon>mdi-close</v-icon>
      </v-btn>
    </div>
  </div>
</template>

<style scoped lang="scss">
</style>