#ifndef GW_H
#define GW_H

#include <ESP_Mail_Client.h>

//Global return value:
#define SUCCESS 0
#define FAILURE 1

//Mail Client:
#define EMAIL_SENDER_ACCOUNT    "iot.baumgasse@gmail.com"
#define EMAIL_SENDER_PASSWORD   "quxlbvyrsratckof" //"psgqopguszwhzqai" //"1ot_baumgasse"
#define SMTP_SERVER            "smtp.gmail.com"
#define SMTP_SERVER_PORT        587 //465 //587
#define EMAIL_RECIPIENT        "oliver.w@live.at"
#define EMAIL_SUBJECT          "ESP32 Mail with Attachment"
#define MAIL_TEXT_LENGTH 1024

//NTP Server:
#define NTP_SERVER "pool.ntp.org"
#define GMT_TIME_ZONE 3600
#define DAYLIGHT_OFFSET 3600

namespace Gateway {

namespace EMail {}

namespace ZamgAPI {}

int init();
int addInfoText(const char* text);
int addData(const char* fileName);
int sendData();

}

#endif /* GW_H */