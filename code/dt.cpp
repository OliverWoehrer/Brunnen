/**
 * @author Oliver Woehrer
 * @date 17.08.2021
 * @file dt.cpp
 * This modul [Data and Time] provides functions to handle the wifi connection. It initialies and
 * connections to the network given via the network credentials macros in dt.h. This allows the 
 * local time to be initialized via a NTP server over the web. Furthermore the file system of the
 * sd card attached over the sd card shield can be used. It contains basic read- and write-functions
 * as well as directory management. In addition to that the collected and stored data can be sent
 * via e-mail (function calls also provided in here). 
 */
#include <Preferences.h>
#include <WiFi.h>
#include <SPIFFS.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include <ESP32_MailClient.h>
#include "Arduino.h"
#include "dt.h"


//===============================================================================================
// PREFERENCES
//===============================================================================================
PREFS::PREFS() {
    Preferences preferences;
}

int PREFS::init() {
    preferences.begin("brunnen", false);
}

void PREFS::setStartTime(tm start, unsigned int i) {
    String start_hr = "start_hour_"+String(i); // concatinate Strings for key-names
    String start_min = "start_min_"+String(i);
    preferences.putUInt(start_hr.c_str(), start.tm_hour);
    preferences.putUInt(start_min.c_str(), start.tm_min);
}

void PREFS::setStopTime(tm stop, unsigned int i) {
    String stop_hr = "stop_hour_"+String(i); // concatinate Strings for key-names
    String stop_min = "stop_min_"+String(i);
    preferences.putUInt(stop_hr.c_str(), stop.tm_hour);
    preferences.putUInt(stop_min.c_str(), stop.tm_min);
}

void PREFS::setWeekDay(unsigned char wday, unsigned int i) {
    String week_day = "wday_"+String(i);
    preferences.putUChar(week_day.c_str(), wday);
}

tm PREFS::getStartTime(unsigned int i) {
    String start_hr = "start_hour_"+String(i); // concatinate Strings for key-names
    String start_min = "start_min_"+String(i);

    struct tm start;
    start.tm_hour = preferences.getUInt(start_hr.c_str()),
    start.tm_min = preferences.getUInt(start_min.c_str()),
    start.tm_sec = 0;

    return start;
}

tm PREFS::getStopTime(unsigned int i) {
    String stop_hr = "stop_hour_"+String(i);
    String stop_min = "stop_min_"+String(i);

    struct tm stop;
    stop.tm_hour = preferences.getUInt(stop_hr.c_str()),
    stop.tm_min = preferences.getUInt(stop_min.c_str()),
    stop.tm_sec = 0;

    return stop;
}

unsigned char PREFS::getWeekDay(unsigned int i) {
    String week_day = "wday_"+String(i);
    return preferences.getUChar(week_day.c_str());
}

PREFS Prefs;


//===============================================================================================
// WLAN
//===============================================================================================
WLAN::WLAN() {
    // default constructor
}

int WLAN::init() {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    int n = WiFi.scanNetworks();
    bool hasFoundNetwork = false;
    for (int i = 0; i < n; ++i) {
        // Print SSID and RSSI for each network found
        if (String(WiFi.SSID(i)).equals(WIFI_SSID_HOME)) {
            WiFi.begin(WIFI_SSID_HOME, WIFI_PASSWORD_HOME);
            hasFoundNetwork = true;
            break;
        } else if (String(WiFi.SSID(i)).equals(WIFI_SSID_FIELD)) {
            WiFi.begin(WIFI_SSID_FIELD, WIFI_PASSWORD_FIELD);
            hasFoundNetwork = true;
            break;
        } // else do nothing
    }
    if (!hasFoundNetwork)
        return FAILURE;
    unsigned long now = millis();
    while (WiFi.status() != WL_CONNECTED) {
        Serial.printf(".");
        delay(500);
        if (millis() > now+100000) { // connection attempt timed out
            Serial.printf("Unable to connect to WiFi\n");
            return FAILURE;
        }
    }
    //Print ESP32 Local IP Address
    delay(500);
    Serial.printf("\nWifi connected at ");Serial.println(WiFi.localIP());
    return SUCCESS;
}

int WLAN::disable() {
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    return SUCCESS;
}

bool WLAN::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}

WLAN Wlan;


//===============================================================================================
// TIME
//===============================================================================================
TIME::TIME() {
    // default constructor
}

