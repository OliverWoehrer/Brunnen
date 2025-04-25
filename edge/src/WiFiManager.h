#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "WiFi.h"
#include <vector>

// Network Credentials:
#ifndef WIFI_SSID
#define WIFI_SSID "DEFAULT_WIFI_NAME"
#endif
#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD "DEFAULT_WIFI_PASSWORD"
#endif

typedef struct {
    std::string ssid;
    std::string password;
} credentials_t;

class WifiManager {
public:
    WifiManager();
    bool init();
    bool connect();
    bool disconnect();
    bool isConnected();
private:
    credentials_t credentials;
    bool login(const char* ssid, const char* pw);
};

extern WifiManager Wlan;

#endif /* WIFI_MANAGER_H */