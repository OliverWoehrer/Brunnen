"""
This module implements the functions to handle routes of /api
"""
from flask import Blueprint, render_template, redirect, url_for, abort, request
from werkzeug.exceptions import BadRequest, Forbidden, MethodNotAllowed, UnprocessableEntity, BadGateway
from datetime import datetime, timedelta
import json
import pandas as pd
from data import data_client as db

api = Blueprint("api", __name__, url_prefix="/api")

@api.route("/data", methods=["GET"])
@api.route("/data/<device_id>", methods=["POST", "DELETE"])
def data(device_id: str = None):
    # TODO: check if secret token is present

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
        # Read Measurement Data:
        total_period = stop - start
        aggregation_period = timedelta(seconds = total_period.total_seconds() // 100)
        (msg,df) = db.queryMeasurements(start_time=start, stop_time=stop, window_size=aggregation_period)
        if df is None:
            raise BadGateway(("Problem while reading data: "+msg))
        if df.empty:
            return {}
        
        # Return JSON Response:
        payload = {}
        data_string = df.to_json(orient="split")
        data_json = json.loads(data_string)
        payload["index"] = data_json["index"]
        for column in df.columns:
            column_string = df[column].to_json(orient="split")
            column_json = json.loads(column_string)
            payload[column] = column_json["data"]
        return payload

    # Check Device ID:
    (msg,df) = db.querySettings() # read latest settings
    if df is None:
        raise BadGateway(("Problem while reading settings: "+msg))
    idx = df["devices"].last_valid_index()
    latest_devices = df["devices"][idx]
    devices_json = json.loads(latest_devices)
    if device_id not in devices_json["device_ids"]:
        raise Forbidden(("Unknown device id: "+str(device_id)))

    if request.method == "POST":
        # Parse Request Body:
        body = request.data.decode("utf-8")
        payload = json.loads(body)
        if "columns" not in payload:
            raise UnprocessableEntity("Missing 'columns' field.")
        COLUMNS = ["flow", "pressure", "level"]
        if not all(column in COLUMNS for column in payload["columns"]):
            raise UnprocessableEntity("Missing required column.")
        if "data" not in payload:
            raise UnprocessableEntity("Missing 'data' field.")
        for row in payload["data"]:
            if len(payload["columns"]) is not len(payload["data"][row]):
                raise UnprocessableEntity("Number of given columns and actual data columns does not match.")
            break

        # Initalize Dataframe:
        df = pd.DataFrame.from_dict(payload["data"], orient="index", columns=payload["columns"])
        df = df.set_index(pd.to_datetime(df.index)) # convert to datetime

        # Check Parameters:
        if start != df.index[0]:
            raise UnprocessableEntity("Parameter 'start' does not match with the start of the given data.")
        if stop != df.index[-1]:
            raise UnprocessableEntity("Parameter 'stop' does not match with the stop of the given data.")

        # Write Measurement Data:
        msg = db.insertMeasurements(data=df)
        if msg:
            raise BadGateway(("Problem while inserting data: "+str(msg)))

        # Return Response:
        return "ok", 200

    if request.method == "DELETE":
        msg = db.deleteMeasurements(start_time=start, stop_time=stop)
        if msg:
            BadGateway(("Problem while deleting data: "+str(msg)))

        return "ok", 200

    else:
        raise MethodNotAllowed(valid_methods=["GET","POST","DELETE"]) # Method Not Allowed


