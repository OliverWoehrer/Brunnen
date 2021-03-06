# Brunnen IoT - Pumpensteuerung mit Internetanbindung
**Dislaimer:** This is a hobby project planed, built, programmed and tested by me. All design choices are based on the individual and unique use case of my groundwater well in my garden in Vienna. 
## Introduction
### Idea
The idea was to build a remote control unit and meassurement unit in my groundwater well in my garden for the automatic irrigation system installed.
### Concept
It was necessary to control the electrical water pump of the automatic irrigation system and monitor the water flow in order to ensure the proper functioning of the hardware. Therefore the water flow out of the well, the water pressure inside the system and the water level of the well are being constantly meassured. In addition to that the power supply of the electrical water pump is switched on/off according to the user-defined intervals. The water pump is running as soon as it is connected to 230 VAC.<br/>
Due to the fact that only on weekends somebody is there, the hole device has internet access and sends an daily e-mail with the meassurement data of the that day. The heart of the project is the ESP32 micro controller which supports multiple I/Os as well as a WiFi module.
### Sketch

<img src="https://user-images.githubusercontent.com/49563376/147362103-900f02ee-e877-4663-85be-f86254f6e69e.JPG" width="70%" height="70%">


## User Manual
### Operating the Device
#### Physical Button
The device itself is located inside the well and has only one physical button. The button has two functions: short press and long press.
- Short Press: Tells the the device to connect to the wifi and toggles the web server. The push needs to be at least 100 ms long. This prevents a unintended button push when working inside the well.
- Long Press: Toggles the relais and therefore the connected water pump. Has to be pushed three seconds or longer as a safty meassure to ensure that the relais only gets toggled on purpose. 
#### User Interface Web Server
This is the main user interface and is a simple HTTP web server consisting of two pages:
- Homepage (index.html): Displays the current time and sensor values meassured by the device. This allows for real time screening and refreshes automatically every second to poll the newest data.
- Interval (interval.html): This sub-page allows to check and set the intervals of the water pump. This means when to switch on/off the relais.
### LEDs and their meaning
The PCB hold four (colored) status LEDs and one small red power supply LED indicating the microcontroller has power.
- **Blue LED:** Is the cycle LED which is turned on during the meassurement cycle. This leads to a blinking behaviour once a second.
- **Red LED:** Indicates an error, the log file should be read for further information.
- **Yellow LED:** Warning purpose, lights up when the relais and therefore the connected water pump is active
- **Green LED:** Indicates that the user interface web server is up and running


## Hardware and Schematics
### Sensors
There are three main physical values that are messured: The water flow in the exit pipe of the well, the water pressure inside the pipe and the water pressure at the bottom of the well.
- **Water Level Sensor TL231:** The sensor is a two wire analog sensor with a voltage supply of 24 VDC and and current flow between 4...20 mA. The water level sensor is placed at the bottom of the well and uses the pressure of the surrounding water to conclude the current flow trough the wires. Because this sensor has a relatively high energy consumption of up to 0.5 W it was a requirement to make the power supply switchable. To read the current flow of the sensor into the microcontroller a 160 Ohm resistor is connected in series in the GND connected wire of the sensor. This makes for a voltage of 640...3200 mV.</br>
Because of this resistor the 24 VDC sensor 
- **Water Flow Sensor YF-B10:**
- **Water Pressure Sensor:**
out of the well to check weather the irrigation system actually uses water,  
### Relais
### SD Card Shield
### Power Supply
The device is powered over 230 VAC and is plugged in. Because of multiple parts, controllers and sensors there are different voltage levels required across the device (230VAC, 24 VDC, 5 VDC and 3.3 VDC). These levels are achieved through two AC-DC converters from 230 VAC to 24 VDC and 5 VDC respectively. The ESP32 needs 3.3 VDC. The 24 VDC from the water pressure sensor are used for the relais coil as well since it is the highest DC voltage level available on the PCB.</br>
The onboard LDO of the ESP32 development board helps to convert 3.3 VDC from the onboard 5Vext wire. Because it is also possible to power the ESP32 development board over the USB cable connection (which is used when debugging and flashing the controller) there is the possibility that two potential power sources are connected at the same time to the ESP32: The 5 VDC comming from the AC-DC-converter over the 5Vext pin and the 5 VDC comming from the USB cable. The ESP32 development board has an onboard 5N5819 Shottky diode from USB to the 5Vext wire to protect the USB driver from high voltages. To also protect the 5 VDC converter as the other potential power source the same Shottky diode is used in simmilar fashion. The 5N5819 is chossen deliberate to reduce any differences between the two protecting diodes such as different temperature behaviours, etc.
### Sketch
![Brunnen IoT v2_2](https://user-images.githubusercontent.com/49563376/147362033-647a96f1-f405-41fe-82bf-06309251729c.png)
### Layout
### Connections
To protect the device from colder temperatures during the winter, it is recommended to remove the device from the well during this time. In order to help with reconnecting the cables and sensors properly the cables are labled with color as followed.

|  Connection  |   Port [color]  |  Pin [color]  |
| ------------- | -------------| ------------- |
|  Water Flow Sensor (signal)  |  Port 1 [black]  |  1 [white]  |
|  Water Flow Sensor (+5V)  | Port 1 [black] | 2 [green] |
|  Water Flow Sensor (GND)  | Port 1 [black] | 3 [brown] |
|  Button  |  Port 2  |  1 [red]  |
|  Button  | Port 2 | 2 [green] |
|  Water Level Sensor (GND)  |  Port 3 [blue] | 1 [brown] |
|  Water Level Sensor (+24V)  | Port 3 [blue] | 2 [green] |
|  Water Pressure Sensor (signal)  | Port 4 [yellow] | 1 [green] |
|  Water Pressure Sensor (GND)  |  Port 4 [yellow] | 2 [brown] |
|  Water Pressure Sensor (+5V)  |  Port 4 [yellow] | 3 [white] |
|  Power Supply 230AC (L) |  Port 5  |  1 [brown]  |
|  Power Supply 230AC (N)  |  Port 5  |  2 [blue]  |
|  Switched 230VAC (N)  |  Port 5  |  1 [blue]  |
|  Switched 230VAC (L)  |  Port 5  |  2 [brown]  |

<img src="https://user-images.githubusercontent.com/49563376/163880120-ba4b2bb9-7013-4a93-bdce-eee33a221b99.jpg" width="70%" height="70%">


## Software
### Flashing Software
The easier way is to use the popular [Arduino IDE](https://www.arduino.cc/en/software) with the easy-to-install ESP32 extension available on the Arduino eco system. Just add the additional board manager as shown in this [Tutorial](https://randomnerdtutorials.com/installing-the-esp32-board-in-arduino-ide-windows-instructions/). After a sucessful installation, download the code in this repository and simply connect to the controller via micro USB. Finally click the upload icon in the IDE. When the four colored LEDs on the PCB start to light up after each other the device booted properly.

### Main Code (code.ino)
### Hardware (hw)
### Data and Time (dt)
### User Interface (ui)

## Data Results
## Lessons Learned
