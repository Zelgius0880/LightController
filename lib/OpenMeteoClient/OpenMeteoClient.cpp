#include "OpenMeteoClient.h"
#include "configuration.h"
#include "logger/task_logger.h"
#include "allocator/psram_allocator.h"

extern PsramAllocator allocator;

OpenMeteoClient::OpenMeteoClient() : _secureClient() {
    _secureClient.setInsecure();
}

OpenMeteoClient::~OpenMeteoClient() = default;

bool OpenMeteoClient::getForecast(OpenMeteoForecastResponse& response) {
    String url = "https://api.open-meteo.com/v1/forecast?latitude=";
    url += OPEN_METEO_LAT;
    url += "&longitude=";
    url += OPEN_METEO_LON;
    url += "&daily=weather_code,temperature_2m_max,temperature_2m_min,precipitation_probability_max&timezone=";
    url += OPEN_METEO_TIMEZONE;
    url += "&timeformat=unixtime";

    JsonDocument doc(&allocator);
    const int status = sendRequest(url, doc);
    if (status) {
        response.fromJson(doc);
    }
    return status;
}

bool OpenMeteoClient::sendRequest(const String &url, JsonDocument &responseDoc) {
    _http.setReuse(false);
    _http.begin(_secureClient, url);

    const int httpCode = _http.GET();
    if (httpCode > 0) {
        String responseBody = _http.getString();
        const DeserializationError error = deserializeJson(responseDoc, responseBody);
        if (error != DeserializationError::Ok) {
            LogEvent::post("Failed to deserialize OpenMeteo JSON: %s\n", error.c_str());
            return false;
        }
    } else {
        LogEvent::post("OpenMeteo HTTP GET failed: %s\n", HTTPClient::errorToString(httpCode).c_str());
        return false;
    }
    _http.end();
    return true;
}
