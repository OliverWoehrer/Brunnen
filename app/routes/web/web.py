"""
This module implements the functions to handle routes of web web.
"""
from flask import Blueprint, render_template, redirect, request, url_for, session, flash
from werkzeug.exceptions import Unauthorized
from datetime import datetime, timezone
from .dashboard import dashboard
from .profile import profile
from .settings import settings
from .update import update

web = Blueprint("web", __name__, template_folder="../../templates", static_folder="../../static")
web.register_blueprint(dashboard)
web.register_blueprint(profile)
web.register_blueprint(settings)
web.register_blueprint(update)
last_visit = datetime.now(timezone.utc).replace(microsecond=0)
request_count = 0

@web.before_request
def check_authentication():
    username = session.get("username")
    if username is None:
        flash("Unauthorized. Login to continue.")
        next_url = request.path
        return redirect(url_for("login", next=next_url))

@web.route("/", methods=["GET"])
def index():
    return redirect(url_for("web.dashboard.index"))

@web.after_request
def log_request(response):
    global request_count, last_visit
    request_count = request_count + 1
    last_visit = datetime.now(timezone.utc).replace(microsecond=0)
    return response

@web.context_processor
def inject_request_counter():
    global request_count
    return dict(debug=request_count)

def get_last_visit() -> datetime:
    global last_visit
    return last_visit

def set_last_visit(time: datetime):
    global last_visit
    last_visit = time