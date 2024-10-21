#include <OSIFilemanager.h>
#include <OSIMTProfile.h>
#ifdef ESP32
#include "SPIFFS.h"                 // SPIFFS-Dateisystem für ESP32
#define LittleFS SPIFFS 
#else
#include "LittleFS.h"               // LittleFS-Dateisystem für ESP8266
#endif

OSIFileManager* OSIFileManager::instance = nullptr;
volatile bool shouldReboot = false;

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html lang="en">
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <meta charset="UTF-8">
  <title>ESP8266 File Manager</title>
</head>
<body>
  <p>Main page</p>
  <p>Firmware: %FIRMWARE%</p>
  <p>Free Storage: <span id="freespiffs">%FREESPIFFS%</span> | Used Storage: <span id="usedspiffs">%USEDSPIFFS%</span> | Total Storage: <span id="totalspiffs">%TOTALSPIFFS%</span></p>
  <p>
    <button onclick="logoutButton()">Logout</button>
    <button onclick="rebootButton()">Reboot</button>
    <button onclick="listFilesButton()">List Files</button>
    <button onclick="showUploadButton()">Upload File</button>
    <!--<button onclick="showFileContentButton()">Show File Content</button>-->
  </p>
  <p id="status"></p>
  <p id="detailsheader"></p>
  <p id="details"></p>
<script>
function _(el) { return document.getElementById(el); }
function makeRequest(method, url, async = true) { const xhr = new XMLHttpRequest(); xhr.open(method, url, async); xhr.send(); return xhr; }
function upEl(id, content) { _(id).innerHTML = content;}
function logoutButton() { makeRequest("GET", "/logout"); setTimeout(() => window.open("/logged-out", "_self"), 1000); }
function rebootButton() { upEl("status", "Invoking Reboot ..."); makeRequest("GET", "/reboot"); window.open("/reboot","_self"); }
// listFilesRemoteCallAddCell 
function lifiAdd(row, elemtype, value) { header = document.createElement(elemtype); header.setAttribute("align", "left"); header.innerText = value; row.appendChild(header); }
// listFilesRemoteCallAddBtn
function lifiBtn(row, filename, fnct) { 
  tdata = document.createElement("td"); row.appendChild(tdata); button = document.createElement("button"); tdata.appendChild(button);
  button.setAttribute("onclick", "dlDBtn(\'" + filename + "\', \'" + fnct + "\')"); button.innerText = fnct;
}
function listFilesRemoteCall() {
  xmlhttp = makeRequest("GET", "/listfiles", false);
  var details = _("details");
  details.childNodes.forEach(function(item, index) {
    details.removeChild(item);
  })
  table = document.createElement("table"); row = document.createElement("tr");
  table.appendChild(row);
  lifiAdd(row, "th", "Name"); lifiAdd(row, "th", "Size"); lifiAdd(row, "th", ""); lifiAdd(row, "th", "");

  const myLines = xmlhttp.responseText.split("~"); // Seperate Lines
  
  myLines.forEach(function (item, index) {
    row = document.createElement("tr");
    table.appendChild(row);
    if (item == "" || item.indexOf("*") == -1) return;
    const myFile = item.split("*");
    lifiAdd(row, "td", myFile[0]); lifiAdd(row, "td", myFile[1]); lifiBtn(row, myFile[0], "Download"); lifiBtn(row, myFile[0], "Delete"); lifiBtn(row, myFile[0], "Show");
    console.log(item, index);
  });
  document.getElementById("details").appendChild(table);
}
function listFilesButton() { listFilesRemoteCall(); upEl("detailsheader", "<h3>Files</h3>"); }
function dlDBtn(filename, action) {
  var urltocall = "/file?name=" + filename + "&action=" + action;
  if (action == "del" || action == "Delete") {
    makeRequest("GET", urltocall, false);
    upEl("status", xmlhttp.responseText);
    listFilesRemoteCall();
  }
  if (action == "dnl" || action == "Download") {
    upEl("status", "Downloading " + filename);
    window.open(urltocall, "_self");
  }
  if (action == "shw" || action == "Show") {
    var xhr = new XMLHttpRequest();
    xhr.open("GET", urltocall, true);
    xhr.onload = function() {
      if (xhr.status === 200) {
        upEl("detailsheader", "<h3>File Content: " + filename + "</h3>");
        document.getElementById("details").innerText = xhr.responseText;
        //upEl("details", xhr.responseText);
      } else {
        alert("Failed to fetch file content. Make sure the file exists.");
      }
    };
    xhr.send();
  }
}
function uploadFile() {
  var file = _("file1").files[0];
  var formdata = new FormData();
  formdata.append("file1", file);
  var ajax = new XMLHttpRequest();
  ajax.upload.addEventListener("progress", function(event) {
    _("loaded_n_total").innerHTML = "Uploaded " + event.loaded + " bytes";
    var percent = (event.loaded / event.total) * 100;
    _("progressBar").value = Math.round(percent);
    _("status").innerHTML = Math.round(percent) + "% uploaded... please wait";
    if (percent >= 100) _("status").innerHTML = "Please wait, writing file to filesystem";
  });
  ajax.addEventListener("load", function() { upEl("status", "Upload Complete"); _("progressBar").value = 0; listFilesButton(); });
  ajax.addEventListener("error", function() { upEl("status", "Upload Failed"); });
  ajax.addEventListener("abort", function() { upEl("status", "Upload Aborted"); });
  ajax.open("POST", "/upload");
  ajax.send(formdata);
}
function showUploadButton() {
  upEl("detailsheader", "<h3>Upload File</h3>");
  upEl("status", "");
  upEl("details",'<form id="upload_form" action="/upload" enctype="multipart/form-data" method="post"><input type="file" name="file1" id="file1" onchange="uploadFile()"><br><progress id="progressBar" value="0" max="100" style="width:300px;"></progress><h3 id="status"></h3><p id="loaded_n_total"></p></form>');
}
</script>
</body>
</html>
)rawliteral";   

