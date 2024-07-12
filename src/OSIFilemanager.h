#ifndef OSI_FILE_MANAGER_H
#define OSI_FILE_MANAGER_H
#include <Arduino.h>
#if !defined(USE_ASYNC_WEBSERVER)
#include <ESP8266WebServer.h>
extern ESP8266WebServer myserver;
#else
#include <ESPAsyncWebServer.h>
extern AsyncWebServer myserver;
#endif

extern volatile bool shouldReboot;            // schedule a reboot

#ifndef FIRMWARE_VERSION
#define FIRMWARE_VERSION "v0.1.1"
#endif 

class OSIFileManager {
private:
    static OSIFileManager *instance; 
    String username = "admin";
    String password = "admin";
    long totBytes;
    long usedBytes;

    void updateFSStat();
    void rebootESP(String message);

#if !defined(USE_ASYNC_WEBSERVER)
    void send(int code, String message);
    static void logClientAndURL(const __FlashStringHelper *text);
    static bool checkUserWebAuth(bool noLogging = false);
    static void notFound();
    static void handleUpload();
#else
    void send(AsyncWebServerRequest *request, int code, String message);
    static void logClientAndURL(AsyncWebServerRequest *request, const __FlashStringHelper *text);
    static void notFound(AsyncWebServerRequest *request);
    static void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);
#endif    

    String humanReadableSize(size_t bytes);
    String processor(const String& var);
    String processTemplate(const char* templateStr);
            
    // handles uploads to the filserver
    
    void configureWebServer();

public:
    OSIFileManager();
    void setUsernameAndPassword(const String username, const String password);
    void initFileSystem();
    String listFiles(bool ishtml = false);

    static OSIFileManager *getInstance();
    void setupWebserver();

#if !defined(USE_ASYNC_WEBSERVER)
    static bool checkUserWebAuth(bool noLogging = false);
#else
    static String getRequestParameterByName(AsyncWebServerRequest *request, const char *name);
    static bool existsRequestParameterByName(AsyncWebServerRequest *request, const char *name);
    static bool checkUserWebAuth(AsyncWebServerRequest *request, bool noLogging = false);
#endif    

};
#endif // OSI_FILE_MANAGER_H