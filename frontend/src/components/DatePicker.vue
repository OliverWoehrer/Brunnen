<script setup>
import { useTemplateRef } from 'vue';
import 'mdui/components/button.js';
import 'mdui/components/button-icon.js';
import 'mdui/components/dialog.js';
import 'mdui/components/top-app-bar-title.js';
import 'mdui/components/text-field.js';
import "@/assets/DatePicker.js";
import { toDateString, toISOString } from '@/assets/helpers.js';


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
    <mdui-text-field label="Date" readonly v-bind:disabled="props.disabled" v-bind:value="toDateString(props.dateObj)" v-on:click="openDialog">
        <mdui-icon slot="icon" name="calendar_month"></mdui-icon>
    </mdui-text-field>
    <input type="hidden" v-bind:name="name" v-bind:value="toISOString(props.dateObj)" />

    <mdui-dialog ref="dialog-ref" close-on-esc close-on-overlay-click @close="closeDialog">
        <date-picker ref="picker-ref" v-bind:date="toISOString(props.dateObj)" @confirm="confirmDate" @reset="closeDialog">
            <span slot="supporting-text">Select a date</span>
            <mdui-top-app-bar-title slot="headline"></mdui-top-app-bar-title>
            <mdui-button-icon slot="prev-month-btn" icon="keyboard_arrow_left"></mdui-button-icon>
            <mdui-button-icon slot="next-month-btn" icon="keyboard_arrow_right"></mdui-button-icon>
            <mdui-button slot="cancel-btn" variant="text">Cancel</mdui-button>
            <mdui-button slot="confirm-btn">OK</mdui-button>
        </date-picker>
    </mdui-dialog>
</template>