#include "OSIExceptionCount.h"
#include "MultiplexedHardwareSerial.h"
#include <EEPROM.h>
#include "user_interface.h"

/**
 * Log the crash information to Serial an thus to File
 * This function is called automatically if ESP8266 suffers an exception
 * It should be kept quick / concise to be able to execute before hardware wdt may kick in
 */
extern "C" void custom_crash_callback(struct rst_info * rst_info, uint32_t stack, uint32_t stack_end ) {
    uint32_t crashTime = millis();
    // Print crash information to Serial
    Serial.println(F("EC: Crash information saved to EEPROM:"));
    Serial.printf_P(PSTR("EC: Crash Time: %ld ms\n"), crashTime);
    Serial.printf_P(PSTR("EC: Restart Reason: %d\n"), rst_info->reason);
    Serial.printf_P(PSTR("EC: Exception Cause: %d\n"), rst_info->exccause);
    Serial.printf_P(PSTR("EC: Exception Count: %d\n"), osiExceptionCount.get());
    Serial.printf_P(PSTR("EC: epc1=0x%08x epc2=0x%08x epc3=0x%08x excvaddr=0x%08x depc=0x%08x\n"),
                    rst_info->epc1, rst_info->epc2, rst_info->epc3, rst_info->excvaddr, rst_info->depc);
    Serial.printf_P(PSTR("EC: Stack Start: 0x%08x Stack End: 0x%08x\n"), stack, stack_end);
    Serial.println(F(">>>Stack Trace>>>"));
    for (uint32_t iAddress = stack; iAddress < stack_end; iAddress += 16) {
        Serial.printf("0x%08x: ", iAddress);
        for (uint32_t j = 0; j < 16 && iAddress + j < stack_end; j++) {
            byte byteValue = *((byte*) (iAddress + j));
            Serial.printf("%02x ", byteValue);
        }
        Serial.println();
    }
    Serial.println(F("EC: <<<Stack Trace<<<"));
    Serial.flush();
#if defined(MULTIPLEX_SERIAL)
    Serial.disableFileLog();
#endif
}

/**
 * @brief Konstruktor der ExceptionCount-Klasse.
 * 
 * Liest die aktuelle Zählung aus dem EEPROM-Speicher und überprüft deren Plausibilität.
 * Dies geschieht durch die globale Variable osiExceptionCount genau einmal beim Programmstart.
 */
OSIExceptionCount::OSIExceptionCount() {
}

/**
 * @brief Wird nie aufgerufen.  
 */
OSIExceptionCount::~OSIExceptionCount() {
}

void OSIExceptionCount::setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println();
    Serial.println();
    // Lädt den Exception-Zähler aus dem EEPROM
    loadCountAndCheck();
    
    // Überprüfung der letzten Neustartgründe
    uint32_t reason = ESP.getResetInfoPtr()->reason;
    Serial.printf_P(PSTR("Last boot reason: %u. \n"), reason);
    if (reason == REASON_WDT_RST || reason == REASON_EXCEPTION_RST || reason == REASON_SOFT_WDT_RST) {
        printBootReason(reason);
        // Erhöht den Zähler, falls ein vorheriger Neustart aufgrund einer Ausnahme erfolgte
        increase();
        Serial.printf_P(PSTR(" --> increased exception count to %u\n"), get());
        

        // Aktiviert den Wartungsmodus, falls zu viele Ausnahmen
        if (get() == MAX_EXCEPTIONS) {
            Serial.println(F("Too many exceptions, activating maintenance mode"));
            maintenanceMode = true;
        } else {
            if (get() == MAX_EXCEPTIONS - 1) {
                Serial.println(F("One more exception until maintenance mode. Logging mode activated"));
                loggingMode = true;
            }   
        } 
    } else {
        Serial.printf_P(PSTR("\nNo previous exception. Normal boot detected. Exception count (%u) reseted to 0.\n"), get());
        reset();
    }
    commit();
}



/**
 * @brief Lade counter aus EEPROM
 * 
 * Lädt die aktuelle Zählung aus dem EEPROM-Speicher und überprüft deren Plausibilität.
 */
void OSIExceptionCount::loadCountAndCheck() {
    EEPROM.begin(64);                               // Initialisiert EEPROM
    count = EEPROM.read(EXCEPTION_COUNT_ADDR);      // Liest Zählerstand
    originalCount = count;

    // Setzt den Ausnahmezähler zurück, falls unplausibler Wert
    if (count > MAX_EXCEPTIONS + 2) {
        Serial.println(F("EC: Exception count is too high, resetting"));
        reset();
    } 
    EEPROM.end();
}

/**
 * @brief Gibt den Grund des letzten Neustarts aus
 */
void OSIExceptionCount::printBootReason(uint32_t iReason) {
    uint8_t reason = (iReason > 7) ? 7 : iReason;
    const __FlashStringHelper* TEXT[] = {F("Normal boot"), F("Hardware watchdog reset"), F("Exception boot"),
        F("Soft WDT boot"), F("Software restart"), F("Wake up from deep sleep"), F("External system boot"), F("Unknown reason")};
    Serial.printf_P(PSTR("%s"), TEXT[reason]);    
}


/**
 * @brief Schreibt die aktuelle Zählung ins EEPROM, falls geändert
 */
void OSIExceptionCount::commit() {
    if (count != originalCount) {
        EEPROM.begin(64);                           // Initialisiert EEPROM
        EEPROM.write(EXCEPTION_COUNT_ADDR, count);
        if (!EEPROM.commit()) {
            Serial.println(F("EC: Error: EEPROM commit failed"));
        }
        originalCount = count;
        EEPROM.end();
    } 
}

/**
 * @brief Erhöht die Ausnahmzählung um eins
 */
void OSIExceptionCount::increase() {
    count++;
}

/**
 * @brief Setzt die Zählung zurück
 */
void OSIExceptionCount::reset() {
    Serial.println(F("EC: Counter resetted"));
    count = 0;
}

/**
 * @brief Gibt die aktuelle Zählung zurück
 * @return Zählstand
 */
uint8_t OSIExceptionCount::get() {
    return count;
}

/**
 * @brief Prüfe ob Maintenance-Mode aktiviert werden soll
 * 
 */
bool OSIExceptionCount::isMaintenanceMode() {
    return maintenanceMode;
}

/**
 * @brief Prüfe ob Logging-Mode aktiviert werden soll
 * 
 */
bool OSIExceptionCount::isLoggingMode() {
    return loggingMode;
}

/**
 * @brief Instanz der ExceptionCount-Klasse
 */ 
OSIExceptionCount osiExceptionCount;