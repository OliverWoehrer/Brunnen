# Database: InfluxDB

## Buckets
The database consits of multiple buckets. Each bucket is meant for a specific entity.

### Credentials
This bucket holds the user credentials of the application and access tokens for the edge devices (API). It allows user management with different privileges.

#### Users (`users`)
| Category | Name | Type | Example |
| :--- | :--- | :--- | :--- |
| Field | `password` | `String(hashed)` | `"d34jsd3fdj93df4kdf3"` |
| Field | `groups` | `String` | `"user,admin,maintainer"` |
| Tag | `username` | `String(no white spaces!)` | `"john_doe"` |

#### Devices (`devices`)
| Category | Name | Type | Example |
| :--- | :--- | :--- | :--- |
| Field | `token` | `String(hashed)` | `"A67BBLP9"` |
| Tag | `id` | `String(no white spaces!)` | `"brunnen"` |


### Logs
This bucket holds log messages produced by the application itself. Log messages of edge devices are stored in their designated buckets respectively.


### Brunnen
This bucket holds all data of the pump device. The following measurements are recorded...

#### Data (`data`)
| Category | Name | Type |
| :--- | :--- | :--- |
| Field | `flow` | `Float64` |
| Field | `pressure` | `Float64` |
| Field | `level` | `Float64` |

#### Logs (`logs`)
| Category | Name | Type |
| :--- | :--- | :--- |
| Field | `message` | `String` |
| Tag | `level` | `info`,`warning`,`error`,`debug` |

#### Settings (`settings`)
| Category | Name | Type | Usage |
| :--- | :--- | :--- | :--- | 
| Field | `exchange_periods` | `String(JSON)` | Read |
| Field | `intervals` | `String(JSON)` | Read |
| Field | `pump` | `String(JSON)` | Read & Write |
| Field | `rain_threshold` | `String(JSON)` | Read |
| Field | `software` | `String(JSON)` | Read |

~~~
[Exchange Periods]
{
    "long": Integer(<seconds>),
    "medium": Integer(<seconds>),
    "short": Integer(<seconds>),
    "mode": String(<"long"|"medium"|"short">)
}

[Intervals]
{
	"intervals": [
        { "start":String(<HH:MM:SS>), "stop":String(<HH:MM:SS>), "wdays":Integer(<mask>) },
    	{ "start":String(<HH:MM:SS>), "stop":String(<HH:MM:SS>), "wdays":Integer(<mask>) },
        ...
	]
}

[Pump]
{
	"state": Boolean
}

[Rain Threshold]
{
	"rain_threshold": Integer(millimeter)
}

[Software]
{
    "version": Integer(<number>)
}
~~~
