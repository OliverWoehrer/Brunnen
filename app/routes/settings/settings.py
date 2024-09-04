"""
This module implements the functions to handle routes of settings (web interface).
"""
from flask import Blueprint, render_template, redirect, url_for

settings = Blueprint("settings", __name__, url_prefix="/settings")

@settings.route("/intervals", methods=["GET"])
def intervals():
    return render_template("sub_pages/intervals.html")


