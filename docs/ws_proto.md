WebSocket "wifiswitch" protocol.
================================

Everything is passed as a JSON object of the form:
	
	{type: *data_type*, *data*}
	
type: "fw".
-----------

|  Name | Description | Values | Access |
| :---: | :---------- | :----: | :----: |
| mode | Network mode | "station" or "ap" | R/W |
| ver | Firmware version | "x.y.z" |  RO |

type: "networks".
----------------

|  Name | Description | Values | Access |
| :---: | :---------- | :----: | :----: |
| ssids | Accessible networks | Array of ssids | R |

type: "station".
----------------

|  Name | Description | Values | Access |
| :---: | :---------- | :----: | :----: |
| ssid | Network name | "name" | R/W |
| passwd | Network password | "password" | W |
| hostname | Switch hostname | hostname | R/W |
| ip | IP address | ip | RO |

type: "ap".
-----------

|  Name | Description | Values | Access |
| :---: | :---------- | :----: | :----: |
| ssid | Network name | "name" | R/W |
| passwd | Network password | "password" | W |
| channel | Network channel | channel | R/W |
| hostname | Switch hostname | hostname | R/W |
| ip | IP address | ip | RO |

type: "gpio".
------------

|  Name | Description | Values | Access |
| :---: | :---------- | :----: | :----: |
| gpios | Get/set GPIO states. | Array of gpio states | R/W* |

* Writes will be silently dropped, if the GPIO support only read operations.

