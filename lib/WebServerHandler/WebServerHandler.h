#ifndef WEB_SERVER_HANDLER_H
#define WEB_SERVER_HANDLER_H

#include <ESPAsyncWebServer.h>
#include <HueApiClient.h>
#include <NetatmoModels.h>

#define LOG_BUFFER_SIZE 5

class WebServerHandler {
public:
    WebServerHandler(AsyncWebServer &server);

    void setup();

    // New methods for switch attribution mode
    void setSwitchAttributionMode(bool enable);
    bool isSwitchAttributionModeEnabled() const;
    void setLastReceivedSwitchData(uint64_t data);
    uint64_t getLastReceivedSwitchData() const;

private:
    AsyncWebServer &_server;
    
    uint8_t* _importBuffer;
    size_t _importBufferLen;

    bool _switchAttributionModeEnabled = false;
    uint64_t _lastReceivedSwitchData = 0;

    static String processor(const String& var);
};

// Initialize the static member

#endif // WEB_SERVER_HANDLER_H
