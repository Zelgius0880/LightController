#ifndef OPENMETEO_MODELS_H
#define OPENMETEO_MODELS_H

#include <ArduinoJson.h>

struct WeatherData {
    long dt;
    double tempMin;
    double tempMax;
    double pop; // Probability of precipitation (0.0 to 1.0)
    int weatherId; // Weather condition ID (WMO code)
};

struct OpenMeteoForecastResponse {
    WeatherData forecast[7];
    uint8_t count;

    void fromJson(const JsonDocument &doc) {
        JsonVariantConst daily = doc["daily"];
        count = 0;
        if (daily) {
            JsonArrayConst time = daily["time"];
            JsonArrayConst weather_code = daily["weather_code"];
            JsonArrayConst temperature_2m_max = daily["temperature_2m_max"];
            JsonArrayConst temperature_2m_min = daily["temperature_2m_min"];
            JsonArrayConst precipitation_probability_max = daily["precipitation_probability_max"];

            if (time) {
                const size_t timeSize = time.size();
                for (size_t i = 0; i < timeSize && count < 7; i++) {
                    forecast[count].dt = time[i];
                    forecast[count].weatherId = weather_code[i];
                    forecast[count].tempMax = temperature_2m_max[i];
                    forecast[count].tempMin = temperature_2m_min[i];
                    forecast[count].pop = static_cast<double>(precipitation_probability_max[i].as<int>()) / 100.0;
                    
                    count++;
                }
            }
        }
    }
};

#endif // OPENMETEO_MODELS_H
