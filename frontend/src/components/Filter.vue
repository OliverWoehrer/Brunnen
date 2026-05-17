<script setup>
import { onMounted, onUnmounted, ref, useTemplateRef, watch } from 'vue';
import 'mdui/components/card.js';
import 'mdui/components/segmented-button-group.js';
import 'mdui/components/segmented-button.js';
import DatePicker from '@/components/DatePicker.vue';
import TimePicker from '@/components/TimePicker.vue';


// Props:
const props = defineProps({
    name: { type: String, default: "" },
    start: { type: Date, default: () => new Date(0) },
    stop: { type: Date, default: () => new Date() },
});


// State & Refs:
const quickFilter = useTemplateRef("quick-filter");
const isDisabled = ref(true); // disables date and time picker inputs
const latestData = ref(new Date());


// Emitted Events:
const emit = defineEmits(['update']);


// Fetch Data:
import { useFetchStructData } from '@/composables/useFetchData';
const { isLoading, payload, fetchData } = useFetchStructData("/api/web/sync");
fetchData();
watch(payload, async(newPayload) => {
    latestData.value = new Date(newPayload["last_data"]);
    onFilterChange();
});


// Helper Functions:
function calculateStartDate(period) {
    const stop = latestData.value;
    const start = new Date(stop.getTime() - period);
    emit("update", { start:start, stop:stop }); // emit 'update' event to parent
}
function updateStartDate(newDate) {
    console.log("updateStartDate(); emitting update event...");
    emit("update", { start:newDate, stop:props.stop }); // emit 'update' event to parent
}
function updateStopDate(newDate) {
    console.log("updateStopDate(); emitting update event...");
    emit("update", { start:props.start, stop:newDate }); // emit 'update' event to parent
}
function onFilterChange() {
    if(!quickFilter.value) return;
    const selection = quickFilter.value.value;
    if(selection !== "0") { // quick filter selected, disable custom inputs
        isDisabled.value = true;
        const period = selection * 60 * 60 * 1000; // selection in milliseconds
        const stop = latestData.value;
        const start = new Date(stop.getTime() - period);
        emit("update", { start:start, stop:stop }); // emit 'update' event to parent
    } else { // no quick filter selected, enable custom inputs
        isDisabled.value = false;
    }
}
</script>

<template>
    <mdui-top-app-bar-title>Browse history data archive</mdui-top-app-bar-title>
    <section>
        <mdui-segmented-button-group ref="quick-filter" full-width selects="single" value="24" @change="onFilterChange">
            <mdui-segmented-button value="1">1 hour</mdui-segmented-button>
            <mdui-segmented-button value="12">12 hours</mdui-segmented-button>
            <mdui-segmented-button value="24">24 hours</mdui-segmented-button>
            <mdui-segmented-button value="0">Custom</mdui-segmented-button>
        </mdui-segmented-button-group>
    </section>
    <section>
        <section class="flex-row">
            <mdui-card variant="filled" style="width: 100%;">
                <div className="info-text">Earliest timestamp</div>
                <DatePicker v-bind:dateObj="props.start" v-bind:disabled="isDisabled" @confirm="updateStartDate"/>
                <TimePicker v-bind:dateObj="props.start" v-bind:disabled="isDisabled" @confirm="updateStartDate"/>
            </mdui-card>
            <mdui-card variant="filled" style="width: 100%;">
                <div className="info-text">Latest timestamp</div>
                <DatePicker v-bind:dateObj="props.stop" v-bind:disabled="isDisabled" @confirm="updateStopDate"/>
                <TimePicker v-bind:dateObj="props.stop" v-bind:disabled="isDisabled" @confirm="updateStopDate"/>
            </mdui-card>
        </section>
    </section>
</template>

<style scoped>
section {
    margin-bottom: 0.5rem;
    margin-top: 0.5rem;
}
mdui-card {
    padding: 0.75rem;
}
</style>