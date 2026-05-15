<script setup>
import 'mdui/components/button.js';
defineProps({
    code: { type: String, default: 500 },
    message: { type: String, default: "Unknown Error" },
})
</script>

<template>
    <img src="/logo-broken.png" style="max-height: 30vh;">
    <h1>Something went wrong.</h1>
    <p>
        Error<span v-if="code">&nbsp;{{ code }}</span>: <span v-if="message">{{ message }}</span>
    </p>
    <p>
        <mdui-button href="/" icon="home">Back to home</mdui-button>
    </p>
</template>

<style scoped>
</style>