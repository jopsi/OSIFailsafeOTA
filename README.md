# OSIFailsafeOTA

Here are the minimal settings for the platformio.ini:

build_flags = 
	;-D _DEBUG_ESP_HTTP_CLIENT=true
	-D _DEBUG_ESP_PORT=Serial
	;-D _DEBUG_ESP_WIFI=true
	-D USE_LITTLEFS=true
    -D USING_W5500=true
    -D USE_SPIFFS=false
	-D NO_GLOBAL_SERIAL=true
	-D NO_GLOBAL_SERIAL1=true
	-D MULTIPLEX_SERIAL=true
	-D USE_ASYNC_WEBSERVER=true
	-D MAX_EXCEPTIONS=2
lib_deps = 
	esphome/ESPAsyncWebServer-esphome
	OSIFailsafeOTA

# ESP8266/ESP32 Maintenance Mode

This project implements a maintenance mode for ESP8266/ESP32 microcontrollers. The maintenance mode monitors system performance, identifies boot errors, handles exceptions, and enables maintenance mode if the exceptions exceed a defined threshold. In maintenance mode, a developer can update the firmware via Over-the-Air (OTA) updates.

## Features

- Monitors exceptions and stores them in EEPROM for persistent error logging.
- Activates maintenance mode upon reaching a specified number of exceptions.
- Provides a web interface for configuring WiFi settings.
- Supports OTA updates for firmware and file system.
- Logs system information and exceptions to a file system (LittleFS).
- Allows file uploads and downloads via a web interface.

## Getting Started

### Prerequisites

- Arduino IDE with ESP8266/ESP32 board support.
- LittleFS library installed.
- A board with ESP8266 or ESP32 microcontroller.

### Installation

1. Clone this repository to your local machine:
	```sh
	git clone https://github.com/jopsi/esp-maintenance-mode.git
	```
2. Open the project in Arduino IDE.

3. Copy the `MaintenanceMode.h` and `MaintenanceMode.cpp` files to your project directory.

4. Include the `MaintenanceMode.h` file in your main program:
	```cpp
	#include "MaintenanceMode.h"
	```

### Usage

1. Initialize the maintenance logic in the `setup` function after initializing the Serial:
	```cpp
	void setup() {
		Serial.begin(115200);
		if (maintenanceMode.setup(JOP_USERNAME, JOP_PASSWORD)) return;
	}
	```

2. Add the maintenance logic in the `loop` function at the beginning:
	```cpp
	void loop() {
		if (shouldReboot) ESP.restart();
		if (maintenanceMode.loop()) return;
		delay(100);
	}
	```

### Configuration

- Define your username and password for the maintenance mode in the code:
	```cpp
	#ifndef JOP_USERNAME
	#define JOP_USERNAME "admin"
	#endif
	#ifndef JOP_PASSWORD
	#define JOP_PASSWORD "admin"
	#endif
	```

### Web Interface

- The maintenance mode provides a web server for configuring WiFi settings and managing files. Connect to the web server by accessing the IP address assigned to the ESP module.

### Exception Handling

- The custom crash callback function logs crash information to Serial and to a file.
- The `ExceptionCount` class tracks and stores the frequency of exceptions.
- The `MaintenanceMode` class manages the maintenance mode activation and OTA updates.

## File Structure

- `MaintenanceMode.h`: Header file for the maintenance mode.
- `MaintenanceMode.cpp`: Implementation file for the maintenance mode.
- `MultiplexHardwareSerial.h`: Header file for the multiplexed serial logging.
- `index_html`: Main HTML page for the web interface.
- `logout_html`: HTML page for logout.
- `reboot_html`: HTML page for reboot.
- `text_html`: Plain text MIME type.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgements

- [Arduino](https://www.arduino.cc/)
- [ESP8266/ESP32 Arduino Core](https://github.com/esp8266/Arduino)
- [LittleFS](https://github.com/lorol/LITTLEFS)

## Author

- Oliver Siepmann - [jopsi](https://github.com/jopsi)

