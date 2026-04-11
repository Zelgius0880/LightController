#ifndef OPENWEATHERMAPCLIENT_H
#define OPENWEATHERMAPCLIENT_H

#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include "OpenWeatherMapModels.h"

class OpenWeatherMapClient {
public:
    explicit OpenWeatherMapClient(WiFiClientSecure& client);
    ~OpenWeatherMapClient();

    int getForecast(OWMForecastResponse& response);

private:
    WiFiClientSecure& _secureClient;
    HTTPClient _http;

    bool sendRequest(const String &url, JsonDocument &responseDoc);
};

#endif // OPENWEATHERMAPCLIENT_H
