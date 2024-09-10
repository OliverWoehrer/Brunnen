"""
[INFO]
This the main file of the web app "TreeAPI". This a web based application, which allows the sensor
in a water well ("Water Well Water" = W3) to communicate. This application implements a http server
and is powered by the python library "Flask". It supports a web interface for user interaction.

[START APPLICATION]
To start this application, run "python app.py"
"""

import os
from data import data_client as database
import config
from datetime import datetime, timedelta
import pandas as pd
from flask import Flask, request






if __name__ == "__main__":
    # Initalize Database Client:
    token = os.environ.get("INFLUXDB_TOKEN")
    org = config.readInfluxOrganization()
    influx_host = config.readInfluxHost()
    influx_port = config.readInfluxPort()
    url = influx_host+":"+str(influx_port)
    database.setup(url=url, token=token, organization=org)

    # Initalize Flask App:
    from routes.app import app
    from routes.dashboard.dashboard import dashboard
    app.register_blueprint(dashboard)
    from routes.settings.settings import settings
    app.register_blueprint(settings)

    # Start App at Desired Port:
    port = config.readPort()
    app.run(host="localhost", port=port, debug=True)