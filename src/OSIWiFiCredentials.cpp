#include <OSIWiFiCredentials.h>
#include <FS.h>
#include <LittleFS.h>

void OSIWiFiCredentials::loadFromFile() {
    if (LittleFS.exists(filename)) {
        File file = LittleFS.open(filename, "r");
        if (file) {
            file.read((uint8_t*)credentials, sizeof(credentials));
            file.close();
        }
    } else {
        for (uint8_t i = 0; i < maxSSIDs; i++) {
            clearCredential(i);
        }
    }
}

void OSIWiFiCredentials::saveToFile() {
    if (!LittleFS.begin()) {
        Serial.println("==> ERROR: LittleFS Mount Failed");
        return;
    }

    File file = LittleFS.open(filename, "w");
    if (file) {
        file.write((uint8_t*)credentials, sizeof(credentials));
        file.close();
        Serial.println("==> WiFi Credentials saved to file");
    } else {
        Serial.println("==> ERROR: Could not save WiFi Credentials to file");   
    }
}

OSIWiFiCredentials::OSIWiFiCredentials() {
    if (!LittleFS.begin()) {
        Serial.println("==> ERROR: LittleFS Mount Failed");
        return;
    }
    loadFromFile();
    Serial.println("==> WiFi Credentials:");
    for (uint8_t i = 0; i < maxSSIDs; i++) {
        if (credentials[i].used) {
            Serial.printf("SSID %d: %s, %s\n", i, credentials[i].ssid, credentials[i].password);
        }
    }
}

OSIWiFiCredentials::~OSIWiFiCredentials() {
    saveToFile();
    LittleFS.end();
}

void OSIWiFiCredentials::setCredential(uint8_t index, const char* ssid, const char* password) {
    if (index >= maxSSIDs) return;
    strncpy(credentials[index].ssid, ssid, maxSSIDLength);
    credentials[index].ssid[maxSSIDLength] = '\0'; // Ensure null-termination
    strncpy(credentials[index].password, password, maxPasswordLength);
    credentials[index].password[maxPasswordLength] = '\0'; // Ensure null-termination
    credentials[index].used = true;
}

void OSIWiFiCredentials::getCredential(uint8_t index, char* ssid, char* password, size_t SSIDBufLen, size_t passwordBufLength) {
    if (index >= maxSSIDs) return;
    if (credentials[index].used) {
        memset(ssid, 0, SSIDBufLen);
        memset(password, 0, passwordBufLength);
        strncpy(ssid, credentials[index].ssid, min((const long)maxSSIDLength + 1, (const long)SSIDBufLen));
        strncpy(password, credentials[index].password, min((const long)maxPasswordLength + 1, (const long)passwordBufLength));
    } else {
        ssid[0] = '\0';
        password[0] = '\0';
    }
}

bool OSIWiFiCredentials::isUsed(uint8_t index) {
    if (index >= maxSSIDs) return false;
    return credentials[index].used;
}

void OSIWiFiCredentials::clearCredential(uint8_t index) {
    if (index >= maxSSIDs) return;
    credentials[index].ssid[0] = '\0';
    credentials[index].password[0] = '\0';
    credentials[index].used = false;
}

bool OSIWiFiCredentials::credentialsFileExists() {
    return LittleFS.exists(filename);
}

uint8_t OSIWiFiCredentials::getUsedCount() {
    uint8_t count = 0;
    for (uint8_t i = 0; i < maxSSIDs; i++) {
        if (credentials[i].used) {
            count++;
        }
    }
    return count;
}

uint8_t OSIWiFiCredentials::getMaxSSIDs() {
    return maxSSIDs;
}