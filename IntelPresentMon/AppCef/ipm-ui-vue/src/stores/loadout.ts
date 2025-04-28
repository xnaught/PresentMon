import { ref, reactive, readonly, computed } from 'vue'
import { defineStore } from 'pinia'
import { Api } from '@/core/api'
import { getEnumValues } from '@/core/meta'
import { asGraph, type Widget } from '@/core/widget'
import { signature, type LoadoutFile } from '@/core/loadout'
import type { QualifiedMetric } from '@/core/qualified-metric'
import { makeDefaultGraph, type Graph } from '@/core/graph'

export const useLoadoutStore = defineStore('loadout', () => {
    // === State ===
    const widgets = ref<Widget[]>([])
    const debounceToken = ref<number|null>(null)
    
    // === Computed ===
    const fileContents = computed(() => {
        const file: LoadoutFile = {
            signature,
            widgets: widgets.value,
        }
        return JSON.stringify(file, null, 3)
    })

    // === Actions ===
    // graph-specific
    async function addGraph() {
         // TODO: inject these defaults instead of hardcoding
        const qualifiedMetric: QualifiedMetric = {
            metricId: 8,
            arrayIndex: 0,
            statId: 1,
            deviceId: 0,
            desiredUnitId: 0
        }
        widgets.value.push(makeDefaultGraph(qualifiedMetric))
        await serializeCurrent()
    }
    async function setGraphAttribute<K extends keyof Graph>(payload: {index:number, attr: K, val: Graph[K]}) {
        asGraph(widgets.value[payload.index])[payload.attr] = payload.val
        await serializeCurrent()
    }

    async function addReadout() {
        // Mocked Introspection.metrics
        // Original: const metric = Introspection.metrics[0];
        const mockMetrics = [{ id: 1, availableStatIds: [1] }];
        const metric = mockMetrics[0];
        const qualifiedMetric = {
            metricId: metric.id,
            arrayIndex: 0,
            statId: metric.availableStatIds[0],
            deviceId: 0,
            desiredUnitId: 0
        };
        widgets.value.push(makeDefaultReadout(qualifiedMetric));
        await serializeCurrent();
    }

    async function setReadoutAttribute<K extends keyof Readout>(payload: { index: number, attr: K, val: Readout[K] }) {
        asReadout(widgets.value[payload.index])[payload.attr] = payload.val;
        await serializeCurrent();
    }

    async function removeWidget(index: number) {
        widgets.value.splice(index, 1);
        await serializeCurrent();
    }

    async function setWidgetMetrics(payload: { index: number, metrics: WidgetMetric[] }) {
        const widget = widgets.value[payload.index];
        if (widget.widgetType !== 'Graph' || widget.graphType?.name !== 'Line') {
            if (payload.metrics.length > 1) {
                console.warn(`Widget #${payload.index} is not Line Graph but trying to set ${payload.metrics.length} metrics`);
                widget.metrics = [payload.metrics[0]];
            }
        }
        widget.metrics = payload.metrics;
        await serializeCurrent();
    }

    async function addWidgetMetric(payload: { index: number, metric: QualifiedMetric }) {
        const widget = widgets.value[payload.index];
        if (widget.widgetType !== 'Graph' || widget.graphType?.name !== 'Line') {
            console.warn(`Widget #${payload.index} is not Line Graph but trying to add metric`);
            throw new Error('bad addition of metric to widget');
        }
        widget.metrics.push(makeDefaultWidgetMetric(payload.metric));
        await serializeCurrent();
    }

    async function removeWidgetMetric(payload: { index: number, metricIdIdx: number }) {
        const widget = widgets.value[payload.index];
        if (widget.metrics.length < 2) {
            console.warn('Not enough metrics in widget to allow a remove operation');
            return;
        }
        widget.metrics.splice(payload.metricIdIdx, 1);
        await serializeCurrent();
    }

    async function setWidgetMetric(payload: { index: number, metricIdx: number, metric: WidgetMetric }) {
        const widget = widgets.value[payload.index];
        widget.metrics.splice(payload.metricIdx, 1, payload.metric);
        await serializeCurrent();
    }

    async function resetWidgetAs(payload: { index: number, type: WidgetType }) {
        let qualifiedMetric = widgets.value[payload.index].metrics[0]?.metric || null;
        // Mocked Introspection.metrics
        // Original: const metric = Introspection.metrics.find(m => m.id === qualifiedMetric.metricId);
        const mockMetrics = [{ id: 1, numeric: true }];
        if (qualifiedMetric && payload.type === 'Graph') {
            const metric = mockMetrics.find(m => m.id === qualifiedMetric.metricId);
            if (!metric || !metric.numeric) {
                qualifiedMetric = null;
            }
        }
        let newWidget;
        if (payload.type === 'Graph') {
            newWidget = makeDefaultGraph(qualifiedMetric);
        } else {
            newWidget = makeDefaultReadout(qualifiedMetric);
        }
        widgets.value.splice(payload.index, 1, newWidget);
        await serializeCurrent();
    }

    async function moveWidget(payload: { from: number, to: number }) {
        const movedItem = widgets.value.splice(payload.from, 1)[0];
        widgets.value.splice(payload.to, 0, movedItem);
        await serializeCurrent();
    }

    async function parseAndReplace(payload: { payload: string }) {
        const loadout = JSON.parse(payload.payload);
        if (loadout.signature.code !== signature.code) {
            throw new Error(`Bad loadout file format; expect:${signature.code} actual:${loadout.signature.code}`);
        }
        if (loadout.signature.version !== signature.version) {
            // Mock migration
            // Original: migrateLoadout(loadout);
            console.info(`loadout migrated to ${signature.version}`);
        }
        loadout.widgets = loadout.widgets.filter(w => w.metrics.length > 0);
        widgets.value.splice(0, widgets.value.length, ...loadout.widgets);
    }

    // --- Mocked Actions ----
    async function browseAndSerialize() {
        // await Api.browseStoreSpec(this.fileContents);
        console.log(`serialize browse: ${fileContents}`)
    }
    async function serializeCurrent() {
        if (debounceToken.value !== null) {
            clearTimeout(debounceToken.value)
        }
        const token = setTimeout(() => {
            debounceToken.value = null
            // Api.storeConfig(this.fileContents, 'custom-auto.json');
            console.log(`serialize current: ${fileContents}`)
        }, 400);
        debounceToken.value = token
    }

    // === Exports ===
    return {
        // === State ===   
        widgets,

        // === Actions ===
        addGraph,
        setGraphAttribute,
        addReadout,
        setReadoutAttribute,
        removeWidget,
        setWidgetMetrics,
        addWidgetMetric,
        removeWidgetMetric,
        setWidgetMetric,
        resetWidgetAs,
        moveWidget,
        parseAndReplace,
        browseAndSerialize,
        serializeCurrent
    }
})