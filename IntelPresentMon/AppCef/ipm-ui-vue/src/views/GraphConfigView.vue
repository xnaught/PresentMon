<script setup lang="ts">
import { ref, computed, watch, onMounted } from 'vue';
import ColorPicker from '@/components/ColorPicker.vue';
import type { RgbaColor } from '@/core/color';
import type { Graph } from '@/core/graph';
import { WidgetType, type Widget } from '@/core/widget';
import { asGraph, WidgetType as WidgetTypeEnum } from '@/core/widget';
import { useLoadoutStore } from '@/stores/loadout';

defineOptions({ name: 'GraphConfigView' });

interface Props {
  index: number;
}
const props = defineProps<Props>();

const loadoutStore = useLoadoutStore();

const widget = computed(() => loadoutStore.widgets[props.index]);
const graph = computed(() => asGraph(widget.value));

const timeRange = ref(10); // Mocked value
const metricPollRate = ref(40); // Mocked value
const totalCount = computed(() => timeRange.value * metricPollRate.value);

const height = computed({
  get: () => graph.value.height,
  set: (val) => loadoutStore.setGraphAttribute(props.index, 'height', val),
});

const vDivs = computed({
  get: () => graph.value.vDivs,
  set: (val) => loadoutStore.setGraphAttribute(props.index, 'vDivs', val),
});

const hDivs = computed({
  get: () => graph.value.hDivs,
  set: (val) => loadoutStore.setGraphAttribute(props.index, 'hDivs', val),
});

const textSize = computed({
  get: () => graph.value.textSize,
  set: (val) => loadoutStore.setGraphAttribute(props.index, 'textSize', val),
});

const gridColor = computed({
  get: () => graph.value.gridColor,
  set: (val) => loadoutStore.setGraphAttribute(props.index, 'gridColor', val),
});

const backgroundColor = computed({
  get: () => graph.value.backgroundColor,
  set: (val) => loadoutStore.setGraphAttribute(props.index, 'backgroundColor', val),
});

const textColor = computed({
  get: () => graph.value.textColor,
  set: (val) => loadoutStore.setGraphAttribute(props.index, 'textColor', val),
});

const typeName = computed({
  get: () => graph.value.graphType.name,
  set: (name) => loadoutStore.setGraphTypeAttribute(props.index, 'name', name),
});

const typeRange = computed({
  get: () => graph.value.graphType.range,
  set: (range) => loadoutStore.setGraphTypeAttribute(props.index, 'range', range),
});

const typeRangeRight = computed({
  get: () => graph.value.graphType.rangeRight,
  set: (rangeRight) => loadoutStore.setGraphTypeAttribute(props.index, 'rangeRight', rangeRight),
});

const autoLeft = computed({
  get: () => graph.value.graphType.autoLeft,
  set: (val) => loadoutStore.setGraphTypeAttribute(props.index, 'autoLeft', val),
});

const autoRight = computed({
  get: () => graph.value.graphType.autoRight,
  set: (val) => loadoutStore.setGraphTypeAttribute(props.index, 'autoRight', val),
});

const autoCount = computed({
  get: () => graph.value.graphType.autoCount,
  set: (val) => loadoutStore.setGraphTypeAttribute(props.index, 'autoCount', val),
});

const typeBinCount = computed({
  get: () => graph.value.graphType.binCount,
  set: (binCount) => loadoutStore.setGraphTypeAttribute(props.index, 'binCount', binCount),
});

const typeCountRange = computed({
  get: () => graph.value.graphType.countRange,
  set: (countRange) => loadoutStore.setGraphTypeAttribute(props.index, 'countRange', countRange),
});
</script>

