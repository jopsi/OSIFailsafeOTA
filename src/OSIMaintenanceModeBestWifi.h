#ifndef OSI_MAINTENANCE_MODE_BEST_WIFI_H
#define OSI_MAINTENANCE_MODE_BEST_WIFI_H
#include <OSIWiFiCredentials.h> 

/**
 * @brief Class to find the best WiFi network in maintenance mode
 * 
 * This class scans for available WiFi networks and selects the one with the best signal strength.
 * The class is derived from the ILineProcessor interface to process the WiFi scan results.
 */
struct OSIWifiExtendedScanResult{
    int32_t rssi;
    char    ssid[32+1];
    char    password[64+1];
    uint8_t bssid[6];
    int32_t channel;
};


class OSIMaintenanceModeBestWifi{
private:
    static uint16_t getNumberOfNetworks();
    static bool isNetworkKnown(uint8_t index, const char *ssid);

public:
    OSIMaintenanceModeBestWifi() {}
    ~OSIMaintenanceModeBestWifi() {}
    
    static bool connect2Network(const char *ssid, const char *password, uint8_t *bssid, uint8_t channel, uint32_t timeout);
    static bool connect2BestKnownNetwork(OSIWiFiCredentials *wifiCredentials, uint32_t timeout);
};

#endif
