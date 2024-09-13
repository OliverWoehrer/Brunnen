"""
This module implements the functions to handle main routes of "/".
"""
from flask import Flask, render_template, redirect, url_for, flash
from werkzeug.exceptions import HTTPException
from datetime import datetime, timezone

app = Flask(__name__, template_folder="../templates", static_folder="../static")
last_request_time = datetime.now(timezone.utc).replace(microsecond=0)
request_count = 0

@app.before_request
def log_last_request():
    global last_request_time, request_count
    last_request_time = datetime.now(timezone.utc).replace(microsecond=0)
    request_count = request_count + 1

@app.route("/", methods=["GET"])
def index():
    return redirect(url_for("dashboard"))

@app.route("/dashboard", methods=["GET"])
def dashboard():
    return render_template("main_pages/dashboard.html")

@app.route("/settings", methods=["GET"])
def settings():
    return render_template("main_pages/settings.html")

@app.route("/update", methods=["GET"])
def update():
    return render_template("main_pages/update.html")

@app.route("/profile", methods=["GET"])
def profile():
    return render_template("special_pages/login.html")

@app.errorhandler(Exception)
def error(e):
    if isinstance(e, HTTPException): # handle call HTTP errors
        return render_template("special_pages/error.html", code=e.code, message=e.description), e.code
    else: # return unknown errors
        # TODO: dont pass message for deployment!
        return render_template("special_pages/error.html", code=500, message=str(e)), 500

@app.context_processor
def inject_request_counter():
    global request_count
    timestamp = last_request_time.strftime("%H:%M:%S")
    return dict(last_req=timestamp)
