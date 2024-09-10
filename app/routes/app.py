"""
This module implements the functions to handle main routes of app (web interface).
"""
from flask import Flask, render_template, redirect, url_for, flash
from werkzeug.exceptions import HTTPException

app = Flask(__name__, template_folder="../templates", static_folder="../static")

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
        flash(e.description)
        return render_template("special_pages/error.html", code=e.code)
    else: # return unknown errors
        return e
