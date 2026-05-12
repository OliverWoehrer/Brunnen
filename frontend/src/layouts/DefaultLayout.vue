<script setup>
import { computed } from 'vue';
import { RouterLink, RouterView, useRoute, useRouter } from 'vue-router';
import 'mdui/components/layout.js';
import 'mdui/components/layout-item.js';
import 'mdui/components/layout-main.js';
import { useScreenSize } from '@/composables/useScreenSize';
import Navigation from '@/components/Navigation.vue';

const { isAtLeast } = useScreenSize();
const isLargeScreen = isAtLeast('large');

const router = useRouter();
const route = useRoute();
const isProfileView = computed(() => route.path === "/profile");
</script>

<template>
    <mdui-layout full-height>
        <Navigation />
        <!-- App Bar -->
        <mdui-top-app-bar style="justify-content: space-between;">
            <template v-if="isLargeScreen"> <!-- large screens -->
                <div>&nbsp;</div> <!-- placeholder so logo is middle item -->
                <img src="/favicon-96x96.png" height="100%">
                <template v-if="isProfileView">
                    <mdui-button v-on:click="router.push('/profile')" variant="tonal" end-icon="account_circle">Profile</mdui-button>
                </template>
                <template v-else>
                    <mdui-button v-on:click="router.push('/profile')" variant="outlined" end-icon="account_circle--outlined">Profile</mdui-button>
                </template>
            </template>
            <template v-else> <!-- smaller screens -->
                <mdui-button-icon v-on:click="router.push('/')">
                    <mdui-icon src="/favicon.svg"></mdui-icon>
                </mdui-button-icon>
                <template v-if="isProfileView">
                    <mdui-button-icon v-on:click="router.push('/profile')" variant="tonal" icon="account_circle"></mdui-button-icon>
                </template>
                <template v-else>
                    <mdui-button-icon v-on:click="router.push('/profile')" variant="text" icon="account_circle--outlined"></mdui-button-icon>
                </template>
            </template>
        </mdui-top-app-bar>
        <!-- Main Content -->
        <mdui-layout-main>
            <RouterView />
        </mdui-layout-main>
    </mdui-layout>
</template>


<style scoped>
mdui-layout-main {
    box-sizing: content-box;
}
</style>
