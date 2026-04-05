#ifndef MOCK_DATA_H
#define MOCK_DATA_H

#if !defined(ENABLE_NETATMO) || !defined(ENABLE_OWM)

#include <NetatmoModels.h>
#include <OpenWeatherMapModels.h>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef ENABLE_OWM
extern NTPClient timeClient;
#endif

/**
 * Populates the given objects with mock data for testing and development.
 */
inline void populateMocks(
    NetatmoMeasureResponse &tempMain,
    NetatmoMeasureResponse &tempModule,
    NetatmoMeasureResponse &pressureMain,
    NetatmoMeasureResponse &humidityMain,
    OWMForecastResponse &owmForecast
) {
#ifndef ENABLE_NETATMO
    // Indoor Temperature (Main) - 24 hours of data
    tempMain.isModule = false;
    tempMain.size = 48;
    for (int i = 0; i < 48; ++i) {
        // Simulating a daily cycle around 21°C
        tempMain.values[i] = 21.0 + 1.5 * std::sin((i - 8) * M_PI / 12.0);
    }

    // Outdoor Temperature (Module) - 24 hours of data
    tempModule.isModule = true;
    tempModule.size = 48;
    for (int i = 0; i < 48; ++i) {
        // Simulating outdoor cycle around 12°C
        tempModule.values[i] = 12.0 + 6.0 * std::sin((i - 10) * M_PI / 12.0);
    }

    // Pressure (Main) - 24 hours of data
    pressureMain.isModule = false;
    pressureMain.size = 48;
    for (int i = 0; i < 48; ++i) {
        // Slowly rising pressure
        pressureMain.values[i] = 1012.0 + (i * 0.2);
    }

    // Humidity (Main) - Single current value
    humidityMain.isModule = false;
    humidityMain.size = 48;
    for (int i = 0; i < 48; ++i) {
        // Slowly rising pressure
        humidityMain.values[i] = 20.0 + (i * 0.2);
    }
#endif
#ifndef ENABLE_OWM
    // OWM Forecast - 5 days
    const long now = timeClient.getEpochTime(); // Fixed timestamp for consistency

    const int weatherIds[] = {800, 801, 500, 803, 800, 300, 700};
    for (int i = 0; i < 7; ++i) {
        WeatherData data;
        data.dt = now + i * 86400L;
        data.tempMin = 5.0 + i;
        data.tempMax = 15.0 + i;
        data.pop = (i == 2) ? 0.8 : 0.1; // High precipitation probability on day 3
        data.weatherId = weatherIds[i];
        owmForecast.forecast[i] = data;
    }
    owmForecast.count = 7;
#endif
}

#endif // MOCK_DATA_H
#endif // MOCK_DATA_H
