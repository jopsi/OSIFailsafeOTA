#ifndef OSIMTProfile_h
#define OSIMTProfile_h
#include <MultiplexedHardwareSerial.h>

#define MTProfile(A) { OSIMemTimeProfiling MTProfile(F(#A)); { A; }}

class OSIMemTimeProfiling {
private: 
    const __FlashStringHelper *topic;
    uint32_t startMem;
    uint32_t startTime;
public:
    OSIMemTimeProfiling(const __FlashStringHelper *topic) {
        this->topic = topic;
        startMem = ESP.getFreeHeap();
        startTime = millis();
        Serial.printf_P(PSTR("Entering %s with %d free mem at %ld ms.\n"), topic, startMem, startTime);
    }
    ~OSIMemTimeProfiling() {
        Serial.printf_P(PSTR("Leaving %s with %d less free mem after %ld ms.\n"), topic, startMem-ESP.getFreeHeap(), millis()-startTime);  
    }
};
#endif