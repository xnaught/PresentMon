<script setup lang="ts">
import Sortable from 'sortablejs';
import { useIntrospectionStore } from '@/stores/introspection';
import type { Widget } from '@/core/widget';
import LoadoutRow from '@/components/LoadoutRow.vue';
import { onMounted, ref } from 'vue';
import { useLoadoutStore } from '@/stores/loadout';

defineOptions({name: 'LoadoutConfigView'})

const intro = useIntrospectionStore();
const loadout = useLoadoutStore()
const activeAdapterId = ref<number|null>(null);
let sort: Sortable|null = null;

onMounted(() => {  
  sort = new Sortable(document.querySelector('#sortable-row-container')!, {
    draggable: '.sortable-row',
    handle: '.sortable-handle',
    forceFallback: true,
    onChoose: e => e.target.classList.add('sortable-grabbing'),
    onUnchoose: e => e.target.classList.remove('sortable-grabbing'),
    onStart: e => e.target.classList.add('sortable-grabbing'),
    onEnd: e => {
        e.target.classList.remove('sortable-grabbing')
        dragReorder(e)
    },
  })
})

const dragReorder = (e: Sortable.SortableEvent) => {
  if (e.oldIndex !== undefined && e.newIndex !== undefined) {
    loadout.moveWidget(e.oldIndex, e.newIndex)
  }
}

const save = () => {
  console.log('Save called');
};

const load = () => {
  console.log('Load called');
};

const addWidget = () => {
  loadout.addGraph()
};

const removeWidget = (widgetIdx:number) => {
  console.log('Remove Widget called with index:', widgetIdx)
};
</script>

<template>
  <div class="flex-grow-1 flex-column mx-lg-12 mx-xl-16">
    <h2 class="mt-5 ml-5 link-head" @click="$router.back()">
        <v-icon style="vertical-align: 0" color="inherit">mdi-chevron-left</v-icon>
        Loadout Configuration
    </h2>

    <v-row class="mt-5 loadout-table" id="sortable-row-container">
    <loadout-row
        v-for="(w, i) in loadout.widgets" :key="w.key" :stats="intro.stats"
        :widgetIdx="i" :widgets="loadout.widgets" :metrics="intro.metrics" 
        :metricOptions="intro.metricOptions" :adapterId="activeAdapterId" :locked="false" 
        @delete="removeWidget" 
    ></loadout-row>
    <div class="add-btn-row">
        <v-btn @click="addWidget()" class="add-btn">Add New Widget</v-btn>
    </div>
    </v-row>

    <v-row>
    <v-col cols="6" style="text-align: center"><v-btn @click="save()">Save</v-btn></v-col>
    <v-col cols="6" style="text-align: center"><v-btn @click="load()">Load</v-btn></v-col>
    </v-row>
  </div>
</template>

<style lang="scss">
.loadout-table { 
  user-select: none; 
  .sortable-handle {
    cursor: grab;
  }
  .sortable-handle:active {
    cursor: grabbing !important;
  }
  .sortable-ghost {
      background-color: #444 !important;
  }
  .widget-btn, .line-btn {
    visibility: hidden;
    opacity: 0.4;
  }
  .widget-row:hover .widget-btn {
    visibility: visible;
  }
  .widget-row:hover .widget-btn.remove-btn:hover {
    opacity: 1;
    color: red !important;
  }
  .widget-row:hover .widget-btn.add-line-btn:hover {
    opacity: 1;
    color: rgb(47, 255, 64) !important;
  }
  .widget-row:hover .widget-btn.details-btn:hover {
    opacity: 1;
    color: rgb(92, 148, 253) !important;
  }
}
.widget-row {
  width: 100%;
  display: flex;
  margin: 3px;
  border: 1px solid rgb(32, 32, 32);
  border-radius: 3px;
  background-color: rgb(26, 26, 26);
}
.grip-wrap {
  flex-grow: 0;
  align-self: center;
  padding-left: 2px;
}
.line-wrap {
  flex-grow: 1;
}
.widget-line {
  display: flex;
  width: 100%;
  &:hover .line-btn {    
    visibility: visible;
  }
  &:hover .line-btn:hover {
    opacity: 1;
    color: red !important;
  }
}
.widget-row .widget-cell {
  margin: 2px;
  padding: 2px;
}
.widget-row .widget-cell.col-metric {
  flex: 3;
}
.widget-row .widget-cell.col-stat {
  width: 110px;
}
.widget-row .widget-cell.col-type {
  width: 120px;
}
.widget-row .widget-cell.col-subtype {
  width: 140px;
}
.widget-row .widget-cell.col-line-color {
  max-width: 50px;
  flex: 0.7;
  display: flex;
  flex-direction: column;
}
.widget-row .widget-cell.col-controls {
  width: 160px;
  vertical-align: middle;
}
.add-btn-row {
  width: 100%;
  display: flex;
  justify-content: center;
}
.sortable-grabbing * {
  cursor: grabbing !important;
}
button.v-btn.add-btn.add-btn {
  padding: 24px 96px;
  margin: 8px
}
.link-head { 
  color: white; 
  cursor: pointer; 
  user-select: none; 
  transition: color .2s; 
} 
.link-head:hover { 
  color: #2196f3; 
} 
</style>