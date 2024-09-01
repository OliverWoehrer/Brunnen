"""
This module implements the functions to handle routes of main app (web interface).
"""
from flask import Flask, render_template, redirect, url_for
from __main__ import app # app is initalized in main

@app.route("/", methods=["GET"])
def index():
    return render_template("special_pages/login.html")

@app.route("/dashboard", methods=["GET"])
def dashboard():
    return render_template("dashboard.html")

@app.route("/settings", methods=["GET"])
def settings():
    return render_template("settings.html")

@app.route("/profile", methods=["GET"])
def profile():
    return render_template("profile.html")

@app.route("/error", methods=["GET"])
def error():
    return render_template("special_pages/error.html")
