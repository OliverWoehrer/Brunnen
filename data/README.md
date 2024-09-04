# Database: InfluxDB

## Structure
The database consits of multiple buckets. Each bucket is meant for a specific entity.

### Users
This bucket holds the user credentials of the application. It allows user management with different privileges.

### Logs
This bucket holds log messages produced by the application itself. Log messages of edge devices are stored in their designated buckets respectively.

### Brunnen
This bucket holds all data of the pump device. The following measurements are recorded... 
- water (fields: flow, pressure, level)(no tags)
- logs (fields: message)(tags: level)
- settings (fields: json-string)(tags: setting property)

## Credentials
There is a user (protected with a password) which has access to perform all tasks (admin rights).