const char logout_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html lang="en">
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <meta charset="UTF-8">
</head>
<body>
  <p><a href="/">Log Back In</a></p>
</body>
</html>
)rawliteral";

const char reboot_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html lang="en">
<head>
  <meta charset="UTF-8">
</head>
<body>
<h3>
  Rebooting, returning to main page in <span id="countdown">30</span> seconds
</h3>
<script type="text/javascript">
  var seconds = 20;
  function countdown() {
    seconds = seconds - 1;
    if (seconds < 0) {
      window.location = "/fm";
    } else {
      document.getElementById("countdown").innerHTML = seconds;
      window.setTimeout("countdown()", 1000);
    }
  }
  countdown();
</script>
</body>
</html>
)rawliteral";

const char text_html[] PROGMEM = R"rawliteral(
text/plain
)rawliteral";

void OSIFileManager::updateFSStat() {
#ifdef ESP8266    
    FSInfo fs_info;
    LittleFS.info(fs_info);
    totBytes = fs_info.totalBytes;
    usedBytes = fs_info.usedBytes;
#else    
    totBytes = LittleFS.totalBytes();
    usedBytes = LittleFS.usedBytes();
#endif
}

void OSIFileManager::rebootESP(String message) {
    Serial.printf_P(PSTR("Rebooting ESP: %s"), message.c_str()); 
    ESP.restart();
}

#if !defined(USE_ASYNC_WEBSERVER)
void OSIFileManager::send(int code, String message) {
    myserver.send(code, "text/html", message);
}
#else
void OSIFileManager::send(AsyncWebServerRequest *request, int code, String message) {
    request->send(code, "text/html", message);
}
#endif    
    
#if !defined(USE_ASYNC_WEBSERVER)    
void OSIFileManager::logClientAndURL(const __FlashStringHelper *text) {
    Serial.printf_P(PSTR("Client: %s %s %s\n"), myserver.client().remoteIP().toString().c_str(), 
                                                myserver.uri().c_str(), 
                                                (text != nullptr ? text : F("")));   
}
#else
void OSIFileManager::logClientAndURL(AsyncWebServerRequest *request, const __FlashStringHelper *text) {
    Serial.printf_P(PSTR("Client: %s %s %s\n"), request->client()->remoteIP().toString().c_str(), 
                                                request->url().c_str(), 
                                                (text != nullptr ? String(text).c_str() : String(F(""))).c_str());
}
#endif

