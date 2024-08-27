"""
This module implements functions to read and write configuration data from "config.json".
TODO: write doc
"""
import json

CONFIG_FILE = "./config.json"
DEFAULT_FILE = "./defaults/config.json"

def readPort() -> str:
    config = _loadConfig()
    return config["port"]

def writePort(port: str):
    config = _loadConfig()
    config["port"] = port
    _storeConfig(config)






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