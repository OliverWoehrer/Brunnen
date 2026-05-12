<script setup>
import { nextTick, onMounted } from 'vue'
import 'mdui/components/card.js';
import 'mdui/components/layout.js';
import 'mdui/components/layout-item.js';
import 'mdui/components/layout-main.js';
import * as echarts from 'echarts';

let flowChart;
let pressureChart;
let levelChart;
let option = {
    tooltip: {
        formatter: '{a} <br/>{b} : {c}%'
    },
    series: [
        {
        name: 'Pressure',
        type: 'gauge',
        progress: {
            show: true
        },
        detail: {
            valueAnimation: true,
            formatter: '{value}'
        },
        data: [
            {
            value: 50,
            name: 'SCORE'
            }
        ]
        }
    ]
};


function handleResize() {
    flowChart?.resize();
}


onMounted(() => {
    // Initialize Chart Plots:
    let flowGaugeElement = document.getElementById("flow_gauge");
    flowChart = echarts.init(flowGaugeElement);
    flowChart.setOption(option);
    // const flowObserver = new ResizeObserver(() => flowChart.resize());
    // observer.observe(flowGaugeElement);

    let pressureGaugeElement = document.getElementById("pressure_gauge");
    pressureChart = echarts.init(pressureGaugeElement);
    pressureChart.setOption(option);

    let levelGaugeElement = document.getElementById("level_gauge");
    levelChart = echarts.init(levelGaugeElement);
    levelChart.setOption(option);

    const observer = new ResizeObserver(() => {
        flowChart.resize();
        pressureChart.resize();
        levelChart.resize();
    });
    observer.observe(levelGaugeElement);

})

</script>

<template>
    <header>
        <span id="latest_sync"></span>
    </header>
    <main class="chart-wrapper">
        <div id="flow_gauge" class="chart"></div>
        <div id="pressure_gauge" class="chart"></div>
        <div id="level_gauge" class="chart"></div>
    </main>
</template>

<style scoped>

.chart-wrapper {
    display: flex;
    width: 100%;
    height: 100%;
}

.chart {
    width: 100%;
    height: 100%;
}

</style>





