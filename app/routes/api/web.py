"""
This module implements the functions to handle routes of "/api/ui"
"""
from flask import Blueprint, session, render_template, redirect, send_file, request, flash, current_app
from werkzeug.security import generate_password_hash
from werkzeug.exceptions import HTTPException, BadRequest, UnprocessableEntity, BadGateway, Unauthorized, Forbidden, InternalServerError
from werkzeug.utils import secure_filename
from datetime import datetime, timedelta, timezone
import hashlib
import re
import config
from data import data_client as db
from .device import get_last_sync
from ..web.web import set_last_visit

# Register Blueprint Hierarchy:
web = Blueprint("web", __name__, url_prefix="/web")

@web.before_request
def check_authentication():
    username = session.get("username")
    if username is None:
        raise Unauthorized("Login to continue.")

@web.route("/sync", methods=["GET"])
def sync():
    # Read Logs From Database:
    (msg,timestamp) = db.queryLatestTimestamp()
    if timestamp is None:
        raise BadGateway(("Problem while reading latest sync: "+msg))
    
    # Return JSON Response:
    last_sync = get_last_sync()
    payload = {
        "last_sync": int(datetime.timestamp(last_sync)*1000),
        "last_data": int(datetime.timestamp(timestamp)*1000)
    }
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
    window_size = (total_period/1000) + timedelta(seconds=1)
    (msg,data) = db.queryData(start_time=start, stop_time=stop, window_size=window_size)
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

    if request.method == "GET":
        # Read All Users:
        (msg,users) = db.queryUsers()
        if users is None:
            raise BadGateway(("Problem while reading users: "+msg))
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
    
@web.route("/intervals", methods=["GET","POST"])
def intervals():
    # Read Intervals From Database:
    (msg,settings) = db.querySettings()
    if settings is None:
        raise BadGateway(("Problem while reading settings: "+msg))
    intervals = settings.get("intervals", [])
    
    if request.method == "GET":
        return intervals

    if request.method == "POST":
        # Parse Parameters:
        action = request.form.get("action")
        if action is None:
            raise BadRequest("Missing parameter 'action'.")
        
        if action == "create":
            # Parse Start Parameter:
            start_param = request.form.get("start")
            if start_param is None:
                raise UnprocessableEntity("Missing parameter 'start'.")
            try:
                start = datetime.strptime(start_param, '%H:%M').time()
            except ValueError as e:
                raise BadRequest(("Problem while parsing parameter 'start': "+str(e)))
            
            # Parse Stop Parameter:
            stop_param = request.form.get("stop")
            if stop_param is None:
                raise UnprocessableEntity("Missing parameter 'stop'.")
            try:
                stop = datetime.strptime(stop_param, '%H:%M').time()
            except ValueError as e:
                raise BadRequest(("Problem while parsing parameter 'stop': "+str(e)))
            
            # Input Cleaning:
            if start > stop:
                raise UnprocessableEntity("Invalid time period: Stop time has to be larger then start time.")

            # Build New Interval:
            weekdays = 0
            keys = request.form.keys()
            if "mon" in keys:
                weekdays = weekdays | 0b00000001
            if "tue" in keys:
                weekdays = weekdays | 0b00000010
            if "wed" in keys:
                weekdays = weekdays | 0b00000100
            if "thu" in keys:
                weekdays = weekdays | 0b00001000
            if "fri" in keys:
                weekdays = weekdays | 0b00010000
            if "sat" in keys:
                weekdays = weekdays | 0b00100000
            if "sun" in keys:
                weekdays = weekdays | 0b01000000
            interval = { "start": start.strftime("%H:%M:%S"), "stop": stop.strftime("%H:%M:%S"), "wdays": weekdays }
            intervals.append(interval)

        elif action == "delete":
            interval_id_param = request.form.get("interval_id")
            if interval_id_param is None:
                raise UnprocessableEntity("Missing parameter 'interval_id'.")
            try:
                interval_id = int(interval_id_param)
            except ValueError as e:
                raise BadRequest(("Problem while parsing parameter 'interval_id': "+str(e)))
            
            if 0 <= interval_id and interval_id < len(intervals):
                del intervals[interval_id]
            else:
                raise BadRequest("Invalid interval id.")

        else:
            raise BadRequest("Unknown value for field 'action'.")

        # Write Update Settings to Database:
        updatedSettings = { "intervals": intervals }
        msg = db.insertSettings(updatedSettings)
        if msg:
            raise BadGateway(("Problem while writing settings: "+msg))

        # Return JSON Response:
        return redirect(request.referrer)