<template>
  <div class="page-wrap">
    <h2 class="mt-5 ml-5 link-head" @click="$router.back()">
      <v-icon style="vertical-align: 0" color="inherit">mdi-chevron-left</v-icon>
      Detailed Graph Configuration
    </h2>

    <v-card class="page-card my-7">
      <v-card-title class="mt-0 text-medium-emphasis">Graph Settings</v-card-title>
      <v-divider class="ma-0"></v-divider>

      <v-row class="mt-8">
        <v-col v-if="typeName !== 'Line'" cols="3">
          Value Range
          <p class="text-medium-emphasis text-caption mb-0">Range of metric values for the bins this histogram</p>
        </v-col>
        <v-col v-else cols="3">
          Left Value Range
          <p class="text-medium-emphasis text-caption mb-0">Range of metric values to be displayed by the left axis of this graph</p>
        </v-col>
        <v-col cols="9">
          <v-row dense>
            <v-range-slider
              v-model="typeRange"
              :min="0"
              :max="5000"
              :step="0.1"
              :disabled="autoLeft"
              hide-details
            ></v-range-slider>
          </v-row>
          <v-row no-gutters class="mt-2">
            <v-col cols="2">
              <v-text-field
                v-model="typeRange[0]"
                :disabled="autoLeft"
                type="number"
                hide-details
                variant="outlined"
                density="compact"
                hide-spin-buttons
              ></v-text-field>
            </v-col>
            <v-col cols="2" offset="8">
              <v-text-field
                v-model="typeRange[1]"
                :disabled="autoLeft"
                type="number"
                hide-details
                variant="outlined"
                density="compact"
                hide-spin-buttons
              ></v-text-field>
            </v-col>
          </v-row>
        </v-col>
      </v-row>

      <v-row class="mt-3">
        <v-col v-if="typeName !== 'Line'" cols="3">
          Autoscale Range
          <p class="text-medium-emphasis text-caption mb-0">Automatically adjust the range of values for the bins this histogram</p>
        </v-col>
        <v-col v-else cols="3">
          Autoscale Left
          <p class="text-medium-emphasis text-caption mb-0">Automatically adjust the range of the left y-axis</p>
        </v-col>
        <v-col cols="9">
          <v-switch v-model="autoLeft" hide-details></v-switch>
        </v-col>
      </v-row>

      <v-row v-if="typeName === 'Line'" class="mt-8">
        <v-col cols="3">
          Right Value Range
          <p class="text-medium-emphasis text-caption mb-0">Range of metric values to be displayed by the right axis of this graph</p>
        </v-col>
        <v-col cols="9">
          <v-row dense>
            <v-range-slider
              v-model="typeRangeRight"
              :min="0"
              :max="5000"
              :step="0.1"
              :disabled="autoRight"
              hide-details
            ></v-range-slider>
          </v-row>
          <v-row no-gutters class="mt-2">
            <v-col cols="2">
              <v-text-field
                v-model="typeRangeRight[0]"
                :disabled="autoRight"
                type="number"
                hide-details
                variant="outlined"
                density="compact"
                hide-spin-buttons
              ></v-text-field>
            </v-col>
            <v-col cols="2" offset="8">
              <v-text-field
                v-model="typeRangeRight[1]"
                :disabled="autoRight"
                type="number"
                hide-details
                variant="outlined"
                density="compact"
                hide-spin-buttons
              ></v-text-field>
            </v-col>
          </v-row>
        </v-col>
      </v-row>

      <v-row v-if="typeName === 'Line'" class="mt-3">
        <v-col cols="3">
          Autoscale Right
          <p class="text-medium-emphasis text-caption mb-0">Automatically adjust the range of the right y-axis</p>
        </v-col>
        <v-col cols="9">
          <v-switch v-model="autoRight" hide-details></v-switch>
        </v-col>
      </v-row>

      <div v-show="typeName === 'Histogram'">
        <v-row class="mt-8">
          <v-col cols="3">
            Number of Bins
            <p class="text-medium-emphasis text-caption mb-0">Number of bins (bars) in histogram</p>
          </v-col>
          <v-col cols="9">
            <v-slider
              v-model="typeBinCount"
              :min="5"
              :max="200"
              thumb-label="always"
                hide-details
            ></v-slider>
          </v-col>
        </v-row>

        <v-row class="mt-8">
          <v-col cols="3">
            Count Range
            <p class="text-medium-emphasis text-caption mb-0">Range of bin counts displayed by this histogram</p>
          </v-col>
          <v-col cols="9">
            <v-row dense>
              <v-range-slider
                v-model="typeCountRange"
                :min="0"
                :max="5000"
                :step="0.1"
                :disabled="autoCount"
                hide-details
              ></v-range-slider>
            </v-row>
            <v-row no-gutters class="mt-2">
              <v-col cols="2">
                <v-text-field
                  v-model="typeCountRange[0]"
                  :disabled="autoCount"
                  type="number"
                  hide-details
                  variant="outlined"
                  density="compact"
                  hide-spin-buttons
                ></v-text-field>
              </v-col>
              <v-col cols="2" offset="8">
                <v-text-field
                  v-model="typeCountRange[1]"
                  :disabled="autoCount"
                  type="number"
                  hide-details
                  variant="outlined"
                  density="compact"
                  hide-spin-buttons
                ></v-text-field>
              </v-col>
            </v-row>
          </v-col>
        </v-row>

        <v-row v-if="typeName !== 'Line'" class="mt-3">
          <v-col cols="3">
            Autoscale Count
            <p class="text-medium-emphasis text-caption mb-0">Automatically adjust the range of bin counts</p>
          </v-col>
          <v-col cols="9">
            <v-switch v-model="autoCount" hide-details></v-switch>
          </v-col>
        </v-row>

        <v-row v-if="typeName !== 'Line'" class="mt-3">
          <v-col cols="3">
            Total Count
            <p class="text-medium-emphasis text-caption mb-0">Total expected data points being counted into bins</p>
          </v-col>
          <v-col cols="9">
            <p class="text-medium-emphasis text-caption">
              The total count of data points displayed is controlled by <span style="color: orange;">Time Scale</span>
              (<router-link class="app-link" :to="{name: 'overlay-config'}">Settings>Overlay</router-link>) multiplied by <span style="color: orange;">Metric Poll Rate</span>
              (<router-link class="app-link" :to="{name: 'metric-processing'}">Settings>Data Processing</router-link>). <br> Currently it is
              <span style="color: green;">{{ timeRange }}s</span> * <span style="color: green;">{{ metricPollRate }}Hz</span> =
              <span style="color: violet;">{{ totalCount }}</span> data points.
            </p>
          </v-col>
        </v-row>
      </div>
    </v-card>

    <v-card class="page-card my-7">
      <v-card-title class="mt-0 text-medium-emphasis">Style Settings</v-card-title>
      <v-divider class="ma-0"></v-divider>

      <v-row class="mt-5">
        <v-col cols="3">
          Graph Height
          <p class="text-medium-emphasis text-caption mb-0">Vertical size of the graph</p>
        </v-col>
        <v-col cols="9">
          <v-slider
            v-model="height"
            :min="20"
            :max="450"
            thumb-label="always"
            hide-details
          ></v-slider>
        </v-col>
      </v-row>

      <v-row class="mt-8">
        <v-col cols="3">
          Grid Vertical
          <p class="text-medium-emphasis text-caption mb-0">Number of vertical divisions in the grid</p>
        </v-col>
        <v-col cols="9">
          <v-slider
            v-model="vDivs"
            :min="1"
            :max="40"
            thumb-label="always"
            hide-details
          ></v-slider>
        </v-col>
      </v-row>

      <v-row class="mt-8">
        <v-col cols="3">
          Grid Horizontal
          <p class="text-medium-emphasis text-caption mb-0">Number of horizontal divisions in the grid</p>
        </v-col>
        <v-col cols="9">
          <v-slider
            v-model="hDivs"
            :min="1"
            :max="100"
            thumb-label="always"
            hide-details
          ></v-slider>
        </v-col>
      </v-row>

      <v-row class="mt-8">
        <v-col cols="3">
          Font Size
          <p class="text-medium-emphasis text-caption mb-0">Size of text in this readout widget</p>
        </v-col>
        <v-col cols="9">
          <v-slider
            v-model="textSize"
            :min="5"
            :max="80"
            :step="0.5"
            thumb-label="always"
            hide-details
          ></v-slider>
        </v-col>
      </v-row>

      <v-row class="mt-8">
        <v-col cols="3">
          Colors
          <p class="text-medium-emphasis text-caption mb-0">Colors of various elements of the graph</p>
        </v-col>
        <v-col cols="9">
          <v-row dense>
            <v-col cols="4">
              <color-picker v-model="gridColor" class="color-picker" label="Grid"></color-picker>
            </v-col>
            <v-col cols="4">
              <color-picker v-model="backgroundColor" class="color-picker" label="Background"></color-picker>
            </v-col>
            <v-col cols="4">
              <color-picker v-model="textColor" class="color-picker" label="Text"></color-picker>
            </v-col>
          </v-row>
        </v-col>
      </v-row>
    </v-card>
  </div>
</template>

<style scoped>
.color-picker {
  max-width: 150px;
}
i.v-icon.v-icon {
  color: inherit;
}
.link-head {
  color: white;
  cursor: pointer;
  user-select: none;
  transition: color .2s;
}
.link-head:hover {
  color: rgb(var(--v-theme-primary));
}
.page-card {
  margin: 15px 0;
  padding: 0 15px 15px;
}
.page-wrap {
  max-width: 750px;
}
a.app-link {
  color: rgb(var(--v-theme-primary));
}
</style>