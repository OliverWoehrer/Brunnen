/**
 * @author Oliver Woehrer
 * @date 17.08.2022
 * @file gw.cpp
 * This modul [Gateway] provides functions to handle data transfers to and from the system such as
 * sending e-mails with data files attached or making HTTP request to known REST interfaces.
 */

#include <ESP_Mail_Client.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "Arduino.h"
#include "gw.h"

namespace Gateway {

//===============================================================================================
// EMail
//===============================================================================================
namespace EMail {
    ESP_Mail_Session session;
    SMTPSession smtp;
    SMTP_Message message;
    char mailText[MAIL_TEXT_LENGTH];
    char errorText[ERROR_TEXT_LENGTH];
    size_t index = 0;

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
     * @brief clears mail data by resetting mail and error text to NUL and cleaning the attachments
     * @return SUCCESS
     */
    int clearData() {
        for (; index > 0; index--) mailText[index-1] = '\0'; // clear mail text
        errorText[0] = '\0'; // clear error text
        message.clear();
        smtp.closeSession();
        smtp.sendingResult.clear(); // clear data to free memory
        return SUCCESS;
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

        //Make Connection to SMTP Server:
        if (!smtp.connect(&session)) {
            Serial.println("Failed to establish STMP session, " + smtp.errorReason());
            sprintf(errorText,"%s",smtp.errorReason().c_str());
            return FAILURE; // server connection failed
        }

        //Start Sending the Email and Close the Session:
        bool mailStatus = MailClient.sendMail(&smtp, &message, true);
        if (!mailStatus) {
            Serial.println("Error sending Email, " + smtp.errorReason());
            sprintf(errorText,"%s",smtp.errorReason().c_str());
            return FAILURE;
        }

        //Clean up Mail Data:
        clearData();
        return SUCCESS;
    }

    /**
     * @brief returns the error text. Containes error message in case of error. Needs to be cleaned
     * by calling the clearData() method in case of unsuccessful send of mail
     * @return error text
     */
    const char* getErrorText() {
        return errorText;
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
            ESP_MAIL_PRINTF("Recipient: %s\n", result.recipients.c_str() );
            ESP_MAIL_PRINTF("Subject: %s\n", result.subject.c_str());
            }
        }
    }
}  

//===============================================================================================
// OpenMeteoAPI
//===============================================================================================
namespace OpenMeteoAPI {
    char responseBuffer[RESPONSE_BUFFER_SIZE] = "";
    int precipitation = 0;

    int init() {
        return SUCCESS; // no initialization needed atm
    }

    int requestWeatherData(const char* startDate, const char* endDate) {
        char requestBuffer[256];
        sprintf(requestBuffer,"https://api.open-meteo.com/v1/forecast?latitude=48.11&longitude=16.39&timezone=auto&daily=precipitation_sum&start_date=%s&end_date=%s",startDate,endDate);

        // Initialize and Make GET Request:
        HTTPClient http;
        if (!http.begin(requestBuffer)) {
            sprintf(responseBuffer,"Failed to begin request!");
            http.end(); // clear http object
            return FAILURE;
        }
        int httpCode = http.GET(); // start connection and send HTTP header
        if(httpCode < 0) { // httpCode will be negative on error
            sprintf(responseBuffer,"GET request failed! %s", http.errorToString(httpCode).c_str());
            http.end(); // clear http object
            return FAILURE;
        }
        if (http.getSize() > RESPONSE_BUFFER_SIZE) { // check reponse body size
            sprintf(responseBuffer,"Response body too large.");
            http.end(); // clear http object
            return FAILURE;
        }

        //Read HTTP Response Body:
        sprintf(responseBuffer,"%s",http.getString().c_str());
        Serial.printf("requestWeatherData response: %s\r\n",responseBuffer);
        http.end(); // clear http object
        if (httpCode != HTTP_CODE_OK) { // other response code, unhandled
            return FAILURE;
        } // else: succsess response code, handle response

        // Parse JSON Data:
        char jsonString[RESPONSE_BUFFER_SIZE];
        memcpy(jsonString, responseBuffer, strlen(responseBuffer)+1);
        DynamicJsonDocument doc(2048);
        deserializeJson(doc, jsonString);
        double rawPrecipitation = doc["daily"]["precipitation_sum"][0];
        precipitation = (int) rawPrecipitation;

        return SUCCESS;
    }

    int getWeatherData(const char* data) {
        if (strcmp(data,"precipitation") == 0) {
            return precipitation;
        } else {
            return 0;
        }
    }

    /**
     * @brief returns the response message or error message in case of error. Needs to be cleaned
     * by calling the clearData() method in case of unsuccessful send of mail
     * @return error text
     */
    const char* getResponseMsg() {
        return responseBuffer;
    }

    /**
     * @brief clears http data by ending http object and setting error text to NUL
     * @return SUCCESS
     */
    int clearData() {
        responseBuffer[0] = '\0'; // clear error text
        return SUCCESS;
    }
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

/**
 * @brief returns the error text. Containes error message in case of error
 * @return error text
 */
const char* getErrorMsg() {
    return EMail::getErrorText();
}

/**
 * @brief Clears all the data set by other methods previously.
 * @return SUCCESS
 */
int clearData() {
    return EMail::clearData();
}

/**
 * @brief Request weather data for the fiven intervall/date periode over an api call
 * @param startDate start of the periode (including)
 * @param endDate end of the periode (including)
 * @return SUCCESS if the request was successful, FAILURE otherwise
 */
int requestWeatherData(const char* startDate, const char* endDate) {
    return OpenMeteoAPI::requestWeatherData(startDate,endDate);
}

/**
 * @brief Read the actual data previously requested idendified by the given handle
 * @param data handle string of the given data to extract
 * @return value of the data
 */
int getWeatherData(const char* data) {
    return OpenMeteoAPI::getWeatherData(data);
}

/**
 * @brief Read the response of the previously requested api call
 * @return response or info message
 */
const char* getWeatherResponse() {
    return OpenMeteoAPI::getResponseMsg();
}

}