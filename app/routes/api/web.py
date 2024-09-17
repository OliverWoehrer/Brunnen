"""
This module implements the functions to handle routes of "/api/ui"
"""
from flask import Blueprint, session, render_template, redirect, url_for, request, flash
from werkzeug.security import generate_password_hash
from werkzeug.exceptions import HTTPException, BadRequest, UnprocessableEntity, BadGateway, Unauthorized, Forbidden
from datetime import datetime, timedelta, timezone
import json
import pandas as pd
import re
from data import data_client as db
from ..web.web import set_last_visit

web = Blueprint("web", __name__, url_prefix="/web")

# TODO: enable authentication
# @web.before_request
# def check_authentication():
#     username = session.get("username")
#     if username is None:
#         raise Unauthorized("Login to continue.")

@web.route("/sync", methods=["GET"])
def sync():
    # Read Logs From Database:
    (msg,timestamp) = db.queryLatestData()
    if timestamp is None:
        raise BadGateway(("Problem while reading latest sync: "+msg))
    
    # Return JSON Response:
    payload = { "last_sync": int(datetime.timestamp(timestamp)*1000) }
    return payload, 200

@web.route("/logs", methods=["GET"])
def logs():
    # Parse Start Parameter:
    start_param = request.args.get("start")
    if start_param is None:
        raise UnprocessableEntity("Missing parameter 'start'.")
    try:
        start = datetime.fromisoformat(start_param)
    except ValueError as e:
        raise BadRequest(("Problem while parsing parameter 'start': "+str(e)))
    
    # Parse Stop Parameter:
    stop_param = request.args.get("stop")
    if stop_param is None:
        raise UnprocessableEntity("Missing parameter 'stop'.")
    try:
        stop = datetime.fromisoformat(stop_param)
    except ValueError as e:
        raise BadRequest(("Problem while parsing parameter 'stop': "+str(e)))
    
    # Input Cleaning:
    if start > stop:
        raise UnprocessableEntity("Invalid time period: Stop time has to be larger then start time.")
    
    # Read Logs From Database:
    total_period = stop - start
    (msg,logs) = db.queryLogs(start_time=start, stop_time=stop)
    if logs is None:
        raise BadGateway(("Problem while reading logs: "+msg))
    
    # Return JSON Response:
    return logs, 200

@web.route("/data", methods=["GET"])
def data():
    # Parse Start Parameter:
    start_param = request.args.get("start")
    if start_param is None:
        raise UnprocessableEntity("Missing parameter 'start'.")
    try:
        start = datetime.fromisoformat(start_param)
    except ValueError as e:
        raise BadRequest(("Problem while parsing parameter 'start': "+str(e)))
    
    # Parse Stop Parameter:
    stop_param = request.args.get("stop")
    if stop_param is None:
        raise UnprocessableEntity("Missing parameter 'stop'.")
    try:
        stop = datetime.fromisoformat(stop_param)
    except ValueError as e:
        raise BadRequest(("Problem while parsing parameter 'stop': "+str(e)))
    
    # Input Cleaning:
    if start > stop:
        raise UnprocessableEntity("Invalid time period: Stop time has to be larger then start time.")
    
    # Read Data From Database:
    total_period = stop - start
    (msg,data) = db.queryData(start_time=start, stop_time=stop, window_size=timedelta(seconds=5*60))
    if data is None:
        raise BadGateway(("Problem while reading data: "+msg))
    
    # Return JSON Response:
    return data, 200

@web.route("/user", methods=["GET","POST"])
def user():
    # Check Authentication:
    group = session.get("group")
    if group is None:
        raise Unauthorized("Log in to continue")
    if group != "admin":
        raise Forbidden("You do not have permissions to create new users.")
    
    # Build Next Destination:
    root_url = request.root_url
    refferer = request.referrer
    origin = refferer.replace(root_url,"")

    if request.method == "GET":
        # Read All Users:
        (msg,users) = db.queryUsers()
        if users is None:
            raise BadGateway(("Problem while reading logs: "+msg))
        return users
    
    if request.method == "POST":
        # Parse URL Params:
        username = request.form.get("username")
        if username is None:
            raise BadRequest("Missing parameter 'username'.")
        action = request.form.get("action")
        if action is None:
            raise BadRequest("Missing parameter 'action'.")
        if action != "create" and action != "delete":
            raise BadRequest("Unknown value for field 'action'.")
        
        if action == "create":
            # Parse URL Params:
            password = request.form.get("password")
            if password is None:
                raise BadRequest("Missing parameter 'password'.")
            group = request.form.get("group")
            if group is None:
                raise BadRequest("Missing parameter 'group'.")
            
            # Check For Existing User:
            (msg,user) = db.queryUser(username=username)
            if user is None:
                flash(("Problem while reading user: "+str(msg)))
                return redirect(request.referrer)
            if user:
                flash("Error! User already exists.")
                return redirect(request.referrer)
            
            # Check Username:
            pattern = r"^[A-Za-z0-9]+$"
            if re.match(pattern, username) is None:
                flash("Invalid username! It can only contain letters or numbers and not have spaces.")
                return redirect(request.referrer)

            # Creat New User in Database:
            login_token = username + password
            hashed_token = generate_password_hash(login_token)
            user = { "username": username, "group": group, "token": hashed_token }
            msg = db.insertUser(user)
            if msg is not None:
                raise BadGateway(("Problem while inserting new user: "+str(msg)))

            # Inform User:
            flash(("Created user "+username+"."))
            return redirect(request.referrer)

        elif action == "delete":
            # Check Current User:
            if username == session.get("username"):
                flash("Error! Cannot delete user while logged in.")
                return redirect(request.referrer)

            # Check For Existing User:
            (msg,user) = db.queryUser(username=username)
            if user is None:
                raise BadGateway(("Problem while reading user: "+str(msg)))
            if not user:
                flash("Error! User "+username+" not found.")
                return redirect(request.referrer)
            
            # Delete User From Database:
            msg = db.deleteUser(username)
            if msg is not None:
                raise BadGateway(("Problem while deleting user: "+str(msg)))        
        
            # Inform User:
            flash("Delete user "+username+".")
            return redirect(request.referrer)

@web.after_request
def log(response):
    set_last_visit(datetime.now(timezone.utc).replace(microsecond=0))
    return response

@web.errorhandler(Exception)
def error(e):
    if isinstance(e, HTTPException): # display HTTP errors
        return render_template("special_pages/error.html", code=e.code, message=e.description), e.code
    else: # return unknown errors
        # TODO: remove for deployment!
        return render_template("special_pages/error.html", code=500, message=str(e)), 500