#ifndef MULTIPLEX_HARDWARE_SERIAL_H
#define MULTIPLEX_HARDWARE_SERIAL_H

#include <LittleFS.h>
#include <HardwareSerial.h>

class MultiplexedHardwareSerial : public HardwareSerial {
private:
    HardwareSerial& originalSerial;
    File file;
    bool _append;
    uint32_t _maxFileSize;
    uint32_t _maxFiles;
    const char* _logFileName;
    volatile bool _enabled = false;
    bool _fsStarted = false;

    void startFilesystem();
    void openFile();

public:
    MultiplexedHardwareSerial(HardwareSerial& original, const char* filename, bool append = true, uint32_t maxFileSize = 10 * 1024, uint32_t maxFiles = 5);

    virtual ~MultiplexedHardwareSerial();

    void enableFileLog();
    void disableFileLog();
    bool isEnabled();
    // New function to handle log file rotation
    void rollLogFiles();
    void begin(unsigned long baud);
    void begin(unsigned long baud, SerialConfig config);
    void begin(unsigned long baud, SerialConfig config, SerialMode mode);
    void begin(unsigned long baud, SerialConfig config, SerialMode mode, uint8_t tx_pin);
    void begin(unsigned long baud, SerialConfig config, SerialMode mode, uint8_t tx_pin, bool invert);
    void end();
    void updateBaudRate(unsigned long baud);
    size_t setRxBufferSize(size_t size);
    size_t getRxBufferSize();
    bool swap();
    bool swap(uint8_t tx_pin);
    bool set_tx(uint8_t tx_pin);
    bool pins(uint8_t tx, uint8_t rx);
    bool hasPeekBufferAPI () const override;
    int available() override;
    int peek() override;
    const char* peekBuffer () override;
    size_t peekAvailable () override;
    void peekConsume (size_t consume) override;
    int read() override;
    int read(char* buffer, size_t size);
    int read(uint8_t* buffer, size_t size) override;
    size_t readBytes(char* buffer, size_t size) override;
    size_t readBytes(uint8_t* buffer, size_t size) override;
    int availableForWrite() override;
    void flush() override;
    // Update write methods to handle log rotation
    virtual size_t write(uint8_t c) override;

    virtual size_t write(const uint8_t* buffer, size_t size) override;
    size_t write(const char *str);
    size_t write(const char *buffer, size_t size);

    operator bool() const;
    void setDebugOutput(bool debug);
    bool isTxEnabled();
    bool isRxEnabled();
    int baudRate();
    bool hasOverrun();
    bool hasRxError();
    void startDetectBaudrate();
    unsigned long testBaudrate();
    unsigned long detectBaudrate(time_t timeoutMillis);

};
#if (defined(MULTIPLEX_SERIAL))
extern MultiplexedHardwareSerial Serial;
#endif
#endif