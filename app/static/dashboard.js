/**
 * @author Oliver Woehrer
 * @date September 2024
 * 
 * This file provides functionality for the dashboard to visualize states, logs and data. In
 * general there are three stages of async visualization: Request, Load and Action. A request
 * function fetches the raw data from the endpoint and calls the load function when done
 * (=callback function). A load function then converts raw data into usuable format and calls the
 * action function when done (=callback function). An action function does the actual visualization
 * or other actions if needed.
 * 
 */

// Color Codes:
const COLORS = {
    "Flow": "#4472C4",
    "Pressure": "#ED7D31",
    "Level": "#A5A5A5"
}

// Scaling Factors:
const FLOW_SCALING = 0.002577; // 388 pulses per litre
const PRESSURE_SCALING = 0.002513; // 398 increments per bar
const LEVEL_SCALING = 0.00157356; // 635 increments per meter

// Maximum Limits:
const MAX_LIMITS = {
    "Flow": 100,
    "Pressure": 5,
    "Level": 5,
}

const UNITS = {
    "Flow": "L/s",
    "Pressure": "bar",
    "Level": "m",
}


//===============================================
// Updating Charts
//===============================================

/**
 * Fetches log messages between start and stop and adds them to the table element with the given
 * ID
 * @param {Date} start erliest date-time of logs to fetch
 * @param {Date} stop latest date-time of logs to fetch
 * @param {String} tableID id of the table element
 */
async function updateLogs(start, stop, tableID) {
    const {columns, data} = await fetchLogs(start, stop);
    const tableElem = document.getElementById(tableID);
    if(typeof tableElem === "undefined") {
        console.warn("Could not get element by '"+tableID+"'");
        return;
    }
    appendToTable(tableElem, columns, data);
}

/**
 * Fetches data between start and stop and adds it to the charts given
 * @param {Date} start erliest date-time of logs to fetch
 * @param {Date} stop latest date-time of logs to fetch
 * @param {JSON} gauges element id of gauges canvas
 * @param {String} lineCanvasId element if of lines canvas
 */
async function updateData(start, stop, gauges, lineCanvasId) {
    // Fetch Data:
    let datasets = await fetchData(start, stop);

    function generateData() {
        const labels = [];
        const randFlow = [];
        const randPressure = [];
        const randLevel = [];
        let currentDate = new Date();
        for(let i = 0; i < 5; i++) {
            labels.push(new Date(currentDate));
            randFlow.push(Math.random() * 100); 
            randPressure.push(Math.random() * 5);
            randLevel.push(Math.random() * 5);
            currentDate.setSeconds(currentDate.getSeconds() + 1);
        }
        return { "Time": labels, "Flow": randFlow, "Pressure": randPressure, "Level": randLevel };
    }
    // datasets = generateData();
    
    // Update Gauges Chart(s):
    if(gauges) {
        const values = {
            "Flow": datasets["Flow"].length ? datasets["Flow"].at(-1) : 0,
            "Pressure": datasets["Pressure"].length ? datasets["Pressure"].at(-1) : 0,
            "Level": datasets["Level"].length ? datasets["Level"].at(-1) : 0
        }
        updateGauges(gauges, values);
    }

    // Update Lines:
    if(lineCanvasId) {
        updateLines(lineCanvasId, datasets["Time"], datasets["Flow"], datasets["Pressure"], datasets["Level"]);
    }
}




//===============================================
// Data Fetching
//===============================================

/**
 * Make an asynchronious request to the backend for the timestamp of the latetest data.
 * @returns an async promise for the request
 */
async function fetchLatestTimestamp() {
    let response;
    try {
        response = await fetch("/api/web/sync", {
            method: "GET",
            headers: { Accept: "application/json" }
        });
        if(!response.ok) {
            const res = await response.text();
            throw Error("Server responded: "+res);
        }
    } catch(error) {
        throw Error("Problem while fetching timestamp: "+error);
    }

    try {
        const res = await response.json();
        const lastSync = res["last_sync"];
        return new Date(lastSync);
    } catch(error) {
        throw Error("Failed to parse timestamp: "+error);
    }
}

/**
 * Make an asynchronious request to the backend to request log messages between start and stop.
 * @param {Date} start earliest date-time of logs to fetch
 * @param {Date} stop latest date-time of logs to fetch
 * @returns object of column names and actual logs: { columns: [...], data: [...] }
 */
async function fetchLogs(start, stop) {
    // Request Logs:
    let response;
    try {
        startString = start.toISOString().replace("Z","");
        stopString = stop.toISOString().replace("Z","");
        const params = new URLSearchParams({ start: startString, stop: stopString });
        response = await fetch("/api/web/logs"+"?"+params, {
            method: "GET",
            headers: { Accept: "application/json" }
        });
    } catch(error) {
        throw Error("Problem while fetching logs: "+error);
    }

    // Parse Response:
    if(!response.ok) {
        const res = await response.text();
        throw Error("Server responded: "+res);
    }
    const logs = await response.json();
    if(isEmpty(logs)) {
        return { columns: [], data: [] };
    }
    return { columns: logs["columns"], data: logs["data"] };
}