@web.route("/synchronization", methods=["GET","POST"])
def synchronization():
    # Read Intervals From Database:
    (msg,settings) = db.querySettings()
    if settings is None:
        raise BadGateway(("Problem while reading settings: "+msg))
    sync = settings.get("sync", config.readBrunnenSettings("sync"))
    
    if request.method == "GET":
        return sync

    if request.method == "POST":
        # Parse Sleep Mode:
        sleep_mode_input = request.form.get("sleep_mode_period")
        if sleep_mode_input is None:
            raise UnprocessableEntity("Missing parameter 'sleep_mode_period'.")
        try:
            sleep_mode_period = int(sleep_mode_input)
        except ValueError as e:
            raise BadRequest("Parameter 'sleep_mode_period' is not an integer.")

        # Parse Standby Mode:
        standby_mode_input = request.form.get("standby_mode_period")
        if standby_mode_input is None:
            raise UnprocessableEntity("Missing parameter 'standby_mode_period'.")
        try:
            standby_mode_period = int(standby_mode_input)
        except ValueError as e:
            raise BadRequest("Parameter 'standby_mode_period' is not an integer.")

        # Parse Realt-Time Mode:
        rt_mode_input = request.form.get("rt_mode_period")
        if rt_mode_input is None:
            raise UnprocessableEntity("Missing parameter 'rt_mode_period'.")
        try:
            rt_mode_period = int(rt_mode_input)
        except ValueError as e:
            raise BadRequest("Parameter 'rt_mode_period' is not an integer.")

        # Update Sync Settings:
        sync["long"] = sleep_mode_period
        sync["medium"] = standby_mode_period
        sync["short"] = rt_mode_period

        # Write Update Settings to Database:
        updatedSettings = { "sync": sync }
        msg = db.insertSettings(updatedSettings)
        if msg:
            raise BadGateway(("Problem while writing settings: "+msg))

        # Return JSON Response:
        return redirect(request.referrer)

@web.route("/thresholds", methods=["GET","POST"])
def thresholds():
    # Read Thresholds From Database:
    (msg,settings) = db.querySettings()
    if settings is None:
        raise BadGateway(("Problem while reading settings: "+msg))
    thresholds = settings.get("thresholds", config.readBrunnenSettings("thresholds"))
    
    if request.method == "GET":
        return thresholds

    if request.method == "POST":
        # Parse Rain Threshold:
        rain_threshold_input = request.form.get("rain_threshold")
        if rain_threshold_input is None:
            raise UnprocessableEntity("Missing parameter 'rain_threshold'.")
        try:
            rain_threshold = float(rain_threshold_input)
        except ValueError as e:
            raise BadRequest("Parameter 'rain_threshold' is not a float.")

        # Parse Marker Threshold:
        marker_threshold_input = request.form.get("marker_threshold")
        if marker_threshold_input is None:
            raise UnprocessableEntity("Missing parameter 'marker_threshold'.")
        try:
            marker_threshold = float(marker_threshold_input)
        except ValueError as e:
            raise BadRequest("Parameter 'marker_threshold' is not a float.")

        # Update Threshold Settings:
        thresholds["rain"] = rain_threshold
        thresholds["marker"] = marker_threshold

        # Write Update Settings to Database:
        updatedSettings = { "thresholds": thresholds }
        msg = db.insertSettings(updatedSettings)
        if msg:
            raise BadGateway(("Problem while writing settings: "+msg))

        # Return JSON Response:
        return redirect(request.referrer)

@web.route("/firmware", methods=["GET","POST"])
def firmware():
    if request.method == "GET":
        filepath = current_app.config["files"] + "/firmware.bin"
        file = open(filepath, mode="rb")
        if file is None:
            raise InternalServerError("Failed to open file")
        return send_file(file, as_attachment=True, download_name="firmware.bin", mimetype="application/octet-stream")

    if request.method == "POST":
        # Parse File Upload:
        file = request.files['my_firmware']
        if file is None:
            raise BadRequest("Missing firmware file.")
        filename = file.filename
        if filename == '':
            raise UnprocessableEntity("No selected firmware file.")
        if filename.rsplit('.', 1)[1].lower() != "bin":
            raise BadRequest(f"Unexpected file extension. Expected '.bin' but got '{filename}'")
    
        # Save Firmware File:
        filename = secure_filename(filename) # convert to ASCII friendly format
        try:
            file.save(f"{current_app.config['files']}/firmware.bin")
        except Exception as e:
            raise InternalServerError(f"Could not save uploaded file ({e})")

        # Read Firmware Version From Database:
        (msg,settings) = db.querySettings()
        if settings is None:
            raise BadGateway(("Problem while reading settings: "+msg))
        firmware = settings.get("firmware", config.readBrunnenSettings("firmware"))

        # Update Firmware Version:
        DATETIME_FORMAT = "%Y-%m-%dT%H:%M:%S"
        timestamp = datetime.now(timezone.utc).replace(microsecond=0)
        firmware["version"] = timestamp.strftime(DATETIME_FORMAT)

        # Write Update Settings to Database:
        updatedSettings = { "firmware": firmware }
        msg = db.insertSettings(updatedSettings)
        if msg:
            raise BadGateway(("Problem while writing settings: "+msg))

        # Return JSON Response:
        return redirect(request.referrer)

@web.route("/firmwarestatus", methods=["GET"])
def firmwarestatus():
    # Read Firmware Version From Database:
    (msg,settings) = db.querySettings()
    if settings is None:
        raise BadGateway(("Problem while reading settings: "+msg))
    firmware = settings.get("firmware", config.readBrunnenSettings("firmware"))
    return firmware

@web.after_request
def log(response):
    set_last_visit(datetime.now(timezone.utc).replace(microsecond=0))
    return response

@web.errorhandler(Exception)
def error(e):
    if isinstance(e, HTTPException): # display HTTP errors
        current_app.logger.error(f"{e.code} {e.name}: {e.description}\r\n{e.__traceback__}")
        return e.description, e.code
    else: # return unknown errors
        current_app.logger.error(f"{e}:\r\n{e.__traceback__}")
        return str(e), 500