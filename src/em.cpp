/**
 * @author Oliver Woehrer
 * @date 17.08.2022
 * @file em.cpp
 * This modul [E-Mail] provides functions to handle sending and receiving e-mails.
 */

#include <ESP_Mail_Client.h>
#include "Arduino.h"
#include "em.h"

namespace EMail {
    SMTPSession smtp;
    ESP_Mail_Session session;

    void smtpCallback(SMTP_Status status);

    int init() {
        session.server.host_name = SMTP_SERVER;
        session.server.port = SMTP_SERVER_PORT;
        session.login.email = EMAIL_SENDER_ACCOUNT;
        session.login.password = EMAIL_SENDER_PASSWORD;
        session.login.user_domain = ""; //F("gmail.com");

        session.time.ntp_server = NTP_SERVER;
        session.time.gmt_offset = GMT_TIME_ZONE;
        session.time.day_light_offset = DAYLIGHT_OFFSET;

        smtp.debug(1);
        smtp.callback(smtpCallback);

        return SUCCESS;
    }

    int addText(const char* text) {}

    int attachFile(const char* fileName) {}

    int send(const char* mailText) {
        // [INFO] Disable Google Security for less secure apps: https://myaccount.google.com/lesssecureapps?pli=1

        //Set E-Mail credentials:
        SMTP_Message message;
        message.sender.name = "ESP32";
        message.sender.email = EMAIL_SENDER_ACCOUNT;
        message.subject = EMAIL_SUBJECT;
        message.addRecipient("Oliver Wohrer", EMAIL_RECIPIENT);
        // message.addCc("Peter Wohrer", );
        message.text.content = mailText;
        
        // //Declare Attachment Data Objects:
        // SMTP_Attachment att[2]; // [0]...data file, [1]...log file

        // //Attach Data File:
        // att[0].descr.filename = F(FileSystem.getFileName());
        // att[0].descr.mime = F("text/plain");
        // att[0].file.path = F(FileSystem.getFileName());
        // att[0].file.storage_type = esp_mail_file_storage_type_sd;
        // // att[0].descr.transfer_encoding = Content_Transfer_Encoding::enc_base64;
        // message.addAttachment(att[0]);

        //Attach Log File:
        /*att[1].descr.filename = F("log.txt");
        att[1].descr.mime = F("text/plain");
        att[1].file.path = F("/log.txt");
        att[1].file.storage_type = esp_mail_file_storage_type_flash;
        // att[1].descr.transfer_encoding = Content_Transfer_Encoding::enc_base64;
        message.addAttachment(att[0]);*/

        //Make Connection to SMTP Server:
        if (!smtp.connect(&session)) {
            Serial.printf("smtp.connect() failed\r\n");
            return FAILURE; // server connection failed
        }

        //Start Sending the Email and Close the Session:
        bool mailStatus = MailClient.sendMail(&smtp, &message, true);
        if (!mailStatus) {
            Serial.println("Error sending Email, " + smtp.errorReason());
            return FAILURE;
        }

        smtp.sendingResult.clear(); // clear data to free memory
        return SUCCESS;
    }

    void smtpCallback(SMTP_Status status) {
        Serial.println(status.info());

        if (status.success()){
            Serial.println("----------------");
            ESP_MAIL_PRINTF("Message sent success: %d\n", status.completedCount());
            ESP_MAIL_PRINTF("Message sent failled: %d\n", status.failedCount());
            Serial.println("----------------\n");
            struct tm dt;

            for (size_t i = 0; i < smtp.sendingResult.size(); i++){
            /* Get the result item */
            SMTP_Result result = smtp.sendingResult.getItem(i);
            time_t ts = (time_t)result.timestamp;
            localtime_r(&ts, &dt);

            ESP_MAIL_PRINTF("Message No: %d\n", i + 1);
            ESP_MAIL_PRINTF("Status: %s\n", result.completed ? "success" : "failed");
            ESP_MAIL_PRINTF("Date/Time: %d/%d/%d %d:%d:%d\n", dt.tm_year + 1900, dt.tm_mon + 1, dt.tm_mday, dt.tm_hour, dt.tm_min, dt.tm_sec);
            ESP_MAIL_PRINTF("Recipient: %s\n", result.recipients);
            ESP_MAIL_PRINTF("Subject: %s\n", result.subject);
            }
            Serial.println("----------------\n");
        }
    }
    
}