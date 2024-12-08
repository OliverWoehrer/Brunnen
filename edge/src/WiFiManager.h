#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "WiFi.h"
#include <vector>

// Network Credentials:
#define WIFI_SSID_HOME "RadlerfreieWohnung_2.4G"
#define WIFI_PASSWORD_HOME "radlerraus"
#define WIFI_SSID_MOBILE "hotcon"
#define WIFI_PASSWORD_MOBILE "1gutespasswort"
#define WIFI_SSID_FIELD "TP-Link_BCDC"
#define WIFI_PASSWORD_FIELD "69001192"

typedef struct {
    std::string ssid;
    std::string password;
} credentials_t;

class WifiManager {
public:
    WifiManager();
    bool init();
    bool connect();
    bool isConnected();
private:
    credentials_t credentials;
    bool login(const char* ssid, const char* pw);
};

#endif /* WIFI_MANAGER_H */