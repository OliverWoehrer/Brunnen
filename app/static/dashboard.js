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
const LEVEL_COLOR = "#A5A5A5";
const PRESSURE_COLOR = "#ED7D31";
const FLOW_COLOR = "#4472C4";

// Scaling Factors:
const FLOW_SCALING = 0.002577; // 388 pulses per litre
const PRESSURE_SCALING = 0.002513; // 398 increments per bar
const LEVEL_SCALING = 0.00157356; // 635 increments per meter




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
    const logsTable = document.getElementById(tableID);
    fillTable(logsTable, columns, data);
}

/**
 * Fetches data between start and stop and adds it to the charts given
 * @param {Date} start erliest date-time of logs to fetch
 * @param {Date} stop latest date-time of logs to fetch
 * @param {JSCharting} lineChart object holding the line chart
 * @param {JSCharting} gaugesChart object holding the gauges chart
 */
async function updateData(start, stop, lineChart, gaugesChart) {
    const datasets = await fetchData(start, stop);
    if(typeof gaugesChart !== 'undefined') {
        let flow = 0;
        let pressure = 0;
        let level = 0;
        for(const dataset of datasets) {
            // Points: [[Date(),value], [Date(),value], ...]
            const lastPoint = dataset.points[dataset.points.length - 1];
            last = parseFloat(lastPoint[1].toFixed(1));
            if(dataset.name == "Flow") flow = last;
            if(dataset.name == "Pressure") pressure = last;
            if(dataset.name == "Level") level = last;
        }
        updateGauges(gaugesChart, flow, pressure, level);
    }
    if(typeof lineChart !== 'undefined') {
        // Points: [[Date(),value], [Date(),value], ...]
        const dataset = datasets[0];
        const firstPoint = dataset.points[0];
        const lastPoint = dataset.points[dataset.points.length - 1];

        const options = { year:"2-digit", month:"2-digit", day:"2-digit" };
        const firstLabel = firstPoint[0].toLocaleDateString("de-AT", options);
        const lastLabel = lastPoint[0].toLocaleDateString("de-AT", options);
        updateLines(lineChart, firstLabel+" - "+lastLabel, datasets);
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
            const msg = parseErrorPage(res);
            throw Error("Server Response: "+msg);
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
        const msg = parseErrorPage(res);
        throw Error(msg);
    }
    const logs = await response.json();
    if(isEmpty(logs)) {
        throw Error("No logs returned.");
    }
    return { columns: logs["columns"], data: logs["data"] };
}

/**
 * Make an asynchronious request to the backend to request data between start and stop.
 * @param {Date} start earliest date-time of data to fetch 
 * @param {Date} stop latest date-time of data to fetch
 * @returns array of datasets: [{name: "Flow", color: #F0C5B5, points: [...]}, ...]
 */
async function fetchData(start, stop) {
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
        throw Error("Problem while fetching logs: "+error);
    }
    

    // Parse Reponse:
    if(!response.ok) {
        const res = await response.text();
        const msg = parseErrorPage(res);
        throw Error(msg);
    }
    const data = await response.json();
    if(isEmpty(data)) {
        throw Error("No data returned.");
    }
    columnNames = Object.keys(data);

    // Parse Labels from Response:
    let labels;
    let titleText = ["No Data Received"];
    if("Time" in data) {
        // Parse Labels from Response:
        labels = Object.values(data["Time"]);
        
        // Build Canvas Title:
        const firstLabel = labels[0];
        const lastLabel = labels[labels.length - 1];
        titleText = toLocalDate(firstLabel)+" - "+toLocalDate(lastLabel);

        // Convert UNIX Labels to String:
        labels = labels.map(function(label) { return new Date(label); });
    }

    // Build Datasets:
    let datasets = new Array();
    let latest = {}
    if("Flow" in data) {
        let values = Object.values(data["Flow"]);
        values = values.map(function(value) { return value*FLOW_SCALING; }); // scale all values
        const latestValue = values[values.length - 1];
        latest["flow"] = parseFloat(latestValue.toFixed(1));
        points = values.map(function(value, idx) { return [labels[idx], value]; }); // zip with labels
        set = {
            name: "Flow",
            color: FLOW_COLOR,
            points: points,
        }
        datasets.push(set);
    }
    if("Pressure" in data) {
        values = Object.values(data["Pressure"]);
        values = values.map(function(value) { return value*PRESSURE_SCALING; });
        const latestValue = values[values.length - 1];
        latest["pressure"] = parseFloat(latestValue.toFixed(1));
        points = values.map(function(value, idx) { return [labels[idx], value]; }); // zip with labels
        set = {
            name: "Pressure",
            color: PRESSURE_COLOR,
            points: points,
        }
        datasets.push(set);
    }
    if("Level" in data) {
        values = Object.values(data["Level"]);
        values = values.map(function(value) { return value*LEVEL_SCALING; });
        const latestValue = values[values.length - 1];
        latest["level"] = parseFloat(latestValue.toFixed(1));
        points = values.map(function(value, idx) { return [labels[idx], value]; }); // zip with labels
        set = {
            name: "Level",
            color: LEVEL_COLOR,
            points: points,
        }
        datasets.push(set);
    }

    return datasets;
}




//===============================================
// Data Visualization (Action Functions)
//===============================================

/**
 * Generates a JSCharting object in the given container. Should be called once on page load and can
 * be filled with the matching update function.
 * @see updateGauges
 * @param {HTMLElement} gaugesChartDiv chart container to plot into
 * @returns JSCharting object of the newly generate chart
 */
