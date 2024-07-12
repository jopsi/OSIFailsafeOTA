#include <OSIMaintenanceModeWifi.h>
#include <OSIMaintenanceModeBestWifi.h>
#include <OSIFilemanager.h>
#include <MultiplexedHardwareSerial.h>
#include <ESP8266WiFi.h>
#include <OSIMTProfile.h>

// Helper function to scan WiFi networks and get the strongest ones
void OSIMaintenanceModeWifi::scanForNetworks() {
    int n;
    MTProfile(n = WiFi.scanNetworks())
    if (n == 0) {
        Serial.println(F("No networks found"));
        return;
    }

    // Create a list to store SSIDs and their signal strengths
    struct WiFiNetwork {
        String ssid;
        int32_t rssi;
    };
    WiFiNetwork networks[n];
    
    for (int i = 0; i < n; ++i) {
        networks[i].ssid = WiFi.SSID(i);
        networks[i].rssi = WiFi.RSSI(i);
    }

    // Sort the list by signal strength (rssi)
    std::sort(networks, networks + n, [](const WiFiNetwork& a, const WiFiNetwork& b) {
        return a.rssi > b.rssi;
    });

    // Erase the SSID list
    for (int i = 0; i < 4; i++) {
        ssids[i] = "";
    }   

    // Select the top 4 networks
    for (int i = 0; i < 4 && i < n; ++i) {
        ssids[i] = networks[i].ssid;
        Serial.printf_P(PSTR("Found SSID: %s with RSSI: %d\n"), networks[i].ssid.c_str(), networks[i].rssi);
    }
}

/**
 * @brief Update the configuration file with the new SSIDs and passwords
 * 
 * @param ssids array of SSIDs
 * @param passes array of passwords
 */
void OSIMaintenanceModeWifi::updateConfigFile(AsyncWebServerRequest *request) {
    // Adoption to SycnWebServer is missing and needs to be added in the future.
    //int args = request->args();
    /*for (int i = 0; i < args; i++) {
        Serial.printf("ARG[%d]: %s = %s\n", i, request->argName(i).c_str(), request->arg(i).c_str());
    }*/
    char sbuf[20];
    char pbuf[20];
    std::vector<String> ssids;
    std::vector<String> passes;
    for (int i = 1; i < 7; i++) {
        sprintf(sbuf, "ssid%d", i);
        if (request->hasArg(sbuf)) {
            Serial.printf("SSID%d: %s\n", i, request->arg(sbuf).c_str());
            sprintf(pbuf, "pass%d", i);
            if (request->hasArg(pbuf)) {
                Serial.printf("PASS%d: %s\n", i, request->arg(pbuf).c_str());
                ssids.push_back(request->arg(sbuf));
                passes.push_back(request->arg(pbuf));
            }
        }
    }
    int idx = 0;
    for (int i = 0; i < 4; i++) {
        if (ssids[i] != "" && passes[i] != "") {
            Serial.printf("Setting config SSID%d: %s, Password%d: %s\n", idx, ssids[i].c_str(), idx, passes[i].c_str());
            wifiCredentials->clearCredential(idx);
            wifiCredentials->setCredential(idx, ssids[i].c_str(), passes[i].c_str());
            idx++;
        }
    }
    wifiCredentials->saveToFile();
    request->send_P(200, PSTR("text/html"), PSTR("<p>Configuration updated! Rebooting...</p>"));
    Serial.println(F("Configuration updated! Rebooting in 1000ms ..."));
    delay(1000);
    ESP.restart();  // Restart to apply changes
}

/**
 * @brief Generate the configuration page
 * 
 * @return String the configuration page
 */
String OSIMaintenanceModeWifi::generateConfigPage() {
    page = F("<!DOCTYPE html><html><head>\n");
    page += F("<title>WiFi Configuration</title>\n");
    page += F("<meta name='viewport' content='width=device-width, initial-scale=1'>\n");
    page += F("</head><body>\n<h1>WiFi Configuration</h1>\n<form method='post'>");

    lineNumber = 0;
    Serial.println(F("Generating SSIDs and PWs: "));
    for (int i = 0; i < 4; i++) {
        if (wifiCredentials->isUsed(i)) {
            char c_ssid[32+1];
            char c_password[64+1];
            char buffer[200];
            lineNumber = i + 1;
            memset(c_ssid, 0, 32); memset(c_password, 0, 64);
            wifiCredentials->getCredential(i, c_ssid, c_password, sizeof(c_ssid), sizeof(c_password));
            Serial.printf_P(PSTR("Page gen: SSID%d: %s, Password%d: %s\n"), lineNumber, c_ssid, lineNumber, c_password);
            for (int j = 0; j < 4; j++) {
                if (ssids[j].equals(c_ssid)) {
                    ssids[j] = "";
                } 
            }
            sprintf_P(buffer, PSTR("SSID%d: <input type='text' name='ssid%d' value='%s'><br>\nPassword%d: <input type='password' name='pass%d' value='%s'><br>"), lineNumber, lineNumber, c_ssid, lineNumber, lineNumber, c_password);            
            page += buffer;
        }
    }
    for (int i = 0; i < 4; i++) {
        if (ssids[i] != "") {
            page += F("SSID");
            page += String(lineNumber + 1);
            page += F(": <input type='text' name='ssid");
            page += String(lineNumber + 1);
            page += F("' value='");
            page += ssids[i];
            page += F("'><br>\nPassword");
            page += String(lineNumber + 1);
            page += F(": <input type='password' name='pass");
            page += String(lineNumber + 1);
            page += F("'><br>\n");
            lineNumber++;
        }
    }
    
    page += F("<input type='submit' value='Save'>\n");
    page += F("</form>\n");

    // Add Scan button
    page += F("<form action='/scan' method='get'>\n");
    page += F("<input type='submit' value='Scan for Networks'>\n");
    page += F("</form>\n");

    // Add Reboot button
    page += F("<form action='/reboot' method='get'>\n");
    page += F("<input type='submit' value='Reboot'>\n");
    page += F("</form>\n");

    page += F("</body></html>");
    return page;
}

