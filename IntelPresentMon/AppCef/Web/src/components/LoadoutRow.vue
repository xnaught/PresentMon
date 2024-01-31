<!-- Copyright (C) 2022 Intel Corporation -->
<!-- SPDX-License-Identifier: MIT -->

<template>
<div class="widget-row sortable-row">
  <div class="grip-wrap">
    <v-icon v-show="!locked" class="sortable-handle">mdi-drag-horizontal-variant</v-icon>
  </div>
  <div class="line-wrap">
    <loadout-line
      v-for="(oneBased, i) in metrics.length" :key="lineKeys[i]"
      :widgetIdx="widgetIdx" :lineIdx="i" :widgets="widgets"
      :metrics="metrics" :metricOptions="metricOptions" :locked="locked"
      @delete="handleDelete" @add="handleAdd" @clearMulti="handleClearMulti"
    ></loadout-line>
  </div>
</div>
</template>

<script lang="ts">
import Vue from 'vue'
import { Widget } from '@/core/widget'
import { Metric } from '@/core/metric'
import { MetricOption } from '@/core/metric-option'
import { Stat } from '@/core/stat'
import LoadoutLine from './LoadoutLine.vue'
import { Loadout } from '@/store/loadout'

export default Vue.extend({
    name: 'LoadoutRow',

    components: {
      LoadoutLine,
    },

    props: {
      widgetIdx: {required: true, type: Number},
      widgets: {required: true, type: Array as () => Widget[]},
      metrics: {required: true, type: Array as () => Metric[]},
      stats: {required: true, type: Array as () => Stat[]},
      metricOptions: {required: true, type: Array as () => MetricOption[]},
      locked: {default: false, type: Boolean}
    },

    data: () => ({
      lineKeys: [] as number[],
      nextKey: 0,
    }),

    mounted() {
      this.lineKeys = Array(this.metrics.length).fill(0).map((n, i) => i);
      this.nextKey = this.metrics.length;
    },

    methods: {
      deleteLine(lineIdx: number) {
          Loadout.removeWidgetMetric({index: this.widgetIdx, metricIdIdx: lineIdx});
          this.lineKeys.splice(lineIdx, 1);
      },
      handleDelete(lineIdx: number) {
        if (lineIdx === 0) {
          this.$emit('delete', this.widgetIdx);
        } else {
          this.deleteLine(lineIdx);
        }
      },
      handleAdd() {
        try {
          Loadout.addWidgetMetric({index: this.widgetIdx, metric: null});
          this.lineKeys.push(this.nextKey++);
        } catch (e) {}
      },
      handleClearMulti() {
        while (this.lineKeys.length > 1) {
          this.deleteLine(1);
        }
      }
    },

    computed: {
      widget(): Widget {
        return this.widgets[this.widgetIdx];
      },
    }
});
</script>

<style scoped lang="scss">
</style>
