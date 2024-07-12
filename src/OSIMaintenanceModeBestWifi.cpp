#include "OSIMaintenanceModeBestWifi.h"
#include <ESP8266WiFi.h>
#include <MultiplexedHardwareSerial.h>

bool OSIMaintenanceModeBestWifi::connect2Network(const char *ssid, const char *password, uint8_t *bssid, uint8_t channel, uint32_t timeout) {
    Serial.printf_P(PSTR("Connecting to SSID <%s> with (%s)"), ssid, password);
    WiFi.begin(ssid, password, channel, bssid);
    uint32_t connectTimeout = millis() + timeout;
    while (WiFi.status() != WL_CONNECTED && connectTimeout > millis()) {
        Serial.print(F("."));
        delay(200);
    }
    return WiFi.status() == WL_CONNECTED;
}

bool OSIMaintenanceModeBestWifi::connect2BestKnownNetwork(OSIWiFiCredentials *wifiCredentials, uint32_t timeout) {
    uint16_t numberOfNetworks = 0;
    WiFi.mode(WIFI_STA);
    if ((numberOfNetworks = WiFi.scanNetworks()) == 0) {
        // No networks are in reach. Need to open own AP.
        Serial.println(F("No networks found. Need to open own AP."));
        return false;
    } 

    char ssid[32];
    char password[64];
    OSIWifiExtendedScanResult result;
    for (int j=0; j < 4; j++) {
        // We just look on the networks we know and have stored credentials for
        if (!wifiCredentials->isUsed(j)) continue;
        wifiCredentials->getCredential(j, ssid, password);
        if (strlen(ssid) == 0) continue;
        if (strlen(password) == 0) continue;
        // There are valid credentials available
        Serial.printf_P(PSTR("Checking stored SSID: %s against available networks:\n"), ssid);
        for (int i = 0; i < numberOfNetworks; i++) {
            // Do we know this network?
            if (!WiFi.SSID(i).equalsIgnoreCase(ssid)) { // We do not know this network
                Serial.printf_P(PSTR("Ignoring network SSID: %s\n"), WiFi.SSID(i).c_str());
                continue;
            }   
            
            // We know this network                      
            Serial.printf_P(PSTR("Found matching SSID: %s with RSSI: %d on %d - "), ssid, WiFi.RSSI(i), WiFi.channel(i)); 
            // Is the signal strength better than the current best?
            if (WiFi.RSSI(i) < result.rssi) { // No, the signal strength is not better than the current best
                Serial.println(F("weaker signal then current best.\n"));
                continue;
            }
            // Disconnect from the last network if we are connected
            if (result.rssi != -100) WiFi.disconnect(true);

            // Connect to the network
            if (!connect2Network(ssid, password, WiFi.BSSID(i), WiFi.channel(i), timeout)) {
                Serial.println(F(" - Failed to connect to WiFi"));
                continue;
            }
            
            Serial.printf_P(PSTR("Connected with IP: %s\n"), WiFi.localIP().toString().c_str());
            result.rssi = WiFi.RSSI(i);
            result.channel = WiFi.channel(i);
            memccpy((void *)result.bssid, WiFi.BSSID(i), 6, 6);
            strncpy(result.ssid, ssid, sizeof(result.ssid));
            strncpy(result.password, password, sizeof(result.password));
        }
    }

    if (result.rssi != -100) {
        Serial.printf_P(PSTR("Best WiFi network: %s with RSSI: %d\n"), result.ssid, result.rssi);
        return connect2Network(result.ssid, result.password, result.bssid, result.channel, timeout);
    }
    Serial.println(F("No matching SSID found. Need to open own AP."));
    return false;
}
