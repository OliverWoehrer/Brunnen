# Brunnen IoT - Pumpensteuerung mit Internetanbindung
**Dislaimer:** This is a hobby project planed, built, programmed and tested by me. All design choices are based on the individual and unique use case of my groundwater well in my garden in Vienna. 
## Introduction
### Idea
The idea was to build and implement a remote control and meassurement unit in my groundwater well in my garden for the automatic irrigation system installed.
### Concept
It was necessary to control the electrical water pump and monitor the water parameters in order to ensure the proper functioning of the hardware. Therefore the water flow out of the well, the water pressure inside the system and the water level of the well are being constantly meassured. In addition to that the power supply of the electrical water pump is switched on/off according to the user-defined intervals.  Due to the fact that there is only somebody there on the weekends the hole device has internet access and sends an daily e-mail with the meassurement data of the that day. The heart of the project is the ESP32 micro controller shich supports multiple I/Os as well as a WiFi module.
### Sketch
[PLACE SKETCH HERE]
![concept_sketch](doc/Concept_sketch.JPG?raw=true "Title")


## User Manual
### Getting Started
The easier way is to use the popular [Arduino IDE](https://www.arduino.cc/en/software) with the easy-to-install ESP32 extension available on the Arduino eco system. Just add the additional board manager as shown in this [Tutorial](https://randomnerdtutorials.com/installing-the-esp32-board-in-arduino-ide-windows-instructions/). After a sucessful installation, download the code in this repository and simply connect to the controller via micro USB. Finally click the upload icon in the IDE. When the four colored LEDs on the PCB start to light up after each other the device booted properly.
### Operating the Device
#### Physical Button
The device itself is located inside the well and has only one physical button. The button has two functions: short press and long press.
- Short Press: Tells the the device to connect to the wifi and toggles the web server. he push needs to be at least 100 ms long. This prevents a unintended button push when working inside the well.
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
### Power Supply
Shottky Diode
### Sensors
### Relais
### SD Card Shield
### Layout

## Software
### Main Code (code.ino)
### Hardware (hw)
### Data and Time (dt)
### User Interface (ui)

## Data Results
## Lessons Learned
