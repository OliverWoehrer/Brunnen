/**
 * This file provides composable helper functions (=stateful logic) across the application. It
 * fetches data from the backend and sets some stateful variables.
 */

import { ref, watch, onMounted } from 'vue';
import { snackbar } from 'mdui/functions/snackbar.js';

// Helper Function:
function printMessage(msg, delay = 0) {
    snackbar({ message: msg, autoCloseDelay: delay, closeable: true });
}

export function useFetchTimeData(endpoint) {
    const payload = ref([]);
    const isLoading = ref(false);

    async function fetchData(start = new Date(0), stop = new Date()) {
        // Input Validation:
        console.assert(start instanceof Date, "Given parameter has to be of type 'Date'");
        console.assert(stop instanceof Date, "Given parameter has to be of type 'Date'");
        
        try {
            isLoading.value = true; // enable loading animation
            const params = new URLSearchParams({ start: start.toISOString(), stop: stop.toISOString() });
            const response = await fetch(endpoint+"?"+params, { method:"GET", headers: {Accept:"application/json"} });
            if(!response.ok) {
                const text = await response.text();
                printMessage(`Failed to fetch data from "${endpoint}". ${response.status} ${response.statusText}: ${text}`);
                return;
            }
            
            const parsed = await response.json();
            payload.value = parsed;
        } catch(error) {
            if (error instanceof SyntaxError) {
                printMessage(`Failed to parse response from "${endpoint}": ${error}.`);
            } else {
                printMessage(`Failed to fetch data from "${endpoint}": ${error}.`);
            }
        } finally {
            isLoading.value = false; // disable loading animation
        }
    }

    return { isLoading, payload, fetchData };
}


export function useFetchStructData(endpoint) {
    const payload = ref([]);
    const isLoading = ref(false);

    async function fetchData() {
        try {
            isLoading.value = true; // enable loading animation
            const response = await fetch(endpoint, { method: "GET" });
            if(!response.ok) {
                const text = await response.text();
                printMessage(`Failed to fetch data from "${endpoint}". ${response.status} ${response.statusText}: ${text}`);
                return;
            }
            
            const parsed = await response.json();
            payload.value = parsed;
        } catch(error) {
            if (error instanceof SyntaxError) {
                printMessage(`Failed to parse response from "${endpoint}": ${error}.`);
            } else {
                printMessage(`Failed to fetch data from "${endpoint}": ${error}.`);
            }
        } finally {
            isLoading.value = false; // disable loading animation
        }
    }

    // Fetch data immediately when the component mounts
    // onMounted(() => {
    //     fetchData();
    // });

    return { isLoading, payload, fetchData };
}