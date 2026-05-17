<script setup>
// Props
const props = defineProps({
    start: { type: Date, default: () => new Date(0) },
    stop: { type: Date, default: () => new Date() }
});
</script>

<template>
    <div>Graphs</div>
    <ul>
        <li>start: {{ props.start }}</li>
        <li>stop: {{ props.stop }}</li>
    </ul>
</template>