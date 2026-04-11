#ifndef OWM_MODELS_H
#define OWM_MODELS_H

#include <ArduinoJson.h>

struct WeatherData {
    long dt;
    double tempMin;
    double tempMax;
    double pop; // Probability of precipitation (0.0 to 1.0)
    int weatherId; // Weather condition ID (worst for the day)
};

struct OWMForecastResponse {
    WeatherData forecast[7];
    uint8_t count;

    void fromJson(const JsonDocument &doc) {
        const JsonArrayConst list = doc["list"];
        count = 0;
        if (list) {
            for (JsonObjectConst item: list) {
                if (count >= 7) break;
                forecast[count].dt = item["dt"];
                forecast[count].tempMin = item["temp"]["min"];
                forecast[count].tempMax = item["temp"]["max"];
                forecast[count].pop = item["pop"];

                //  It is possible to meet more than one weather condition for a requested location.
                //  The first weather condition in API respond is primary
                JsonArrayConst weather = item["weather"];
                if (weather && weather.size() > 0) {
                    forecast[count].weatherId = weather[0]["id"];
                } else {
                    forecast[count].weatherId = 800; // Default to clear sky if missing
                }

                ++count;
            }
        }
    }
};

#endif // OWM_MODELS_H
