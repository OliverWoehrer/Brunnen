"""
This module implements functions to read and write configuration data from "config.json".
"""
import json

CONFIG_FILE = "./config.json"
DEFAULT_FILE = "./defaults/config.json"

def readPort() -> str:
    config = _loadConfig()
    if "port" not in config:
        raise ValueError("No port in config")
    return config.get("port")

def readInfluxHost() -> str:
    config = _loadConfig()
    if "influx" not in config:
        raise ValueError("No influx in config")
    influx = config.get("influx")
    if "host" not in influx:
        raise ValueError("No host in influx")
    return influx.get("host")

def readInfluxPort() -> int:
    config = _loadConfig()
    if "influx" not in config:
        raise ValueError("No influx in config")
    influx = config.get("influx")
    if "port" not in influx:
        raise ValueError("No port in influx")
    return influx.get("port")

def readInfluxOrganization() -> str:
    config = _loadConfig()
    if "influx" not in config:
        raise ValueError("No influx in config")
    influx = config.get("influx")
    if "organization" not in influx:
        raise ValueError("No organization in influx")
    return influx.get("organization")






""" PRIVATE METHODS """

def _loadConfig() -> dict:
    """
    This is a helper function to read the configuration file, named CONFIG_FILE and load its content before returning it
    
    Returns:
        config (dict): json dictionary object
    """
    config = None
    try:
        file = open(CONFIG_FILE, mode="r")
        config = json.load(file)
    except FileNotFoundError as e:
        file = open(DEFAULT_FILE, mode="r")
        config = json.load(file)
    finally:
        file.close()
    return config

def _storeConfig(configuration: dict):
    """
    This is a helper function to write the configuration file, named CONFIG_FILE and dump the given config json to it
    
    Parameters:
        config (dict): json dictionary object
    """
    with open(CONFIG_FILE, mode="w") as file:
        json.dump(configuration, file ,indent=4)