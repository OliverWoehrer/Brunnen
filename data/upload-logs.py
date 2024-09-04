import argparse
import pandas as pd
import re
from datetime import datetime
import requests

HOST = "localhost:8086"
PATH = "/api/v2/write"
INFLUX_API_TOKEN = "djWUq7Fj1_1nIDR1va4ORRMvQHinkYg9J4YQX6k9TVVSUGIIJYu3tt8-45-jq2QygLkHWWlr2wGZNIp2w8qjyw=="
PAYLOAD_LIMIT = 100E+6
output_index = 0

LOG_LEVEL = {
    "[INFO]": "info",
    "[WARNING]": "warning",
    "[ERROR]": "error",
    "[DEBUG]": "debug"
}

def uploadData(payload):
    url = "http://"+HOST+PATH
    query_params = {
        "bucket": "Brunnen",
        "org": "Private",
        "precision": "ns"
    }
    headers = {
        "Accept": "application/json",
        "Authorization": "Token "+INFLUX_API_TOKEN,
    }
    proxies = {
        "http": "http://localhost:8086",
    }
    res = requests.post(url, params=query_params, headers=headers, data=payload, verify=False, proxies=proxies)
    if res.ok:
        print("Uploaded data successfully!")
    else:
        print(res.json())

def writeData(payload):
    global output_index
    output_path = f"export{output_index}.line"
    output_file = open(output_path, 'w')
    output_file.write(payload)
    output_file.close()
    output_index += 1
    print("Data written to:",output_path)


# Initalize Argument Parser:
parser = argparse.ArgumentParser(prog="upload-data.py", description="This script reads input files (CSV format) and uploads them to the influx database.")
parser.add_argument("input_file", help="Relative path to the log file")
parser.add_argument("-u", "--upload", action="store_true", help="Upload the data in batches, each 100MB in size")
parser.add_argument("-e", "--export", action="store_true", help="Export the data to file(s), each 100MB in size")
args = parser.parse_args()

# Read Log File:
file = open(args.input_file, mode="r")
logs = file.readlines()


# Iterate All Rows in CSV File:
data = ""
for log in logs:
    try: # extract timestamp, log level, and message using regular expressions
        match = re.match(r'(\d{2}-\d{2}-\d{4} \d{2}:\d{2}:\d{2}) (\[.*\]|##) (.*)', log)
        if match:
            timestamp, log_level, message = match.groups()
            time = datetime.strptime(timestamp, "%d-%m-%Y %H:%M:%S")
            if log_level == "##":
                log_level = "[DEBUG]"
    except Exception as e: # skip bad logs
        print("skipping log:",e)
    else:
        unix_timestamp = datetime.timestamp(time) * 1E9 # convert to nanoseconds
        log_level = LOG_LEVEL[log_level]
        line = 'logs,level=%s message="%s" %u\n' % (log_level,message,unix_timestamp)
        if len(data) + len(line) > PAYLOAD_LIMIT: # upload if accumulated data would be larger then PAYLOAD_LIMIT
            if args.upload:
                uploadData(data)
            if args.export:
                writeData(data)
            data = line
        else:
            data = data + line

# Upload Rest of Data:
if len(data) > 0:
    if args.upload:
        uploadData(data)
    if args.export:
        writeData(data)
