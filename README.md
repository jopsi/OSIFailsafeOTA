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