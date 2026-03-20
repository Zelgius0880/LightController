#ifndef WEB_SERVER_HANDLER_H
#define WEB_SERVER_HANDLER_H

#include <ESPAsyncWebServer.h>
#include <HueApiClient.h>

#define LOG_BUFFER_SIZE 5

class WebServerHandler {
public:
    WebServerHandler(AsyncWebServer &server);

    void setup();

    void setHueUsername(const char *hueUsername) {
        strncpy(_hueUsername, hueUsername, min(strlen(hueUsername) + 1, sizeof(_hueUsername) - 1));
    }

    void setErrorMessage(const char *msg) {
        strncpy(_errorBuffer, msg, min(strlen(msg) + 1, sizeof(_errorBuffer) - 1));
    }

    void addLogMessage(const char *msg);

private:
    AsyncWebServer &_server;
    char _hueUsername[64];
    char _errorBuffer[128];
    char _logBuffer[LOG_BUFFER_SIZE][128];
    size_t _logIndex = 0; // points to next write position
    size_t _logCount = 0; // number of valid entries (max 10)

    // Static pointer for the decoder callback

    void handleRoot(AsyncWebServerRequest *request);

    void handleStatus(AsyncWebServerRequest *request) const;

    void handleImage(AsyncWebServerRequest *request);

    void handleLogs(AsyncWebServerRequest *request) const;

    void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len,
                      bool final);
};

// Initialize the static member

#endif // WEB_SERVER_HANDLER_H
