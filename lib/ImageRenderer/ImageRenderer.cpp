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

bool ImageRenderer::renderImage(const NetatmoMeasureResponse &tempMain, const NetatmoMeasureResponse &tempModule) {
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
    _canvas.setColorDepth(16);

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

    // 4. Draw Temperature Charts
    if (tempMain.size > 0) {
        // BRG
        drawTemperatureChart(_canvas, tempMain, "Inside", 20, 50, 300, 150, TFT_WHITE, _canvas.color24to16(0x00ff00),
                             0.3f);
    }

    if (tempModule.size > 0) {
        drawTemperatureChart(_canvas, tempModule, "Outside", 20, 220, 300, 150, TFT_WHITE,
                             _canvas.color24to16(0x00ff00), 0.3f);
    }

    // 5. Encode back to JPEG (output to PSRAM)
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

    const uint8_t r = r_fg * alpha + r_bg * (1 - alpha);
    const uint8_t g = g_fg * alpha + g_bg * (1 - alpha);
    const uint8_t b = b_fg * alpha + b_bg * (1 - alpha);

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

void ImageRenderer::drawTemperatureChart(
    TFT_eSprite &canvas,
    const NetatmoMeasureResponse &temperature,
    const String &title,
    const uint16_t x, const uint16_t y, const uint16_t w, const uint16_t h,
    const uint32_t backgroundColor,
    const uint32_t lineColor,
    const float alpha
) {
    if (temperature.size < 1) return;

    drawAlphaRoundRect(canvas, x, y, w, h, 8, backgroundColor, alpha);

    if (temperature.size < 2) {
        // Just draw the current temp if not enough data for chart
        canvas.setTextColor(TFT_BLACK);
        canvas.setTextDatum(TL_DATUM);
        canvas.drawString(title, x + w - 50, y + 10, 2);
        canvas.setTextDatum(MC_DATUM);
        canvas.drawString(String(temperature.values[0], 1) + "°C", x + w / 2, y + h / 2, 4);
        return;
    }

    double minVal = temperature.values[0];
    double maxVal = temperature.values[0];

    for (int i = 1; i < temperature.size; i++) {
        if (temperature.values[i] < minVal) minVal = temperature.values[i];
        if (temperature.values[i] > maxVal) maxVal = temperature.values[i];
    }
    double minTemperature = minVal;
    double maxTemperature = maxVal;


    if (maxVal - minVal < 1.0) {
        maxVal = minVal + 1.0;
    }

    // Add some padding to min/max
    minVal -= 1.0;
    maxVal += 1.0;

    const int chartX = x + 35; // Leave space for Y axis labels
    const int chartY = y + 30; // Leave space for Title and current temp
    const int chartW = w - 50;
    const int chartH = h - 50; // Leave space for X axis

    auto getY = [&](const double val) -> int {
        return chartY + chartH - static_cast<int>((val - minVal) / (maxVal - minVal) * chartH);
    };

    canvas.setTextColor(TFT_BLACK);

    // Draw Title
    canvas.setTextDatum(TR_DATUM);
    canvas.setFreeFont(&FreeSans12pt7b);
    canvas.drawString(title, x + w - 10, y + 5);

    // Draw current temperature (last record) in bigger font
    canvas.setTextDatum(MC_DATUM);
    canvas.setFreeFont(&FreeSansBold18pt7b);
    canvas.drawString(String(temperature.values[temperature.size - 1], 1) + "°C", x + w / 2, y + 20);

    // Draw Axis
    canvas.drawLine(chartX, chartY, chartX, chartY + chartH, TFT_BLACK); // Y axis
    canvas.drawLine(chartX, chartY + chartH, chartX + chartW, chartY + chartH, TFT_BLACK); // X axis

    // Y Axis Labels
    canvas.setTextDatum(MR_DATUM);
    canvas.setFreeFont(&FreeSans9pt7b);
    canvas.drawString(String(maxTemperature, 1), chartX - 5, chartY);
    canvas.drawString(String(minTemperature, 1), chartX - 5, chartY + chartH - 10);

    // X Axis Labels
    canvas.setTextDatum(TC_DATUM);
    canvas.setFreeFont(&FreeSans9pt7b);
    canvas.drawString("-24h", chartX, chartY + chartH + 5);
    canvas.drawString("0h", chartX + chartW, chartY + chartH + 5);

    // Draw Line Chart
    for (int i = 0; i < temperature.size - 1; i++) {
        const int x1 = chartX + (i * chartW) / (temperature.size - 1);
        const int y1 = getY(temperature.values[i]);
        const int x2 = chartX + ((i + 1) * chartW) / (temperature.size - 1);
        const int y2 = getY(temperature.values[i + 1]);

        canvas.drawLine(x1, y1, x2, y2, lineColor);
        canvas.drawLine(x1, y1 + 1, x2, y2 + 1, lineColor); // thicker line
    }
}
