/**
 * @author Oliver Woehrer
 * @date 17.08.2022
 * @file gw.cpp
 * This modul [Gateway] provides functions to handle data transfers to and from the system such as
 * sending e-mails with data files attached or making HTTP request to known REST interfaces.
 */

#include <ESP_Mail_Client.h>
#include "Arduino.h"
#include "gw.h"

namespace Gateway {

//===============================================================================================
// EMail
//===============================================================================================
namespace EMail {
    SMTP_Message message;
    SMTPSession smtp;
    ESP_Mail_Session session;
    char mailText[MAIL_TEXT_LENGTH];
    int index = 0;

    void smtpCallback(SMTP_Status status);

    /**
     * @brief Initializes the E-Mail module by setting the mail-server and time zone variables. Furthermore
     * the callback function for the SMTP connnection is set as well as the debug level (relevant for the
     * detail of debug messages)
     * @return SUCCESS
     */
    int init() {
        session.server.host_name = SMTP_SERVER;
        session.server.port = SMTP_SERVER_PORT;
        session.login.email = EMAIL_SENDER_ACCOUNT;
        session.login.password = EMAIL_SENDER_PASSWORD;
        session.login.user_domain = ""; //F("gmail.com");

        session.time.ntp_server = NTP_SERVER;
        session.time.gmt_offset = GMT_TIME_ZONE;
        session.time.day_light_offset = DAYLIGHT_OFFSET;

        smtp.debug(0);
        smtp.callback(smtpCallback);

        return SUCCESS;
    }

    /**
     * @brief Appends the given text to the mail text. There is a total amount of 1KiB reserved for text.
     * Make sure to not overflow that buffer. To keep track of how much of the reserved memory is already
     * used the number of free memory space is returned
     * @param text message to append to the mail text
     * @return number of free memory in bytes
     */
    int addText(const char* text) {
        sprintf(&mailText[index],"%s",text);
        index += strlen(text);
        return MAIL_TEXT_LENGTH - index;
    }

    /**
     * @brief Creats an attachment which gets used when actually sending the mail. This is done by loading
     * the file from the given fileName path. It also appends the file name as a new line to the mail text.
     * There is a total amount of 1KiB reserved for text. Make sure to not overflow that buffer. To keep
     * track of how much of the reserved memory is already used the number of free memory space is returned
     * @param fileName path to the file to attach
     * @return number of free memory in bytes
     */
    int attachFile(const char* fileName) {
        //Attach File Data:
        /** TODO: actually attach file from SD card*/
        SMTP_Attachment att; // declare attachment data objects
        att.descr.filename = F(fileName);
        att.descr.mime = F("text/plain");
        att.file.path = F(fileName);
        att.file.storage_type = esp_mail_file_storage_type_sd;
        // att.descr.transfer_encoding = Content_Transfer_Encoding::enc_base64;
        message.addAttachment(att);

        //Add Text to Mail:
        sprintf(&mailText[index],"%s\r\n",fileName);
        index += strlen(fileName)+2;

        return MAIL_TEXT_LENGTH - index;
    }

    /**
     * @brief Sends the mail which was created by the other functions.
     * @return SUCCESS if the mail and all its attachments could be send, FAILURE otherwise
     */
    int send() {
        // [INFO] Disable Google Security for less secure apps: https://myaccount.google.com/lesssecureapps?pli=1

        //Attach Log File:
        SMTP_Attachment att; // declare attachment data objects
        att.descr.filename = F("/log.txt");
        att.descr.mime = F("text/plain");
        att.file.path = F("/log.txt");
        att.file.storage_type = esp_mail_file_storage_type_flash;
        // att.descr.transfer_encoding = Content_Transfer_Encoding::enc_base64;
        message.addAttachment(att);
        
        //Add Text to Mail:
        const char* greetingsText = "\r\nHave a nice day!\r\n";
        sprintf(&mailText[index],greetingsText);
        index += strlen(greetingsText);

        //Set E-Mail credentials:
        message.sender.name = "ESP32";
        message.sender.email = EMAIL_SENDER_ACCOUNT;
        message.subject = EMAIL_SUBJECT;
        message.addRecipient("Oliver Wohrer", EMAIL_RECIPIENT);
        // message.addCc("Peter Wohrer", );
        message.text.content = mailText;
        Serial.printf("EMail::mailText: %s\r\n",mailText);

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
            struct tm dt;
            for (size_t i = 0; i < smtp.sendingResult.size(); i++){
            SMTP_Result result = smtp.sendingResult.getItem(i);
            time_t ts = (time_t)result.timestamp;
            localtime_r(&ts, &dt);

            ESP_MAIL_PRINTF("Message No: %d\n", i + 1);
            ESP_MAIL_PRINTF("Status: %s\n", result.completed ? "success" : "failed");
            ESP_MAIL_PRINTF("Date/Time: %d/%d/%d %d:%d:%d\n", dt.tm_year + 1900, dt.tm_mon + 1, dt.tm_mday, dt.tm_hour, dt.tm_min, dt.tm_sec);
            ESP_MAIL_PRINTF("Recipient: %s\n", result.recipients);
            ESP_MAIL_PRINTF("Subject: %s\n", result.subject);
            }
        }
    }
}  

//===============================================================================================
// ZamgAPI
//===============================================================================================
namespace ZamgAPI {

}

//===============================================================================================
// Gateway
//===============================================================================================
/**
 * @brief Initializes the Gateway module by setting up the e-mail module.
 * @return SUCCESS
 */
int init() {
    if(EMail::init()) {
        Serial.printf("[ERROR] Failed to initialize EMail module!\r\n");
        return FAILURE;
    }
    return SUCCESS;
}

/**
 * @brief Appends the given text to the mail text. There is a total amount of 1KiB reserved for text.
 * Make sure to not overflow that buffer. To keep track of how much of the reserved memory is already
 * used the number of free memory space is returned
 * @param text message to append to the mail text
 * @return number of free memory in bytes
 */
int addInfoText(const char* text) {
    return EMail::addText(text);
}

/**
 * @brief Sends the mail which was created by the other functions.
 * @return number of free memory in bytes
 */
int addData(const char* fileName) {
    return EMail::attachFile(fileName);
}

/**
 * @brief Sends the mail which was created by the other functions.
 * @return SUCCESS if the mail and all its attachments could be send, FAILURE otherwise
 */
int sendData() {
    return EMail::send();
}

}