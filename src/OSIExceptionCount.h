#ifndef OSIEXCEPTIONCOUNT_H
#define OSIEXCEPTIONCOUNT_H
#include <Arduino.h>

#ifndef EXCEPTION_COUNT_ADDR
#define EXCEPTION_COUNT_ADDR 10     // EEPROM-Adresse für Ausnahmezähler
#endif

#ifndef MAX_EXCEPTIONS
#define MAX_EXCEPTIONS 3            // Max. erlaubte Anzahl an Ausnahmen
#endif

/**
 * @brief Klasse zur Verfolgung der Ausnahmen.
 * 
 * Diese Klasse zählt und speichert die während des Programmablaufs auftretenden Ausnahmen.
 * Es stehen Methoden zum Erhöhen der Zählung, Zurücksetzen und Lesen der aktuellen Zählung
 * sowie zum Persistieren dieser Werte zur Verfügung.
 */
class OSIExceptionCount {
private:
    uint8_t count = 0;               // 8-Bit Integer zur Speicheroptimierung
    uint8_t originalCount = 0;       // Original gespeicherte Zählung für Vergleich
    bool maintenanceMode = false;    // Wartungsmodus deaktiviert
    bool loggingMode = false;        // Protokollierung deaktiviert

    void loadCountAndCheck();        // Lädt Zählung aus EEPROM und prüft auf Plausibilität
    void printBootReason(uint32_t iReason); // Gibt den Grund des letzten Neustarts aus
public:
    OSIExceptionCount();             // Konstruktor: Lädt Zählung aus EEPROM über loadCountAndCheck()
    ~OSIExceptionCount();            // Destruktor wird nie aufgerufen
    void setup();
    void increase();                 // Zählt Ausnahmen um 1 hoch
    void reset();                    // Setzt die Zählung zurück
    uint8_t get();                   // Gibt aktuelle Zählung zurück
    void commit();                   // Schreibt Zählung ins EEPROM
    bool isMaintenanceMode();        // Überprüft, ob Wartungsmodus aktiviert werden soll
    bool isLoggingMode();            // Überprüft, ob Protokollierung aktiviert werden soll
};

extern OSIExceptionCount osiExceptionCount;
#endif // OSIEXCEPTIONCOUNT_H