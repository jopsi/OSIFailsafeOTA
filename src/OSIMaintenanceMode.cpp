#include <OSIMaintenanceMode.h>
#include <ArduinoOTA.h>
#include <OSIMTProfile.h>

bool OSIMaintenanceMode::ota_uploading = false;  // OTA-Upload-Fortschritt
uint8_t OSIMaintenanceMode::ota_lastperc = 0;    // Letzter OTA-Fortschritt

OSIMaintenanceMode maintenanceMode;  // Instanz des Wartungsmodus

#if !defined(USE_ASYNC_WEBSERVER)
#include <ESP8266WebServer.h>
ESP8266WebServer myserver;
#else
#include <ESPAsyncWebServer.h>
AsyncWebServer myserver(80);
#endif

/**
 * @brief Registriert die OTA-Handler für verschiedene Ereignisse
 */
void OSIMaintenanceMode::registerOTA() {
    ArduinoOTA.onStart([]() {
        ota_uploading = true;
        Serial.print(F("Start OTA update -> Updating "));
        if (ArduinoOTA.getCommand() == U_FLASH) {
            Serial.println(F("sketch"));
        } else {
            Serial.println(F("filesystem"));
            //LittleFS.end();
        }
        ota_lastperc = 0;
    });

    ArduinoOTA.onEnd([]() {
        Serial.println(F("\nEnd"));
        ota_uploading = false;
    });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        uint8_t a = progress / (total / 100);
        if (a != ota_lastperc) {
            Serial.printf_P(PSTR("Progress: %u%%\r\n"), a);
            ota_lastperc = a;
        }
        ota_uploading = true;
    });

    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf_P(PSTR("Error[%u]: "), error);
        if (error == OTA_AUTH_ERROR) Serial.println(F("Auth Failed"));
        else if (error == OTA_BEGIN_ERROR) Serial.println(F("Begin Failed"));
        else if (error == OTA_CONNECT_ERROR) Serial.println(F("Connect Failed"));
        else if (error == OTA_RECEIVE_ERROR) Serial.println(F("Receive Failed"));
        else if (error == OTA_END_ERROR) Serial.println(F("End Failed"));
        ota_uploading = false;
    });

    ArduinoOTA.begin();
}

/**
 * @brief Verarbeitet OTA-Ereignisse und kehrt zurück, falls ein Upload aktiv ist
 * @return True, wenn OTA-Upload aktiv
 */
bool OSIMaintenanceMode::handleOTAAndQuit() {
    ArduinoOTA.handle();
    if (ota_uploading) {
        delay(2);
        return true;
    }
    return false;
}

/**
 * @brief Konstruktor der MaintenanceMode-Klasse
 */
OSIMaintenanceMode::OSIMaintenanceMode() : wifi(nullptr) {
}

/**
 * @brief Destruktor der MaintenanceMode-Klasse
 */
OSIMaintenanceMode::~OSIMaintenanceMode() {
}

void OSIMaintenanceMode::initWifiAndOTA(const String username, const String password, uint32_t connectTimeout) {
    wifi = new OSIMaintenanceModeWifi(username, password, connectTimeout);
    registerOTA();
}


/**
 * @brief Setzt den Wartungsmodus auf und überprüft die Boot-Gründe.
 * 
 * @return True, wenn Wartungsmodus aktiviert wurde.
 */
bool OSIMaintenanceMode::setup(const String username, const String password, uint32_t connectTimeout) {
    Serial.println(F("Failsafe over the air (FOTA) update for ESP8266 and ESP32 devices"));


    // Überprüfung ob Wartungsmodus aktiviert werden soll
    if (osiExceptionCount.isMaintenanceMode()) {
        osiExceptionCount.reset();
        osiExceptionCount.commit();
        // WiFi-Instanz für Wartungsmodus initialisieren
        initWifiAndOTA(username, password, connectTimeout);
        // Keine verzögerte Speicherung des Ausnahmezähler erforderlich
        maintenanceMode = true;
        Serial.println(F("Maintenance mode activated"));
    } else {
        maintenanceMode = false;
        Serial.println(F("Normal operation mode"));
    }
    return maintenanceMode;
}

/**
 * @brief Führt die laufende Wartungsprüfung und OTA-Verarbeitung durch.
 * 
 * @return True, wenn Wartungsmodus aktiv.
 */
bool OSIMaintenanceMode::loop() {
    if (handleOTAAndQuit()) 
        return true;
    if (wifi != nullptr) {
        wifi->loop();
    }
    return maintenanceMode;
}

OSIMaintenanceModeWifi *OSIMaintenanceMode::getWifi() {
    return wifi;
}






