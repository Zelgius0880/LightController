//
// Created by Zelgius on 18-03-26.
//

#ifndef LIGHTCONTROLLER_IMAGERENDERER_H
#define LIGHTCONTROLLER_IMAGERENDERER_H

#include <NetatmoModels.h>
#include <OpenMeteoModels.h>

#include "GUI_Paint.h"

struct OpenMeteoForecastResponse;

class ImageRenderer {
public:
    ImageRenderer();

    bool renderImage(
        const NetatmoMeasureResponse &tempMain,
        const NetatmoMeasureResponse &tempModule,
        const NetatmoMeasureResponse &pressureMain,
        const NetatmoMeasureResponse &humidityMain, const OpenMeteoForecastResponse &owmForecast
    );

    static ImageRenderer *instance;

private:
    GuiPaint _paint;
    void drawLineChart(
        const NetatmoMeasureResponse &temperature,
        const String &title,
        const String &units, uint16_t x, uint16_t y, uint16_t w,
        uint16_t h, uint32_t lineColor
    ) const;

    void drawBarChart(
        const NetatmoMeasureResponse &pressure,
        const String &title,
        const String &units, uint16_t x, uint16_t y, uint16_t w,
        uint16_t h, uint32_t barColor
    ) const;

    void drawIcon(uint16_t x, uint16_t y, const uint8_t *image, size_t width = 72, size_t height = 72) const;

    void drawWeather(uint16_t x, uint16_t y, uint16_t iconCode) const;

    void drawWeatherForecast(
        const OpenMeteoForecastResponse &forecast,
        uint16_t x = 895, uint16_t y = 410, uint16_t w = 700, uint16_t h = 150
    ) const;

};


#endif //LIGHTCONTROLLER_IMAGERENDERER_H