#if !defined(USE_ASYNC_WEBSERVER)
bool OSIFileManager::checkUserWebAuth(bool noLogging) {
    bool isAuthenticated = false;
    OSIFileManager *fm = OSIFileManager::getInstance();
    if (fm == nullptr) {
        if (!noLogging) Serial.println(F("ERROR: OSIFileManager instance is null"));
        return false;
    }
    if (myserver.authenticate(fm->username.c_str(), fm->password.c_str())) {
        if (!noLogging) logClientAndURL(F(" Auth: Success"));
        isAuthenticated = true;
    } else {
        if (!noLogging) logClientAndURL(F(" Auth: Failed"));
        myserver.requestAuthentication();
    }
    return isAuthenticated;
}

void OSIFileManager::notFound() {
    logClientAndURL();
    myserver.send(404, FPSTR(text_html), PSTR("Not found"));
}
#else

String OSIFileManager::getRequestParameterByName(AsyncWebServerRequest *request, const char *name) {
    int paramsNr = request->params();
    for(int i = 0; i < paramsNr; i++) {
        AsyncWebParameter* p = request->getParam(i);
#ifdef DEBUG_REQUESTPARAM        
        Serial.print(F("Param name: "));
        Serial.print(p->name());
        Serial.print(F("  Param value: "));
        Serial.println(p->value());
#endif        
        if (String(p->name()).equals(name)) {
            return (p->value());
        }
    }
    return String("");
}

bool OSIFileManager::existsRequestParameterByName(AsyncWebServerRequest *request, const char *name) {
    bool flag = false;
    int paramsNr = request->params();
    for(int i = 0; i < paramsNr; i++) {
        AsyncWebParameter* p = request->getParam(i);
#ifdef DEBUG_REQUESTPARAM        
        Serial.print(F("Param name: "));
        Serial.print(p->name());
        Serial.print(F("  Param value: "));
        Serial.println(p->value());
#endif        
        if (String(p->name()).equals(name)) {
            flag = true;
            break;
        }
    }
    return flag;
}

bool OSIFileManager::checkUserWebAuth(AsyncWebServerRequest *request, bool noLogging) {
    bool isAuthenticated = false;
    OSIFileManager *fm = OSIFileManager::getInstance();
    if (fm == nullptr) {
        if (!noLogging) Serial.println(F("ERROR: OSIFileManager instance is null"));
        return false;
    }
    if (request->authenticate(fm->username.c_str(), fm->password.c_str())) {
        if (!noLogging) logClientAndURL(request, F(" Auth: Success"));
        isAuthenticated = true;
    } else {
        if (!noLogging) logClientAndURL(request, F(" Auth: Failed"));
        request->requestAuthentication();
    }
    return isAuthenticated;
}

void OSIFileManager::notFound(AsyncWebServerRequest *request) {
    logClientAndURL(request, F(" Not Found - 404"));
    request->send(404, F("text/html"), F("Not found"));
}
#endif


String OSIFileManager::humanReadableSize(size_t bytes) {
    if (bytes < 1024) return String(bytes) + " B";
    else if (bytes < (1024 * 1024)) return String(bytes / 1024.0) + " KB";
    else if (bytes < (1024 * 1024 * 1024)) return String(bytes / 1024.0 / 1024.0) + " MB";
    else return String(bytes / 1024.0 / 1024.0 / 1024.0) + " GB";
}

String OSIFileManager::processor(const String& var) {
    if (var == F("FIRMWARE")) return FIRMWARE_VERSION;
    updateFSStat();
    if (var == F("FREESPIFFS")) return humanReadableSize(totBytes - usedBytes);
    if (var == F("USEDSPIFFS")) return humanReadableSize(usedBytes);
    if (var == F("TOTALSPIFFS")) return humanReadableSize(totBytes);
    return String();
}

String OSIFileManager::processTemplate(const char* templateStr) {
    String html = FPSTR(templateStr);
    html.replace(F("%FIRMWARE%"), processor(F("FIRMWARE")));
    html.replace(F("%FREESPIFFS%"), processor(F("FREESPIFFS")));
    html.replace(F("%USEDSPIFFS%"), processor(F("USEDSPIFFS")));
    html.replace(F("%TOTALSPIFFS%"), processor(F("TOTALSPIFFS")));
    return html;
}

