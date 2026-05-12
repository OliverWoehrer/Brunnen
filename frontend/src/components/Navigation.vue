<script setup>
import { computed } from 'vue';
import { useRoute, useRouter } from 'vue-router';
import { useScreenSize } from '../composables/useScreenSize';
import 'mdui/components/navigation-bar.js';
import 'mdui/components/navigation-bar-item.js';
import 'mdui/components/navigation-rail.js';
import 'mdui/components/navigation-rail-item.js';

const { isAtLeast } = useScreenSize();
const isLargeScreen = isAtLeast('large');

const route = useRoute();
const router = useRouter();
const navValue = computed(() => {
    const value = route.path.split("/")[1]; // path "/settings" -> value "settings"
    return value || ''; // default to "/"
});

function onNavigate(event) {
    const value = event.target.value;
    router.push("/" + value);
}
</script>


<template>
    <template v-if="isLargeScreen">
        <mdui-navigation-rail v-bind:value="navValue" v-on:change="onNavigate" order="-1">
            <mdui-navigation-rail-item value="dashboard" icon="dashboard--outlined" active-icon="dashboard">Dashboard</mdui-navigation-rail-item>
            <mdui-navigation-rail-item value="data" icon="data_exploration--outlined" active-icon="data_exploration">Data</mdui-navigation-rail-item>
            <mdui-navigation-rail-item value="settings" icon="settings--outlined" active-icon="settings">Settings</mdui-navigation-rail-item>
            <!-- <mdui-navigation-rail-item value="profile" icon="cloud_upload--outlined" active-icon="cloud_upload">Update</mdui-navigation-rail-item> -->
        </mdui-navigation-rail>
    </template>
    <template v-else>
        <mdui-navigation-bar v-bind:value="navValue" v-on:change="onNavigate" label-visibility="labeled">
            <mdui-navigation-bar-item value="dashboard" icon="dashboard--outlined" active-icon="dashboard">Dashboard</mdui-navigation-bar-item>
            <mdui-navigation-bar-item value="data" icon="data_exploration--outlined" active-icon="data_exploration">Data</mdui-navigation-bar-item>
            <mdui-navigation-bar-item value="settings" icon="settings--outlined" active-icon="settings">Settings</mdui-navigation-bar-item>
            <!-- <mdui-navigation-bar-item value="profile" icon="cloud_upload--outlined" active-icon="cloud_upload">Update</mdui-navigation-bar-item> -->
        </mdui-navigation-bar>
    </template>
</template>