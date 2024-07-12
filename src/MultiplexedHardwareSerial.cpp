#include "MultiplexedHardwareSerial.h"

void MultiplexedHardwareSerial::startFilesystem() {
    if (!LittleFS.begin()) {
        originalSerial.println(F("Failed to start LittleFS"));
    } else {
        originalSerial.println(F("LittleFS started"));
    }
    _fsStarted = true;
}   

void MultiplexedHardwareSerial::openFile() {
    startFilesystem();
    originalSerial.printf("Opening log file <%s>\n", _logFileName);
    file = LittleFS.open(_logFileName, (_append ? "a" : "w"));
    if (!file) {
        if (!_append && LittleFS.exists(_logFileName)) {
            originalSerial.printf_P(PSTR("Removing existing file <%s>\n"), _logFileName);
            LittleFS.remove(_logFileName);
            file = LittleFS.open(_logFileName, "w");
            if (!file) {
                originalSerial.printf_P(PSTR("Failed to open log file <%s>\n"), _logFileName);
            } else {
                originalSerial.printf_P(PSTR("Opened log file <%s>\n"), _logFileName);
            }
        } else {
            originalSerial.printf_P(PSTR("Failed to open log file <%s>\n"), _logFileName);
        }
    } else {
        originalSerial.printf_P(PSTR("Opened log file <%s>\n"), _logFileName);
    }
}

MultiplexedHardwareSerial::MultiplexedHardwareSerial(HardwareSerial& original, const char* filename, bool append, uint32_t maxFileSize, uint32_t maxFiles)
    : HardwareSerial(10), // Dummy initialization for the base class
    originalSerial(original) {
    _logFileName = filename;
    _maxFileSize = maxFileSize;
    _maxFiles = maxFiles;
    _append = append;
    _enabled = false;
    _fsStarted = false;
}

MultiplexedHardwareSerial::~MultiplexedHardwareSerial() {
    if (_enabled) {
        disableFileLog();
    }
}

void MultiplexedHardwareSerial::enableFileLog() {
    if (!_enabled) {
        openFile();
    } else {
        originalSerial.println(F("File logging already enabled"));
    }
    _enabled = true;
}

void MultiplexedHardwareSerial::disableFileLog() {
    if (_enabled) {
        file.flush();
        file.close();
    } else {
        originalSerial.println(F("File logging already disabled"));
    }
    _enabled = false;
}   

bool MultiplexedHardwareSerial::isEnabled() {
    return _enabled;
}
    
// New function to handle log file rotation
void MultiplexedHardwareSerial::rollLogFiles() {
    file.flush();
    file.close();

    // Rename existing files, e.g., log.txt -> log1.txt, log1.txt -> log2.txt, etc.
    for (int i = _maxFiles - 1; i > 0; i--) {
        char oldName[32];
        char newName[32];
        snprintf(oldName, sizeof(oldName), "%s%d", _logFileName, i - 1);
        snprintf(newName, sizeof(newName), "%s%d", _logFileName, i);
        originalSerial.printf_P(PSTR("Renaming %s to %s\n"), oldName, newName);
        if (LittleFS.exists(newName)) {
            originalSerial.printf_P(PSTR("Removing %s\n"), newName);
            LittleFS.remove(newName);
        }
        if (LittleFS.exists(oldName)) {
            originalSerial.printf_P(PSTR("Renaming %s to %s\n"), oldName, newName);
            LittleFS.rename(oldName, newName);
        }
    }

    // Rename the current log file to log0.txt
    char logFileNameZero[32];
    snprintf(logFileNameZero, sizeof(logFileNameZero), "%s0", _logFileName);
    if (LittleFS.exists(_logFileName)) {
        originalSerial.printf_P(PSTR("Renaming %s to %s\n"), _logFileName, logFileNameZero);
        LittleFS.rename(_logFileName, logFileNameZero);
    }

    // Open a new log file
    file = LittleFS.open(_logFileName, "w");
    if (!file) {
        originalSerial.println(F("Failed to open new log file"));
    }
}


// Update write methods to handle log rotation
size_t MultiplexedHardwareSerial::write(uint8_t c)  {
    if (_enabled) {
        if (file) {
            if (file.size() >= _maxFileSize) {
                rollLogFiles();
            }
            file.write(c);
            if (c == '\n') {
                file.flush();
            }
        }
    }
    return originalSerial.write(c);
}

