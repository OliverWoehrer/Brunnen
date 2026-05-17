<script setup>
import { onMounted, onUnmounted, ref } from 'vue';
import 'mdui/components/button-icon.js';
import 'mdui/components/card.js';
import 'mdui/components/chip.js';
import 'mdui/components/divider.js';
import 'mdui/components/icon.js';
import 'mdui/components/list.js';
import 'mdui/components/list-item.js';
import 'mdui/components/segmented-button-group.js';
import 'mdui/components/segmented-button.js';
import 'mdui/components/tabs.js';
import 'mdui/components/tab.js';
import 'mdui/components/tab-panel.js';
import 'mdui/components/top-app-bar-title.js';

import Filter from '@/components/Filter.vue';
import Graphs from '@/components/Graphs.vue';
import LogsList from '@/components/LogsList.vue';


// State & Refs:
const startDate = ref(new Date(1000));
const stopDate = ref(new Date());


// Screen Size:
import { useScreenSize } from '@/composables/useScreenSize';
const { isAtLeast } = useScreenSize();
const isLargeScreen = isAtLeast('large');


// Drag&Drop Resize:
const mainWidth = ref(50);
const isResizing = ref(false);
function startResizing() { isResizing.value = true; }
function stopResizing() { isResizing.value = false; }
function resize(mouseMoveEvent) {
    if(!isResizing.value) return;
    const width = ((mouseMoveEvent.clientX-56) / window.innerWidth) * 100; // calculate percentage based on window width
    if(20 < width && width < 80) { // constrain between 20% and 80%
        mainWidth.value = width;
    }
}


// Filter Update Logic:
function onUpdate(payload) {
    startDate.value = payload.start;
    stopDate.value = payload.stop;
    console.log(`onUpdate(start=${startDate.value}, stop=${stopDate.value});`);
}


// Lifecycle Hooks:
onMounted(() => {
    window.addEventListener("mousemove", resize);
    window.addEventListener("mouseup", stopResizing);
});

onUnmounted(() => {
    window.removeEventListener("mousemove", resize);
    window.removeEventListener("mouseup", stopResizing);
});
</script>

<template>
    <!-- Layout: Large Screens -->
    <div  v-if="isLargeScreen" className="aside-layout">
        <main v-bind:style="{ width: `${mainWidth}%` }">
            <section>
                <Filter v-bind:start="startDate" v-bind:stop="stopDate" @update="onUpdate"/>
            </section>
            <section>
                <Graphs v-bind:start="startDate" v-bind:stop="stopDate"/>
            </section>
        </main>
        <div class="divider" v-on:mousedown="isResizing = true">
            <mdui-icon name="drag_indicator"></mdui-icon>
        </div>
        <aside class="layout">
            <header>
                Logs
            </header>
            <main>
                <LogsList v-bind:start="startDate" v-bind:stop="stopDate"/>
            </main>
        </aside>
    </div>

    <!-- Layout: Smaller Screens -->
    <div v-else class="layout">
        <!-- Header: Time Filter -->
        <header>
            <Filter v-bind:start="startDate" v-bind:stop="stopDate" @update="onUpdate"/>
        </header>

        <!-- Tabs: Graphs and Logs -->
        <main>
            <mdui-tabs value="graphs" full-width style="height: 100%;">
                <!-- Titles -->
                <mdui-tab value="graphs">Graphs</mdui-tab>
                <mdui-tab value="logs">Logs</mdui-tab>

                <!-- Panel 1: Graphs -->
                <mdui-tab-panel slot="panel" value="graphs">
                    <Graphs v-bind:start="startDate" v-bind:stop="stopDate"/>
                </mdui-tab-panel>

                <!-- Panel 2: Logs -->
                <mdui-tab-panel slot="panel" value="logs">
                    <LogsList v-bind:start="startDate" v-bind:stop="stopDate"/>
                </mdui-tab-panel>
            </mdui-tabs>
        </main>
    </div>
</template>

<style scoped>
article {
    margin-bottom: 12px;
}
header {
    box-sizing: border-box;
    padding: 0.75rem 0.75rem;
}
section {
    margin-bottom: 0.5rem;
    margin-top: 0.5rem;
}
mdui-card {
    box-sizing: content-box;
    padding: 0.75rem;
}
.center {
    height: 100%;
    display: flex;
    justify-content: center;
    align-items: center;
}


.layout {
    display: flex;
    flex-direction: column;
    height: 100%;
}
.layout > header {
    flex-grow: 0;
}
.layout > main {
    flex-grow: 1;
    /* overflow-x: hidden; */
    overflow-y: auto;
}


.aside-layout {
    align-items: stretch;
    box-sizing: border-box;
    display: flex;
    flex-direction: row;
    height: 100%;
}
.aside-layout > main {
    flex-basis: auto;
    flex-grow: 0;
    flex-shrink: 1;
    min-width: 0;
    height: 100%;
    overflow-x: hidden;
}
.aside-layout > aside {
    box-sizing: border-box;
    flex-basis: 0;
    flex-grow: 1;
    flex-shrink: 1;

    height: 100%;
    overflow-y: auto;
}
.divider {
    align-items: center;
    cursor: col-resize;
    display: flex;
    justify-content: center;
    user-select: none;
}
.divider:hover {
    background-color: rgb(var(--mdui-color-surface-container));
}
</style>