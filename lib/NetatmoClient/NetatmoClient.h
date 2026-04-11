#ifndef NETATMOCLIENT_H
#define NETATMOCLIENT_H

#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <NetatmoModels.h>

class NetatmoClient {
public:
    NetatmoClient(WiFiClientSecure& client);

    // API methods
    bool getToken(const String& code);
    bool refreshToken();
    
    struct MeasureParams {
        String deviceId;
        String moduleId;
        String scale = "30min";
        unsigned long dateEnd = 0;
        unsigned long dateBegin = 0;
        bool optimize = false;
        bool realTime = true;
        String type = "temperature";
    };

    int getMeasure(const MeasureParams& params, NetatmoMeasureResponse& response);

    // Convenience helpers for common queries (24h @ 30min => <= 48 samples)
    int getLast24hTemperature(const String& moduleId, NetatmoMeasureResponse& response);
    int getLast24hPressure(const String& moduleId, NetatmoMeasureResponse& response);
    int getLast24hHumidity(NetatmoMeasureResponse& response); // main device (no module), single latest value

    // Lifecycle
    bool begin();
    bool isAuthenticated();
    
    void loadToken();
    void saveToken() const;

    const NetatmoToken& getTokenInfo() const { return _token; }

private:
    NetatmoToken _token;
    WiFiClientSecure& _secureClient;
    HTTPClient _http;

    bool sendRequest(const String &method, const String &url, const String &body, JsonDocument &responseDoc,
                     bool authenticated = true);
    bool handleTokenResponse(const JsonDocument &doc);

    static MeasureParams makeLast24hParams(const String& type, const String& moduleId = "") ;
};

#endif // NETATMOCLIENT_H