/**
 * Make an asynchronious request to the backend to request data between start and stop.
 * @param {Date} start earliest date-time of data to fetch 
 * @param {Date} stop latest date-time of data to fetch
 * @returns JSON of datasets: {"Time": [...], "Flow": [...], "Pressure": [...], etc.}
 */
async function fetchData(start, stop) {
    // Request Data:
    let response;
    try {
        startString = start.toISOString().replace("Z","");
        stopString = stop.toISOString().replace("Z","");
        const params = new URLSearchParams({ start: startString, stop: stopString });
        response = await fetch("/api/web/data"+"?"+params, {
            method: "GET",
            headers: { Accept: "application/json" }
        });
    } catch(error) {
        throw Error("Problem while fetching data: "+error);
    }
    
    // Parse Reponse:
    if(!response.ok) {
        const res = await response.text();
        throw Error("Server responded: "+res);
    }
    const data = await response.json();
    if(isEmpty(data)) {
        return { "Time": [], "Flow": [], "Pressure": [], "Level": [] };
    }

    // Parse Datasets from Response:
    let datasets = {};
    if("Time" in data) {
        labels = Object.values(data["Time"]);
        labels = labels.map(function(label) { return new Date(label); }); // convert UNIX labels to string
        datasets["Time"] = labels;
    }
    if("Flow" in data) {
        let values = Object.values(data["Flow"]);
        values = values.map(function(value) { return value*FLOW_SCALING; }); // scale all values
        datasets["Flow"] = values;
    }
    if("Pressure" in data) {
        let values = Object.values(data["Pressure"]);
        values = values.map(function(value) { return value*PRESSURE_SCALING; });
        datasets["Pressure"] = values;
    }
    if("Level" in data) {
        let values = Object.values(data["Level"]);
        values = values.map(function(value) { return value*LEVEL_SCALING; });
        datasets["Level"] = values;
    }

    return datasets;
}




//===============================================
// Data Visualization (Action Functions)
//===============================================

/**
 * Generates a ChartJS object for the given canvas. Should be called when the windows size changes
 * and can be filled with the matching update function.
 * @see updateGauges
 * @param {JSON} gauges json map of element ids to plot into
 */
function plotGauges(gauges) {
    // Colors Palette:
    const bodyStyles = window.getComputedStyle(document.body);
    const backgroundColor = bodyStyles.getPropertyValue('--background-color');
    const neutralColor = bodyStyles.getPropertyValue('--neutral-color');
    const neutralTextColor = bodyStyles.getPropertyValue('--neutral-text');

    Chart.defaults.color = neutralTextColor;

    values = {}
    for(gauge in gauges) {
        // Chart Setup:
        const ctx = document.getElementById(gauges[gauge]);
        if(ctx == null) {
            console.log("Could not get element '"+gauges[gauge]+"'");
            return;
        }
        const config = {
            type: 'doughnut',
            data: {
                labels: [gauge],
                datasets: [{
                    data: [],
                    borderWidth: 0,
                    backgroundColor: [
                        COLORS[gauge],
                        backgroundColor,
                    ]
                }]
            },
            options: {
                maintainAspectRatio: false,
                rotation: 270,
                circumference: 180,
                cutout: "90%",
                plugins: {
                    legend: { display: false },
                    tooltip: { enabled: false },
                    customCanvasBackgroundColor: {
                        color: neutralColor // Light gray background
                    },
                    customLabels: {}, // update config to use the custom plugin
                }
            },
            plugins: [
                {
                    id: 'customLabels',
                    beforeDraw: (chart) => { // custom plugin for label
                        chart.ctx.font = 'bold 12px Arial';
                        chart.ctx.textAlign = 'center';
                        chart.ctx.textBaseline = 'middle';
                    
                        // Draw labels
                        const centerX = chart.ctx.canvas.width / 2;
                        const centerY = chart.ctx.canvas.height / 2;
                        chart.ctx.fillStyle = neutralTextColor;
                        chart.ctx.fillText(chart.data.datasets[0].data[0]+" "+UNITS[chart.data.labels[0]], centerX, centerY);
                    },
                }
            ]
        };

        // Create Chart:
        let myChart = Chart.getChart(gauges[gauge]);
        if(myChart) {
            values[gauge] = parseFloat(myChart.data.datasets[0].data[0]);
            myChart.clear();
            myChart.destroy();
        }
        new Chart(ctx, config);
        
    }

    if(!isEmpty(values)) {
        updateGauges(gauges, values);
    }
    return;
}

/**
 * Updates the given chart object with the given values. The chart object is returned by the matching
 * plot function.
 * @see plotGauges
 * @param {JSON} gauges JSON mapping for element ids
 * @param {JSON} values values of gauge elements
 * @param {Float} pressure value to plot on the pressure gauge
 * @param {Float} levelvalue to plot on the level gauge
 */
