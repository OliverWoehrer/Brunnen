import argparse
import os
import csv
from datetime import datetime
import requests

HOST = "localhost:8086"
PATH = "/api/v2/write"
INFLUX_API_TOKEN = "djWUq7Fj1_1nIDR1va4ORRMvQHinkYg9J4YQX6k9TVVSUGIIJYu3tt8-45-jq2QygLkHWWlr2wGZNIp2w8qjyw=="
PAYLOAD_LIMIT = 100E+6
output_index = 0

def uploadData(payload):
    url = "http://"+HOST+PATH
    query_params = {
        "bucket": "Brunnen",
        "org": "Private",
        "precision": "s"
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
parser.add_argument("input_directory", help="Relative path to directory containing the input files (CSV format)")
parser.add_argument("-u", "--upload", action="store_true", help="Upload the data in batches, each 100MB in size")
parser.add_argument("-e", "--export", action="store_true", help="Export the data to file(s), each 100MB in size")
args = parser.parse_args()

# Build Input File Paths:
file_paths = []
dir_list = os.listdir(args.input_directory)
for dir_item in dir_list:
    if dir_item.endswith(".txt"):
        file_paths.append(args.input_directory+dir_item)

# Iterate All Input Files:
data = ""
for file_path in file_paths:
    # Read CSV Data:
    file = open(file_path, mode="r")
    csv_reader = csv.DictReader(file, delimiter=',', lineterminator='\r\n')

    # Iterate All Rows in CSV File:
    for row in csv_reader:
        try: # parse csv cells
            time = datetime.strptime(row["Timestamp"], "%d-%m-%Y %H:%M:%S")
            flow = int(row["Flow"])
            pressure = int(row["Pressure"])
            level = int(row["Level"])
        except Exception as e: # skip bad rows
            print("skipping row:",file_path,">>>",e)
        else:
            unix_timestamp = datetime.timestamp(time)
            line = "water flow=%d,pressure=%d,level=%d %u\n" % (flow,pressure,level,unix_timestamp)
            if len(data) + len(line) > PAYLOAD_LIMIT+6: # upload if accumulated data would be larger then PAYLOAD_LIMIT
                if args.upload:
                    uploadData(data)
                if args.export:
                    writeData(args.export, data)
                data = line
            else:
                data = data + line

# Upload Rest of Data:
if len(data) > 0:
    if args.upload:
        uploadData(data)
    if args.export:
        writeData(args.export, data)
