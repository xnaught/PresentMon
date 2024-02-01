<!-- Copyright (C) 2022 Intel Corporation -->
<!-- SPDX-License-Identifier: MIT -->

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
        <template slot="selection" slot-scope="data">
          {{ widgetTypeToString(data.item) }}
        </template>
        <template slot="item" slot-scope="data">
          {{ widgetTypeToString(data.item) }}
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

<script lang="ts">
import Vue from 'vue'
import { AsGraph, Widget, WidgetType } from '@/core/widget'
import { Metric } from '@/core/metric'
import { MetricOption } from '@/core/metric-option'
import { QualifiedMetric } from '@/core/qualified-metric'
import { Stat } from '@/core/stat'
import { Loadout } from '@/store/loadout'
import { AxisAffinity, makeDefaultWidgetMetric, WidgetMetric }  from '@/core/widget-metric'
import ColorPicker from './ColorPicker.vue'
import { RgbaColor } from '@/core/color'

export default Vue.extend({
    name: 'LoadoutLine',

    components: {
      ColorPicker,
    },

    props: {
      widgetIdx: {required: true, type: Number},
      lineIdx: {required: true, type: Number},
      widgets: {required: true, type: Array as () => Widget[]},
      metrics: {required: true, type: Array as () => Metric[]},
      stats: {required: true, type: Array as () => Stat[]},
      metricOptions: {required: true, type: Array as () => MetricOption[]},
      locked: {default: false, type: Boolean},
    },

    data: () => ({
      metricOption: null as MetricOption|null,
      stat: null as Stat|null,
    }),

    beforeMount() {
      // this.metricOption = this.metricOptions.find(o =>
      //   this.qualifiedMetric.metricId === o.metricId &&
      //   this.qualifiedMetric.arrayIndex === o.arrayIndex) ?? null;
      let option: MetricOption|null = null;
      let qualifiedMetric = this.qualifiedMetric;
      for (const opt of this.metricOptions) {
        let matches = true;
        matches = matches && qualifiedMetric.metricId === opt.metricId;
        matches = matches && qualifiedMetric.arrayIndex === opt.arrayIndex;
        if (matches) {
          option = opt;
          break;
        }
      }
      this.metricOption = option;
      if (this.metricOption !== null) {
        if (this.metric.availableStatIds.includes(this.qualifiedMetric.statId)) {
          this.stat = this.stats[this.qualifiedMetric.statId];
        }
      }
    },

    methods: {
      widgetTypeToString(t: WidgetType): string {
        return WidgetType[t];
      },
    },

    computed: {
      widget(): Widget {
        return this.widgets[this.widgetIdx];
      },
      widgetMetric(): WidgetMetric {
        return this.widget.metrics[this.lineIdx];
      },
      // qualifiedMetric: see the computed set/get below in spec binding
      metric(): Metric {
        return this.metrics[this.qualifiedMetric.metricId];
      },
      widgetTypeOptions(): WidgetType[] {
        const opts = [WidgetType.Readout];
        if (this.metric.numeric) {
          opts.push(WidgetType.Graph);
        }
        return opts;
      },
      metricOptionsFiltered(): MetricOption[] {
        // subordinate metrics cannot choose a non-numeric metric
        // because only graphs support multi-metrics, and only
        // numeric metrics work with graphs
        return this.isMaster ?
          this.metricOptions :
          this.metricOptions.filter(o => this.metrics[o.metricId].numeric);
      },
      widgetSubtypeOptions(): string[] {
        return this.widget.widgetType === WidgetType.Graph ? ['Line', 'Histogram'] : [];
      },
      statOptions(): Stat[] {
        const opts = this.stats.filter(s => this.metric.availableStatIds.includes(s.id));
        return opts;
      },
      isMaster(): boolean {
        return this.lineIdx === 0;
      },
      isGraphWidget(): boolean {
        return this.widgetType == WidgetType.Graph;
      },
      isLineGraphWidget(): boolean {
        return this.isGraphWidget && AsGraph(this.widget).graphType.name === 'Line';
      },
      isReadoutWidget(): boolean {
        return this.widgetType == WidgetType.Readout;
      },
      // spec bindings
      qualifiedMetric: {
        get(): QualifiedMetric {
          return this.widgetMetric.metric;
        },
        set(metric: QualifiedMetric) {
          const widgetMetric: WidgetMetric = {...this.widgetMetric, metric};
          Loadout.setWidgetMetric({index: this.widgetIdx, metricIdx: this.lineIdx, metric: widgetMetric});
        }
      },
      widgetType: {        
        set(type: WidgetType) {
          if (type !== this.widget.widgetType) {
            Loadout.resetWidgetAs({index: this.widgetIdx, type});
          }
        },
        get(): WidgetType {
          return this.widget.widgetType;
        }
      },
      widgetSubtype: {        
        set(val: string) {
          if (this.widgetType === WidgetType.Graph) {
            Loadout.setGraphTypeAttribute({index: this.widgetIdx, attr: 'name', val})
          }
        },
        get(): string {
          return this.widgetType === WidgetType.Graph ? AsGraph(this.widget).graphType.name : '';
        }
      },
      lineColor: {
        get(): RgbaColor {
          return this.widgetMetric.lineColor;
        },
        set(lineColor: RgbaColor) {
          const metric: WidgetMetric = {...this.widgetMetric, lineColor};
          Loadout.setWidgetMetric({index: this.widgetIdx, metricIdx: this.lineIdx, metric});
        }
      },
      fillColor: {
        get(): RgbaColor {
          return this.widgetMetric.fillColor;
        },
        set(fillColor: RgbaColor) {
          const metric: WidgetMetric = {...this.widgetMetric, fillColor};
          Loadout.setWidgetMetric({index: this.widgetIdx, metricIdx: this.lineIdx, metric});
        }
      },
      axisAffinityRight: {
        get(): boolean {
          return this.widgetMetric.axisAffinity === AxisAffinity.Right;
        },
        set(affinityRight: boolean) {
          const axisAffinity = affinityRight ? AxisAffinity.Right : AxisAffinity.Left;
          const metric: WidgetMetric = {...this.widgetMetric, axisAffinity};
          Loadout.setWidgetMetric({index: this.widgetIdx, metricIdx: this.lineIdx, metric});
        }
      }
    },

    watch: {
      // TODO: rework this binding after reading is complete
      // metricOption(newOption: MetricOption) {
      //   // if a new option is selected, reset to first available stat-id
      //   // but don't reset if current id is in the set (happens when loading file)
      //   if (!newOption.metricIds.includes(this.metricId)) {
      //     this.metricId = newOption.metricIds[0];
      //   }
      //   if (this.metric.className !== 'Numeric') {
      //     this.widgetType = WidgetType.Readout;
      //   }
      // },
      widgetSubtype(newSubtype: string) {
        if (newSubtype !== 'Line') {
          this.$emit('clearMulti');
        }
      }
    }
});
</script>

<style scoped lang="scss">
</style>
