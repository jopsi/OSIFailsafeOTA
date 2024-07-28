#ifndef OSI_WIFI_HANDLER_H
#define OSI_WIFI_HANDLER_H
#include <OSIWifiCredentials.h>

struct OSINetwork {
    String ssid;
    String password;
    int32_t rssi;
};

class OSIWifiHandler {
private:
    std::vector<OSINetwork> *networks;

    uint8_t getWLANCount();
    void readFromWifiCredentials(OSIWiFiCredentials *wifiCredentials);
    void scanForNetworks(uint8_t netNum);        

public:
    OSIWifiHandler(OSIWiFiCredentials *wifiCredentials);
    ~OSIWifiHandler();
    uint8_t getNetworkCount() { return networks->size(); }
    OSINetwork getNetwork(uint8_t index) { return (index < getNetworkCount()) ? networks->at(index) : OSINetwork(); }
    void setNetwork(uint8_t index, const OSINetwork &network) { if (index < getNetworkCount()) networks->at(index) = network; }
    void updateCredentials(OSIWiFiCredentials *wifiCredentials);
};

#endif