#ifndef OWM_MODELS_H
#define OWM_MODELS_H

#include <ArduinoJson.h>
#include <vector>

struct WeatherData {
    long dt;
    double tempMin;
    double tempMax;
    double pop; // Probability of precipitation (0.0 to 1.0)
    int weatherId; // Weather condition ID (worst for the day)
};

struct OWMForecastResponse {
    WeatherData forecast [7];
    uint8_t count;

    static OWMForecastResponse fromJson(const JsonDocument& doc) {
        OWMForecastResponse response;
        JsonArrayConst list = doc["list"];
        if (list) {
            response.count = 0;
            for (JsonObjectConst item : list) {
                WeatherData data;
                data.dt = item["dt"];
                data.tempMin = item["temp"]["min"];
                data.tempMax = item["temp"]["max"];
                data.pop = item["pop"];
                
                //  It is possible to meet more than one weather condition for a requested location.
                //  The first weather condition in API respond is primary
                JsonArrayConst weather = item["weather"];
                if (weather && weather.size() > 0) {
                    data.weatherId = weather[0]["id"];
                } else {
                    data.weatherId = 800; // Default to clear sky if missing
                }

                response.forecast[response.count] = data;
                ++response.count;

            }
        }
        return response;
    }
};

#endif // OWM_MODELS_H
