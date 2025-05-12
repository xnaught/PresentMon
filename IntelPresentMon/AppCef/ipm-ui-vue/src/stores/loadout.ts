import { ref, reactive, readonly, computed } from 'vue'
import { defineStore } from 'pinia'
import { Api } from '@/core/api'
import { getEnumValues } from '@/core/meta'
import { asGraph, asReadout, WidgetType, type Widget } from '@/core/widget'
import { signature, type LoadoutFile } from '@/core/loadout'
import type { QualifiedMetric } from '@/core/qualified-metric'
import { makeDefaultGraph, type Graph } from '@/core/graph'
import { makeDefaultReadout, type Readout } from '@/core/readout'
import { makeDefaultWidgetMetric, type WidgetMetric } from '@/core/widget-metric'
import { debounce, type DelayedTask } from '@/core/timing'

export const useLoadoutStore = defineStore('loadout', () => {
    // === State ===
    const widgets = ref<Widget[]>([])
    
    // === Nonreactive State ===
    const debounceSerializeTask: DelayedTask<void>|null = null

    // === Computed ===
    const fileContents = computed(() => {
        const file: LoadoutFile = {
            signature,
            widgets: widgets.value
        }
        return JSON.stringify(file, null, 3)
    })

    // === Functions ===
    // TODO: move private functions here

    // === Actions ===
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
    }

    async function addReadout() {
        // Mocked Introspection.metrics
        // Original: const metric = Introspection.metrics[0]
        const mockMetrics = [{ id: 1, availableStatIds: [1] }]
        const metric = mockMetrics[0]
        const qualifiedMetric = {
            metricId: metric.id,
            arrayIndex: 0,
            statId: metric.availableStatIds[0],
            deviceId: 0,
            desiredUnitId: 0
        }
        widgets.value.push(makeDefaultReadout(qualifiedMetric))
    }

    async function removeWidget(index: number) {
        widgets.value.splice(index, 1)
    }

    async function setWidgetMetrics(index: number, metrics: WidgetMetric[]) {
        const widget = widgets.value[index]
        if (widget.widgetType === WidgetType.Graph) {
            if ((widget as Graph).graphType.name === 'Line') {
                widget.metrics = metrics
                await serializeCurrent()
                return
            }
        }
        if (metrics.length > 1) {
            console.warn(`Widget #${index} is not Line Graph but trying to set ${metrics.length} metrics`)
        }
        widget.metrics = [metrics[0]]
    }

    async function addWidgetMetric(index: number, metric: QualifiedMetric|null) {
        const widget = widgets.value[index]
        if (widget.widgetType === WidgetType.Graph) {
            if ((widget as Graph).graphType?.name !== 'Line') {
                console.warn(`Widget #${index} is not Line Graph but trying to add metric`)
                throw new Error('bad addition of metric to widget')
            }
            widget.metrics.push(makeDefaultWidgetMetric(metric))
        } else {
            console.warn(`Widget #${index} is not Graph but trying to add metric`)
            throw new Error('bad addition of metric to widget')
        }
    }

    async function removeWidgetMetric(index: number, metricIdIdx: number) {
        const widget = widgets.value[index]
        if (widget.metrics.length < 2) {
            console.warn('Not enough metrics in widget to allow a remove operation')
            return
        }
        widget.metrics.splice(metricIdIdx, 1)
    }

    async function setWidgetMetric(index: number, metricIdx: number, metric: WidgetMetric) {
        const widget = widgets.value[index]
        widget.metrics.splice(metricIdx, 1, metric)
    }

    async function resetWidgetAs(index: number, type: WidgetType) {
        let qualifiedMetric: QualifiedMetric | null = widgets.value[index].metrics[0]?.metric || null
        // Mocked Introspection.metrics
        // Original: const metric = Introspection.metrics.find(m => m.id === qualifiedMetric.metricId)
        const mockMetrics = [{ id: 1, numeric: true }]
        if (qualifiedMetric && type === WidgetType.Graph) {
            const metric = mockMetrics.find(m => m.id === qualifiedMetric!.metricId)
            if (!metric || !metric.numeric) {
                qualifiedMetric = null
            }
        }
        let newWidget: Widget
        if (type === WidgetType.Graph) {
            newWidget = makeDefaultGraph(qualifiedMetric)
        } else {
            newWidget = makeDefaultReadout(qualifiedMetric)
        }
        widgets.value.splice(index, 1, newWidget)
    }

    async function moveWidget(from: number, to: number) {
        const movedItem = widgets.value.splice(from, 1)[0]
        widgets.value.splice(to, 0, movedItem)
    }

    // loads loadout from json string data without any error handling
    async function parseAndReplace(payload: string) {
        const loadout = JSON.parse(payload) as LoadoutFile
        if (loadout.signature.code !== signature.code) {
            throw new Error(`Bad loadout file format; expect:${signature.code} actual:${loadout.signature.code}`)
        }
        if (loadout.signature.version !== signature.version) {
            // Mock migration
            // Original: migrateLoadout(loadout)
            console.info(`loadout migrated to ${signature.version}`)
        }
        loadout.widgets = loadout.widgets.filter(w => w.metrics.length > 0)
        widgets.value.splice(0, widgets.value.length, ...loadout.widgets)
    }

    // wraps parseAndReplace in try/catch and handles errors
    async function loadConfigFromPayload(payload: string, err: string) {
        try {
            await parseAndReplace(payload);
        } catch (e: any) {
            if (e.noticeOverride) {
                err += e.message ?? '';
            }
            // Mocked Notifications.notify
            // Original: await Notifications.notify({ text: err });
            console.error(`Notification: ${err}`);
            console.error([err, e]);
        }
    }
    
    async function browseAndSerialize() {
        await Api.browseStoreSpec(fileContents.value)
    }

    function serializeCurrent() {
        debounce(() => {
            Api.storeConfig(fileContents.value, 'custom-auto.json')
        }, 400, debounceSerializeTask)
    }

    // === Exports ===
    return {
        widgets,
        addGraph,
        addReadout,
        removeWidget,
        setWidgetMetrics,
        addWidgetMetric,
        removeWidgetMetric,
        setWidgetMetric,
        resetWidgetAs,
        moveWidget,
        parseAndReplace,
        loadConfigFromPayload,
        browseAndSerialize,
        serializeCurrent
    }
})