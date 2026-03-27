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
    std::vector<WeatherData> forecast;

    static OWMForecastResponse fromJson(const JsonDocument& doc) {
        OWMForecastResponse response;
        JsonArrayConst list = doc["list"];
        if (list) {
            for (JsonObjectConst item : list) {
                WeatherData data;
                data.dt = item["dt"];
                data.tempMin = item["temp"]["min"];
                data.tempMax = item["temp"]["max"];
                data.pop = item["pop"];
                
                // Find the worst weather ID (lowest ID usually corresponds to more severe conditions,
                // but OWM actually categorizes them; for simple logic, we'll take the first one or
                // implement worst-case if there were multiple. Daily forecast usually has one main entry.)
                JsonArrayConst weather = item["weather"];
                if (weather && weather.size() > 0) {
                    data.weatherId = weather[0]["id"];
                } else {
                    data.weatherId = 800; // Default to clear sky if missing
                }

                response.forecast.push_back(data);
            }
        }
        return response;
    }
};

#endif // OWM_MODELS_H
