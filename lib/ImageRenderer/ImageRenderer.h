//
// Created by Zelgius on 18-03-26.
//

#ifndef LIGHTCONTROLLER_IMAGERENDERER_H
#define LIGHTCONTROLLER_IMAGERENDERER_H

#include <TFT_eSPI.h>
#include <JPEGenc.h>
#include "../NetatmoClient/NetatmoModels.h"

class ImageRenderer {
public:
    ImageRenderer();

    bool renderImage(const NetatmoMeasureResponse &tempMain, const NetatmoMeasureResponse &tempModule);

    uint8_t *getJpgOutput() const { return _jpgOutput; }
    size_t getJpgSize() const { return _jpgSize; }
    static ImageRenderer *instance;

private:
    TFT_eSPI _tft;
    TFT_eSprite _canvas;
    JPEGENC _encoder{};

    uint8_t *_jpgOutput = nullptr;
    size_t _jpgSize = 0;


    static void drawTemperatureChart(
        TFT_eSprite &canvas,
        const NetatmoMeasureResponse &temperature,
        const String &title,
        uint16_t x, uint16_t y, uint16_t w, uint16_t h,
        uint32_t backgroundColor,
        uint32_t lineColor,
        float alpha
    );

    static uint16_t alphaBlend565(uint16_t fg, uint16_t bg, float alpha);

    static void drawAlphaRoundRect(
        TFT_eSprite &canvas,
        uint16_t x, uint16_t y, uint16_t w, uint16_t h,
        uint16_t radius,
        uint16_t color,
        float alpha
    );

    // This bridge connects the global TJpg_Decoder to our specific Sprite
    static bool decode_callback(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap) {
        if (instance) {
            instance->_canvas.pushImage(x, y, w, h, bitmap);
            return true;
        }
        return false;
    }
};


#endif //LIGHTCONTROLLER_IMAGERENDERER_H
