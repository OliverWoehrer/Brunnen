"""
[INFO]
This the main file of the web app "TreeAPI". This a web based application, which allows the sensor
in a water well ("Water Well Water" = W3) to communicate. This application implements a http server
and is powered by the python library "Flask". It supports a web interface for user interaction.

[START APPLICATION]
To start this application, run "python app.py"
"""

from flask import Flask, request
from flask import render_template, redirect, send_file
from modules import configHandler



# Initalize RET-App:
app = Flask(__name__, static_folder="static", template_folder="templates")


"""
Static Web Pages:
"""
@app.route("/", methods=["GET"])
def getIndex():
    return "Hello, World!"
    # liveString = "mein live string."
    # return render_template("index.html", liveString = liveString)


"""
Start the RET-Application:
"""
if __name__ == "__main__":
    # Start App at Desired Port:
    port = configHandler.readPort()
    app.run(host="localhost", port=port, debug=True)