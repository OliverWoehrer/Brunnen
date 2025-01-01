"""
[INFO]
This the main file of the web app "TreeAPI". This a web based application, which allows the sensor
in a water well ("Water Well Water" = W3) to communicate. This application implements a http server
and is powered by the python library "Flask". It supports a web interface for user interaction.

[START APPLICATION]
To start this application, run "python app.py"
"""

import os
import argparse
import logging
import config
from data import data_client as database
from routes.app import app




if __name__ == "__main__":
    # Parse Input Argument:
    parser = argparse.ArgumentParser(description="Brunnen (TreeAPI)")
    parser.add_argument("--log-level", type=str, choices=["debug", "info", "warning", "error", "critical"], default="info", help="Set the logging level (default: info)")
    args = parser.parse_args()
    numeric_level = getattr(logging, args.log_level.upper(), None)
    if not isinstance(numeric_level, int):
        raise ValueError('Invalid log level: %s' % args.log_level)

    # Setup Logger:
    logging.basicConfig(level=numeric_level, format="%(asctime)s [%(levelname)s] %(message)s")
    logger = logging.getLogger(__name__)

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
