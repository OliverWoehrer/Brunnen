# Brunnen IoT - Pumpensteuerung mit Internetanbindung
**Dislaimer:** This is a hobby project planed, built, programmed and tested by me. All design choices are based on the individual and unique use case of my groundwater well in my garden in Vienna. 
## Introduction
### Idea
The idea was to build a remote control unit and meassurement unit in my groundwater well in my garden for the automatic irrigation system installed.
### Concept
It was necessary to control the electrical water pump of the automatic irrigation system and monitor the water flow in order to ensure the proper functioning of the hardware. Therefore the water flow out of the well, the water pressure inside the system and the water level of the well are being constantly meassured. In addition to that the power supply of the electrical water pump is switched on/off according to the user-defined intervals. The water pump is running as soon as it is connected to 230 VAC. Due to the fact that only on weekends somebody is there, the hole device has internet access and sends an daily e-mail with the meassurement data of the that day. The heart of the project is the ESP32 micro controller which supports multiple I/Os as well as a WiFi module.
### Sketch
[PLACE SKETCH HERE]
![concept_sketch](doc/Concept_sketch.JPG?raw=true "Title")


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
### Flashing Software
The easier way is to use the popular [Arduino IDE](https://www.arduino.cc/en/software) with the easy-to-install ESP32 extension available on the Arduino eco system. Just add the additional board manager as shown in this [Tutorial](https://randomnerdtutorials.com/installing-the-esp32-board-in-arduino-ide-windows-instructions/). After a sucessful installation, download the code in this repository and simply connect to the controller via micro USB. Finally click the upload icon in the IDE. When the four colored LEDs on the PCB start to light up after each other the device booted properly.


## Hardware and Schematics

### Sensors
### Relais
### SD Card Shield
### Power Supply
The device is powered over 230 VAC and is plugged in. Because of multiple parts, controllers and sensors there are different voltage levels required across the device (230VAC, 24 VDC, 5 VDC and 3.3 VDC). These levels are achieved through two AC-DC converters from 230 VAC to 24 VDC and 5 VDC respectively. The ESP32 needs 3.3 VDC. The 24 VDC from the water pressure sensor are used for the relais coil as well since it is the highest DC voltage level available on the PCB. The onboard LDO of the ESP32 development board helps to convert 3.3 VDC from the onboard 5Vext wire. Because it is also possible to power the ESP32 development board over the USB cable connection (which is used when debugging and flashing the controller) there is the possibility that two potential power sources are connected at the same time to the ESP32: The 5 VDC comming from the AC-DC-converter over the 5Vext pin and the 5 VDC comming from the USB cable. The ESP32 development board has an onboard 5N5819 Shottky diode from USB to the 5Vext wire to protect the USB driver from high voltages. To also protect the 5 VDC converter as the other potential power source the same Shottky diode is used in simmilar fashion. The 5N5819 is chossen deliberate to reduce any differences between the two protecting diodes such as different temperature behaviours, etc. 
### Layout

## Software
### Main Code (code.ino)
### Hardware (hw)
### Data and Time (dt)
### User Interface (ui)

## Data Results
## Lessons Learned