int TIME::init() {
    configTime(GMT_TIME_ZONE, DAYLIGHT_OFFSET, NTP_SERVER);
    delay(700);
    Serial.printf("[INF0] System time initialized at %s\n", toString());
    return SUCCESS;
}

tm TIME::getTimeinfo() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) { // set time manually
        Serial.printf("Failed to obtain time\n");
        timeinfo.tm_sec = 0;
        timeinfo.tm_min = 0;
        timeinfo.tm_min = 0;
        timeinfo.tm_hour = 0;
        timeinfo.tm_mday = 0;
        timeinfo.tm_mon = 0;
        timeinfo.tm_year = 0;
        timeinfo.tm_wday = 0;
        timeinfo.tm_yday = 0;
        timeinfo.tm_isdst = 0;
    }
    return timeinfo;
}

char* TIME::toString() {
    struct tm timeinfo = getTimeinfo();
    strftime(timeString, TIME_STRING_LENGTH, "%d-%m-%Y %H:%M:%S", &timeinfo);
    return timeString;
}

TIME Time;


//===============================================================================================
// LOG SYSTEM
//===============================================================================================
LOG::LOG() {/*default constructor*/}

int LOG::init() {
    //Mount internal file system:
    if(!SPIFFS.begin(true)) {
        Serial.printf("Unable to mount SPIFFS.\n");
        return FAILURE;
    }

    //Write first line to file:
    File fileToWrite = SPIFFS.open("/log.txt", FILE_APPEND);
    if(!fileToWrite){
        Serial.printf("Unable to open log file for writing.\n");
        return FAILURE;
    }
    String logMsg = String(Time.toString())+" [INFO] System booted!\n";
    if(!fileToWrite.printf(logMsg.c_str())) {
        Serial.printf("Failed to write to log file.\n");
        return FAILURE;
    }
    fileToWrite.close();

    return SUCCESS;
}

int LOG::msg(log_mode_t mode, const char* str) {
    String buffer = Time.toString();
    String msg = str;
    switch (mode) {
    case INFO:
        buffer += " [INFO] "+msg+"\n";
        break;
    case WARNING:
        buffer += "  > "+msg+"\n";
        break;
    case ERROR:
        buffer += " [ERROR] "+msg+"\n";
        break;
    case DEBUG:
        buffer += " ## "+msg+"\n";
        break;
    default:
        buffer += " "+msg+"\n";
        break;
    }
    Serial.printf(buffer.c_str());

    //Write buffer to Log File:
    File logFile = SPIFFS.open("/log.txt", FILE_APPEND); //FILE_APPEND
    logFile.printf(buffer.c_str());
    logFile.flush();
    logFile.close();
    
    return SUCCESS;
}

const char* LOG::readFile() {
    File file2 = SPIFFS.open("/log.txt", FILE_READ);
    if (!file2) {
        Serial.print("Failed to open log file\n");
        return String("").c_str();
    }
    String logString = "";
    while(file2.available()){
        logString = logString + String(file2.readString()) + "\r\n";
    }
    file2.close();
    return logString.c_str();
}

/*int LOG::readFile() {
    File file2 = SPIFFS.open("/log.txt", FILE_READ);
    if (!file2) {
        Serial.print("Failed to open log file\n");
        return FAILURE;
    }
    Serial.print(" <<< LOG FILE /log.txt >>>\n");
    while(file2.available()){
        Serial.write(file2.read());
    }
    Serial.print(" >>> END OF LOG FILE <<<\n");
    file2.close();
    return SUCCESS;
}*/

int LOG::getFileSize() {
    File file2 = SPIFFS.open("/log.txt", FILE_READ);
    if (!file2) {
        Serial.print("Failed to open log file\n");
        return FAILURE;
    }
    int fileSize = file2.size();
    file2.close();
    return fileSize;
}

int LOG::clearFile() {
    File file2 = SPIFFS.open("/log.txt", FILE_WRITE);
    if (!file2) {
        Serial.print("Failed to open log file\n");
        return FAILURE;
    }
    Log.msg(LOG::INFO, "Log File cleared.");
    return SUCCESS;
}

LOG Log;


//===============================================================================================
// FILE SYSTEM
//===============================================================================================
FILE_SYSTEM::FILE_SYSTEM() {
    String fileName;
}

