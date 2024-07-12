#include <OSIReverseDNS.h>
#include <MultiplexedHardwareSerial.h>
#include <ESP8266WiFi.h>

OSIReverseDNS *OSIReverseDNS::instance = nullptr;

OSIReverseDNS::OSIReverseDNS(IPAddress dnsServerIP, int dnsServerPort) : dnsServerIP(dnsServerIP), dnsServerPort(dnsServerPort) {
    queryInProgress = false;
    successfulQuery = false;
}

OSIReverseDNS::~OSIReverseDNS() {
}

OSIReverseDNS *OSIReverseDNS::getInstance() {
    if (instance == nullptr) {
        instance = new OSIReverseDNS(WiFi.dnsIP(0), 53);
    }
    return instance;
}

OSIReverseDNS *OSIReverseDNS::getInstance(IPAddress dnsServerIP, int dnsServerPort) {
    if (instance == nullptr) {
        instance = new OSIReverseDNS(dnsServerIP, dnsServerPort);
    }
    return instance;
}

void OSIReverseDNS::dumpmem(byte *buffer, int size) {
        for (int i = 0; i < size; i++) {
        if (i % 16 == 0) {
            Serial.printf_P(PSTR("%04X: "), i);
        }
        Serial.printf_P(PSTR("%02X "), buffer[i]);
        if (i % 16 == 15 || i == size - 1) {
            for (int j = 0; j < 15 - i % 16; j++) {
                Serial.print(F("   "));
            }
            Serial.print(F("  "));
            for (int j = i - i % 16; j <= i; j++) {
                char c = buffer[j];
                if (c < 32 || c > 126) {
                    c = '.';
                }
                Serial.print(c);
            }
            Serial.println();
        }
    }
}

int OSIReverseDNS::buildDNSQuery(byte* buffer, IPAddress &ip) {
    memset(buffer, 0, 32);
    buffer[0] = 0;  // Transaction ID high byte
    buffer[1] = 1;  // Transaction ID low byte
    buffer[2] = 0;  // Flags high byte
    buffer[3] = 0;  // Flags low byte
    buffer[4] = 0;  // Questions high byte
    buffer[5] = 1;  // Questions low byte
    buffer[6] = 0;  // Answer RRs high byte
    buffer[7] = 0;  // Answer RRs low byte
    buffer[8] = 0;  // Authority RRs high byte
    buffer[9] = 0;  // Authority RRs low byte
    buffer[10] = 0; // Additional RRs high byte
    buffer[11] = 0; // Additional RRs low byte

    byte* ptr = buffer + 12;
    for (int i = 3; i >= 0; i--) {
        byte octet = ip[i];
        int len = sprintf((char*)ptr + 1, "%d", octet);
        *ptr = len;
        ptr += len + 1;
    }

    *ptr++ = 7;  // Length of "in-addr"
    memcpy_P(ptr, PSTR("in-addr"), 7);
    ptr += 7;
    *ptr++ = 4;  // Length of "arpa"
    memcpy_P(ptr, PSTR("arpa"), 4);
    ptr += 4;
    *ptr++ = 0;  // End of domain name

    *ptr++ = 0;  // Type high byte
    *ptr++ = 12; // Type low byte (PTR)
    *ptr++ = 0;  // Class high byte
    *ptr++ = 1;  // Class low byte (IN)

    return ptr - buffer;
}

void OSIReverseDNS::analyseDNSHeader(byte *buffer) {
    // DNS header structure
    OSIReverseDNS::DNSHeader *dns = (OSIReverseDNS::DNSHeader *)buffer;
    if (debug) {
        Serial.printf_P(PSTR("q_count: %d\n"), ntohs(dns->q_count));
        Serial.printf_P(PSTR("ans_count: %d\n"), ntohs(dns->ans_count));
    }
    q_count = ntohs(dns->q_count);
    ans_count = ntohs(dns->ans_count);
}

byte *OSIReverseDNS::movePRT2AnswerSection(byte* buffer, int size) {

    if (debug) {
        // Dump the DNS response as hex and ascii columns
        Serial.println(F("DNS Response:"));
        dumpmem(buffer, size);
    }

    //int q_count = getQuestionCount(buffer);
    // Move the pointer to the question section
    uint8_t *ptr = buffer + sizeof(DNSHeader);
    // Skip over the question section
    for (int i = 0; i < q_count; i++) {
        // Move the pointer over the domain name
        while (*ptr != 0) {
            ptr += (*ptr) + 1;
        }
        ptr += 5; // Skip null byte at the end of the domain name and QTYPE and QCLASS
    }
    return ptr;
}


