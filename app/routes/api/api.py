"""
This module implements the functions to handle routes of /api
"""
from flask import Blueprint

# Register Blueprint Hierarchy:
from .device import device
from .web import web
api = Blueprint("api", __name__, url_prefix="/api")
api.register_blueprint(device)
api.register_blueprint(web)