int FILE_SYSTEM::init() {
    if (!SD.begin(SPI_CD)) {
        Serial.printf("Card Mount Failed");
        return FAILURE; // error code: no card shield found
    }

    //Create a file on the SD card if data.txt does not exist:
    struct tm timeinfo = Time.getTimeinfo();
    fileName = "/data_"+String(timeinfo.tm_mday)+"-"+String(timeinfo.tm_mon+1)+"-"+String(timeinfo.tm_year+1900)+".txt";
    File file = SD.open(fileName.c_str());
    if(!file) {
        writeFile(SD, fileName.c_str(), "Timestamp, Flow, Pressure, Level\r\n");
    }
    file.close();

    //Check SD card size:
    unsigned long long usedBytes = SD.usedBytes() / (1024 * 1024);
    Serial.printf("Mounted SD card with %llu MB used.\n", usedBytes);
    return SUCCESS;
}

void FILE_SYSTEM::listDirectory(fs::FS &fs, const char *dirname, uint8_t levels) {
    Serial.printf("Listing directory: %s\n", dirname);
    File root = fs.open(dirname);
    if (!root) {
        Serial.println("Failed to open directory");
        return;
    }
    if (!root.isDirectory()) {
        Serial.println("Not a directory");
        return;
    }
    File file = root.openNextFile();
    while (file) {
        if (file.isDirectory()) {
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if (levels) listDirectory(fs, file.name(), levels - 1);
        } else {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("  SIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}

void FILE_SYSTEM::createDir(fs::FS &fs, const char *path) {
    Serial.printf("Creating Dir: %s\n", path);
    if (fs.mkdir(path)) Serial.println("Dir created");
    else Serial.println("mkdir failed");
}

void FILE_SYSTEM::removeDir(fs::FS &fs, const char *path) {
    Serial.printf("Removing Dir: %s\n", path);
    if (fs.rmdir(path)) Serial.println("Dir removed");
    else Serial.println("rmdir failed");
}

void FILE_SYSTEM::readFile(fs::FS &fs, const char *path) {
    Serial.printf("Reading file: %s\n", path);
    File file = fs.open(path);
    if (!file) {
        Serial.println("Failed to open file for reading");
        return;
    }
    Serial.print("Read from file: ");
    while (file.available()) Serial.write(file.read());
    file.close();
}

void FILE_SYSTEM::writeFile(fs::FS &fs, const char *path, const char *message) {
    Serial.printf("Writing file: %s\n", path);
    File file = fs.open(path, FILE_WRITE);
    if (!file) {
        Serial.println("Failed to open file for writing");
        return;
    }
    if (file.print(message)) Serial.println("File written");
    else Serial.println("Write failed");
    file.close();
}

void FILE_SYSTEM::appendFile(fs::FS &fs, const char *path, const char *message) {
    File file = fs.open(path, FILE_APPEND);
    if (!file) {
        Serial.println("Failed to open file for appending");
        return;
    }
    if (!file.print(message)) Serial.println("Append failed");
    file.close();
}

void FILE_SYSTEM::deleteFile(fs::FS &fs, const char *path) {
    Serial.printf("Deleting file: %s\n", path);
    if (fs.remove(path)) Serial.println("File deleted");
    else Serial.println("Delete failed");
}

const char* FILE_SYSTEM::getFileName() {
    return fileName.c_str();
}

FILE_SYSTEM FileSystem;


//===============================================================================================
// MAIL
//===============================================================================================
MAIL::MAIL() {
    SMTPData smtpData; // the object contains config and data to send
}

int MAIL::init() {
    // when using SD-Card, use the following line, otherswise it is not needed
    return MailClient.sdBegin(14, 2, 15, 13) ? SUCCESS : FAILURE; // (SCK, MISO, MOSI, SS)   
}
    
void MAIL::callbackSend(SendStatus msg) {
    Serial.println(msg.info());
    if (msg.success()) {
        Serial.println("----------------");
    }
}

int MAIL::send(const char* mailText) {
    // For library version 1.2.0 and later which STARTTLS protocol was supported,the STARTTLS will be 
    // enabled automatically when port 587 was used, or enable it manually using setSTARTTLS function.
    // smtpData.setSTARTTLS(true);
    // Disable Google Security for less secure apps: https://myaccount.google.com/lesssecureapps?pli=1

    //Set E-Mail credentials:
    smtpData.setLogin(SMTP_SERVER, SMTP_SERVER_PORT, EMAIL_SENDER_ACCOUNT, EMAIL_SENDER_PASSWORD);
    smtpData.setSender("ESP32", EMAIL_SENDER_ACCOUNT);
    smtpData.setPriority("High");
    smtpData.setSubject(EMAIL_SUBJECT);

    //Make new E-Mail Message:
    smtpData.addRecipient(EMAIL_RECIPIENT);
    //smtpData.setMessage("<div style=\"color:#2f4468;\"><h1>Hello World!</h1><p>- Sent from ESP32 board</p></div>", true);
    smtpData.setMessage(mailText, false); // Set the email message in text format (raw)
    
    //Attach File to Mail:
    smtpData.addAttachFile(FileSystem.getFileName());
    smtpData.setFileStorageType(MailClientStorageType::SD);
    //smtpData.addAttachFile("/log.txt");
    //smtpData.setFileStorageType(MailClientStorageType::SPIFFS);

    smtpData.setSendCallback(this->callbackSend);

    //Start sending Email, can be set callback function to track the status
    if (!MailClient.sendMail(smtpData)) {
        String logMsg = "Error sending Email, " + MailClient.smtpErrorReason();
        Log.msg(LOG::WARNING, logMsg.c_str());
        return FAILURE;
    }
    smtpData.empty();  // clear data to free memory
    return SUCCESS;
}

MAIL Mail;

/* alternative, non-working implementation (using newer ESP Mail client):
MAIL::MAIL() {
    SMTPSession smtp; // the object contains config and data to send
    ESP_Mail_Session session; // holds session config data
}

int MAIL::init() {
    session.server.host_name = SMTP_SERVER;
    session.server.port = SMTP_SERVER_PORT;
    session.login.email = EMAIL_SENDER_ACCOUNT;
    session.login.password = EMAIL_SENDER_PASSWORD;
    // session.login.user_domain = F("gmail.com");

    session.time.ntp_server = NTP_SERVER;
    session.time.gmt_offset = GMT_TIME_ZONE;
    session.time.day_light_offset = DAYLIGHT_OFFSET;

    return SUCCESS;
}
    
void MAIL::callbackSend(SMTP_Status status) {
    String logString = String("Email Status Response: ") + status.info();
    Log.msg(LOG::INFO, logString.c_str()); // maybe status.info().c_str();
    if (status.success()) {
        Serial.println("----------------");
    }
}

int MAIL::send(const char* mailText) {
    // [INFO] Disable Google Security for less secure apps: https://myaccount.google.com/lesssecureapps?pli=1

    //Set E-Mail credentials:
    SMTP_Message message;
    message.sender.name = "ESP32";
    message.sender.email = EMAIL_SENDER_ACCOUNT;
    message.subject = EMAIL_SUBJECT;
    message.addRecipient("Oliver Wohrer", EMAIL_RECIPIENT);
    // message.addCc("Peter Wohrer", );
    message.text.content = mailText;
    
    //Declare Attachment Data Objects:
    SMTP_Attachment att[2]; // [0]...data file, [1]...log file

    //Attach Data File:
    att[0].descr.filename = F(FileSystem.getFileName());
    att[0].descr.mime = F("text/plain");
    att[0].file.path = F(FileSystem.getFileName());
    att[0].file.storage_type = esp_mail_file_storage_type_sd;
    // att[0].descr.transfer_encoding = Content_Transfer_Encoding::enc_base64;
    message.addAttachment(att[0]);

    //Attach Log File:
    att[1].descr.filename = F("log.txt");
    att[1].descr.mime = F("text/plain");
    att[1].file.path = F("/log.txt");
    att[1].file.storage_type = esp_mail_file_storage_type_flash;
    // att[1].descr.transfer_encoding = Content_Transfer_Encoding::enc_base64;
    message.addAttachment(att[0]);

    if (!smtp.connect(&session)) {
        return FAILURE; // server connection failed
    }

    //Start Sending the Email and Close the Session:
    smtp.callback(callbackSend);
    if (!MailClient.sendMail(&smtp, &message, true)) {
        String logMsg = "Error sending Email, " + smtp.errorReason();
        Log.msg(LOG::WARNING, logMsg.c_str());
        return FAILURE;
    }

    smtp.sendingResult.clear(); // clear data to free memory
    return SUCCESS;
}

MAIL Mail;

*/