void OSIReverseDNS::parseDNSResponse(byte* buffer, int size) {
    analyseDNSHeader(buffer);
    uint8_t *ptr = movePRT2AnswerSection(buffer, size);
    if (debug)
        dumpmem(ptr, size - (int)(ptr - buffer));

    // Now ptr should point to the beginning of the answer section
    //int ans_count = getAnswerCount(buffer);
    
    for (int i = 0; i < ans_count; i++) {
        // Read the answer name
        if ((*ptr & 0xC0) == 0xC0) {
            ptr += 2; // Name is a pointer, skip 2 bytes
        } else {
            while (*ptr != 0) {
                ptr += (*ptr) + 1;
            }
            ptr += 1; // Skip the null byte
        }
        if (debug)
            dumpmem(ptr, size - (int)(ptr - buffer));

        // Read the answer type, class, TTL, and data length
        uint16_t type = (ptr[0] << 8) | ptr[1];
        ptr += 2;
        uint16_t _class = (ptr[0] << 8) | ptr[1];
        ptr += 2;
        uint32_t ttl = (ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] << 8) | ptr[3];
        ptr += 4;
        uint16_t data_len = (ptr[0] << 8) | ptr[1];
        ptr += 2;
        if (debug)
            Serial.printf_P(PSTR("Type: %d, Class: %d, TTL: %d, Data Length: %d\n"), type, _class, ttl, data_len);

        if (type == 12) { // Type 12 is PTR (domain name pointer)
            // Read the PTR record data
            //char domain_name[256];
            //uint8_t *domain_ptr = (uint8_t *)domain_name;
            uint8_t *domain_ptr = packetBuffer;
            while (*ptr != 0) {
                if ((*ptr & 0xC0) == 0xC0) {
                    // Handle pointer to a previous name
                    uint16_t offset = ntohs(*(uint16_t *)ptr) & 0x3FFF;
                    ptr = buffer + offset;
                } else {
                    uint8_t len = *ptr++;
                    for (int j = 0; j < len; j++) {
                        *domain_ptr++ = *ptr++;
                    }
                    *domain_ptr++ = '.';
                }
            }
            *domain_ptr = '\0';
            Serial.print("Domain Name: ");
            Serial.println((const char *)packetBuffer);
        } else {
            // If not PTR, skip the data
            break;
        }
    }
}

bool OSIReverseDNS::reverseDNS(const char* ip) {
    if (queryInProgress) {
        if (debug)
            Serial.println(F("DNS query already in progress"));
        return false;
    }
    if (successfulQuery) {
        if (debug) 
            Serial.println(F("Successfull DNS query not yet processed"));
        return false;
    }
    unsigned int localPort = 8888;
    udp.begin(localPort);
    IPAddress ipAddr;
    ipAddr.fromString(ip);

    // Build the reverse DNS query packet
    int packetSize = buildDNSQuery(packetBuffer, ipAddr);

    // Send the DNS query
    udp.beginPacket(dnsServerIP, 53);
    udp.write(packetBuffer, packetSize);
    udp.endPacket();
    Serial.printf_P(PSTR("Sent DNS query for %s\n"), ip);
    startTime = millis() + 10000;
    queryInProgress = true;
    return true;
}

bool OSIReverseDNS::reverseDNSBlocking(const char* ip, char *result, size_t len) {
    if (!reverseDNS(ip)) {
        return false;
    }

    while (queryInProgress) {
        delay(10);
        loop();
    }
    return transferQueryResult(result, len);
}

void OSIReverseDNS::loop() {
    int responseSize;
    byte myBuffer[128];
    if (!queryInProgress) {
        return;         // nothing to do
    }
    if ((responseSize = udp.parsePacket()) == 0 && millis() < startTime) {
        errorCode = 0;        
        return;         // no response yet and not timed out
    } 
    if (millis() >= startTime) {
        errorCode = OSIReverseDNS::ERR_TIMEDOUT;
        Serial.println(F("DNS query timed out"));
        successfulQuery = false;
        queryInProgress = false;
        return;
    } else {
        errorCode = 0;
        if (debug)
            Serial.printf_P(PSTR("Received DNS response of %d bytes\n"), responseSize);
        responseSize = (responseSize > MAXPACKETLEN) ? MAXPACKETLEN : responseSize;
        udp.read(myBuffer, responseSize);
        parseDNSResponse(myBuffer, responseSize);
        successfulQuery = true;
        queryInProgress = false;
    } 
}

bool OSIReverseDNS::transferQueryResult(char *result, size_t len){
    if (!queryInProgress && successfulQuery) {
        memset(result, 0, len);
        strncpy(result, (char *)packetBuffer, len);
        successfulQuery = false;
        return true;
    } else {
        return false;
    }
}