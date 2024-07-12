#ifndef OSIREVERSEDNS_H
#define OSIREVERSEDNS_H
#include <Arduino.h>
#include <DNSServer.h>

class OSIReverseDNS {
struct DNSHeader {
    uint16_t id; // Identification number
    uint8_t rd :1; // Recursion Desired
    uint8_t tc :1; // Truncated Message
    uint8_t aa :1; // Authoritative Answer
    uint8_t opcode :4; // Purpose of message
    uint8_t qr :1; // Query/Response Flag
    uint8_t rcode :4; // Response Code
    uint8_t cd :1; // Checking Disabled
    uint8_t ad :1; // Authenticated Data
    uint8_t z :1; // Reserved
    uint8_t ra :1; // Recursion Available
    uint16_t q_count; // Number of question entries
    uint16_t ans_count; // Number of answer entries
    uint16_t auth_count; // Number of authority entries
    uint16_t add_count; // Number of resource entries
};

private:
    uint8_t errorCode;
    const uint8_t ERR_TIMEDOUT = 1;
    bool debug = false;
    WiFiUDP udp;
    IPAddress dnsServerIP;
    uint16_t dnsServerPort;
    static OSIReverseDNS *instance;
    uint32_t startTime;
    int q_count;
    int ans_count;
    bool queryInProgress = false;
    bool successfulQuery = false;
    const uint8_t MAXPACKETLEN = 128;
    byte packetBuffer[128];

    void analyseDNSHeader(byte *buffer);
    int buildDNSQuery(byte* buffer, IPAddress &ip);
    byte *movePRT2AnswerSection(byte* buffer, int size);
    void parseDNSResponse(byte* buffer, int size);

protected:
    OSIReverseDNS(IPAddress dnsServerIP, int dnsServerPort);
    ~OSIReverseDNS();


public:
    // Auxiliary functions for internal state retrieval
    void debugOn() { debug = true; }
    void debugOff() { debug = false; }
    uint8_t getErrorCode() { return errorCode; }
    static void dumpmem(byte *buffer, int size);
    void resetQuery() { queryInProgress = false; successfulQuery = false; }
    bool isQueryInProgress() { return queryInProgress; }
    bool isSuccessfulQuery() { return successfulQuery; }

    // Main functions
    static OSIReverseDNS *getInstance();
    static OSIReverseDNS *getInstance(IPAddress dnsServerIP, int dnsServerPort);
    void loop();
    bool reverseDNS(const char* ip);
    bool reverseDNSBlocking(const char* ip, char *result, size_t len);
    bool transferQueryResult(char *result, size_t len);
};
#endif