// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
import { ref, computed } from 'vue';
import { defineStore } from 'pinia';
import { Api } from '@/core/api';
import { type Preferences as PreferencesType, type PreferenceFile, makeDefaultPreferences, Preset } from '@/core/preferences';
import { combinationsAreSame } from '@/core/hotkey';
import { signature } from '@/core/preferences';
import { useHotkeyStore } from './hotkey';
import { type DelayToken, dispatchDelayedTask } from '@/core/timing';
//import { Adapters } from './adapters';
import { migratePreferences } from '@/core/preferences-migration';
import type { Widget } from '@/core/widget';
import { useLoadoutStore } from './loadout';
import { useIntrospectionStore } from './introspection';

export const usePreferencesStore = defineStore('preferences', () => {
  // === Dependent Stores ===
  const loadout = useLoadoutStore()
  const intro = useIntrospectionStore()

  // === State ===
  const preferences = ref<PreferencesType>(makeDefaultPreferences())
  const capturing = ref(false)
  const captureDurationToken = ref<DelayToken | null>(null)
  const capturingActive = ref(false)
  const pid = ref<number | null>(null)
  const debounceToken = ref<number | null>(null)
  const hotkeys = useHotkeyStore()

  // === Actions ===
  function setCapture(active: boolean) {
    capturing.value = active;
  }

  function setCaptureDurationToken(token: DelayToken | null) {
    captureDurationToken.value = token;
  }

  function setPid(newPid: number | null) {
    pid.value = newPid;
  }

  function setDebounceToken(token: number | null) {
    debounceToken.value = token;
  }

  function setAllPreferences(prefs: PreferencesType) {
    preferences.value = { ...preferences.value, ...prefs };
    capturing.value = false;
    captureDurationToken.value = null;
    capturingActive.value = false;
    pid.value = null;
    debounceToken.value = null;
  }

  async function writeAdapterId(id: number) {
    await Api.setAdapter(id);
    preferences.value.adapterId = id;
    serialize();
  }

  function serialize() {
    if (debounceToken.value !== null) {
      clearTimeout(debounceToken.value);
    }
    const token = setTimeout(() => {
      debounceToken.value = null;
      const file: PreferenceFile = {
        signature,
        preferences: preferences.value,
        hotkeyBindings: hotkeys.bindings,
      };
      Api.storePreferences(JSON.stringify(file, null, 3));
    }, 400);
    debounceToken.value = token;
  }

  function resetPreferences() {
    setAllPreferences(makeDefaultPreferences());
    preferences.value.selectedPreset = Preset.Slot1;
    serialize();
  }

  async function writeCapture(active: boolean) {
    if (active) {
      if (preferences.value.enableCaptureDuration) {
        setCaptureDurationToken(
          dispatchDelayedTask(() => setCapture(false), preferences.value.captureDuration * 1000).token
        );
      }
      setCapture(true);
    } else {
      if (captureDurationToken.value) {
        captureDurationToken.value.cancel();
        setCaptureDurationToken(null);
      }
      setCapture(false);
    }
  }

  async function parseAndReplaceRawPreferenceString(payload: { payload: string }) {
    const config = JSON.parse(payload.payload) as PreferenceFile;
    if (config.signature.code !== signature.code) throw new Error('Bad file format');
    if (config.signature.version !== signature.version) {
      migratePreferences(config);
    }

    Object.assign(preferences.value, config.preferences);

    for (const key in config.hotkeyBindings) {
      const newBinding = config.hotkeyBindings[key];
      const oldBinding = hotkeys.bindings[key];
      if (newBinding.combination === null) {
        if (oldBinding.combination !== null) {
          await hotkeys.clearHotkey(newBinding.action);
        }
      } else if (oldBinding.combination === null || !combinationsAreSame(newBinding.combination, oldBinding.combination)) {
        await hotkeys.bindHotkey(newBinding);
      }
    }

    // TODO: implement adapters
    // if (preferences.value.adapterId !== null) {
    //   if (preferences.value.adapterId >= Adapters.adapters.length) {
    //     setAttribute({ attr: 'adapterId', val: null });
    //   } else {
    //     await Api.setAdapter(preferences.value.adapterId);
    //   }
    // }
  }
  
  async function pushSpecification() {
    // TODO: try structuredClone instead of JSON.parse(JSON.stringify())
    const widgets = JSON.parse(JSON.stringify(loadout.widgets)) as Widget[];
    console.log('Widgets before processing:', JSON.stringify(widgets, null, 2)); // Log widgets array with pretty print
    for (const widget of widgets) {
      // Filter out the widgetMetrics that do not meet the condition, modify those that do
      widget.metrics = widget.metrics.filter(widgetMetric => {
        const metric = intro.metrics.find(m => m.id === widgetMetric.metric.metricId);
        if (metric === undefined || metric.availableDeviceIds.length === 0) {
          // If the metric is undefined, this widgetMetric will be dropped
          return false;
        }
        // If the metric is found, set up the deviceId and desiredUnitId as needed
        widgetMetric.metric.deviceId = 0; // establish universal device id
        // Check whether metric is a gpu metric, then we need non-universal device id
        if (!metric.availableDeviceIds.includes(0)) {
          // if no specific adapter id set, assume adapter id = 1 is active
          const adapterId = preferences.value.adapterId !== null ? preferences.value.adapterId : 1;
          // Set adapter id for this query element to the active one if available
          if (metric.availableDeviceIds.includes(adapterId)) {
            widgetMetric.metric.deviceId = adapterId;
          } else { // if active adapter id is not available drop this widgetMetric
            return false;
          }
        }
        // Fill out the unit
        widgetMetric.metric.desiredUnitId = metric.preferredUnitId;
        // Since the metric is defined, keep this widgetMetric by returning true
        return true;
      });
    }
    await Api.pushSpecification({
      pid: pid.value,
      preferences: preferences.value,
      widgets: widgets.filter(w => w.metrics.length > 0),
    });
  }

  // === Exports ===
  return {
    preferences,
    capturing,
    captureDurationToken,
    capturingActive,
    pid,
    debounceToken,
    setCapture,
    setCaptureDurationToken,
    setPid,
    setDebounceToken,
    setAllPreferences,
    writeAdapterId,
    serialize,
    resetPreferences,
    writeCapture,
    parseAndReplaceRawPreferenceString,
    pushSpecification
  };
});