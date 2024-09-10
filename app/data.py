"""
This module implements functions to insert and query data from an (influx) database. It uses the
API interface provided via the python library.
"""
import influxdb_client
from influxdb_client import InfluxDBClient
from influxdb_client.client.exceptions import InfluxDBError
from influxdb_client.client.write_api import SYNCHRONOUS
from datetime import datetime, timedelta
import pandas as pd

MEASUREMENT_BUCKET = "Brunnen"
MEASUREMENT = "water"
LOGS = "logs"
SETTINGS = "settings"

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

        self._client = influxdb_client.InfluxDBClient(url=url, token=token, org=organization)
        self._write_api = self._client.write_api(write_options=SYNCHRONOUS)
        self._query_api = self._client.query_api()
        self._delete_api = self._client.delete_api()
        if not self._client.ping():
            raise RuntimeError("Failed to initialize database client.")

    def insertMeasurements(self, data: pd.DataFrame) -> str:
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
            self._write_api.write(bucket=MEASUREMENT_BUCKET, record=rec, data_frame_measurement_name=MEASUREMENT)
        except InfluxDBError as e:
            return e.message
        else:
            return None

    def queryMeasurements(self, start_time: datetime, stop_time: datetime, window_size: timedelta = None) -> (str,pd.DataFrame):
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
        """.format(bucket=MEASUREMENT_BUCKET, start=start, stop=stop, meas=MEASUREMENT, agg=aggregate_string)

        try:
            tables = self._query_api.query(query)
        except InfluxDBError as e:
            return (e.message, None)
        else:
            if not tables:
                return ("Did not find measurements", pd.DataFrame())
            values = tables.to_values(columns=["_time", "_field", "_value"])
            df = pd.DataFrame(values, columns=["Timestamp", "Field", "Value"])
            df = df.pivot_table(index="Timestamp", columns="Field", values="Value")
            df.index = df.index.to_pydatetime() # convert to datetime format
            df.index = df.index.tz_convert(None) # remove time zone info
            return ("success", df)

    def queryLatestMeasurement(self) -> (str,datetime):
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
        """.format(bucket=MEASUREMENT_BUCKET, meas=MEASUREMENT)
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
            df.index = df.index.tz_convert(None) # remove time zone info
            return ("success", df.index[0].to_pydatetime())

    def deleteMeasurements(self, start_time: datetime, stop_time: datetime) -> str:
        """
        This function deletes all the measurement data between the start and stop time.

        :param start_time: earliest time of measurement to delete
        :param stop_time: latest time of measurement to delete
        
        :return: error message on failure, None on success
        """
        start = start_time.strftime("%Y-%m-%dT%H:%M:%SZ") # format: YYYY-MM-DDTHH:MM:SSZ
        stop = stop_time.strftime("%Y-%m-%dT%H:%M:%SZ")
        predicate = '_measurement="%s"' % MEASUREMENT
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

    def queryLogs(self, start_time: datetime, stop_time: datetime) -> (str,pd.DataFrame):
        """
        This function querys the logs between the start and stop time. This can be slow, when
        quering long time periods.

        :param start_time: earliest time of logs to include
        :param stop_time: latest time of logs to include
        
        :return: Tuple(error_message: str, df: pd.DataFrame)
            error_message: "success" on success, errror message from database otherwise
            df: dataframe with the queried data on success, None otherwise
        """
        start = start_time.strftime("%Y-%m-%dT%H:%M:%SZ")
        stop = stop_time.strftime("%Y-%m-%dT%H:%M:%SZ")
        query = """
            from(bucket: "{bucket}")
            |> range(start: {start}, stop: {stop})
            |> filter(fn: (r) => r._measurement == "{meas}")
        """.format(bucket=MEASUREMENT_BUCKET, start=start, stop=stop, meas=LOGS)

        try:
            tables = self._query_api.query(query)
        except InfluxDBError as e:
            return (e.message, None)
        else:
            if not tables:
                return ("Did not find logs", pd.DataFrame())
            values = tables.to_values(columns=["_time", "_value", "level"])
            df = pd.DataFrame(values, columns=["Timestamp", "message", "level"])
            df = df.set_index("Timestamp")
            df.index = df.index.to_pydatetime() # convert to datetime format
            df.index = df.index.tz_convert(None) # remove time zone info
            return ("success", df)

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

    def insertSettings(self, settings: pd.DataFrame) -> str:
        """
        This function takes the given settings and inserts them into the settings. The index column
        of the dataframe needs to be timestamps (datetime). Allowed columns are: intervals,
        rain_threshold, update_periods, version.
        Example dataframe:
                               rain_threshold        version    etc.
        2024-09-05 00:05:23  {"threshold":50}            NaN    ...
        2024-09-05 00:05:24  {"threshold":40}            NaN    ...
        2024-09-05 00:05:25  {"threshold":30}  {"version":1}    ...
        2024-09-05 00:05:26               NaN  {"version":2}    ...

        :param settings: dataframe holding the settings

        :return: Error message on failure, None on success
        """
        columns = []
        if not isinstance(settings.index, pd.DatetimeIndex):
            return "Dataframe index column not a datetime type"
        if "intervals" in settings:
            columns.append("intervals")
        if "rain_threshold" in settings:
            columns.append("rain_threshold")
        if "update_periods" in settings:
            columns.append("update_periods")
        if "version" in settings:
            columns.append("version")
        if not columns:
            return "Dataframe missing at least on setting as column, e.g. rain_threshold"
        try:
            rec = settings[columns] # only use defined and available field columns
            self._write_api.write(bucket=MEASUREMENT_BUCKET, record=rec, data_frame_measurement_name=SETTINGS)
        except InfluxDBError as e:
            return e.message
        else:
            return None

    def querySettings(self, stop_time: datetime = None) -> (str,pd.DataFrame):
        """
        This function querys the settings between the start and stop time. This can be slow, when
        quering long time periods.

        :param stop_time: time from which to return the latest settings at that point
        
        :return: Tuple(error_message: str, df: pd.DataFrame)
            error_message: "success" on success, errror message from database otherwise
            df: dataframe with the queried data on success, None otherwise
        """
        if stop_time is None:
            stop_string = ""
        else:
            stop = stop_time.strftime("%Y-%m-%dT%H:%M:%SZ")
            stop_string = ", stop {stop}".format(stop=stop)
        
        query = """
            from(bucket: "{bucket}")
            |> range(start: 0{stop_string})
            |> filter(fn: (r) => r._measurement == "{meas}")
            |> last()
        """.format(bucket=MEASUREMENT_BUCKET, stop_string=stop_string, meas=SETTINGS)

        try:
            tables = self._query_api.query(query)
        except InfluxDBError as e:
            return (e.message, None)
        else:
            if not tables:
                return ("Did not find settings", pd.DataFrame())
            values = tables.to_values(columns=["_time", "_field", "_value"])
            df = pd.DataFrame(values, columns=["Timestamp", "Field", "Value"])
            df = df.pivot_table(index="Timestamp", columns="Field", values="Value", aggfunc=lambda v: ','.join(v))
            df.index = df.index.to_pydatetime() # convert to datetime format
            df.index = df.index.tz_convert(None) # remove time zone info
            return ("success", df)

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



data_client = InfluxDataClient()