/**
 * @brief Set up the web server
 */
void OSIMaintenanceModeWifi::setupWebServer() {
    
#if !defined(USE_ASYNC_WEBSERVER)    
    myserver.on("/wifi", HTTP_GET, [this]() { myserver.send(200, "text/html", generateConfigPage()); page = ""; });
    myserver.on("/hotspot-detect.html", HTTP_GET, [this]() { myserver.send(200, "text/html", generateConfigPage()); page = ""; });
    myserver.on("/wifi", HTTP_POST, [this]() {
        String ssids[3] = { myserver.arg(F("ssid1")), myserver.arg(F("ssid2")), myserver.arg(F("ssid3")) };
        String passes[3] = { myserver.arg(F("pass1")), myserver.arg(F("pass2")), myserver.arg(F("pass3")) };
        updateConfigFile(ssids, passes);
        myserver.send_P(200, PSTR("text/html"), PSTR("<p>Configuration updated! Rebooting...</p>"));
        delay(1000);
        ESP.restart();  // Restart to apply changes
    });
    myserver.onNotFound([this]() {
        myserver.sendHeader("Location", String("http://") + WiFi.softAPIP().toString(), true);
        myserver.send(302, "text/plain", "");
    });
    myserver.on("/scan", HTTP_GET, [this](AsyncWebServerRequest *request){ scan = true;  myserver.redirect("/wifi"); });
    myserver.on("/reboot", HTTP_GET, [this](AsyncWebServerRequest *request){ myserver.send(200, "text/html", "Rebooting..."); delay(1000); ESP.restart(); });
#else
    myserver.on("/wifi", HTTP_GET, [this](AsyncWebServerRequest *request) { request->send(200, "text/html", generateConfigPage()); page = "";});
    myserver.on("/hotspot-detect.html", HTTP_GET, [this](AsyncWebServerRequest *request) { request->send(200, "text/html", generateConfigPage()); page = "";});
    myserver.on("/wifi", HTTP_POST, [this](AsyncWebServerRequest *request) {
        Serial.println("POST request received");
        updateConfigFile(request);
    });
    myserver.on("/hotspot-detect.html", HTTP_POST, [this](AsyncWebServerRequest *request) {
        Serial.println("POST request received");
        updateConfigFile(request);
    });
    myserver.onNotFound([this](AsyncWebServerRequest *request) {
        request->redirect(String("http://") + WiFi.softAPIP().toString());
    });
    myserver.on("/scan", HTTP_GET, [this](AsyncWebServerRequest *request){ scan = true;  request->redirect("/wifi"); });
    myserver.on("/reboot", HTTP_GET, [this](AsyncWebServerRequest *request){ request->send(200, "text/html", "Rebooting..."); delay(1000); ESP.restart(); });
#endif


    myserver.begin();
}

/**
 * @brief Create an access point for the web server
 */
void OSIMaintenanceModeWifi::createAP(String password) {
    String chipID = String(ESP.getChipId());
    String ssid = "ESP_" + chipID;
    //String password = "0123456789"; // Consider using a more secure password in production

    // Set up a static IP address, gateway, and subnet for the AP
    IPAddress local_IP(192, 168, 4, 1);
    IPAddress gateway(192, 168, 4, 1); // Usually the same as the local IP
    IPAddress subnet(255, 255, 255, 0);

    // Configure the softAP with IP settings
    WiFi.softAPConfig(local_IP, gateway, subnet);

    // Start the AP with SSID and password
    WiFi.softAP(ssid.c_str(), password.c_str());

    // Print the network details

    Serial.printf_P(PSTR("Created AP: %s with password %s and IP: %s\n"), ssid.c_str(), password.c_str(), WiFi.softAPIP().toString().c_str());

    // Start the DNS server
    dnsServer.start(53, "*", local_IP);
    
}

/**
 * @brief Construct a new Maintenance Mode Wifi object
 */
OSIMaintenanceModeWifi::OSIMaintenanceModeWifi(const String username, const String password, uint32_t connectTimeout) {
    wifiCredentials = new OSIWiFiCredentials();
    connected = false;
    if (!OSIMaintenanceModeBestWifi::connect2BestKnownNetwork(wifiCredentials, 5000)) {
        Serial.println(F("Failed to connect to any WiFi network. Opening own AP."));
        createAP(password);
    }
    setupWebServer();
    OSIFileManager *getInstance = OSIFileManager::getInstance();
    getInstance->setUsernameAndPassword(username, password);
    getInstance->initFileSystem();
    getInstance->setupWebserver();
}

/**
 * @brief Destroy the Maintenance Mode Wifi object
 */
OSIMaintenanceModeWifi::~OSIMaintenanceModeWifi() { 
    delete wifiCredentials;
}

/**
 * @brief Loop function for the maintenance mode
 */
void OSIMaintenanceModeWifi::loop() {
    if (scan && !scanInProgress) {
        scanInProgress = true;
        scan = false;
        scanForNetworks();
        scanInProgress = false;
    }
    dnsServer.processNextRequest();
#if !defined(USE_ASYNC_WEBSERVER)
    myserver.handleClient();
#endif
}
