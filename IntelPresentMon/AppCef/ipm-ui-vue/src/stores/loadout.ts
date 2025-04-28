import { ref, reactive, readonly, computed } from 'vue'
import { defineStore } from 'pinia'
import { Api } from '@/core/api'
import { getEnumValues } from '@/core/meta'
import { asGraph, type Widget } from '@/core/widget'
import { signature, type LoadoutFile } from '@/core/loadout'
import type { QualifiedMetric } from '@/core/qualified-metric'
import { makeDefaultGraph, type Graph } from '@/core/graph'

export const useHotkeyStore = defineStore('loadout', () => {
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
        // === Actions ===
      }
})