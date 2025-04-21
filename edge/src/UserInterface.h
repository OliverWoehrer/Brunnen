#ifndef USER_INTERFACE_H
#define USER_INTERFACE_H

#include <ESPAsyncWebServer.h>
#include "Output.h"

#define LED_GREEN 16
#define UI_PORT 80

typedef struct {
    uint8_t *buffer;
    size_t len;
} upload_context_t;

class UserInterfaceClass {
public:
    UserInterfaceClass();
    bool enable();
    bool disable();
    bool toggle();
private:
    Output::Digital led;
    AsyncWebServer server; // server object on port 80
    bool state; // user interface enabled? (true=enabled, false=disabled)
};

extern UserInterfaceClass UserInterface;

#endif /* USER_INTERFACE_H */