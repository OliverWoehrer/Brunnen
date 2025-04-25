#include "WiFiManager.h"

/**
 * @brief Default constructor initalizes the credentials
 */
WifiManager::WifiManager() {
    this->credentials = { .ssid = WIFI_SSID, .password = WIFI_PASSWORD };
}

/**
 * @brief Initalizes the Wlan module by setting the mode to STA (=station mode: the ESP32
 * connects to an access point). Connects afterwards to the WiFi network. And to prevent any
 * unexpected failures the device is disconnted (in case it was before).
 * @return true on success, false otherwise
 */
bool WifiManager::init() {
    return WiFi.mode(WIFI_STA);
}

/**
 * @brief Tries to build a connection to the credentials of 'this'.
 * @return true on success, false otherwise
 */
bool WifiManager::connect() {
    if(WiFi.isConnected()) {
        log_i("Already connected at %s", WiFi.localIP().toString());
        return true;
    }
    unsigned char retries = 2; // number of tries to login
    while(retries > 0) {
        if(login(this->credentials.ssid.c_str(), this->credentials.password.c_str())) {
            log_i("Connected successfully");
            return true;
        }
        retries--;
    }
    log_e("Failed to connect WiFi after multple retries");
    return false;    
}

/**
 * @brief Disconnects the system from the WiFi
 * @return true on success, false otherwise
 */
bool WifiManager::disconnect() {
    return WiFi.disconnect();
}

/**
 * @brief Returns the connection status of the system
 * @return true if connected, false otherwise
 */
bool WifiManager::isConnected() {
    return WiFi.isConnected();
}

/**
 * @brief Does the actual linking to the WiFi by the provided credentials. If a failure occures
 * during login the process is repeated/retried 3 times.
 * @param ssid wifi name
 * @param pw password
 * @return true on success, false otherwise
 */
bool WifiManager::login(const char* ssid, const char* pw) {
    // Make Connection Attempt:
    WiFi.begin(ssid, pw);
    unsigned long now = millis();
    while(WiFi.status() != WL_CONNECTED) {
        delay(500);
        if (millis() > now+5000) { break; } // wait for max. 5 sec for WiFi to connect
    }

    // Check Connection Status:
    if(WiFi.status() != WL_CONNECTED) {
        log_e("Failed to connect");
        return false;
    }
    
    // Return Success:
    log_i("Wifi connected at %s", WiFi.localIP().toString());
    return true;
}

WifiManager Wlan = WifiManager();
