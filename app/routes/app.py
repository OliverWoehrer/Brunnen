"""
This module implements the functions to handle main routes of "/".
"""
from flask import Flask, render_template, redirect, url_for, request, flash, session
from werkzeug.security import check_password_hash
from werkzeug.exceptions import HTTPException, BadGateway
from datetime import timedelta
from data import data_client as db

# Register Blueprint Hierarchy:
from .api.api import api
from .web.web import web
app = Flask(__name__, template_folder="../templates", static_folder="../static")
app.register_blueprint(api)
app.register_blueprint(web)

@app.before_request
def refresh_session_livetime():
    # app.permanent_session_lifetime = timedelta(minutes=10)
    pass

@app.route("/", methods=["GET"])
def index():
    username = session.get("username")
    if username:
        return redirect(url_for("web.dashboard.index"))
    else:
        return redirect(url_for("login"))

@app.route("/login", methods=["GET", "POST"])
def login():
    if request.method == "GET":
        # Render Login Page: 
        username = session.get("username")
        if username: # flash info message if logged in
            flash(('Already logged in as '+username+'.'))
        next_url = request.args.get("next", "/")
        return render_template("special_pages/login.html", next_url=next_url)
    
    if request.method == "POST":
        # Parse URL Params:
        username = request.form.get("username")
        password = request.form.get("password")
        remember = True if request.form.get("remember") else False
        next_url = request.form.get("next", "/")

        # Look Up User in Database:
        (msg,user) = db.queryUser(username=username)
        if user is None:
            raise BadGateway(("Problem while reading user: "+str(msg)))
        login_token = username + password
        if not user or not check_password_hash(user.get("token", ""), login_token):
            session.pop("_flashes", None) # clear flash messages before using
            flash("Unauthorized! Please check your login details and try again.")
            return redirect(url_for("login", next=next_url)) # reload the page with flash message

        # Fill Session Properties:
        session["username"] = user.get("username")
        session["group"] = user.get("group")

        # Redirect After Successful Login:
        return redirect(next_url)

@app.route("/logout", methods=["GET"])
def logout():
    session.pop("username", None)
    session.pop("_flashes", None)
    return redirect("/")

@app.errorhandler(Exception)
def error(e):
    if isinstance(e, HTTPException): # display HTTP errors
        return render_template("special_pages/error.html", code=e.code, message=e.description), e.code
    else: # return unknown errors
        # TODO: remove for deployment!
        return render_template("special_pages/error.html", code=500, message=str(e)), 500
