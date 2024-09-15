"""
This module implements the functions to handle routes of "/device"
"""
from flask import Blueprint, g, request
from werkzeug.exceptions import BadRequest, Forbidden, NotFound, MethodNotAllowed, UnprocessableEntity, BadGateway
from datetime import datetime, timedelta, timezone
import json
import pandas as pd
from data import data_client as db
import config
from ..web.web import get_last_visit

device = Blueprint("device", __name__, url_prefix="/device")
last_sync = datetime.now(timezone.utc).replace(microsecond=0)

@device.before_request
def fill_global_context():
    global last_sync, last_visit
    g.last_visit = get_last_visit()
    g.last_sync = last_sync
    g.curr_sync = datetime.now(timezone.utc).replace(microsecond=0)

@device.route("/brunnen", methods=["GET", "POST", "DELETE"])
def brunnen():
    # Check Access Token:
    # TODO: check if secret token is present
    # (msg,df) = db.queryDevices() # read devices settings
    # if df is None:
    #     raise BadGateway(("Problem while reading settings: "+msg))
    # idx = df["devices"].last_valid_index()
    # latest_devices = df["devices"][idx]
    # devices_json = json.loads(latest_devices)
    # if device_id not in devices_json["device_ids"]:
    #     raise Forbidden(("Unknown device id: "+str(device_id)))

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

    # Update Synchronisation Period (Ready State):
    sync = settings.get("sync", config.readBrunnenSettings("sync"))
    old_sync_mode = sync["mode"]
    delta = (g.curr_sync - g.last_visit).total_seconds()
    if g.last_sync <= g.last_visit: # recent visit since last sync; change to "hot" state (=short sync period)
        sync["mode"] = "short"
    elif delta > sync["medium"] and sync["mode"] == "short": # no recent visit; change to "warm" state (=medium sync period)
        sync["mode"] = "medium"
    elif delta > sync["long"]: # visit long ago; change to "cold" state (=long sync period) at night
        current_hour = datetime.now(timezone.utc).hour
        if abs(current_hour - 13) > 6 and sync["mode"] == "medium": # check if night time (19:00 - 07:00 UTC)
            sync["mode"] = "long"
        elif sync["mode"] == "long": # day time (07:00 - 19:00 UTC)
            sync["mode"] = "medium"
    if old_sync_mode != sync["mode"]: # write updated settings
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