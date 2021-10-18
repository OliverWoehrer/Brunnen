/**
   This modul resets and clears the ESP32 device including the Flash storage
*/

#include <SPIFFS.h>
#include <nvs_flash.h>
#include "Arduino.h" // include basic arduino functions

void setup() {
  Serial.begin(115200);
  if (!SPIFFS.begin(true)) {
    Serial.println("Failed to mount SPIFFS");
    return;
  }

  if (SPIFFS.format()) {
    Serial.println("SPIFFS formatted successfully");
  } else {
    Serial.println("Error formatting");
  }

  nvs_flash_erase(); // erase the NVS partition and...
  nvs_flash_init(); // initialize the NVS partition.


}

void loop() {

}
