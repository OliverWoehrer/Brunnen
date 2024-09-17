"""
This module implements the prefix and postfix request handlers for main pages. It only provides
functions that are included before and after the actual request. It can be seen as a wrapper around
the actual request handling code. All blueprints registered here are protected by authentication
and tracked.
"""
from flask import Blueprint, redirect, request, url_for, session, flash
from werkzeug.exceptions import Unauthorized
from datetime import datetime, timezone

# Global Tracking Variables:
last_visit = datetime.now(timezone.utc).replace(microsecond=0)
visit_count = 0

# Register Blueprint Hierarchy:
from .dashboard import dashboard
from .profile import profile
from .settings import settings
from .update import update
web = Blueprint("web", __name__, template_folder="../../templates", static_folder="../../static")
web.register_blueprint(dashboard)
web.register_blueprint(profile)
web.register_blueprint(settings)
web.register_blueprint(update)

@web.before_request
def check_authentication():
    username = session.get("username")
    if username is None:
        flash("Unauthorized. Login to continue.")
        next_url = request.path
        return redirect(url_for("login", next=next_url))

@web.after_request
def log_request(response):
    global visit_count, last_visit
    visit_count = visit_count + 1
    last_visit = datetime.now(timezone.utc).replace(microsecond=0)
    return response

@web.context_processor
def inject_visit_counter():
    global visit_count
    return dict(debug=visit_count)

def get_last_visit() -> datetime:
    global last_visit
    return last_visit

def set_last_visit(time: datetime):
    global last_visit
    last_visit = time