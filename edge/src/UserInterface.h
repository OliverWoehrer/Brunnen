#ifndef USER_INTERFACE_H
#define USER_INTERFACE_H

#include <ESPAsyncWebServer.h>
#include "Output.h"

#define LED_GREEN 16
#define UI_PORT 80

class UserInterfaceClass {
public:
    UserInterfaceClass();
    bool enable();
    bool disable();
    bool toggle();
private:
    Output::Digital led;
    AsyncWebServer server; // server object on port 80
    bool state;
};

extern UserInterfaceClass UserInterface;

#endif /* USER_INTERFACE_H */