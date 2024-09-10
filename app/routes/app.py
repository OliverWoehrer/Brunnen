"""
This module implements the functions to handle main routes of app (web interface).
"""
from flask import Flask, render_template, redirect, url_for

app = Flask(__name__, template_folder="../templates", static_folder="../static")

@app.route("/", methods=["GET"])
def index():
    return redirect(url_for("dashboard"))

@app.route("/dashboard", methods=["GET"])
def dashboard():
    return render_template("dashboard.html")

@app.route("/settings", methods=["GET"])
def settings():
    return render_template("settings.html")

@app.route("/update", methods=["GET"])
def update():
    return render_template("update.html")

@app.route("/profile", methods=["GET"])
def profile():
    return render_template("special_pages/login.html")

@app.route("/error", methods=["GET"])
def error():
    return render_template("special_pages/error.html")
