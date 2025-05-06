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

export const usePreferencesStore = defineStore('preferences', () => {
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

  function setAttribute<K extends keyof PreferencesType>(payload: { attr: K; val: PreferencesType[K] }) {
    preferences.value[payload.attr] = payload.val;
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
    setAttribute({ attr: 'adapterId', val: id });
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
    setAttribute({ attr: 'selectedPreset', val: Preset.Slot1 });
    serialize();
  }

  async function writeAttribute<K extends keyof PreferencesType>(payload: { attr: K; val: PreferencesType[K] }) {
    setAttribute(payload);
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
    setAttribute,
    setDebounceToken,
    setAllPreferences,
    writeAdapterId,
    serialize,
    resetPreferences,
    writeAttribute,
    writeCapture,
    parseAndReplaceRawPreferenceString,
  };
});