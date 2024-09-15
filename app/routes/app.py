"""
This module implements the functions to handle main routes of "/".
"""
from flask import Flask, render_template, redirect, url_for, request, flash, session
from werkzeug.security import generate_password_hash, check_password_hash
from werkzeug.exceptions import HTTPException, BadGateway
from datetime import timedelta
import re
from data import data_client as db

app = Flask(__name__, template_folder="../templates", static_folder="../static")
app.permanent_session_lifetime = timedelta(minutes=5)


@app.route("/login", methods=["GET", "POST"])
def login():
    if request.method == "GET":
        # Render Login Page: 
        username = session.get("username")
        if username: # redirect if already logged in
            flash(("Already logged in as "+username+"."))
            # return redirect(next_url)
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
    flash("Logged Out.")
    return redirect("/")

@app.route("/signup", methods=["GET", "POST"])
def signup():
    username = session.get("username")
    if username is None:
        flash("Unauthorized! Login to sign up new users.")
        return redirect(url_for("login", next="/signup"))
    group = session.get("group")
    if group != "admin":
        flash("Not allowed! You do not have permissions to create new users.")
        return redirect(url_for("login", next="/"))
    
    if request.method == "GET":
        return render_template("special_pages/signup.html")
    
    if request.method == "POST":
        # Parse URL Params:
        username = request.form.get("username")
        password = request.form.get("password")
        group = request.form.get("group")

        # Check For Existing User:
        (msg,user) = db.queryUser(username=username)
        if user is None:
            raise BadGateway(("Problem while reading user: "+str(msg)))
        if user:
            flash("Error! User already exists.")
            return redirect(url_for("signup")) # reload the page with flash message
        
        # Check Username:
        pattern = r"^[A-Za-z0-9]+$"
        if re.match(pattern, username) is None:
            flash("Invalid username! It can only contain letters or numbers and not have spaces.")
            return redirect(url_for("signup")) # reload the page with flash message

        # Creat New User in Database:
        login_token = username + password
        hashed_token = generate_password_hash(login_token)
        user = { "username": username, "group": group, "token": hashed_token }
        msg = db.insertUser(user)
        if msg is not None:
            raise BadGateway(("Problem while inserting new user: "+str(msg)))
        
        # Reload Page:
        flash(("Created user "+username+"."))
        return redirect(url_for("signup")) # reload the page with flash message

@app.route("/signout", methods=["GET", "POST"])
def signout():
    username = session.get("username")
    if username is None:
        flash("Unauthorized! Login to delete users.")
        return redirect(url_for("login", next="/signout"))
    group = session.get("group")
    if group != "admin":
        flash("Not allowed! You do not have permissions to delete users.")
        return redirect(url_for("login", next="/"))
    
    if request.method == "GET":
        return render_template("special_pages/signout.html")
    
    if request.method == "POST":
        # Parse URL Params:
        username_to_delete = request.form.get("username", "")

        # Check Current User:
        if username == session.get("username"):
            flash("Error! Cannot delete user while logged in.")
            return redirect(url_for("signout")) # reload the page with flash message

        # Check For Existing User:
        (msg,user) = db.queryUser(username=username)
        if user is None:
            raise BadGateway(("Problem while reading user: "+str(msg)))
        if not user:
            flash("Error! No user found.")
            return redirect(url_for("signout")) # reload the page with flash message
        
        # Delete User From Database:
        msg = db.deleteUser(username)
        if msg is not None:
            raise BadGateway(("Problem while deleting user: "+str(msg)))
        
        # Reload Page:
        flash(("Deleted user "+username+"."))
        return redirect(url_for("signout")) # reload the page with flash message



@app.errorhandler(Exception)
def error(e):
    if isinstance(e, HTTPException): # handle call HTTP errors
        return render_template("special_pages/error.html", code=e.code, message=e.description), e.code
    else: # return unknown errors
        # TODO: dont pass message for deployment!
        return render_template("special_pages/error.html", code=500, message=str(e)), 500

