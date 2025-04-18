<script lang="ts" setup>
import { ref } from 'vue'

</script>

<template>
<div class="page-wrap">

    <v-card class="page-card my-5 pt-3">
    <v-row>
        <v-col cols="3">
        Process
        <p class="text--secondary text-sm-caption mb-0">Application process to track, overlay and capture</p>
        </v-col>
        <v-col cols="9" class="d-flex align-center">
        <!-- todo mock the process reactives etc. -->
        <v-autocomplete
            :items="processes"
            v-model="pid"
            item-value="pid"
            :filter="selectFilter"
            label="Process"
            @click="refreshProcessList"
            append-icon=""
            :disabled="enableAutotargetting"
            hide-details
            outlined
            clearable
            dense
        >
            <template v-slot:selection="data">
            <template v-if="data.item.windowName">
                {{ makeSelectorName(data.item.windowName) }} 
                <span class="pid-node-inline">[{{ data.item.pid }}]</span>
            </template>            
            <template v-else>
                <div>
                {{ data.item.name }}
                <span class="pid-node">[{{ data.item.pid }}]</span>
                </div>
            </template>
            </template>
            <template v-slot:item="data">            
            <template v-if="data.item.windowName">
                <v-list-item-content>
                <v-list-item-title>{{ makeSelectorName(data.item.windowName) }}</v-list-item-title>
                <v-list-item-subtitle>
                    {{ data.item.name }}
                    <span class="pid-node">[{{ data.item.pid }}]</span>
                </v-list-item-subtitle>
                </v-list-item-content>
            </template>
            <template v-else>
                {{ data.item.name }}
                <span class="pid-node-inline">[{{ data.item.pid }}]</span>
            </template>
            </template>
        </v-autocomplete>
        </v-col>
    </v-row> 
    
    <v-row dense>       
        <v-col cols="3">
        Auto-target
        <p class="text--secondary text-sm-caption mb-0">Automatically target process with the highest GPU utilization</p>
        </v-col>

        <v-col cols="9" class="d-flex align-center">
        <v-switch v-model="enableAutotargetting" label="Enable" hide-details></v-switch>
        </v-col>
    </v-row>   
    
    <!-- todo minimal hotkey-button component -->
    <v-row dense>       
        <v-col cols="3">
        Overlay Hotkey
        <p class="text--secondary text-sm-caption mb-0">Set hotkey to toggle overlay on/off</p>
        </v-col>

        <v-col cols="9" class="d-flex justify-center align-center">
        <hotkey-button :action="toggleOverlayAction"></hotkey-button>
        </v-col>
    </v-row>
    </v-card>
    
    <!-- todo mock selected preset refs -->
    <v-card class="page-card my-5 pt-3">
    <v-row>       
        <v-col cols="3">
        Preset
        <p class="text--secondary text-sm-caption mb-0">Select a preset configuration for overlay widget loadout etc.</p>
        </v-col>

        <v-col cols="9" class="d-flex justify-center align-center">        
        <v-btn-toggle v-model="selectedPreset" :mandatory="selectedPreset !== null">
            <v-btn class="px-5" large>
            Basic
            </v-btn>

            <v-btn class="px-5" large>
            GPU Focus
            </v-btn>

            <v-btn class="px-5" large>
            Power/Temp
            </v-btn>

            <v-btn class="px-5" large :value="1000">
            Custom
            </v-btn>        
        </v-btn-toggle>
        <v-btn
            :to="{name: 'loadout-config'}"
            :disabled="!isCustomPresetSelected"
            color="primary" class="ml-5"
        >
            Edit
        </v-btn>
        </v-col>
    </v-row>

    <!-- minimal hotkey component front -->
    <v-row dense>       
        <v-col cols="3">
        Preset Cycle Hotkey
        <p class="text--secondary text-sm-caption mb-0">Set hotkey for cycling through presets</p>
        </v-col>

        <v-col cols="9" class="d-flex justify-center align-center">    
        <hotkey-button :action="cyclePresetAction"></hotkey-button>
        </v-col>  
    </v-row>
    </v-card>
    
    <v-card class="page-card my-5 pt-3">

    <v-row>
        <v-col cols="3">
        Capture Duration
        <p class="text--secondary text-sm-caption mb-0">Automatically stop capture after N seconds</p>
        </v-col>      
        <v-col cols="2">
            <v-switch v-model="enableCaptureDuration" label="Enable" hide-details></v-switch>
        </v-col>
        <v-col cols="3">
        <v-text-field
            label="Seconds"
            v-model="captureDuration"
            :disabled="!enableCaptureDuration"
            class="mt-4 ml-8"
            hide-details
            type="number"
            outlined
            dense
            hide-spin-buttons
        ></v-text-field>
        </v-col>
    </v-row>

    <!-- minimal hotkey component front -->
    <v-row dense>       
        <v-col cols="3">
        Capture Hotkey
        <p class="text--secondary text-sm-caption mb-0">Set hotkey for capture of per-frame performance data as CSV</p>
        </v-col>

        <v-col cols="9" class="d-flex justify-center align-center">
        <hotkey-button :action="toggleCaptureAction"></hotkey-button>
        </v-col>
    </v-row>

    </v-card>
    
    <v-card class="page-card my-5 pt-3">
    <v-row>       
        <v-col cols="3">
        Capture Storage
        <p class="text--secondary text-sm-caption mb-0">Open the folder containing all frame traces and stats summaries</p>
        </v-col>

        <v-col cols="9" class="d-flex justify-center align-center">
        <v-btn 
            large
            color="secondary"
            class="px-6"
            @click="handleExploreClick"
        >Open in Explorer</v-btn>
        </v-col>
    </v-row>
    </v-card>
    <v-row>
    <v-col cols="12" class="text-right">
        <router-link class="settings-link" :to="{name: 'overlay-config'}">
        Settings
        <v-icon large>mdi-cog</v-icon>
        </router-link>
    </v-col>
    </v-row>
</div>
</template>


<style lang="scss" scoped>
.pid-node {
    font-size: 10px;
    color: grey;
    padding-left: 2px;
}
.pid-node-inline {
    font-size: 12px;
    color: grey;
    padding-left: 8px;
}
.page-card {
    margin: 15px 0;
    padding: 0 15px 15px;
}
.page-wrap {
    max-width: 1024px;
}
.stepper-hilight.stepper-hilight.stepper-hilight.stepper-hilight {
    border:1px solid white;
    box-shadow:
    0 0 2px 1px hsl(125, 100%, 88%),  /* inner white */
    0 0 6px 4px hsl(84, 100%, 59%), /* middle green */
    0 0 9px 5px hsl(266, 100%, 59%); /* outer cyan */
}
.hilight-info {
    color: greenyellow;
}
.settings-link {
    font-size: 24px;
    color: #CCC;
    text-decoration: none;
    &:hover {
    color: #FFF
    }
    transition: color .3s;

    i.v-icon.v-icon {
    color: inherit;
    }
}
</style>

    