size_t MultiplexedHardwareSerial::write(const uint8_t* buffer, size_t size)  {
    if (_enabled) {
        if (file) { 
            if (file.size() + size > _maxFileSize) {
                rollLogFiles();
            }
            file.write(buffer, size);
            if (buffer[size - 1] == '\n') {
                file.flush();
            }
        }
    }
    return originalSerial.write(buffer, size);
}
void MultiplexedHardwareSerial::begin(unsigned long baud)                   { originalSerial.begin(baud); }
void MultiplexedHardwareSerial::begin(unsigned long baud, SerialConfig config)                                                  { originalSerial.begin(baud, config); }
void MultiplexedHardwareSerial::begin(unsigned long baud, SerialConfig config, SerialMode mode)                                 { originalSerial.begin(baud, config, mode); }
void MultiplexedHardwareSerial::begin(unsigned long baud, SerialConfig config, SerialMode mode, uint8_t tx_pin)                 { originalSerial.begin(baud, config, mode, tx_pin); }
void MultiplexedHardwareSerial::begin(unsigned long baud, SerialConfig config, SerialMode mode, uint8_t tx_pin, bool invert)    { originalSerial.begin(baud, config, mode, tx_pin, invert); }
void MultiplexedHardwareSerial::end()                                       { originalSerial.end(); }
void MultiplexedHardwareSerial::updateBaudRate(unsigned long baud)          { originalSerial.updateBaudRate(baud); }
size_t MultiplexedHardwareSerial::setRxBufferSize(size_t size)              { return originalSerial.setRxBufferSize(size); }
size_t MultiplexedHardwareSerial::getRxBufferSize()                         { return originalSerial.getRxBufferSize(); }
bool MultiplexedHardwareSerial::swap()                                      { return originalSerial.swap(); }
bool MultiplexedHardwareSerial::swap(uint8_t tx_pin)                        { return originalSerial.swap(tx_pin); }
bool MultiplexedHardwareSerial::set_tx(uint8_t tx_pin)                      { return originalSerial.set_tx(tx_pin); }
bool MultiplexedHardwareSerial::pins(uint8_t tx, uint8_t rx)                { return originalSerial.pins(tx, rx); }
bool MultiplexedHardwareSerial::hasPeekBufferAPI () const                   { return originalSerial.hasPeekBufferAPI(); }
int MultiplexedHardwareSerial::available()                                  { return originalSerial.available(); }
int MultiplexedHardwareSerial::peek()                                       { return originalSerial.peek(); }
const char* MultiplexedHardwareSerial::peekBuffer ()                        { return originalSerial.peekBuffer(); }
size_t MultiplexedHardwareSerial::peekAvailable ()                          { return originalSerial.peekAvailable(); }
void MultiplexedHardwareSerial::peekConsume (size_t consume)                { originalSerial.peekConsume(consume);}
int MultiplexedHardwareSerial::read()                                       { return originalSerial.read(); }
int MultiplexedHardwareSerial::read(char* buffer, size_t size)              { return originalSerial.read(buffer, size); }
int MultiplexedHardwareSerial::read(uint8_t* buffer, size_t size)           { return originalSerial.read(buffer, size); }
size_t MultiplexedHardwareSerial::readBytes(char* buffer, size_t size)      { return originalSerial.readBytes(buffer, size); }
size_t MultiplexedHardwareSerial::readBytes(uint8_t* buffer, size_t size)   { return originalSerial.readBytes(buffer, size); }
int MultiplexedHardwareSerial::availableForWrite()                          { return originalSerial.availableForWrite(); }
void MultiplexedHardwareSerial::flush()                                     { originalSerial.flush(); }
size_t MultiplexedHardwareSerial::write(const char *str)                    { return write((const uint8_t *) str, strlen(str)); }
size_t MultiplexedHardwareSerial::write(const char *buffer, size_t size)    { return write((const uint8_t *) buffer, size); }
MultiplexedHardwareSerial::operator bool() const                            { return originalSerial.operator bool();  }
void MultiplexedHardwareSerial::setDebugOutput(bool debug)                  { originalSerial.setDebugOutput(debug);  }
bool MultiplexedHardwareSerial::isTxEnabled()                               { return originalSerial.isTxEnabled();  }
bool MultiplexedHardwareSerial::isRxEnabled()                               { return originalSerial.isRxEnabled();  }
int MultiplexedHardwareSerial::baudRate()                                   { return originalSerial.baudRate();  }
bool MultiplexedHardwareSerial::hasOverrun()                                { return originalSerial.hasOverrun(); }
bool MultiplexedHardwareSerial::hasRxError()                                { return originalSerial.hasRxError(); }
void MultiplexedHardwareSerial::startDetectBaudrate()                       { originalSerial.startDetectBaudrate(); }
unsigned long MultiplexedHardwareSerial::testBaudrate()                     { return originalSerial.testBaudrate(); }
unsigned long MultiplexedHardwareSerial::detectBaudrate(time_t timeoutMillis) { return originalSerial.detectBaudrate(timeoutMillis); }
