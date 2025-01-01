import argparse
import os
from datetime import datetime

import influxdb_client
from influxdb_client import InfluxDBClient
from influxdb_client.client.exceptions import InfluxDBError
from influxdb_client.client.write_api import SYNCHRONOUS
import pandas as pd

influx_write_api = None

URL = "https://db.woehrer-consulting.at"
INFLUX_API_TOKEN = "kCL7hzDnV7YBt3DHkxV88FZcGP3fJ7bqmtCX926f89v-MXQ84h1d12Z-FxUwlswPIGbWGGSOwyF6FHq0754Nsg=="
ORGANIZATION = "Private"
MEASUREMENT_BUCKET = "Brunnen"
DATA = "water"

def insertData(data: pd.DataFrame) -> str:
    """
    This function takes the given data and inserts it into the measurements. The index column
    of the dataframe needs to be timestamps (data.index must be pd.DatetimeIndex). The fields
    "flow", "pressure" and "level" must be all present.
    Example dataframe:
                            flow  pressure  level
    2024-09-05 00:05:23     0      1379   1402
    2024-09-05 00:05:24     5      1379   1450
    2024-09-05 00:05:25    10      1200   1500
    2024-09-05 00:05:26    10      1200   1550

    :param data: dataframe holding the data to insert

    :return: Error message on failure, None on success
    """
    global influx_write_api

    if not isinstance(data.index, pd.DatetimeIndex):
        return "Dataframe index must be pd.DatetimeIndex"
    if "flow" not in data:
        return "Dataframe is missing 'flow' field"
    if "pressure" not in data:
        return "Dataframe is missing 'pressure' field"
    if "level" not in data:
        return "Dataframe is missing 'level' field"
    try:
        rec = data[["flow", "pressure", "level"]] # only use defined field columns
        influx_write_api.write(bucket=MEASUREMENT_BUCKET, record=rec, data_frame_measurement_name=DATA)
    except InfluxDBError as e:
        return e.message
    else:
        return None

def convert_to_datetime(date_str):
    try:
        return pd.to_datetime(date_str)
    except ValueError:
        return None

# Initalitze Influx Client:
client = influxdb_client.InfluxDBClient(url=URL, token=INFLUX_API_TOKEN, org=ORGANIZATION, timeout=40_000)
if not client.ping():
    raise RuntimeError("Failed to initialize database client. Could not reach database server.")

# Initalize API Objects:
buckets_api = client.buckets_api()
influx_write_api = client.write_api(write_options=SYNCHRONOUS)

# Check For Required Database Buckets:
bucket = buckets_api.find_bucket_by_name(MEASUREMENT_BUCKET)
if not bucket:
    raise RuntimeError(f"Found no bucket named {MEASUREMENT_BUCKET}", org=organization)

# Initalize Argument Parser:
parser = argparse.ArgumentParser(prog="upload-data.py", description="This script reads data files (txt format) and uploads them to the influx database.")
parser.add_argument("input_directory", help="Relative path to directory containing the input files (CSV format)")
args = parser.parse_args()

# Build Input File Paths:
file_paths = []
dir_list = os.listdir(args.input_directory)
for dir_item in dir_list:
    if dir_item.endswith(".txt"):
        file_paths.append(args.input_directory+dir_item)
file_paths.sort()

# Iterate All Input Files:
num = len(file_paths) - 1
for idx,file_path in enumerate(file_paths):
    # Read CSV Data:
    print(f"[{idx:03}/{num:03}] {file_path}")
    df = pd.read_csv(file_path, sep=",", header=0, names=["flow","pressure","level"], on_bad_lines="warn")
    df.index = pd.to_datetime(df.index, errors='coerce') # convert to datetime, NaN on faulty line
    df = df[df.index.notnull()] # drop rows with faulty index
    df = df.dropna() # drop rows with faulty data
    df = df.astype(int)

    # Write Data Data:
    msg = insertData(data=df)
    if msg:
        print("Problem while inserting data: "+str(msg))
