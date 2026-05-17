<script setup>
import { computed } from 'vue';
import { RouterLink, RouterView, useRoute, useRouter } from 'vue-router';
import 'mdui/components/card.js';
import 'mdui/components/layout.js';
import 'mdui/components/layout-item.js';
import 'mdui/components/layout-main.js';
import 'mdui/components/navigation-bar.js';
import 'mdui/components/navigation-bar-item.js';
import 'mdui/components/navigation-rail.js';
import 'mdui/components/navigation-rail-item.js';
import { useScreenSize } from '@/composables/useScreenSize';

const { isAtLeast } = useScreenSize();
const isLargeScreen = isAtLeast('large');

const router = useRouter();
const route = useRoute();
const navValue = computed(() => {
    const value = route.path.split("/")[1]; // path "/settings" -> value "settings"
    return value || ''; // default to "/"
});
function onNavigate(event) {
    const value = event.target.value;
    router.push("/" + value);
}
const isProfileView = computed(() => route.path === "/profile");

</script>

<template>
    <mdui-layout full-height>
        <!-- Navigation -->
        <mdui-layout-item>
            <template v-if="isLargeScreen">
                <mdui-navigation-rail v-bind:value="navValue" v-on:change="onNavigate" alignment="center">
                    <mdui-navigation-rail-item value="dashboard" icon="dashboard--outlined" active-icon="dashboard">Dashboard</mdui-navigation-rail-item>
                    <mdui-navigation-rail-item value="data" icon="data_exploration--outlined" active-icon="data_exploration">Data</mdui-navigation-rail-item>
                    <mdui-navigation-rail-item value="settings" icon="settings--outlined" active-icon="settings">Settings</mdui-navigation-rail-item>
                    <mdui-navigation-rail-item value="profile" icon="account_circle--outlined" active-icon="account_circle">Profile</mdui-navigation-rail-item>
                </mdui-navigation-rail>
            </template>
            <template v-else>
                <mdui-navigation-bar v-bind:value="navValue" v-on:change="onNavigate" label-visibility="labeled">
                    <mdui-navigation-bar-item value="dashboard" icon="dashboard--outlined" active-icon="dashboard">Dashboard</mdui-navigation-bar-item>
                    <mdui-navigation-bar-item value="data" icon="data_exploration--outlined" active-icon="data_exploration">Data</mdui-navigation-bar-item>
                    <mdui-navigation-bar-item value="settings" icon="settings--outlined" active-icon="settings">Settings</mdui-navigation-bar-item>
                    <mdui-navigation-bar-item value="profile" icon="account_circle--outlined" active-icon="account_circle">Profile</mdui-navigation-bar-item>
                </mdui-navigation-bar>
            </template>
        </mdui-layout-item>

        <!-- Main Content -->
        <mdui-layout-main>
                <RouterView />
        </mdui-layout-main>
    </mdui-layout>
</template>


<style scoped>
mdui-layout-main {
    box-sizing: border-box;
}
</style>
