//
// Created by Zelgius on 18-03-26.
//

#include "ImageRenderer.h"

#include "logger/task_logger.h"
#include "webserver/task_webserver.h"
#include <TJpg_Decoder.h>

extern SemaphoreHandle_t fsMutex;
#define IMAGE_FILENAME "/image.jpg"

ImageRenderer::ImageRenderer() : _canvas(&_tft) {
    instance = this;
}

bool ImageRenderer::renderImage() {
    uint16_t w, h;

    if (xSemaphoreTake(fsMutex, pdMS_TO_TICKS(1000))) {
        const JRESULT result = TJpgDec.getFsJpgSize(&w, &h, IMAGE_FILENAME, LittleFS);
        if (result != JDR_OK) {
            xSemaphoreGive(fsMutex);
            WebServerEvent::printLog("Failed to get image %d \n", result);
            LogEvent::post("Failed to get image %d \n", result);
            return false;
        }
        xSemaphoreGive(fsMutex);
    } else {
        WebServerEvent::printLog("Failed to take fsMutex for getFsJpgSize");
        return false;
    }

    // 1. Prepare Canvas in PSRAM
    _canvas.setAttribute(PSRAM_ENABLE, true);
    if (!_canvas.createSprite(w, h)) {
        WebServerEvent::printLog("Failed to create Sprite");
        LogEvent::post("Failed to create Sprite");
        return false;
    }

    // 2. Decode file into Sprite
    TJpgDec.setCallback(decode_callback);
    if (xSemaphoreTake(fsMutex, pdMS_TO_TICKS(1000))) {
        TJpgDec.drawFsJpg(0, 0, IMAGE_FILENAME, LittleFS);
        xSemaphoreGive(fsMutex);
    } else {
        WebServerEvent::printLog("Failed to take fsMutex for drawFsJpg");
        LogEvent::post("Failed to take fsMutex for drawFsJpg");
        _canvas.deleteSprite();
        return false;
    }

    const int bw = 160;
    // 3. Draw "Hello World" rounded overlay
    const int bh = 45;
    int bx = (w - bw) / 2;
    int by = 30;

    drawAlphaRoundRect(
        _canvas,
        bx, by,
        bw, bh,
        12, // radius
        TFT_WHITE, // color
        0.7f // alpha
    );

    _canvas.setTextColor(TFT_BLACK);
    _canvas.setTextDatum(MC_DATUM);
    _canvas.drawString("Hello World", bx + (bw / 2), by + (bh / 2), 4);

    // 4. Encode back to JPEG (output to PSRAM)
    if (_jpgOutput) free(_jpgOutput);
    _jpgOutput = static_cast<uint8_t *>(ps_malloc(w * h / 4)); // Guessing 25% compression size

    _encoder.open(_jpgOutput, w * h / 4);

    JPEGENCODE jpe;
    _encoder.encodeBegin(&jpe, w, h, JPEGE_PIXEL_RGB565, JPEGE_SUBSAMPLE_420, JPEGE_Q_HIGH);
    _encoder.addFrame(&jpe, static_cast<uint8_t *>(_canvas.getPointer()), w * 2);
    _jpgSize = _encoder.close();

    _canvas.deleteSprite(); // Clean up raw buffer
    return true;
}


uint16_t ImageRenderer::alphaBlend565(const uint16_t fg, uint16_t bg, const float alpha) {
    uint8_t r_fg = (fg >> 11) & 0x1F;
    uint8_t g_fg = (fg >> 5) & 0x3F;
    uint8_t b_fg = fg & 0x1F;

    uint8_t r_bg = (bg >> 11) & 0x1F;
    uint8_t g_bg = (bg >> 5) & 0x3F;
    uint8_t b_bg = bg & 0x1F;

    uint8_t r = r_fg * alpha + r_bg * (1 - alpha);
    uint8_t g = g_fg * alpha + g_bg * (1 - alpha);
    uint8_t b = b_fg * alpha + b_bg * (1 - alpha);

    return (r << 11) | (g << 5) | b;
}

void ImageRenderer::drawAlphaRoundRect(
    TFT_eSprite &canvas,
    const uint16_t x, const uint16_t y, const uint16_t w, const uint16_t h,
    const uint16_t radius,
    const uint16_t color,
    float alpha
) {
    if (alpha <= 0.0f) return;
    if (alpha > 1.0f) alpha = 1.0f;

    const auto buf = static_cast<uint16_t *>(canvas.getPointer());
    const int canvasW = canvas.width();

    // Precompute alpha (0–255)
    const uint8_t a = alpha * 255;
    const uint8_t invA = 255 - a;

    // Extract foreground color
    const uint8_t r_fg = (color >> 11) & 0x1F;
    const uint8_t g_fg = (color >> 5) & 0x3F;
    const uint8_t b_fg = color & 0x1F;

    for (int iy = 0; iy < h; iy++) {
        for (int ix = 0; ix < w; ix++) {
            int px = x + ix;
            int py = y + iy;

            // --- Bounds check ---
            if (px < 0 || py < 0 || px >= canvasW || py >= canvas.height()) continue;

            // --- Rounded corner clipping ---
            if (radius > 0) {
                int dx = 0, dy = 0;

                if (ix < radius) dx = radius - ix;
                else if (ix >= w - radius) dx = ix - (w - radius - 1);

                if (iy < radius) dy = radius - iy;
                else if (iy >= h - radius) dy = iy - (h - radius - 1);

                if ((dx > 0 || dy > 0) && (dx * dx + dy * dy > radius * radius)) {
                    continue;
                }
            }

            // --- Background pixel ---
            const uint16_t bg = buf[py * canvasW + px];

            const uint8_t r_bg = (bg >> 11) & 0x1F;
            const uint8_t g_bg = (bg >> 5) & 0x3F;
            const uint8_t b_bg = bg & 0x1F;

            // --- Blend (integer math) ---
            const uint8_t r = (r_fg * a + r_bg * invA) >> 8;
            const uint8_t g = (g_fg * a + g_bg * invA) >> 8;
            const uint8_t b = (b_fg * a + b_bg * invA) >> 8;

            buf[py * canvasW + px] = (r << 11) | (g << 5) | b;
        }
    }
}
