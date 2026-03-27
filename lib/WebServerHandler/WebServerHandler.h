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

    void setHueUsername(const char *hueUsername) {
        strncpy(_hueUsername, hueUsername, min(strlen(hueUsername) + 1, sizeof(_hueUsername) - 1));
    }

    void setNetatmoToken(const char* accessToken, const char* refreshToken, long expiresIn, unsigned long creationTimestamp) {
        _netatmoToken.accessToken = accessToken;
        _netatmoToken.refreshToken = refreshToken;
        _netatmoToken.expiresIn = expiresIn;
        _netatmoToken.creationTimestamp = creationTimestamp;
    }

    void setErrorMessage(const char *msg) {
        strncpy(_errorBuffer, msg, min(strlen(msg) + 1, sizeof(_errorBuffer) - 1));
    }

    void addLogMessage(const char *msg);

private:
    AsyncWebServer &_server;
    char _hueUsername[64];
    NetatmoToken _netatmoToken;
    char _errorBuffer[128];
    char _logBuffer[LOG_BUFFER_SIZE][128];
    size_t _logIndex = 0; // points to next write position
    size_t _logCount = 0; // number of valid entries (max 10)

    void handleStatus(AsyncWebServerRequest *request) const;

    void handleLogs(AsyncWebServerRequest *request) const;
    static String processor(const String& var);

    void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len,
                      bool final);
};

// Initialize the static member

#endif // WEB_SERVER_HANDLER_H