function plotGauges(gaugesChartDiv) {
    // Colors Palette:
    const bodyStyles = window.getComputedStyle(document.body);
    const backgroundColor = bodyStyles.getPropertyValue('--background-color');
    const neutralTextColor = bodyStyles.getPropertyValue('--neutral-text');
    
    // Configuration:
    const config = { 
        debug: true,
        legend_visible: false,
        defaultSeries: {
            mouseTracking_enabled: false,
            shape: {
                innerSize: '70%', 
                label: [
                    { verticalAlign: 'middle', style_fontSize: 20 },
                    { text: '%name' },
                ]
            },
            type: 'gauge column roundCaps'
        },
        series: [
            { name: 'Flow', color: FLOW_COLOR, points: [['value', 0]], shape_label: [{text:'%sum L/s'}], yAxis: 'flow' },
            { name: 'Pressure', color: PRESSURE_COLOR, points: [['value', 0]], shape_label: [{text:'%sum bar'}], yAxis: 'pressure' },
            { name: 'Level', color: LEVEL_COLOR, points: [['value', 0]], shape_label: [{text:'%sum m'}], yAxis: 'level' },            
        ],
        xAxis: {
            defaultTick: {
                gridLine: {
                    color: backgroundColor,
                    width: 'column'
                }
            },
            spacingPercentage: 0.15
        },
        yAxis: [
            { line_width: 0, defaultTick_enabled: false, id: 'flow', scale_range: [0, 100] },
            { line_width: 0, defaultTick_enabled: false, id: 'pressure', scale_range: [0, 10] },
            { line_width: 0, defaultTick_enabled: false, id: 'level', scale_range: [0, 5] },
        ],
    }

    // Build Chart:
    return JSC.chart(gaugesChartDiv, config);
}

/**
 * Updates the given chart object with the given values. The chart object is returned by the matching
 * plot function.
 * @see plotGauges
 * @param {JSCharting} gaugesChart object holds the chart
 * @param {Float} flow value to plot on the flow gauge
 * @param {Float} pressure value to plot on the pressure gauge
 * @param {Float} levelvalue to plot on the level gauge
 */
function updateGauges(gaugesChart, flow, pressure, level) {
    const flowSeries = gaugesChart.series("Flow");
    const pressureSeries = gaugesChart.series("Pressure");
    const levelSeries = gaugesChart.series("Level");
    flowSeries.points(0).options({ y: flow });
    pressureSeries.points(0).options({ y: pressure });
    levelSeries.points(0).options({ y: level });
}

/**
 * Generates a JSCharting object for lines in the given container. Should be called once on page
 * load and can be filled with the matching update function.
 * @see updateLines
 * @param {HTMLElement} lineChartDiv chart container to plot into
 * @returns JSCharting object of the newly generate chart
 */
function plotLines(lineChartDiv) {
    // Colors Palette:
    const bodyStyles = window.getComputedStyle(document.body);
    const backgroundColor = bodyStyles.getPropertyValue('--background-color');
    const neutralTextColor = bodyStyles.getPropertyValue('--neutral-text');

    // Configuration:
    const config = {
        debug: true,
        type: 'line',
        title_label_text: 'TItle',
        legend_position: 'outside top',
        defaultAxis: {
            alternateGridFill: 'none',
            line_color: backgroundColor,
            label_color: backgroundColor,
            defaultTick: {
                label_color: neutralTextColor, /* Label Color */
                gridLine_color: backgroundColor,
                gridLine: {
                    color: backgroundColor, /*Y Axis Grid Line*/
                    width: 2
                },
            } 
        },
        defaultCultureName: 'de-AT',
        defaultPoint: {
            marker_type: 'none' // circle
        }, 
        defaultSeries: {
            mouseTracking_enabled: false,
            lastPoint_yAxisTick: { 
                label_text: '%icon %seriesName %yvalue', 
                axisId: 'secondY'
            }
        },
        toolbar_items: {
            'Line Type': {
                type: 'select',
                label_style_fontSize: 13,
                margin: 5,
                items: 'Line,Step,Spline',
                events_change: function(val) {
                chart.series().options({ type: val });
                }
            }
        },
        xAxis: {
            crosshair_enabled: true,
            formatString: 't' // 'd' = days, 't' = time
        },
        yAxis: {
            scale_range: [-1, 5]
        },
        series: [
            { name: 'Flow', color: FLOW_COLOR, points: [] },
            { name: 'Pressure', color: PRESSURE_COLOR, points: [] },
            { name: 'Level', color: LEVEL_COLOR, points: [] }
        ]
    }

    // Build Chart:
    if(lineChart) { lineChart = {} }
    return JSC.chart(lineChartDiv, config);
}

/**
 * Updates the given chart object with the given values. The chart object is returned by the matching
 * plot function.
 * @see plotLines
 * @param {JSCharting} lineChart object holds chart
 * @param {String} titleText title of the chart
 * @param {Array} datasets array of datasets: [{name: "Flow", color: #F0C5B5, points: [...]}, ...]
 */
function updateLines(lineChart, titleText, datasets) {
    // Set Title:
    lineChart.options({title: {label: {text: titleText}}});

    for(const dataset of datasets) {
        // Concatenate Existing and New Points:
        const series = lineChart.series(dataset.name);
        const exisitingPoints = series.points().items;
        const newPoints = dataset.points;
        dataset.points = exisitingPoints.concat(newPoints);

        // Replace Entire Series:
        lineChart.series(dataset.name).remove();
        lineChart.series.add(dataset);
    }
}



//===============================================
// Helper Functions:
//===============================================

/**
 * Parses the common error page and extracts the error message.
 * @param {String} page html string of error page
 * @return error message as string
 */
function parseErrorPage(page) {
    const doc = new DOMParser().parseFromString(page, "text/html");
    let title = doc.getElementsByTagName("title");
    let nodes = doc.getElementsByTagName("p");
    return "["+title[0].innerText+"] "+nodes[0].innerText;
}

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