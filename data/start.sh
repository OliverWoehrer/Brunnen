#!/bin/bash

# Global Variables:
PORT=8086

# Export Influx Access Token:
echo "Setting influx access token:"
export INFLUXDB_TOKEN=Ds0mb4m_-wYE_qzXPyxz-q6ctjCfm8QicbjhZIdzMpWjH_RwLwSvauOna_db2weY7rhX5xkpoFRQR-vB7vWEqg==
printenv INFLUXDB_TOKEN

# Start InfluxDB Server:
echo "Starting InfluxDB server..."
influxd --http-bind-address ":$PORT" --log-level "error"

