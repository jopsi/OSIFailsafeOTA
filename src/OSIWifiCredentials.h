#ifndef OSIWiFiCredentials_h
#define OSIWiFiCredentials_h
#include "MultiplexedHardwareSerial.h"

class OSIWiFiCredentials {
private:
    static const uint8_t maxSSIDs = 4;
    static const uint8_t maxSSIDLength = 32;
    static const uint8_t maxPasswordLength = 64;
    const char* filename = "/wifi.cfg";

    struct Credentials {
        char ssid[maxSSIDLength + 1];
        char password[maxPasswordLength + 1];
        bool used;
    } credentials[maxSSIDs];

    void loadFromFile();

public:
    OSIWiFiCredentials();
    ~OSIWiFiCredentials();
    void saveToFile();
    void setCredential(uint8_t index, const char* ssid, const char* password);
    void getCredential(uint8_t index, char* ssid, char* password, size_t SSIDBufLen=maxSSIDLength+1, size_t passwordBufLength=maxPasswordLength+1);
    bool isUsed(uint8_t index);
    void clearCredential(uint8_t index);
    bool credentialsFileExists(); // New function to check if the credentials file exists
    uint8_t getUsedCount(); // New function to get the number of used credentials
    uint8_t getMaxSSIDs(); // New function to get the maximum number of SSIDs
};
#endif
