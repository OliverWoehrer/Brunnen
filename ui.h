#ifndef UI_H
#define UI_H

//===============================================================================================
// USER INTERFACE
//===============================================================================================
class WEBINTERFACE {
private:
    bool enabled;
public:
    WEBINTERFACE();
    int init();
    void handleClient();
    int toggle();
};

extern WEBINTERFACE Ui;


//===============================================================================================
// REQUEST HANDLER
//===============================================================================================
//void handle_Homepage();
//void handle_NotFound(AsyncWebServerRequest *request);

#endif /* UI_H */