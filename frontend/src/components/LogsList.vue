<script setup>
import { onMounted, onUpdated, ref, watch } from 'vue';
import 'mdui/components/button-icon.js';
import 'mdui/components/divider.js';
import 'mdui/components/list.js';
import 'mdui/components/list-item.js';
import { isEmpty, toDateTimeString } from '@/assets/helpers.js'


// Props
const props = defineProps({
    start: { type: Date, default: () => new Date(0) },
    stop: { type: Date, default: () => new Date() }
});


// State & Refs:
const logs = ref([]);


// Fetch Data:
import { useFetchTimeData } from '@/composables/useFetchData';
const { isLoading, payload, fetchData } = useFetchTimeData("/api/web/logs");
watch(() => [props.start, props.stop], ([newStart,newStop]) => {
    fetchData(newStart, newStop);
});
watch(payload, async(newPayload) => { // update logs on new payload
    // Fallback on Empty Data:
    if(isEmpty(newPayload)) {
        logs.value = [];
        return;
    }

    // Parse Payload:
    function tupleToObject([timestamp, level, message]) {
        return { timestamp: new Date(timestamp), level: level, message: message }
    }
    const parsedLogs = newPayload["data"].map(tupleToObject);

    // Sort By Timestamp:
    function sortByDate(a, b) {
        // obj: { timestamp, level, message }
        if (a.timestamp < b.timestamp) return -1;
        if (a.timestamp > b.timestamp) return +1;
        return 0;
    }
    parsedLogs.sort(sortByDate);
    
    // Update State:
    logs.value = parsedLogs;
});
</script>

<template>
    <div v-if="isLoading" class="center info-text">
        <mdui-circular-progress></mdui-circular-progress>
    </div>
    <mdui-list v-else-if="logs.length > 0">
        <template v-for="log in logs">
            <mdui-divider></mdui-divider>
            <mdui-list-item description-line={1} nonclickable>
                <div>
                    {{ log.message }}
                </div>
                <div slot="description">
                    [{{ log.level }}] {{ toDateTimeString(log.timestamp) }}
                </div>
            </mdui-list-item>
        </template>
    </mdui-list>
    <div v-else class="center">
        <mdui-button-icon icon="search_off" variant="standard"></mdui-button-icon>
        <span>No logs found for the selected time.</span>
    </div>
</template>