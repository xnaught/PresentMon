import { ref, computed } from 'vue';
import { defineStore } from 'pinia';
import { Api } from '@/core/api';
import type { Metric } from '@/core/metric';
import type { Stat } from '@/core/stat';
import type { Unit } from '@/core/unit';
import type { MetricOption } from '@/core/metric-option';

export const useIntrospectionStore = defineStore('introspection', () => {
  // === State ===
  const metrics = ref<Metric[]>([]);
  const stats = ref<Stat[]>([]);
  const units = ref<Unit[]>([]);

  // === Computed ===
  const metricOptions = computed<MetricOption[]>(() => {
    return metrics.value.flatMap(m => 
      Array.from({ length: m.arraySize }, (_, i) => ({
        metricId: m.id,
        name: m.arraySize > 1 ? `${m.name} [${i}]` : m.name,
        arrayIndex: i,
      }))
    );
  });

  // === Functions ===
  function replaceAllMetrics(newMetrics: Metric[]) {
    metrics.value = newMetrics;
  }

  function replaceAllStats(newStats: Stat[]) {
    stats.value = newStats;
  }

  function replaceAllUnits(newUnits: Unit[]) {
    units.value = newUnits;
  }

  // === Actions ===
  async function load() {
    if (metrics.value.length === 0) {
      const intro = await Api.introspect();
      replaceAllMetrics(intro.metrics);
      replaceAllStats(intro.stats);
      replaceAllUnits(intro.units);
    }
  }

  // === Exports ===
  return {
    metrics,
    stats,
    units,
    metricOptions,
    load
  };
});