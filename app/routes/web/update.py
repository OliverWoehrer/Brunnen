"""
This module implements the functions to handle routes of /update.
"""
from flask import Blueprint, render_template, redirect, url_for

update = Blueprint("update", __name__, url_prefix="/update")

@update.route("/", methods=["GET"])
def index():
    return render_template("main_pages/update.html")



