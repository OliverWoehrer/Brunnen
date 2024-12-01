"""
[INFO]
This the main file of the web app "TreeAPI". This a web based application, which allows the sensor
in a water well ("Water Well Water" = W3) to communicate. This application implements a http server
and is powered by the python library "Flask". It supports a web interface for user interaction.

[START APPLICATION]
To start this application, run "python app.py"
"""

import os
import config
from data import data_client as database
from routes.app import app




if __name__ == "__main__":
    # Initalize Database Client:
    token = os.environ.get("INFLUXDB_TOKEN")
    if not token:
        raise RuntimeError("Failed to load INFLUXDB_TOKEN environment variable.")
    org = config.readInfluxOrganization()
    influx_host = config.readInfluxHost()
    influx_port = config.readInfluxPort()
    url = influx_host+":"+str(influx_port)
    database.setup(url=url, token=token, organization=org)

    # Initalize Flask App:
    key = os.environ.get("APP_KEY")
    if not key:
        raise RuntimeError("Failed to load APP_KEY environment variable.")
    app.secret_key = key

    # Start App at Desired Port:
    port = config.readPort()
    app.run(host="0.0.0.0", port=port, debug=True)
