#include "OpenWeatherMapClient.h"
#include "configuration.h"
#include "logger/task_logger.h"
#include "allocator/psram_allocator.h"

extern PsramAllocator allocator;

OpenWeatherMapClient::OpenWeatherMapClient(WiFiClientSecure& client) : _secureClient(client) {
}

OpenWeatherMapClient::~OpenWeatherMapClient() = default;

int OpenWeatherMapClient::getForecast(OWMForecastResponse& response) {
    String url = "https://api.openweathermap.org/data/2.5/forecast/daily?lat=";
    url += OWM_LAT;
    url += "&lon=";
    url += OWM_LON;
    url += "&cnt=7&units=metric&appid=";
    url += OWM_API_KEY;

    JsonDocument doc(&allocator);
    const int status = sendRequest(url, doc);
    if (status) {
        response.fromJson(doc);
    }
    return status;
}

bool OpenWeatherMapClient::sendRequest(const String &url, JsonDocument &responseDoc) {
    _http.begin(_secureClient, url);

    const int httpCode = _http.GET();
    if (httpCode > 0) {
        String responseBody = _http.getString();
        const DeserializationError error = deserializeJson(responseDoc, responseBody);
        if (error != DeserializationError::Ok) {
            LogEvent::post("Failed to deserialize OWM JSON: %s\n", error.c_str());
            return false;
        }
    } else {
        LogEvent::post("OWM HTTP GET failed: %s\n", HTTPClient::errorToString(httpCode).c_str());
        return false;
    }
    _http.end();
    return true;
}
