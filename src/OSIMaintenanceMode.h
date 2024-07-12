#ifndef MaintenanceMode_H
#define MaintenanceMode_H

/*
Der bereitgestellte Quellcode implementiert eine MaintenanceMode-Klasse für ESP8266/ESP32-Mikrocontroller. 
Diese Klasse überwacht die Systemleistung, identifiziert Boot-Fehler, verwaltet Ausnahmefälle (Exceptions)
und aktiviert den Wartungsmodus, falls die Ausnahmen eine definierte Obergrenze überschreiten. Im 
Wartungsmodus kann ein Entwickler die Firmware über Over-the-Air (OTA) aktualisieren.

Die ExceptionCount-Klasse verfolgt die Häufigkeit von Ausnahmen und speichert diese in einem EEPROM-Speicher,
wodurch persistente Fehlerprotokolle beim Neustart verfügbar bleiben.

Einbindung in Projekte:
1. Kopieren Sie die Dateien MaintenanceMode.h und MaintenanceMode.cpp in das Projektverzeichnis.
2. Fügen Sie die Datei MaintenanceMode.h in das Hauptprogramm ein: #include "MaintenanceMode.h"
3. Initialisieren Sie die Wartungslogik im Setup direkt nach der Serial Initialisierung: 
        if (maintenanceMode.setup()) return;
4. Fügen Sie die Wartungslogik in die Hauptschleife direkt am Begin ein: 
        if (maintenanceMode.loop()) return;



riro@20240509
*/
#include <OSIExceptionCount.h>
#include <OSIMaintenanceModeWifi.h>
#include <OSIMaintenanceModeBestWifi.h>
#include <OSIFilemanager.h>
#include <OSIMTProfile.h>


/**
 * @brief Klasse für den Wartungsmodus.
 * 
 * Diese Klasse steuert den Wartungsmodus und enthält Methoden zum Registrieren
 * von OTA-Updates, Handhaben von OTA-Uploads und Drucken von Systeminformationen
 * sowie zur Prüfung der Wartungsbedingungen.
 */
class OSIMaintenanceMode {
private:
    OSIMaintenanceModeWifi *wifi;               // Zeiger auf WiFi-Instanz
    bool maintenanceMode = false;               // Gibt Wartungsmodus an
    static uint8_t ota_lastperc;                // Letzter Uploadfortschritt
    static bool ota_uploading;                  // Gibt an, ob OTA-Update läuft

public:
    OSIMaintenanceMode();                       // Konstruktor
    ~OSIMaintenanceMode();                      // Destruktor
    bool setup(const String username, const String password, uint32_t connectTimeout=10000);                    // Initialisierung der Wartungslogik
    bool loop();                                // Laufende Prüfung der Ausnahmen
    void registerOTA();                         // Registriert die OTA-Ereignisse
    bool handleOTAAndQuit();                    // Verarbeitet OTA-Ereignisse
};

extern OSIMaintenanceMode maintenanceMode;

#define OSIMAINTENANCE_SETUP(USERNAME, PASSWORD, TIMEOUT) { \
    bool ret = false; \
    osiExceptionCount.setup(); \
    MTProfile(ret = maintenanceMode.setup(USERNAME, PASSWORD, TIMEOUT)) \
    if (ret) return;\
}

#define OSIMAINTENANCE_LOOP    if (shouldReboot) ESP.restart(); \
    if (maintenanceMode.loop()) { yield(); return; }

#endif