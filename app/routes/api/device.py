"""
This module implements the functions to handle routes of "/device"
"""
from flask import Blueprint, g, request
from werkzeug.exceptions import BadRequest, Forbidden, NotFound, MethodNotAllowed, UnprocessableEntity, BadGateway, Unauthorized
from datetime import datetime, timedelta, timezone
import time
import json
import pandas as pd
from data import data_client as db
import config
from ..web.web import get_last_visit

# Global Variables:
last_sync = datetime.now(timezone.utc).replace(microsecond=0)
daytime = config.readBrunnenDaytime()
daytime_start = time.strptime(daytime.get("start","08:00:00"), "%H:%M:%S")
daytime_stop = time.strptime(daytime.get("stop","20:00:00"), "%H:%M:%S")

# Register Blueprint Hierarchy:
device = Blueprint("device", __name__, url_prefix="/device")

@device.before_request
def fill_global_context():
    global last_sync, last_visit
    g.last_sync = last_sync
    g.last_visit = get_last_visit()
    g.current_datetime = datetime.now(timezone.utc).replace(microsecond=0)

@device.before_request
def check_credentials():
    # Check Authentication Header:
    if not request.authorization:
        raise Unauthorized("Received no credentials")
    if request.authorization.type != "basic":
        raise BadRequest("Received wrong authentication type")
    
    # Parse Credentials:
    credentials = request.authorization.parameters
    device_id = credentials.get("username","")
    device_token = credentials.get("password","")
    
    # Look for Device Credentials:
    (msg,device) = db.queryDevice(device_id=device_id)
    if device is None:
        raise BadGateway(("Problem while reading settings: "+msg))
    if not device or device_token != device.get("token",""):
        raise Unauthorized("Wrong credentials.")

@device.route("/brunnen", methods=["GET", "POST", "DELETE"])
def brunnen():    
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

    if request.method == "GET":
        pass

    if request.method == "POST":
        # Parse Request Body:
        body = request.data.decode("utf-8")
        payload = json.loads(body)
        if "data" in payload:
            # Check JSON Fields:
            data = payload["data"]
            if "columns" not in data:
                raise UnprocessableEntity("Missing 'columns' field.")
            if "values" not in data:
                raise UnprocessableEntity("Missing 'values' field.")
            for row in data["values"]:
                if len(data["columns"]) is not len(data["values"][row]):
                    raise UnprocessableEntity("Number of given columns and actual value columns does not match.")
                break

            # Initalize Dataframe:
            df = pd.DataFrame.from_dict(data["values"], orient="index", columns=data["columns"])
            df = df.set_index(pd.to_datetime(df.index)) # convert to datetime

            # Check Parameters:
            if start != df.index[0]:
                raise UnprocessableEntity("Parameter 'start' does not match with the start of the given data.")
            if stop != df.index[-1]:
                raise UnprocessableEntity("Parameter 'stop' does not match with the stop of the given data.")

            # Write Data Data:
            msg = db.insertData(data=df)
            if msg:
                raise BadGateway(("Problem while inserting data: "+str(msg)))
        
        if "logs" in payload:
            # Initalize Dataframe:
            logs = payload["logs"]
            df = pd.DataFrame.from_dict(logs, orient="index", columns=["message", "level"])
            df = df.set_index(pd.to_datetime(df.index)) # convert to datetime

            # Write Logs:
            msg = db.insertLogs(logs=df)
            if msg:
                raise BadGateway(("Problem while inserting logs: "+str(msg)))
        
        if "settings" in payload:
            settings = {}
            WRITEABLE_SETTINGS = ["pump"] # list settings writeable by this endpoint
            for key in payload["settings"]: # check each setting if writeable
                if key in WRITEABLE_SETTINGS:
                    settings[key] = payload["settings"][key]
            
            # Write Settings:
            msg = db.insertSettings(settings=settings)
            if msg:
                raise BadGateway(("Problem while inserting settings: "+str(msg)))

    if request.method == "DELETE":
        # Parse Request Body:
        body = request.data.decode("utf-8")
        payload = json.loads(body)
        if "select" not in payload:
            raise BadRequest("Missing selected keys. Dont know what to delete")

        if "data" in payload["select"]: # drop data
            msg = db.deleteData(start_time=start, stop_time=stop)
            if msg:
                raise BadGateway(("Problem while deleting data: "+str(msg)))
        
        if "logs" in payload["select"]: # drop logs:
            msg = db.deleteLogs(start_time=start, stop_time=stop)
            if msg:
                raise BadGateway(("Problem while deleting logs: "+str(msg)))
        
        if "settings" in payload["select"]:
            msg = db.deleteSettings(start_time=start, stop_time=stop)
            if msg:
                raise BadGateway(("Problem while deleting settings: "+str(msg)))

    # Get Settings JSON:
    (msg,settings) = db.querySettings()
    if settings is None:
        raise BadGateway(("Problem while reading settings: "+str(msg)))
    if not settings: # no settings found, use default config
        settings = config.readBrunnenSettings(None)

    # [Synchronisation Mode]
    # "Real-Time Mode": update in short periods, if a recent visit occured ("hot" state)
    # "Standby Mode": update in medium periods, compromise between latency and bandwidth ("warm" state)
    # "Sleep Mode": update in long periods, during night time ("cold" state)

    # Update Synchronisation Period:
    sync = settings.get("sync", config.readBrunnenSettings("sync"))
    old_sync_mode = sync["mode"]
    delta = (g.current_datetime - g.last_visit).total_seconds() # seconds since last visit
    if delta < sync["medium"]: # recent visit, change to real-time mode
        sync["mode"] = "short"
    if delta > sync["medium"]: # no recent visit, change to standby mode
        sync["mode"] = "medium"
    if delta > sync["long"]: # last visit long time ago
        current_time = g.current_datetime.timetz()
        if daytime_start < current_time and current_time < daytime_stop: # check if daytime
            sync["mode"] = "medium" # keep in standby mode during daytime
        else:
            sync["mode"] = "long" # sleep mode during night time
    if old_sync_mode != sync["mode"]: # check if mode is updated
        data = { "sync": sync }
        msg = db.insertSettings(settings=data)
        if msg:
            raise BadGateway(("Problem while inserting settings: "+str(msg)))
    
    # Return Updated Settings as JSON Response:
    (msg,settings) = db.querySettings(start_time=g.last_sync)
    if settings is None:
        raise BadGateway(("Problem while reading settings for response: "+str(msg)))
    g.last_sync = datetime.now(timezone.utc).replace(microsecond=0)
    payload = {} if not settings else { "settings": settings }
    return payload, 200

@device.after_request
def log(response):
    global last_sync
    last_sync = g.last_sync
    return response