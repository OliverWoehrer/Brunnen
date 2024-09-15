"""
This module implements the functions to handle routes of "/api/ui"
"""
from flask import Blueprint, render_template, redirect, url_for, abort, request
from werkzeug.exceptions import BadRequest, Forbidden, NotFound, MethodNotAllowed, UnprocessableEntity, BadGateway
from datetime import datetime, timedelta, timezone
import json
import pandas as pd
from data import data_client as db
import config
from ..web.web import set_last_visit

web = Blueprint("web", __name__, url_prefix="/web")

@web.route("/logs", methods=["GET"])
def logs():
    # TODO: check authentication

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
    (msg,df) = db.queryLogs(start_time=start, stop_time=stop)
    if df is None:
        raise BadGateway(("Problem while reading logs: "+msg))
    
    # Return JSON Response:
    payload = { "logs": [] }
    if df.empty:
        return payload
    df = df.reset_index(names=["timestamp"])
    values = pd.DataFrame.to_json(df, orient="split")
    return values, 200

@web.after_request
def log(response):
    set_last_visit(datetime.now(timezone.utc).replace(microsecond=0))
    return response
