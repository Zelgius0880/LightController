#ifndef OPENMETEOCLIENT_H
#define OPENMETEOCLIENT_H

#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include "OpenMeteoModels.h"

class OpenMeteoClient {
public:
    explicit OpenMeteoClient();
    ~OpenMeteoClient();

    bool getForecast(OpenMeteoForecastResponse& response);

private:
    WiFiClientSecure _secureClient;
    HTTPClient _http;

    bool sendRequest(const String &url, JsonDocument &responseDoc);
};

#endif // OPENMETEOCLIENT_H
