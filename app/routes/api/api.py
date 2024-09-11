"""
This module implements the functions to handle routes of /api
"""
from flask import Blueprint, render_template, redirect, url_for, abort, request
from werkzeug.exceptions import BadRequest, Forbidden, NotFound, MethodNotAllowed, UnprocessableEntity, BadGateway
from datetime import datetime, timedelta, timezone
import json
import pandas as pd
from data import data_client as db

api = Blueprint("api", __name__, url_prefix="/api")

@api.route("/brunnen", methods=["GET", "POST", "DELETE"])
def brunnen():
    # Check Access Token:
    # TODO: check if secret token is present
    # (msg,df) = db.querySettings() # read latest settings
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
        # Read Data Data:
        # total_period = stop - start
        # aggregation_period = timedelta(seconds = total_period.total_seconds() // 100)
        # (msg,df) = db.queryData(start_time=start, stop_time=stop, window_size=aggregation_period)
        # if df is None:
        #     raise BadGateway(("Problem while reading data: "+msg))
        # if df.empty:
        #     return {}
        
        # Return JSON Response:
        (msg,df) = db.querySettings()
        if df is None:
            raise BadGateway(("Problem while reading settings for response: "+str(msg)))
        payload = {}
        payload["settings"] = {}
        data_string = df.to_json(orient="split")
        data_json = json.loads(data_string)
        for setting in df.columns:
            idx = df[setting].last_valid_index()
            last = df[setting][idx]
            payload["settings"][setting] = last

        return payload, 200

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
            settings = payload["settings"]
            # Initalize Dataframe:
            timestamp = datetime.now(timezone.utc).replace(microsecond=0)
            data = {}
            cols = []
            WRITEABLE_SETTINGS = ["pump"]
            for row in settings: # check each setting if writeable
                if row in WRITEABLE_SETTINGS:
                    data[row] = json.dumps(settings[row])
                    cols.append(row)
            df = pd.DataFrame(data, index=[timestamp], columns=cols)

            # Write Settings:
            msg = db.insertSettings(settings=df)
            if msg:
                raise BadGateway(("Problem while inserting settings: "+str(msg)))
                
        # Return JSON Response:
        (msg,df) = db.querySettings()
        if df is None:
            raise BadGateway(("Problem while reading settings for response: "+str(msg)))
        payload = {}
        payload["settings"] = {}
        data_string = df.to_json(orient="split")
        data_json = json.loads(data_string)
        for setting in df.columns:
            idx = df[setting].last_valid_index()
            last = df[setting][idx]
            payload["settings"][setting] = last

        return payload, 200

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

        # Return JSON Response:
        (msg,df) = db.querySettings()
        if df is None:
            raise BadGateway(("Problem while reading settings for response: "+str(msg)))
        payload = {}
        payload["settings"] = {}
        data_string = df.to_json(orient="split")
        data_json = json.loads(data_string)
        for setting in df.columns:
            idx = df[setting].last_valid_index()
            last = df[setting][idx]
            payload["settings"][setting] = last

        return payload, 200

    else:
        raise MethodNotAllowed(valid_methods=["GET","POST","DELETE"])

