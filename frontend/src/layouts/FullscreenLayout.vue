<script setup>
import { RouterLink, RouterView } from 'vue-router';
</script>

<template>
    <main>
        <RouterView />
    </main>
</template>


<style scoped>
main {
    display: flex;
    flex-direction: column;
    justify-content: center;
    align-items: center;
    height: 100%;
}
</style>
