import unittest
import os
from datetime import datetime, timedelta
import pandas as pd
import config
from data import data_client as db

class TestDataClient(unittest.TestCase):
    @classmethod
    def setUpClass(self):
        token = os.environ.get("INFLUXDB_TOKEN")
        org = config.readInfluxOrganization()
        influx_host = config.readInfluxHost()
        influx_port = config.readInfluxPort()
        url = influx_host+":"+str(influx_port)
        db.init(url=url, token=token, organization=org)

        self.timestamps = [datetime.fromisoformat("2024-09-05T00:05:23"), datetime.fromisoformat("2024-09-05T00:05:24"), datetime.fromisoformat("2024-09-05T00:05:25"), datetime.fromisoformat("2024-09-05T00:05:26")]
        self.data = {
            "flow": [0,5,10,10],
            "pressure": [1379, 1379, 1200, 1200],
            "level": [1402, 1450, 1500, 1550]
        }
        self.df = pd.DataFrame(self.data, index=self.timestamps, columns=["flow", "pressure", "level"])

    def test_insertMeasurements(self):
        self.assertIsNone(db.insertMeasurements(self.df))
        self.assertIsNone(db.insertMeasurements(None))

    def test_queryLatestMeasurement(self):
        self.assertEqual(db.queryLatestMeasurement(), ("success",self.timestamps[-1]))
    
    def test_queryMeasurements(self):
        (msg,df) = db.queryMeasurements(start_time=self.timestamps[0], stop_time=self.timestamps[-1])
        self.assertEqual(msg,"success")

    def test_deleteMeasurements(self):
        db.insertMeasurements(self.df)
        self.assertIsNone(db.deleteMeasurements(start_time=self.timestamps[-2], stop_time=self.timestamps[-1]))
        self.assertEqual(db.queryLatestMeasurement(), ("success",self.timestamps[-3]))


if __name__ == "__main__":
    unittest.main()