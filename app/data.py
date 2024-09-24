"""
This module implements functions to insert and query data from an (influx) database. It uses the
API interface provided via the python library.
"""
import influxdb_client
from influxdb_client import InfluxDBClient
from influxdb_client.client.exceptions import InfluxDBError
from influxdb_client.client.write_api import SYNCHRONOUS
from datetime import datetime, timedelta, timezone
import json
import pandas as pd

MEASUREMENT_BUCKET = "Brunnen"
DATA = "water"
LOGS = "logs"
SETTINGS = "settings"
CREDENTIALS_BUCKET = "Credentials"
USERS = "users"
DEVICES = "devices"

class InfluxDataClient():
    def __init__(self):
        pass

    def setup(self, url: str, token: str, organization: str):
        """
        This function initializes the data client and checks the connection to the database server.

        :param url: url of InfluxDB server with API
        :param token: influx access token (keep secret)
        :param organization: organization of the database
        """
        # Initalitze Influx Client:
        self._client = influxdb_client.InfluxDBClient(url=url, token=token, org=organization)
        if not self._client.ping():
            raise RuntimeError("Failed to initialize database client. Could not reach database server.")
        
        # Initalize API Objects:
        self._buckets_api = self._client.buckets_api()
        self._write_api = self._client.write_api(write_options=SYNCHRONOUS)
        self._query_api = self._client.query_api()
        self._delete_api = self._client.delete_api()
        
        # Check For Required Database Buckets:
        bucket = self._buckets_api.find_bucket_by_name(MEASUREMENT_BUCKET)
        if not bucket:
            self._buckets_api.create_bucket(bucket_name=MEASUREMENT_BUCKET, org=organization)
        bucket = self._buckets_api.find_bucket_by_name(CREDENTIALS_BUCKET)
        if not bucket:
            self._buckets_api.create_bucket(bucket_name=CREDENTIALS_BUCKET, org=organization)

    def insertData(self, data: pd.DataFrame) -> str:
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
            self._write_api.write(bucket=MEASUREMENT_BUCKET, record=rec, data_frame_measurement_name=DATA)
        except InfluxDBError as e:
            return e.message
        else:
            return None

    def queryData(self, start_time: datetime, stop_time: datetime, window_size: timedelta = None) -> (str,dict):
        """
        This function querys the measurement data between the start and stop time. To reduce data
        size, it aggregates multiple values inside a windows of given size. This means it takes the
        average over a periode (.i.e. duration) of window_size.

        :param start_time: earliest time of measurement to include
        :param stop_time: latest time of measurement to include
        :param window_size: duration on how much time to average over
        
        :return: Tuple(error_message: str, df: pd.DataFrame)
            error_message: "success" on success, errror message from database otherwise
            df: dataframe with the queried data on success, None otherwise
        """
        # Read From Database:
        start = start_time.strftime("%Y-%m-%dT%H:%M:%SZ")
        stop = stop_time.strftime("%Y-%m-%dT%H:%M:%SZ")
        if window_size is None:
            aggregate_string = ""
        else:
            aggregate_string = "|> aggregateWindow(every: {days}d{seconds}s, fn: mean, createEmpty: false)".format(days=window_size.days, seconds=window_size.seconds)
        query = """
            from(bucket: "{bucket}")
            |> range(start: {start}, stop: {stop})
            |> filter(fn: (r) => r._measurement == "{meas}")
            {agg}
            |> map(fn: (r) => ({{r with _value: int(v: r._value)}}) )
            |> pivot(rowKey: ["_time"], columnKey: ["_field"], valueColumn: "_value")
        """.format(bucket=MEASUREMENT_BUCKET, start=start, stop=stop, meas=DATA, agg=aggregate_string)
        try:
            df = self._query_api.query_data_frame(query)
        except InfluxDBError as e:
            return (e.message, None)
        if df.empty:
            return ("No data found", None)

        # Check Required Fields:
        REQUIERED_FIELDS = ["_time","flow","level","pressure"]
        for key in REQUIERED_FIELDS:
            if not key in df.columns:
                return (("Missing requiered field in database entry. Fields "+str(REQUIERED_FIELDS)+" are requiered."), None)
        
        # Sort and Rename Labels:
        df = df.sort_values(by=["_time"])
        df = df[REQUIERED_FIELDS]
        df.columns = ["Time", "Flow", "Level", "Pressure"] # rename columns
        json_string = df.to_json(orient="columns")
        json_data = json.loads(json_string)
        return ("success", json_data)

    def queryLatestData(self) -> (str,datetime):
        """
        This function querys the latest data available in water meaurements and returns its
        timestamp.
        
        :return: Tuple(error_message: str, timestamp: datetime)
            error_message: "success" on success, errror message from database otherwise
            timestamp: holds the time of latest data on success, None otherwise
        """

        query = """
            from(bucket: "{bucket}")
            |> range(start: 1970-01-01T00:00:00Z)
            |> filter(fn: (r) => r._measurement == "{meas}")
            |> last()
            |> map(fn: (r) => ({{r with _value: int(v: r._value)}}) )
        """.format(bucket=MEASUREMENT_BUCKET, meas=DATA)
        try:
            tables = self._query_api.query(query)
        except InfluxDBError as e:
            return (e.message, None)
        else:
            if not tables:
                return ("Did not find measurements", pd.DataFrame())
            values = tables.to_values(columns=["_time", "_field", "_value"])
            df = pd.DataFrame(values, columns=["Timestamp", "Type", "Value"])
            df = df.pivot_table(index="Timestamp", columns="Type", values="Value")
            df.index = df.index.to_pydatetime() # convert to datetime format
            return ("success", df.index[0].to_pydatetime())

    def deleteData(self, start_time: datetime, stop_time: datetime) -> str:
        """
        This function deletes all the measurement data between the start and stop time.

        :param start_time: earliest time of measurement to delete
        :param stop_time: latest time of measurement to delete
        
        :return: error message on failure, None on success
        """
        start = start_time.strftime("%Y-%m-%dT%H:%M:%SZ") # format: YYYY-MM-DDTHH:MM:SSZ
        stop = stop_time.strftime("%Y-%m-%dT%H:%M:%SZ")
        predicate = '_measurement="%s"' % DATA
        try:
            self._delete_api.delete(start, stop, predicate, bucket=MEASUREMENT_BUCKET)
        except InfluxDBError as e:
            return e.message
        else:
            return None

    def insertLogs(self, logs: pd.DataFrame) -> str:
        """
        This function takes the given logs and inserts them into the logs. The index column
        of the dataframe needs to be timestamps (datetime).
        Example dataframe:
                                      message  level
        2024-09-05 00:05:23   first debug log  debug
        2024-09-05 00:05:24  second debug log  debug
        2024-09-05 00:05:25    first info log   info
        2024-09-05 00:05:26   second info log   info

        :param logs: dataframe holding the logs to insert (columns: "message", "level")

        :return: Error message on failure, None on success
        """
        if not isinstance(logs.index, pd.DatetimeIndex):
            return "Dataframe index column not a datetime type"
        if "message" not in logs:
            return "Dataframe is missing 'message' field"
        if "level" not in logs:
            return "Dataframe is missing 'level' tag"
        try:
            rec = logs[["message", "level"]] # only use defined field and tag columns
            self._write_api.write(bucket=MEASUREMENT_BUCKET, record=rec, data_frame_measurement_name=LOGS, data_frame_tag_columns=["level"])
        except InfluxDBError as e:
            return e.message
        else:
            return None

    def queryLogs(self, start_time: datetime, stop_time: datetime) -> (str,dict):
        """
        This function querys the logs between the start and stop time. This can be slow, when
        quering long time periods.

        :param start_time: earliest time of logs to include
        :param stop_time: latest time of logs to include
        
        :return: Tuple(error_message: str, logs: dict)
            error_message: "success" on success, errror message from database otherwise
            logs: json with the queried data on success, None otherwise
        """
        # Read From Database:
        start = start_time.strftime("%Y-%m-%dT%H:%M:%SZ")
        stop = stop_time.strftime("%Y-%m-%dT%H:%M:%SZ")
        query = """
            from(bucket: "{bucket}")
            |> range(start: {start}, stop: {stop})
            |> filter(fn: (r) => r._measurement == "{meas}")
            |> pivot(rowKey: ["_time"], columnKey: ["_field"], valueColumn: "_value")
        """.format(bucket=MEASUREMENT_BUCKET, start=start, stop=stop, meas=LOGS)
        try:
            df = self._query_api.query_data_frame(query)
        except InfluxDBError as e:
            return (e.message, None)
        if df.empty:
            return ("Did not find logs", {})

        # Check Required Fields:
        REQUIERED_FIELDS = ["_time","level","message"]
        for key in REQUIERED_FIELDS:
            if not key in df.columns:
                return (("Missing requiered field in database entry. Fields "+str(REQUIERED_FIELDS)+" are requiered."), None)
        
        # Sort and Rename Labels:
        df = df.sort_values(by=["_time"])
        df = df[REQUIERED_FIELDS]
        df.columns = ["Time", "Level", "Message"] # rename columns
        json_string = df.to_json(orient="split")
        json_data = json.loads(json_string)
        return ("success", json_data)

    def deleteLogs(self, start_time: datetime, stop_time: datetime) -> str:
        """
        This function deletes all the logs between the start and stop time.

        :param start_time: earliest time of logs to delete
        :param stop_time: latest time of logs to delete
        
        :return: None on success, error message on failure
        """
        start = start_time.strftime("%Y-%m-%dT%H:%M:%SZ") # format: YYYY-MM-DDTHH:MM:SSZ
        stop = stop_time.strftime("%Y-%m-%dT%H:%M:%SZ")
        predicate = '_measurement="%s"' % LOGS
        try:
            self._delete_api.delete(start, stop, predicate, bucket=MEASUREMENT_BUCKET)
        except InfluxDBError as e:
            return e.message
        else:
            return None

    def insertSettings(self, settings: dict) -> str:
        """
        This function takes the given settings and inserts them into the database. The timestamps
        of the values will be the current time.
        Example json:
        {
            "pump": {
                "state": false
            },
            "software": {
                "version": 4
            }
        }

        :param settings: json holding the settings

        :return: Error message on failure, None on success
        """
        data = {}
        cols = []
        SUPPORTED_SETTINGS = ["sync","intervals","pump","thresholds","software"]
        for key in settings:
            if key in SUPPORTED_SETTINGS: # check if each setting is known
                data[key] = json.dumps(settings[key])
                cols.append(key)
        timestamp = datetime.now(timezone.utc).replace(microsecond=0)
        df = pd.DataFrame(data, index=[timestamp], columns=cols)

        try:
            self._write_api.write(bucket=MEASUREMENT_BUCKET, record=df, data_frame_measurement_name=SETTINGS)
        except InfluxDBError as e:
            return e.message
        else:
            return None

    def querySettings(self, start_time: datetime = None) -> (str,dict):
        """
        This function querys the settings updated since start_time. Updates with the same time as
        start_time are not included. If no settings were updated since start_time an empty json
        (json = {}) is returned. If no start time is given, all settings (not only updated ones)
        are returned. Settings (all or only updated) get returned in their latest state, without a
        timestamp. 

        :param start_time: updated since this point in time or None to return all settings 
        
        :return: Tuple(error_message: str, settings: json)
            error_message: "success" on success, errror message from database otherwise
            settings: json with the queried settings on success, None otherwise
        """
        # Read From Database:
        if start_time is None:
            filter_string = "|> last()"
            start_time = datetime(1970, 1, 1, 0, 0, 0, 0, timezone.utc)
        else:
            filter_string = ""
        query = """
            from(bucket: "{bucket}")
            |> range(start: {start})
            |> filter(fn: (r) => r._measurement == "{meas}")
            {filter}
            |> pivot(rowKey: ["_time"], columnKey: ["_field"], valueColumn: "_value")
        """.format(bucket=MEASUREMENT_BUCKET, start=start_time.strftime("%Y-%m-%dT%H:%M:%SZ"), meas=SETTINGS, filter=filter_string)
        try:
            df = self._query_api.query_data_frame(query)
        except InfluxDBError as e:
            return (e.message, None)    
        if df.empty:
            return ("Did not find settings", {})
        
        # Check Available Fields:
        SUPPORTED_FIELDS = ["sync","intervals","pump","software","thresholds"]
        available_fields = []
        for key in SUPPORTED_FIELDS:
            if key in df.columns:
                available_fields.append(key)
        
        # Filter Dataframe:
        df.set_index("_time")
        df = df[available_fields] # only use available supported fields
        df = df.apply(lambda x: x.loc[x.last_valid_index()]) # only get last valid value of each column
        df = df.apply(lambda x: json.loads(x)) # parse json string to json objects
        json_string = df.to_json(orient="columns")
        json_data = json.loads(json_string)
        return ("success", json_data)

    def deleteSettings(self, start_time: datetime, stop_time: datetime) -> str:
        """
        This function deletes all the settings between the start and stop time.

        :param start_time: earliest time of settings to delete
        :param stop_time: latest time of settings to delete
        
        :return: None on success, error message on failure
        """
        start = start_time.strftime("%Y-%m-%dT%H:%M:%SZ") # format: YYYY-MM-DDTHH:MM:SSZ
        stop = stop_time.strftime("%Y-%m-%dT%H:%M:%SZ")
        predicate = '_measurement="%s"' % SETTINGS
        try:
            self._delete_api.delete(start, stop, predicate, bucket=MEASUREMENT_BUCKET)
        except InfluxDBError as e:
            return e.message
        else:
            return None

    def insertUser(self, user: dict) -> str:
        """
        This function takes the given user and inserts it into the database. Make sure to the token
        is hashed(!). Do not store credentials in clear text. The timestamps of the database entry
        will be the current time.
        Example json:
        {
            "username": username,
            "group": group,
            "token": hashed_token
        }

        :param user: json holding the user data

        :return: Error message on failure, None on success
        """
        data = {}
        cols = []
        SUPPORTED_GROUPS = ["user","admin"]
        if user.get("group", "") not in SUPPORTED_GROUPS:
            return "Unknown user group"
        SUPPORTED_FIELDS = ["username","group","token"]
        for key in user:
            if key in SUPPORTED_FIELDS: # check if each field is known
                data[key] = user[key]
                cols.append(key)
        if len(cols) is not len(SUPPORTED_FIELDS):
            return "Missing fields in given user."
        timestamp = datetime.now(timezone.utc).replace(microsecond=0)
        df = pd.DataFrame(data, index=[timestamp], columns=cols)

        try:
            self._write_api.write(bucket=CREDENTIALS_BUCKET, record=df, data_frame_measurement_name=USERS, data_frame_tag_columns=["username"])
        except InfluxDBError as e:
            return e.message
        else:
            return None

    def queryUser(self, username: str) -> (str,dict):
        """
        This function querys the the given username in the database. If no user is found, an empty
        json (json = {}) is returned.

        :param username: username to look up
        
        :return: Tuple(error_message: str, user: dict)
            error_message: "success" on success, errror message from database otherwise
            user: json with the queried user on success, None otherwise
        """
        query = """
            from(bucket: "{bucket}")
            |> range(start: 0)
            |> filter(fn: (r) => r._measurement == "{users}")
            |> filter(fn: (r) => r.username == "{uname}")
            |> last()
        """.format(bucket=CREDENTIALS_BUCKET, users=USERS, uname=username)

        try:
            tables = self._query_api.query(query)
        except InfluxDBError as e:
            return (e.message, None)    
        if not tables:
            return ("No user found", {})
        
        values = tables.to_values(columns=["_time", "_field", "_value"]) # value[0] = time, value[1] = field, value[2] = value        
        if len(values) != 2:
            return (("Expected to find one user. Found"+len(values)+"instead."), None)
        user = { "username": username }
        for value in values:
            if value[1] == "group":
                user["group"] = value[2]
            if value[1] == "token":
                user["token"] = value[2]

        SUPPORTED_FIELDS = ["username","group","token"]
        for key in user: # check if all supported fields are in user
            if key not in SUPPORTED_FIELDS: # check if each user is known
                return (("Unsupported field in user entry. Only "+str(SUPPORTED_FIELDS)+" are allowed."), None)
        return ("success", user)

    def queryUsers(self) -> (str,list):
        """
        This function querys all users in the database. If no users are found, an empty list
        ([]) is returned.

        :return: Tuple(error_message: str, users: list)
            error_message: "success" on success, errror message from database otherwise
            users: list with the queried users on success, None otherwise
        """
        # Read From Database:
        query = """
            from(bucket: "{bucket}")
            |> range(start: 0)
            |> filter(fn: (r) => r._measurement == "{users}")
            |> pivot(rowKey: ["_time"], columnKey: ["_field"], valueColumn: "_value")
        """.format(bucket=CREDENTIALS_BUCKET, users=USERS)
        try:
            df = self._query_api.query_data_frame(query)
        except InfluxDBError as e:
            return (e.message, None)    
        if df.empty:
            return ("No users found", [])

        # Check Required Fields:
        REQUIERED_FIELDS = ["username","group","_time"]
        for key in REQUIERED_FIELDS:
            if not key in df.columns:
                return (("Missing requiered field in user entry. Fields "+str(REQUIERED_FIELDS)+" are requiered."), None)

        # Rename Colums With Labels:
        df = df.sort_values(by=["_time"])
        df = df[REQUIERED_FIELDS]
        df.columns = ["Username", "Group", "Updated On"] # rename columns
        json_string = df.to_json(orient="split")
        json_data = json.loads(json_string)
        return ("success", json_data)

    def deleteUser(self, username: str) -> str:
        """
        This function deletes the entry of the given user in the database.

        :param username: username to delete
        
        :return: None on success, error message on failure
        """
        start = datetime(1970, 1, 1, 0, 0, 0, 0, timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
        stop = datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
        predicate = 'username="%s"' % username
        try:
            self._delete_api.delete(start, stop, predicate, bucket=CREDENTIALS_BUCKET)
        except InfluxDBError as e:
            return e.message
        else:
            return None

    def insertDevice(self, device: dict) -> str:
        """
        This function takes the given device and inserts it into the database. The token is an
        eight byte long key (only letters and numbers). The timestamps of the database entry will
        be the current time.
        device = {
            "id": name,
            "token": token
        }

        :param device: json holding the device data

        :return: Error message on failure, None on success
        """
        data = {}
        cols = []
        SUPPORTED_FIELDS = ["id","token"]
        for key in user:
            if key in SUPPORTED_FIELDS: # check if each key is supported
                data[key] = user[key]
                cols.append(key)
        if len(cols) is not len(SUPPORTED_FIELDS):
            return "Missing fields in given device."
        timestamp = datetime.now(timezone.utc).replace(microsecond=0)
        df = pd.DataFrame(data, index=[timestamp], columns=cols)

        try:
            self._write_api.write(bucket=CREDENTIALS_BUCKET, record=df, data_frame_measurement_name=DEVICES, data_frame_tag_columns=["id"])
        except InfluxDBError as e:
            return e.message
        else:
            return None

    def queryDevice(self, device_id: str) -> dict:
        """
        This function querys the the given device id in the database. If no device is found, an empty
        json (json = {}) is returned.

        :param device_id: device name to look up 
        
        :return: Tuple(error_message: str, device: dict)
            error_message: "success" on success, errror message from database otherwise
            device: json with the queried device on success, None otherwise
        """
        query = """
            from(bucket: "{bucket}")
            |> range(start: 0)
            |> filter(fn: (r) => r._measurement == "{dev}")
            |> filter(fn: (r) => r.id == "{id}")
            |> last()
        """.format(bucket=CREDENTIALS_BUCKET, dev=DEVICES, id=device_id)

        try:
            tables = self._query_api.query(query)
        except InfluxDBError as e:
            return (e.message, None)    
        if not tables:
            return ("No device found", {})
        
        values = tables.to_values(columns=["_time", "_field", "_value"]) # value[0] = time, value[1] = field, value[2] = value        
        if len(values) != 1:
            return (("Expected to find one device. Found"+len(values)+"instead."), None)
        device = { "id": device_id }
        for value in values:
            if value[1] == "token":
                device["token"] = value[2]

        SUPPORTED_FIELDS = ["id","token"]
        for key in device:
            if key not in SUPPORTED_FIELDS: # check if each key is supported
                return (("Unsupported field in user entry. Only "+str(SUPPORTED_FIELDS)+" are allowed."), None)
        return ("success", device)

    def deleteDevice(self, device_id: str) -> str:
        """
        This function deletes the entry of the given device in the database.

        :param device_id: device name to delete
        
        :return: None on success, error message on failure
        """
        start = datetime(1970, 1, 1, 0, 0, 0, 0, timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
        stop = datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
        predicate = 'id="%s"' % device_id
        try:
            self._delete_api.delete(start, stop, predicate, bucket=CREDENTIALS_BUCKET)
        except InfluxDBError as e:
            return e.message
        else:
            return None

data_client = InfluxDataClient()