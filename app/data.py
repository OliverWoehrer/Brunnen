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
MEASUREMENT = "test"

class DataClient():
    def __init__(self):
        pass

    def init(self, url: str, token: str, organization: str):
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
        of the dataframe needs to be timestamps (datetime)

        :param data: dataframe holding the data to insert

        :return: Error message on failure, None on success
        """
        try:
            self._write_api.write(bucket=MEASUREMENT_BUCKET, record=data, data_frame_measurement_name=MEASUREMENT)
        except InfluxDBError as e:
            return e.message
        else:
            return None

    def queryMeasurements(self, start_time: datetime, stop_time: datetime, window_size: timedelta = None) -> pd.DataFrame:
        """
        This function querys the measurement data between the start and stop time. To reduce data
        size, it aggregates multiple values inside a windows of given size. This means it takes the
        average over a periode (.i.e. duration) of window_size.

        :param start_time: earliest time of measurement to include
        :param stop_time: latest time of measurement to include
        :param window_size: duration on how much time to average over
        
        :return Tuple: (error_message: str, timestamp: datetime)
            error_message: "success" on success, errror message from database otherwise
            timestamp: holds the time of latest data on success, None otherwise
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
            values = tables.to_values(columns=["_time", "_field", "_value"])
            df = pd.DataFrame(values, columns=["Timestamp", "Type", "Value"])
            df = df.pivot_table(index="Timestamp", columns="Type", values="Value")
            return ("success", df)

    def queryLatestMeasurement(self) -> (str,datetime):
        """
        This function querys the latest data available in water meaurements and returns its
        timestamp.
        
        :return Tuple: (error_message: str, timestamp: datetime)
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
            values = tables.to_values(columns=["_time", "_field", "_value"])
            if values:
                df = pd.DataFrame(values, columns=["Timestamp", "Type", "Value"])
                df = df.pivot_table(index="Timestamp", columns="Type", values="Value")
                timestamp = df.index[0]
                timestamp = timestamp.to_pydatetime()
                timestamp = timestamp.replace(tzinfo = None)
                return ("success",timestamp)
            else:
                return ("Did not find measurements", None)

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

data_client = DataClient()