function updateGauges(gauges, values) {
    for(gauge in gauges) {
        let myChart = Chart.getChart(gauges[gauge]);
        if(typeof myChart === "undefined") {
            console.log("Could not find chart with from "+gauge);
            continue;
        }
        let v = values[gauge];
        if(typeof v === "undefined") {
            console.log("Dont have values for chart "+gauge);
            continue;
        }
        myChart.data.datasets[0].data = [v.toFixed(1), MAX_LIMITS[gauge]-values[gauge]];
        myChart.update();
    }
}

/**
 * Generates a ChartJS object for lines in the given canvas. Should be called when the windows size changes
 * and can be filled with the matching update function.
 * @see updateLines
 * @param {String} canvasID element id of canvas to plot into
 */
function plotLines(canvasID) {
    // Colors Palette:
    const bodyStyles = window.getComputedStyle(document.body);
    const backgroundColor = bodyStyles.getPropertyValue('--background-color');
    const neutralColor = bodyStyles.getPropertyValue('--neutral-color');
    const neutralTextColor = bodyStyles.getPropertyValue('--neutral-text');

    Chart.defaults.color = neutralTextColor;

    // Chart Setup:
    const ctx = document.getElementById(canvasID);
    if(ctx == null) {
        console.log("Could not get element '"+canvasID+"'");
        return;
    }
    const config = {
        type: 'line',
        data: {
            labels: [],
            datasets: [
                { label: "Flow", data: [], borderColor: COLORS["Flow"] , backgroundColor: COLORS["Flow"] },
                { label: "Pressure", data: [], borderColor: COLORS["Pressure"], backgroundColor: COLORS["Pressure"] },
                { label: "Level", data: [], borderColor: COLORS["Level"], backgroundColor: COLORS["Level"] }
            ]
        },
        options: {
            maintainAspectRatio: false,
            plugins: {
                title: {
                    display: true,
                    text: ""
                },
            },
            scales: {
                x: {
                    type: 'time',
                    time: {
                        unit: 'second',
                        tooltipFormat:'yyyy-MM-dd HH:mm:ss',
                        displayFormats: {
                            second: 'HH:mm:ss',
                            minute: 'HH:mm',
                            hour: 'HH:mm',
                            day: 'yyyy-MM-dd' 
                        }
                    },
                    grid: {
                        color: backgroundColor
                    },
                    ticks: {
                        autoSkip: true,
                        maxTicksLimit: 6
                    }
                },
                y: {
                    grid: {
                        color: backgroundColor
                    },
                },
            }
        },
    };

    // Create Chart:
    let sets = {};
    let myChart = Chart.getChart(canvasID);
    if(myChart) {
        sets["Time"] = myChart.data.labels;
        for(dataset of myChart.data.datasets) {
            sets[dataset.label] = dataset.data;
        }
        myChart.clear();
        myChart.destroy();
    }
    new Chart(ctx, config);
    if(!isEmpty(sets)) {
        updateLines(canvasID, sets["Time"], sets["Flow"], sets["Pressure"], sets["Level"]);
    }
    return;
}

/**
 * Takes the given datasets and appends them to the exisiting charts
 * @param {String} canvasID element id of the canvas to update
 * @param {Array} flowSet dataset of new flow values 
 * @param {Array} pressureSet dataset of new pressure values
 * @param {Array} levelSet dataset of new level values
 */
function updateLines(canvasID, labels, flowSet, pressureSet, levelSet) {
    // Convert Labels:
    // labels = labels.map(function(label) { return toLocalDateTime(label); });
    
    // Append New Data:
    let myChart = Chart.getChart(canvasID);
    if(typeof myChart === "undefined") {
        console.log("Could not find chart with from "+canvasID);
        return;
    }
    myChart.data.labels.push.apply(myChart.data.labels, labels);
    myChart.data.datasets[0].data.push.apply(myChart.data.datasets[0].data, flowSet);
    myChart.data.datasets[1].data.push.apply(myChart.data.datasets[1].data, pressureSet);
    myChart.data.datasets[2].data.push.apply(myChart.data.datasets[2].data, levelSet);

    // Build Canvas Title:
    labels = myChart.data.labels;
    if(labels.length) {
        const firstLabel = labels[0]; 
        const lastLabel = labels[labels.length - 1];
        myChart.options.plugins.title.text = toLocalDateTime(firstLabel)+" - "+toLocalDateTime(lastLabel);
    } else {
        myChart.options.plugins.title.text = "No data received!";
    }

    // Update:
    myChart.update();
}



//===============================================
// Helper Functions:
//===============================================

/**
 * Checks if the given object is empty ({})
 * @param {Object} obj object to check
 * @returns true if the given object is empty, false otherwise
 */
function isEmpty(obj) {
    for (const prop in obj) {
        if (Object.hasOwn(obj, prop)) return false;
    }
    return true;
}