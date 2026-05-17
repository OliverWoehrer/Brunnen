<script setup>
import { useTemplateRef } from 'vue';
import 'mdui/components/button.js';
import 'mdui/components/button-icon.js';
import 'mdui/components/dialog.js';
import 'mdui/components/top-app-bar-title.js';
import 'mdui/components/text-field.js';
import "@/assets/TimePicker.js";
import { toTimeString, toISOString } from '@/assets/helpers.js';


// Props:
const props = defineProps({
    name: { type: String, default: "" },
    dateObj: { type: Date, default: () => new Date() },
    disabled: { type: Boolean, default: false },
});


// Emitted Events:
const emit = defineEmits(['confirm']);


// State & Refs:
const dialog = useTemplateRef("dialog-ref");
const picker = useTemplateRef("picker-ref");


// Helper Functions:
const openDialog = () => { if (dialog.value) dialog.value.open = true; };
const closeDialog = () => { if (dialog.value) dialog.value.open = false; };

function confirmDate() {
    const confirmedDateObj = picker.value?.confirmedDateObj;
    emit("confirm", confirmedDateObj); // emit 'confirm' event to parent
    closeDialog();
};

function closeHandler() {
    if(!picker.value) return;
    picker.value.selectedDateObj = picker.value.confirmedDateObj;
};
</script>

<template>
    <mdui-text-field label="Time" readonly v-bind:disabled="props.disabled" v-bind:value="toTimeString(props.dateObj)" v-on:click="openDialog">
        <mdui-icon slot="icon" name="access_time"></mdui-icon>
    </mdui-text-field>
    <input type="hidden" v-bind:name="name" v-bind:value="toISOString(props.dateObj)" />

    <mdui-dialog ref="dialog-ref" close-on-esc close-on-overlay-click @close="closeDialog">
        <time-picker ref="picker-ref" v-bind:time="toISOString(props.dateObj)" @confirm="confirmDate" @reset="closeDialog">
            <span slot="supporting-text">Select a time</span>
            <mdui-top-app-bar-title slot="headline"></mdui-top-app-bar-title>
            <mdui-text-field slot="hours" type="number"></mdui-text-field>
            <mdui-button-icon slot="inc-hours" icon="keyboard_arrow_up"></mdui-button-icon>
            <mdui-button-icon slot="dec-hours" icon="keyboard_arrow_down"></mdui-button-icon>
            <mdui-text-field slot="minutes" type="number"></mdui-text-field>
            <mdui-button-icon slot="inc-minutes" icon="keyboard_arrow_up"></mdui-button-icon>
            <mdui-button-icon slot="dec-minutes" icon="keyboard_arrow_down"></mdui-button-icon>
            <mdui-text-field slot="seconds" type="number"></mdui-text-field>
            <mdui-button-icon slot="inc-seconds" icon="keyboard_arrow_up"></mdui-button-icon>
            <mdui-button-icon slot="dec-seconds" icon="keyboard_arrow_down"></mdui-button-icon>
            <mdui-text-field slot="millis" type="number"></mdui-text-field>
            <mdui-button-icon slot="inc-millis" icon="keyboard_arrow_up"></mdui-button-icon>
            <mdui-button-icon slot="dec-millis" icon="keyboard_arrow_down"></mdui-button-icon>
            <mdui-button slot="cancel-btn" variant="text">Cancel</mdui-button>
            <mdui-button slot="confirm-btn">OK</mdui-button>
        </time-picker>
    </mdui-dialog>
</template>