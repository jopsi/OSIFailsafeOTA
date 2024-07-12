#ifndef OSI_MAINTENANCE_MODE_WIFI_H
#define OSI_MAINTENANCE_MODE_WIFI_H
#include <DNSServer.h>
#include <OSIWifiCredentials.h>

#if !defined(USE_ASYNC_WEBSERVER)
#include <ESP8266WebServer.h>
extern ESP8266WebServer myserver;
#else
#include <ESPAsyncWebServer.h>
extern AsyncWebServer myserver;
#endif

/**
 * @brief Class to handle WiFi configuration in maintenance mode in case of exceptions
 * 
 * This class provides a simple web server to configure WiFi settings in case of exceptions.
 * The class reads the current WiFi configuration from a file and allows the user to change
 * the SSID and password of up to three networks. The new configuration is stored in the file.
 * The class also creates an access point to allow the user to connect to the web server.
 * The class is derived from the ILineProcessor interface to process the configuration file.
 */
class OSIMaintenanceModeWifi {
private:
    DNSServer dnsServer;
    OSIWiFiCredentials *wifiCredentials;
    int lineNumber;
    String ssids[4];
    String page;
    bool connected;
    bool scan = true;
    volatile bool scanInProgress = false;
    bool invalidConfigFile = false;

protected:
    void addSSIDPWLine(String line, int dummy);
    void setupWebServer();
    String generateConfigPage();
    void updateConfigFile(AsyncWebServerRequest *request);
    void createAP(String password);
    void scanForNetworks();
    
public:
    OSIMaintenanceModeWifi(const String username, const String password, uint32_t connectTimeout = 10000);
    ~OSIMaintenanceModeWifi();
    void loop();
    bool isConnected() { return connected; }
    bool isConfigFileInvalid() { return invalidConfigFile; }
};

#endif // OSI_MAINTENANCE_MODE_WIFI_H