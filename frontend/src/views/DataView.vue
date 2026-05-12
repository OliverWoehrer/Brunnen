<script setup>
import 'mdui/components/button-icon.js';
import 'mdui/components/tabs.js';
import 'mdui/components/tab.js';
import 'mdui/components/tab-panel.js';
import 'mdui/components/top-app-bar.js';
import 'mdui/components/top-app-bar-title.js';
</script>

<template>
    
    <!-- <mdui-top-app-bar variant="small" style="">
        <mdui-top-app-bar-title>Data History</mdui-top-app-bar-title>
        <mdui-button-icon icon="person"></mdui-button-icon>
    </mdui-top-app-bar> -->
    <!-- <div style="position: relative; overflow: hidden;" id="default">
        <mdui-top-app-bar variant="center-aligned" scroll-target="#default">
            <mdui-top-app-bar-title>
                TreeAPI
            </mdui-top-app-bar-title>
            <mdui-button-icon icon="account_circle"></mdui-button-icon>
        </mdui-top-app-bar>
    </div> -->

    <mdui-tabs value="graphs" full-width>
        <mdui-tab value="graphs">Graphs</mdui-tab>
        <mdui-tab value="logs">Logs</mdui-tab>

        <mdui-tab-panel slot="panel" value="graphs">Panel 1</mdui-tab-panel>
        <mdui-tab-panel slot="panel" value="logs">Panel 2</mdui-tab-panel>
    </mdui-tabs>
    

</template>