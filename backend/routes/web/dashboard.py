"""
This module implements the functions to handle routes of /dashboard.
"""
from flask import Blueprint, render_template, redirect, url_for

# Register Blueprint Hierarchy:
dashboard = Blueprint("dashboard", __name__, url_prefix="/dashboard")

@dashboard.route("/", methods=["GET"])
def index():
    return render_template("main_pages/dashboard.html")

@dashboard.route("/logs", methods=["GET"])
def logs():
    return render_template("sub_pages/logs.html")

@dashboard.route("/measurements", methods=["GET"])
def measurements():
    return render_template("sub_pages/measurements.html")