#if !defined(USE_ASYNC_WEBSERVER)       
// handles uploads to the filserver
void OSIFileManager::handleUpload() {
    // make sure authenticated before allowing upload
    if (!checkUserWebAuth(true)) return;

    HTTPUpload& upload = myserver.upload();
    LittleFS.begin();
    String filename = "/" + upload.filename;
    File file = LittleFS.open(filename, (UPLOAD_FILE_START ? "w" : "a"));
    if (!file) {
        Serial.printf_P(PSTR("Failed to open file for writing <%s>\n"), filename.c_str());
    }else{
        if (upload.status == UPLOAD_FILE_START) {
            Serial.printf_P(PSTR("Upload File Name: %s\n"), filename.c_str());
        } else {
            if (upload.status == UPLOAD_FILE_WRITE) {
                // Write the received bytes to the file
                file.write(upload.buf, upload.currentSize);
                Serial.print(F("."));
            } else {
                if (upload.status == UPLOAD_FILE_END) {
                    // Close the file and print the result
                    Serial.printf_P(PSTR("\nUpload Size: %d\n"), upload.totalSize);
                    myserver.sendHeader("Location", "/fm");
                    myserver.send(303);
                } else {
                    if (upload.status == UPLOAD_FILE_ABORTED) {
                        Serial.printf_P(PSTR("Upload was Aborted.\n"));
                        file.close();
                        LittleFS.remove(filename);
                    }
                }
            }
        }
        file.close();
    }
}
#else
// handles uploads to the filserver
void OSIFileManager::handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
    // make sure authenticated before allowing upload
    if (checkUserWebAuth(request)) {
        logClientAndURL(request, F("upload"));

        if (!index) {
            char buf[64];
            // open the file on first call and store the file handle in the request object
            memset(buf, 0, 64);
            snprintf_P(buf, 63, PSTR("/%s"), filename.c_str());
            request->_tempFile = LittleFS.open(buf, "w");
            Serial.print(F("Upload Start: ")); Serial.println(filename);
        }

        if (len) {
            // stream the incoming chunk to the opened file
            request->_tempFile.write(data, len);
            Serial.print(F("Writing file: ")); Serial.print(filename); Serial.print(F(" index=")); Serial.print(index);  Serial.print(F(" len=")); Serial.println(len);
        }

        if (final) {
            // close the file handle as the upload is now done
            request->_tempFile.close();
            request->redirect("/fm");
            Serial.print(F("Upload Complete: ")); Serial.print(filename); Serial.print(F(",size: ")); Serial.println(index + len);
        }
    } else {
        Serial.println(F("Auth: Failed"));
        return request->requestAuthentication();
    }
}
/*
void OSIFileManager::handleUpload() {
    // make sure authenticated before allowing upload
    if (!checkUserWebAuth(true)) return;

    AsyncWebServerRequest *request = myserver.upload();
    LittleFS.begin();
    String filename = "/" + request->url();
    File file = LittleFS.open(filename, (UPLOAD_FILE_START ? "w" : "a"));
    if (!file) {
        Serial.printf_P(PSTR("Failed to open file for writing <%s>\n"), filename.c_str());
    }else{
        if (request->method() == HTTP_POST) {
            request->onUpload([](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
                if (!index) {
                    Serial.printf_P(PSTR("Upload File Name: %s\n"), filename.c_str());
                }
                if (file.write(data, len) != len) {
                    Serial.printf_P(PSTR("Write file failed: %s\n"), filename.c_str());
                }
                if (final) {
                    Serial.printf_P(PSTR("\nUpload Size: %d\n"), index + len);
                    request->send(200);
                }
            });
        }
        file.close();
    }
}
*/
#endif
void OSIFileManager::configureWebServer() {
    // if url isn't found
    myserver.onNotFound(notFound);

#if !defined(USE_ASYNC_WEBSERVER)
    myserver.onFileUpload([]() {
        OSIFileManager::handleUpload();
        myserver.send(200, FPSTR(text_html), F("File Uploaded"));
    });
#else
    myserver.onFileUpload([](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
        OSIFileManager::handleUpload(request, filename, index, data, len, final);
    });
#endif

    // run handleUpload function when any file is uploaded
#if !defined(USE_ASYNC_WEBSERVER)
    myserver.on(PSTR("/upload"), HTTP_POST, []() {
        OSIFileManager::getInstance()->send(200, F("File Uploaded"));
    }, OSIFileManager::handleUpload);

    // visiting this page will cause you to be logged out
    myserver.on(PSTR("/logout"), HTTP_GET, [this]() {
        myserver.requestAuthentication();
        myserver.send(401);
    });
    
    // presents a "you are now logged out webpage
    myserver.on(PSTR("/logged-out"), HTTP_GET, [this]() {
        logClientAndURL();
        OSIFileManager::getInstance()->send(200, FPSTR(logout_html));
    });
    
    myserver.on(PSTR("/fm"), HTTP_GET, [this]() {
        if (!checkUserWebAuth()) return;
        OSIFileManager::getInstance()->send(200, processTemplate(index_html));
    });

    myserver.on(PSTR("/reboot"), HTTP_GET, [this]() {
        if (!checkUserWebAuth()) return;
        OSIFileManager::getInstance()->send(200, FPSTR(reboot_html));
        shouldReboot = true;
    });

    myserver.on(PSTR("/listfiles"), HTTP_GET, [this]() {
        if (!checkUserWebAuth()) return;
        OSIFileManager::getInstance()->send(200, OSIFileManager::getInstance()->listFiles(true));
    });

#else
    myserver.on(PSTR("/upload"), HTTP_POST, [](AsyncWebServerRequest *request) {
        OSIFileManager::getInstance()->send(request, 200, F("File Uploaded"));
    }, OSIFileManager::handleUpload);

    // visiting this page will cause you to be logged out
    myserver.on(PSTR("/logout"), HTTP_GET, [](AsyncWebServerRequest *request) {
        request->requestAuthentication();
        request->send(401);
    });

    // presents a "you are now logged out webpage
    myserver.on(PSTR("/logged-out"), HTTP_GET, [](AsyncWebServerRequest *request) {
        logClientAndURL(request, nullptr);
        OSIFileManager::getInstance()->send(request, 200, FPSTR(logout_html));
    });

    myserver.on(PSTR("/fm"), HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (!checkUserWebAuth(request)) return;
        OSIFileManager::getInstance()->send(request, 200, processTemplate(index_html));
    });

    myserver.on(PSTR("/reboot"), HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (!checkUserWebAuth(request)) return;
        OSIFileManager::getInstance()->send(request, 200, FPSTR(reboot_html));
        shouldReboot = true;
    });

    myserver.on(PSTR("/listfiles"), HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (!checkUserWebAuth(request)) return;
        OSIFileManager::getInstance()->send(request, 200, OSIFileManager::getInstance()->listFiles(true));
    });

