"""
This module implements the functions to handle routes of main app (web interface).
"""
from flask import Blueprint, render_template, redirect, url_for

dashboard = Blueprint("dashboard", __name__, url_prefix="/dashboard")

@dashboard.route("/", methods=["GET"])
def index():
    return render_template("dashboard/index.html", liveString="Test")

@dashboard.route("/logs", methods=["GET"])
def logs():
    return render_template("special/error.html")

@dashboard.route("/live", methods=["GET"])
def live():
    return render_template("special/error.html")

@dashboard.route("/feed", methods=["GET"])
def feed():
    return render_template("special/error.html")


