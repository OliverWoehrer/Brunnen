"""
This module implements functions to insert and query data from an (influx) database. It uses the
API interface provided via the python library.
"""
import influxdb_client
from influxdb_client import InfluxDBClient, Point, WritePrecision
from influxdb_client.client.write_api import SYNCHRONOUS

MEASUREMENT_BUCKET = "Brunnen"

class DataClient():
    def __init__(self, url: str, token: str, organization: str):
        """
        This function initializes the data client and checks the connection to the database server

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

    def queryNewestMeasurements(self):
        query = """
            from(bucket: "{bucket}")
            |> range(start: 2024-08-28T00:00:00Z, stop: 2024-08-31T23:59:00Z)
            |> aggregateWindow(every: 5m, fn: mean, createEmpty: false)
            |> filter(fn: (r) => r._measurement == "water" and r._value > 1000)
        """.format(bucket=MEASUREMENT_BUCKET)

        tables = self.query_api.query(query, org="Private")
        return tables