#endif

#if !defined(USE_ASYNC_WEBSERVER)
    myserver.on(PSTR("/file"), HTTP_GET, [this]() {
        if (!checkUserWebAuth()) return;
        if (!myserver.hasArg(F("name")) || !myserver.hasArg(F("action"))) {
            OSIFileManager::getInstance()->send(400, F("ERROR: name and action params required"));
#else
    myserver.on(PSTR("/file"), HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (!checkUserWebAuth(request)) return;
        if (!request->hasArg(F("name")) || !request->hasArg(F("action"))) {
            OSIFileManager::getInstance()->send(request, 400, F("ERROR: name and action params required"));
#endif        
            return;
        }

#if !defined(USE_ASYNC_WEBSERVER)
        String fileName = myserver.arg(F("name"));
        const char *fileAction = myserver.arg(F("action")).c_str();
#else
        String fileName = request->arg(F("name"));
        const char *fileAction = request->arg(F("action")).c_str();
#endif
        // Make sure filename ends with only one newline
        while (fileName.endsWith("\n")) fileName.remove(fileName.length() - 1);

        Serial.printf_P(PSTR("?name=%s&action=%s\n"), fileName.c_str(), fileAction);   
        if (!LittleFS.exists(fileName)) {
            Serial.println(F(" ERROR: file does not exist"));
#if !defined(USE_ASYNC_WEBSERVER)
            OSIFileManager::getInstance()->send(400, F("ERROR: file does not exist"));
#else
            OSIFileManager::getInstance()->send(request, 400, F("ERROR: file does not exist"));
#endif
        } else {
            if (strcmp(fileAction, "dnl") == 0 || strcmp(fileAction, "Download") == 0) {
                File file = LittleFS.open(fileName, "r");
#if !defined(USE_ASYNC_WEBSERVER)
                myserver.streamFile(file, "application/octet-stream");
#else
                request->send(LittleFS, fileName, String(F("application/octet-stream")));
#endif
                file.close();
            } else if (strcmp(fileAction, "del") == 0 || strcmp(fileAction, "Delete") == 0) {
                LittleFS.remove(fileName);
#if !defined(USE_ASYNC_WEBSERVER)                
                OSIFileManager::getInstance()->send(200, String(F("Deleted File: ")) + fileName);
#else
                OSIFileManager::getInstance()->send(request, 200, String(F("Deleted File: ")) + fileName);
#endif
            } else if (strcmp(fileAction, "shw") == 0 || strcmp(fileAction, "Show") == 0) {
                File file = LittleFS.open(fileName, "r");
                String content = file.readString();
                file.close();
#if !defined(USE_ASYNC_WEBSERVER)
                OSIFileManager::getInstance()->send(200, content);
#else
                OSIFileManager::getInstance()->send(request, 200, content);
#endif
            } else {
                Serial.println(F(" ERROR: invalid action param supplied"));
#if !defined(USE_ASYNC_WEBSERVER)
                OSIFileManager::getInstance()->send(400, String(F("ERROR: invalid action param supplied")));
#else
                OSIFileManager::getInstance()->send(request, 400, String(F("ERROR: invalid action param supplied")));  
#endif
            }
        }
    });
    Serial.println(F("Webserver configured"));
}

OSIFileManager::OSIFileManager() {
}

void OSIFileManager::initFileSystem() {
    Serial.print(F("Firmware: ")); 
    Serial.println(FIRMWARE_VERSION);
    Serial.println(F("Booting ..."));
    Serial.println(F("Mounting LittleFS ..."));
    if (!LittleFS.begin()) {
        // if you have not used SPIFFS before on a ESP32, it will show this error.
        // after a reboot SPIFFS will be configured and will happily work.
        Serial.println(F("ERROR: Cannot mount LittleFS, Rebooting"));
        rebootESP("ERROR: Cannot mount LittleFS, Rebooting");
    }
    updateFSStat();
    Serial.printf_P(PSTR("LittleFS Free: %s\n"), humanReadableSize((totBytes - usedBytes)));
    Serial.printf_P(PSTR("LittleFS Used: %s\n"), humanReadableSize(usedBytes));
    Serial.printf_P(PSTR("LittleFS Total: %s\n"), humanReadableSize(totBytes));
    MTProfile(Serial.println(listFiles()))
}

// list all of the files, if ishtml=true, return html rather than simple text
String OSIFileManager::listFiles(bool ishtml) {
    String returnText = "";
    Serial.println(F("Listing files stored on LittleFS"));
    File root = LittleFS.open("/", "r");
    File foundfile = root.openNextFile();

    while (foundfile) {
        const char *c = foundfile.name();
        if (c == nullptr)
            Serial.println(F("ERROR: found file name is null."));
        String filename = c;
        filename.trim();
        if (ishtml) {
            returnText += filename + String(F("*")) + humanReadableSize(foundfile.size()) + String(F("~"));
        } else {
            returnText += String(F("File: ")) + filename + String(F(" Size: ")) + humanReadableSize(foundfile.size()) + String(F("\n"));
        }
        foundfile = root.openNextFile();
    }
    root.close();
    foundfile.close();
    return returnText;
}

OSIFileManager *OSIFileManager::getInstance() { 
    if (instance == nullptr) 
        instance = new OSIFileManager(); 
    return instance; 
}

void OSIFileManager::setUsernameAndPassword(const String username, const String password) {
    this->username = username;
    this->password = password;
}

void OSIFileManager::setupWebserver() {
    Serial.println(F("Loading Configuration ..."));
    Serial.println(F("\n\nNetwork Configuration:"));
    Serial.println(F("----------------------"));
    Serial.print(F("         SSID: ")); Serial.println(WiFi.SSID());
    Serial.print(F("  Wifi Status: ")); Serial.println(WiFi.status());
    Serial.print(F("Wifi Strength: ")); Serial.print(WiFi.RSSI()); Serial.println(F(" dBm"));
    Serial.print(F("          MAC: ")); Serial.println(WiFi.macAddress());
    Serial.print(F("           IP: ")); Serial.println(WiFi.localIP());
    Serial.print(F("       Subnet: ")); Serial.println(WiFi.subnetMask());
    Serial.print(F("      Gateway: ")); Serial.println(WiFi.gatewayIP());
    Serial.print(F("        DNS 1: ")); Serial.println(WiFi.dnsIP(0));
    Serial.print(F("        DNS 2: ")); Serial.println(WiFi.dnsIP(1));
    Serial.print(F("        DNS 3: ")); Serial.println(WiFi.dnsIP(2));
    Serial.println();

    // configure web server
    Serial.println(F("Configuring Webserver ..."));
    configureWebServer();
}