//
// Created by Zelgius on 18-03-26.
//

#ifndef LIGHTCONTROLLER_IMAGERENDERER_H
#define LIGHTCONTROLLER_IMAGERENDERER_H

#include <NetatmoModels.h>
#include <OpenWeatherMapModels.h>

class ImageRenderer {
public:
    ImageRenderer();

    void begin();

    bool renderImage(
        const NetatmoMeasureResponse &tempMain,
        const NetatmoMeasureResponse &tempModule,
        const NetatmoMeasureResponse &pressureMain,
        const NetatmoMeasureResponse &humidityMain, const OWMForecastResponse &owmForecast
    );

    static ImageRenderer *instance;

private:
    static void drawLineChart(
        const NetatmoMeasureResponse &temperature,
        const String &title,
        const String &units, uint16_t x, uint16_t y, uint16_t w,
        uint16_t h, uint32_t lineColor
    );

    static void drawBarChart(
        const NetatmoMeasureResponse &pressure,
        const String &title,
        const String &units, uint16_t x, uint16_t y, uint16_t w,
        uint16_t h, uint32_t barColor
    );

    static void drawIcon(uint16_t x, uint16_t y, size_t width, size_t height, const uint8_t *image);

    static void drawWeather(uint16_t x, uint16_t y, uint16_t iconCode);

    static void drawWeatherForecast(
        const OWMForecastResponse &forecast,
        uint16_t x, uint16_t y, uint16_t w, uint16_t h
    );

};


#endif //LIGHTCONTROLLER_IMAGERENDERER_H
