#include <OSIWifiHandler.h>
#include <algorithm>
#include <ESP8266WiFi.h>


OSIWifiHandler::OSIWifiHandler(OSIWiFiCredentials *wifiCredentials) {
    int n = 0;
    n = wifiCredentials->getUsedCount();
    Serial.printf_P(PSTR("Configured SSIDs: %d "), n);
    uint8_t netNum = getWLANCount();
    n += netNum;
    Serial.printf_P(PSTR("Scan SSIDs: %d\n"), netNum);
    networks = new std::vector<OSINetwork>(n);
    readFromWifiCredentials(wifiCredentials);
    scanForNetworks(netNum);
}

OSIWifiHandler::~OSIWifiHandler(){
    delete networks;
}

void OSIWifiHandler::readFromWifiCredentials(OSIWiFiCredentials *wifiCredentials){
    for (int i = 0; i < wifiCredentials->getMaxSSIDs(); i++) {
        if (wifiCredentials->isUsed(i)) {
            char ssid[32+1];
            char password[64+1];
            wifiCredentials->getCredential(i, ssid, password, sizeof(ssid), sizeof(password));
            OSINetwork network{ssid, password, -1};
            networks->push_back(network);
        }
    }
}

uint8_t OSIWifiHandler::getWLANCount() { 
    uint8_t n = 0;
    n = WiFi.scanNetworks();
    if (n == 0) {
        Serial.println(F("No networks found"));
    }
    return n;
}

void OSIWifiHandler::scanForNetworks(uint8_t n){
    for (int i = 0; i < n; ++i) {
        OSINetwork network{WiFi.SSID(i), "", WiFi.RSSI(i)};
        networks->push_back(network);
    }
    std::sort(networks->begin(), networks->end(), [](const OSINetwork &a, const OSINetwork &b) {
        return a.rssi > b.rssi; // Sort in descending order
    });
}

void OSIWifiHandler::updateCredentials(OSIWiFiCredentials *wifiCredentials){
    // Clear the credentials
    for (int i = 0; i < wifiCredentials->getMaxSSIDs(); i++) {
        wifiCredentials->clearCredential(i);
    }
    uint8_t idx = 0;
    for (size_t i = 0; i < networks->size(); i++) {
        OSINetwork network = networks->at(i);
        if (network.ssid != "" && network.password != "") {
            // Check if the SSID is already in the list
            bool skip = false;
            for (int j = 0; j < wifiCredentials->getMaxSSIDs(); j++) {
                char ssid[32+1];
                char password[64+1];
                wifiCredentials->getCredential(j, ssid, password, sizeof(ssid), sizeof(password));
                if (network.ssid.equals(ssid)) {
                    skip = true;
                    break;
                }
            }
            if (skip) {
                continue;
            }
            wifiCredentials->setCredential(idx, network.ssid.c_str(), network.password.c_str());
            idx++;
            if (idx == wifiCredentials->getMaxSSIDs()) {
                break;
            }
        }
    }
}    