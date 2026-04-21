"""
This module implements the functions to handle routes of /profile.
"""
from flask import Blueprint, render_template, redirect, url_for

# Register Blueprint Hierarchy:
profile = Blueprint("profile", __name__, url_prefix="/profile")

@profile.route("/", methods=["GET"])
def index():
    return render_template("main_pages/profile.html")



