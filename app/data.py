"""
This module implements functions to insert and query data from an (influx) database. It uses the
API interface provided via the python library.
"""
import influxdb_client
from influxdb_client import InfluxDBClient, Point, WritePrecision
from influxdb_client.client.write_api import SYNCHRONOUS
from datetime import datetime, timedelta
import pandas as pd

MEASUREMENT_BUCKET = "Brunnen"
MEASUREMENT = "water"

class DataClient():
    def __init__(self, url: str, token: str, organization: str):
        """
        This function initializes the data client and checks the connection to the database server.

        Parameters:
            url: url of InfluxDB server with API
            token: influx access token (keep secret)
            organization: organization of the database
        """

        self.client = influxdb_client.InfluxDBClient(url=url, token=token, org=organization)
        self.write_api = self.client.write_api(write_options=SYNCHRONOUS)
        self.query_api = self.client.query_api()
        if not self.client.ping():
            raise RuntimeError("Failed to initialize database client.")

    def insertMeasurements(self, data: pd.DataFrame) -> bool:
        """
        This function takes the given data and inserts it into the measurements.
        
        Returns:
            True if successful, false otherwise
        """
        return False

    def queryLatestMeasurement(self) -> datetime:
        """
        This function querys the latest data available in water meaurements and returns its
        timestamp.
        
        Returns:
            datetime: holds the time of latest data
        """

        query = """
            from(bucket: "{bucket}")
            |> range(start: 1970-01-01T00:00:00Z)
            |> filter(fn: (r) => r._measurement == "water")
            |> last()
            |> map(fn: (r) => ({{r with _value: int(v: r._value)}}) )
        """.format(bucket=MEASUREMENT_BUCKET)

        tables = self.query_api.query(query)
        values = tables.to_values(columns=["_time", "_field", "_value"])
        df = pd.DataFrame(values, columns=["Timestamp", "Type", "Value"])
        df = df.pivot_table(index="Timestamp", columns="Type", values="Value")
        return df.index[0]

    def queryMeasurements(self, start_time: datetime, stop_time: datetime, window_size: timedelta = timedelta(minutes=5)) -> pd.DataFrame:
        """
        This function querys the measurement data between the start and stop time. To reduce data
        size, it aggregates multiple values inside a windows of given size. This means it takes the
        average over a periode (.i.e. duration) of window_size.

        Parameters:
            start_time: earliest time of measurement to include
            stop_time: latest time of measurement to include
            window_size: duration on how much time to average over
        
        Returns:
            pandas.DataFrame: holds the time of latest data
        """
        start = int(datetime.timestamp(start_time))
        stop = int(datetime.timestamp(stop_time))
        formatted_size = "{days}d{seconds}s".format(days=window_size.days, seconds=window_size.seconds)
        
        query = """
            from(bucket: "{bucket}")
            |> range(start: {start}, stop: {stop})
            |> filter(fn: (r) => r._measurement == "{meas}")
            |> aggregateWindow(every: {duration}, fn: mean, createEmpty: false)
            |> map(fn: (r) => ({{r with _value: int(v: r._value)}}) )
        """.format(bucket=MEASUREMENT_BUCKET, start=start, stop=stop, meas=MEASUREMENT, duration=formatted_size)

        tables = self.query_api.query(query)
        values = tables.to_values(columns=["_time", "_field", "_value"])
        df = pd.DataFrame(values, columns=["Timestamp", "Type", "Value"])
        df = df.pivot_table(index="Timestamp", columns="Type", values="Value")
        return df
