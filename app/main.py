"""
[INFO]
This the main file of the web app "TreeAPI". This a web based application, which allows the sensor
in a water well ("Water Well Water" = W3) to communicate. This application implements a http server
and is powered by the python library "Flask". It supports a web interface for user interaction.

[START APPLICATION]
To start this application, run "python app.py"
"""

from flask import Flask, request
import config





"""
Start the RET-Application:
"""
if __name__ == "__main__":
    # Initalize RET-App:
    app = Flask(__name__, template_folder="templates", static_folder="static")

    # Register Blueprints (Routes):
    from routes import routes # responsible for web interface
    from routes.dashboard.dashboard import dashboard
    app.register_blueprint(dashboard)

    # Start App at Desired Port:
    port = config.readPort()
    app.run(host="localhost", port=port, debug=True)