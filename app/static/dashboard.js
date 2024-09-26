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

const LEVEL_COLOR = "#A5A5A5";
const PRESSURE_COLOR = "#ED7D31";
const FLOW_COLOR = "#4472C4";



//===============================================
// Fetching Window Times
//===============================================

async function requestLatestTimestamp() {
    let promise = $.ajax({ 
        url: "/api/web/sync", 
        type: "get",
        accepts: "application/json",
    });
    return promise;
}

async function getLatestTimestamp() {
    const res = await requestLatestTimestamp();
    const lastSync = res["last_sync"];
    return new Date(lastSync);
}



//===============================================
// Data Fetching
//===============================================

function requestLogs(startTime, stopTime) {
    startString = startTime.toISOString().replace("Z","");
    stopString = stopTime.toISOString().replace("Z","");
    $.ajax({ 
        url: "/api/web/logs",
        type: "get",
        accepts: "application/json",
        data: { start: startString, stop: stopString },
        success: loadLogs,
        error: alertErrorResponse
    });
}

function loadLogs(res) {
    // Parse Reponse:
    columns = res["columns"];
    data = res["data"];

    // Get DOM Element for Table:
    let logsTable = document.getElementById("logs_table");

    // Hide Waiting Animation:
    hideElement("logs_table_waiting");
    
    // Insert User as Table Rows:
    plotLogs(logsTable, columns, data);
}

function requestData(startTime, stopTime) {
    startString = startTime.toISOString().replace("Z","");
    stopString = stopTime.toISOString().replace("Z","");
    $.ajax({ 
        url: "/api/web/data", 
        type: "get",
        accepts: "application/json",
        data: { start: startString, stop: stopString },
        success: loadData,
        error: alertErrorResponse
    });
}

function loadData(res) {
    //Declare Global Variables:
    const FLOW_SCALING = 0.002577; // 388 pulses per litre
    const PRESSURE_SCALING = 0.002513; // 398 increments per bar
    const LEVEL_SCALING = 0.00157356; // 635 increments per meter

    // Parse Reponse:
    columnNames = Object.keys(res);

    // Parse Labels from Response:
    let labels;
    let titleText = ["No Data Received"];
    if("Time" in res) {
        // Parse Labels from Response:
        labels = Object.values(res["Time"]);
        
        // Build Canvas Title:
        const firstLabel = labels[0];
        const lastLabel = labels[labels.length - 1];
        titleText = toLocalDate(firstLabel)+" - "+toLocalDate(lastLabel);

        // Convert UNIX Labels to String:
        labels = labels.map(function(label) { return new Date(label); });
    }

    let datasets = new Array();
    let latest = {}
    if("Flow" in res) {
        let values = Object.values(res["Flow"]);
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
    if("Pressure" in res) {
        values = Object.values(res["Pressure"]);
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
    if("Level" in res) {
        values = Object.values(res["Level"]);
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

    // Plot Data:
    if(typeof gaugesChart !== 'undefined') {
        updateGauges(latest["flow"], latest["pressure"], latest["level"]);
    }
    if(typeof lineChart !== 'undefined') {
        updateLines(titleText, datasets);
    }
}

/**
 * This as a common error-callback function for the request functions. It simply parses the
 * response and displays the error message.
 * @param {Object} res response object
 */
function alertErrorResponse(res) {
    const doc = new DOMParser().parseFromString(res.responseText, "text/html");
    let title = doc.getElementsByTagName("title");
    let nodes = doc.getElementsByTagName("p");
    window.alert("Server Response: ["+title[0].innerText+"] "+nodes[0].innerText);
}




//===============================================
// Data Visualization (Action Functions)
//===============================================

function plotLogs(table, columns, data)  {
    fillTable(table, columns, data); // action call
}

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
    gaugesChart = JSC.chart(gaugesChartDiv, config);
}

function updateGauges(flow, pressure, level) {
    // Hide Waiting Animation:
    hideElement("gaugesChart_waiting");

    const flowSeries = gaugesChart.series("Flow");
    const pressureSeries = gaugesChart.series("Pressure");
    const levelSeries = gaugesChart.series("Level");
    flowSeries.points(0).options({ y: flow });
    pressureSeries.points(0).options({ y: pressure });
    levelSeries.points(0).options({ y: level });
}

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
    lineChart = JSC.chart(lineChartDiv, config);

    // Hide Waiting Animation:
    hideElement("lineChart_waiting");
}

function updateLines(titleText, datasets) {
    // Hide Waiting Animation:
    hideElement("lineChart_waiting");

    // Set Title:
    lineChart.options({title: {label: {text: titleText}}});

    for(const dataset of datasets) {
        // Concatenate Existing and New Points:
        let series = lineChart.series(dataset.name);
        const newPoints = dataset.points;
        const exisitingPoints = series.points().items;
        dataset.points = exisitingPoints.concat(newPoints);

        // Replace Entire Series:
        series.remove();
        lineChart.series.add(dataset);
    }
}

