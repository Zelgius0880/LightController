//
// Created by Zelgius on 18-03-26.
//

#include "ImageRenderer.h"

#include "webserver/task_webserver.h"
#include <EPD_13in3e.h>
#include <GUI_Paint.h>

#define STBI_MALLOC ps_malloc
#define STBI_REALLOC ps_realloc
#define STBI_FREE free
#define STBI_NO_STDIO
#define STB_IMAGE_IMPLEMENTATION
#include <LittleFS.h>

#include "assets/clear_day.h"
#include "assets/cloudy.h"
#include "assets/cloudy_3_day.h"
#include "assets/fog_day.h"
#include "assets/haze.h"
#include "assets/rainy_1_small.h"
#include "assets/rainy_3.h"
#include "assets/rainy_3_day.h"
#include "assets/rain_and_snow_mix.h"
#include "assets/snowy_3.h"
#include "assets/thunderstorms.h"
#include "logger/task_logger.h"

extern SemaphoreHandle_t fsMutex;
#define IMAGE_FILENAME "/image.jpg"


ImageRenderer::ImageRenderer() {
    instance = this;
}

bool ImageRenderer::renderImage(
    const NetatmoMeasureResponse &tempMain,
    const NetatmoMeasureResponse &tempModule,
    const NetatmoMeasureResponse &pressureMain,
    const NetatmoMeasureResponse &humidityMain,
    const OWMForecastResponse &owmForecast
) {
    ::File file = LittleFS.open("/image.bin", "rb");
    if (!file || file.isDirectory()) {
        WebServerEvent::printLog("- Failed to open file for reading or it is a directory");
        return false;
    }

    uint8_t *image = nullptr;

    const auto size = file.size();

    if ((image = static_cast<uint8_t *>(ps_malloc(size))) == nullptr) {
        WebServerEvent::printLog("Failed to apply for black memory... . Requested: %d, Remaining: %d\n", size, ESP.getFreePsram());
        DEV_Module_Exit();
        return false;
    }
    Paint_NewImage(image, 1200, 1600, 0, WHITE);
    Paint_SetRotate(270);

    file.readBytes(reinterpret_cast<char *>(image), size);
    file.close();

    //EPD_13IN3E_Clear(EPD_13IN3E_WHITE);
    vTaskDelay(pdMS_TO_TICKS(500));

    Paint_SetScale(6);
    Paint_SelectImage(image);

    // 4. Draw Temperature Charts
    if (tempMain.size > 0) {
        // BRG
        drawLineChart(tempMain, "Inside", " C", 20, 420, 300, 150,EPD_13IN3E_RED);
    }

    if (tempModule.size > 0) {
        drawLineChart(tempModule, "Outside", " C", 20, 590, 300, 150,EPD_13IN3E_RED);
    }

    if (humidityMain.size > 0) {
        drawBarChart(humidityMain, "Humidity", " %", 20, 760, 300, 150,EPD_13IN3E_BLUE);
    }

    if (pressureMain.size > 0) {
        drawBarChart(pressureMain, "", " mbar", 20, 930, 300, 150,EPD_13IN3E_WHITE);
    }


    if (!owmForecast.forecast.empty()) {
        drawWeatherForecast(owmForecast, 895, 410 , 700, 150);
    }

    DEV_Module_Init();
    EPD_13IN3E_Init();
    vTaskDelay(pdMS_TO_TICKS(500));

    EPD_13IN3E_Display(image);
    vTaskDelay(pdMS_TO_TICKS(3000));

    EPD_13IN3E_Sleep();
    DEV_Delay_ms(2000);

    // close 5V
    LogEvent::post("close 5V, Module enters 0 power consumption ...\r\n");
    DEV_Module_Exit();

    free(image);
    return true;
}

void ImageRenderer::drawLineChart(
    const NetatmoMeasureResponse &temperature,
    const String &title,
    const String &units,
    const uint16_t x, const uint16_t y, const uint16_t w, const uint16_t h,
    const uint32_t lineColor
) {
    if (temperature.size < 1) return;

    //drawAlphaRoundRect(canvas, x, y, w, h, 8, backgroundColor, alpha);


    if (temperature.size < 2) {
        Paint_DrawString(x + w / 2, y + h / 2, (String(temperature.values[0], 1) + units).c_str(), &Font20,
                         EPD_13IN3E_WHITE, EPD_13IN3E_WHITE);

        return;
    }

    double minVal = temperature.values[0];
    double maxVal = temperature.values[0];

    for (int i = 1; i < temperature.size; i++) {
        if (temperature.values[i] < minVal) minVal = temperature.values[i];
        if (temperature.values[i] > maxVal) maxVal = temperature.values[i];
    }
    const double minTemperature = minVal;
    const double maxTemperature = maxVal;


    if (maxVal - minVal < 1.0) {
        maxVal = minVal + 1.0;
    }

    // Add some padding to min/max
    minVal -= 1.0;
    maxVal += 1.0;

    const int chartX = x + 45; // Leave space for Y axis labels
    const int chartY = y + 45; // Leave space for Title and current temp
    const int chartW = w - 65;
    const int chartH = h - 70; // Leave space for X axis

    auto getY = [&](const double val) -> int {
        return chartY + chartH - static_cast<int>((val - minVal) / (maxVal - minVal) * chartH);
    };

    // Draw Title
    Paint_DrawString(x + w - 10, y + 5, title.c_str(), &Font16,
                     EPD_13IN3E_WHITE, EPD_13IN3E_WHITE, TEXT_TR);


    // Draw current temperature (last record) in bigger font
    Paint_DrawString(x + w / 2, y + 25, String(temperature.values[temperature.size - 1], 1) + " C",
                     &Font24,EPD_13IN3E_WHITE, EPD_13IN3E_WHITE, TEXT_MC);
    // Draw Axis
    Paint_DrawLine(chartX, chartY, chartX, chartY + chartH, EPD_13IN3E_WHITE, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    Paint_DrawLine(chartX, chartY + chartH, chartX + chartW, chartY + chartH, EPD_13IN3E_WHITE, DOT_PIXEL_1X1,
                   LINE_STYLE_SOLID);

    // Y Axis Labels

    Paint_DrawString(chartX - 5, chartY, String(maxTemperature, 1),
                     &Font12,EPD_13IN3E_WHITE, EPD_13IN3E_WHITE, TEXT_MR);

    Paint_DrawString(chartX - 5, chartY + chartH, String(minTemperature, 1),
                     &Font12,EPD_13IN3E_WHITE, EPD_13IN3E_WHITE, TEXT_MR);
    // X Axis Labels
    Paint_DrawString(chartX, chartY + chartH + 5, "-24h",
                     &Font12,EPD_13IN3E_WHITE, EPD_13IN3E_WHITE, TEXT_TC);

    Paint_DrawString(chartX + chartW, chartY + chartH + 5, "0h",
                     &Font12,EPD_13IN3E_WHITE, EPD_13IN3E_WHITE, TEXT_TC);
    // Draw Line Chart
    for (int i = 0; i < temperature.size - 1; i++) {
        const int x1 = chartX + (i * chartW) / (temperature.size - 1);
        const int y1 = getY(temperature.values[i]);
        const int x2 = chartX + ((i + 1) * chartW) / (temperature.size - 1);
        const int y2 = getY(temperature.values[i + 1]);

        Paint_DrawLine(x1, y1, x2, y2, lineColor, DOT_PIXEL_2X2, LINE_STYLE_SOLID);

        Paint_DrawLine(x1, y1 + 1, x2, y2 + 1, lineColor, DOT_PIXEL_2X2, LINE_STYLE_SOLID);
    }
}

void ImageRenderer::drawBarChart(
    const NetatmoMeasureResponse &pressure,
    const String &title,
    const String &units,
    const uint16_t x, const uint16_t y, const uint16_t w, const uint16_t h,
    const uint32_t barColor
) {
    if (pressure.size < 1) return;

    if (pressure.size < 2) {
        // Just draw the current pressure if not enough data for chart
        Paint_DrawString(title, x + w - 10, y + 5, &Font16, EPD_13IN3E_WHITE,
                         TEXT_TR);

        Paint_DrawString(String(pressure.values[0], 1) + units, x + w / 2, y + h / 2, &Font24, EPD_13IN3E_WHITE,
                         TEXT_MC);
        return;
    }

    double minVal = pressure.values[0];
    double maxVal = pressure.values[0];

    for (int i = 1; i < pressure.size; i++) {
        if (pressure.values[i] < minVal) minVal = pressure.values[i];
        if (pressure.values[i] > maxVal) maxVal = pressure.values[i];
    }
    const double minPressure = minVal;
    const double maxPressure = maxVal;

    if (maxVal - minVal < 2.0) {
        maxVal = minVal + 2.0;
    }

    // Add some padding to min/max
    minVal -= 2.0;
    maxVal += 2.0;

    const int chartX = x + 55; // Leave space for Y axis labels (pressure is usually ~1013)
    const int chartY = y + 45; // Leave space for Title and current value
    const int chartW = w - 75;
    const int chartH = h - 70; // Leave space for X axis

    auto getY = [&](const double val) -> int {
        return chartY + chartH - static_cast<int>((val - minVal) / (maxVal - minVal) * chartH);
    };

    Paint_DrawString(title, x + w - 10, y + 5, &Font16, EPD_13IN3E_WHITE, TEXT_TR);

    // Draw current pressure (last record) in bigger font

    Paint_DrawString(String(pressure.values[pressure.size - 1], 0) + units, x + w / 2, y + 25, &Font24,
                     EPD_13IN3E_WHITE, TEXT_MC);

    // Draw Axis
    Paint_DrawLine(chartX, chartY, chartX, chartY + chartH, EPD_13IN3E_WHITE, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    // Y axis
    Paint_DrawLine(chartX, chartY + chartH, chartX + chartW, chartY + chartH, EPD_13IN3E_WHITE, DOT_PIXEL_1X1,
                   LINE_STYLE_SOLID); // X axis

    // Y Axis Labels
    Paint_DrawString(String(maxPressure, 0), chartX - 5, chartY, &Font12, EPD_13IN3E_WHITE, TEXT_MR);
    Paint_DrawString(String(minPressure, 0), chartX - 5, chartY + chartH, &Font12, EPD_13IN3E_WHITE, TEXT_MR);

    // X Axis Labels
    Paint_DrawString("-24h", chartX, chartY + chartH + 5, &Font12, EPD_13IN3E_WHITE, TEXT_TR);
    Paint_DrawString("0h", chartX + chartW, chartY + chartH + 5, &Font12, EPD_13IN3E_WHITE, TEXT_TR);

    // Draw Bar Chart
    const int barGap = 2;
    const int barWidth = (chartW / pressure.size) - barGap;

    for (int i = 0; i < pressure.size; i++) {
        const int bx = chartX + (i * chartW) / pressure.size + barGap / 2;
        const int by = getY(pressure.values[i]);
        const int bh = chartY + chartH - by;
        if (bh > 0) {
            Paint_DrawRectangle(bx, by, bx + (barWidth > 0 ? barWidth : 1), by + bh, barColor, DOT_PIXEL_1X1,
                                barWidth > 1 ? DRAW_FILL_FULL : DRAW_FILL_EMPTY);
        }
    }
}

void ImageRenderer::drawIcon(const uint16_t x, const uint16_t y, size_t width, size_t height, const uint8_t *image) {
    uint16_t colors[] = {
        EPD_13IN3E_BLACK,
        EPD_13IN3E_WHITE,
        EPD_13IN3E_YELLOW,
        EPD_13IN3E_RED,
        EPD_13IN3E_BLUE,
        EPD_13IN3E_GREEN,

    };

    for (uint32_t i = 0; i < width * height; i++) {
        const uint8_t pixelVal = image[i];

        // 3. Handle Transparency
        // If the value is 0, we skip it to keep the background visible.
        if (pixelVal == 0) continue;

        uint16_t color = colors[pixelVal - 1];

        // 4. Calculate X and Y coordinates
        // i % width gives the column, i / width gives the row.
        UWORD xPos = x + (i % width);
        UWORD yPos = y + (i / width);

        Paint_SetPixel(xPos, yPos, color);
    }
}

void ImageRenderer::drawWeather(const uint16_t x, const uint16_t y, const uint16_t iconCode) {
    size_t width, height;
    const uint8_t *image;

    if (iconCode >= 200 && iconCode < 300) {
        width = thunderstorms_width;
        height = thunderstorms_height;
        image = thunderstorms_data;
    } else if (iconCode >= 300 && iconCode < 500) {
        width = rainy_3_width;
        height = rainy_3_height;
        image = rainy_3_data;
    } else if (iconCode >= 500 && iconCode < 600) {
        width = rainy_3_day_width;
        height = rainy_3_day_height;
        image = rainy_3_day_data;
    } else if (iconCode >= 600 && iconCode < 700) {
        width = snowy_3_width;
        height = snowy_3_height;
        image = snowy_3_data;
        if (iconCode >= 611 && iconCode <= 616) {
            width = rain_and_snow_mix_width;
            height = rain_and_snow_mix_height;
            image = rain_and_snow_mix_data;
        }
    } else if (iconCode >= 700 && iconCode < 800) {
        width = haze_width;
        height = haze_height;
        image = haze_data;
        if (iconCode == 741 || iconCode <= 701) {
            width = fog_day_width;
            height = fog_day_height;
            image = fog_day_data;
        }
    } else if (iconCode == 800) {
        width = clear_day_width;
        height = clear_day_height;
        image = clear_day_data;
    } else if (iconCode == 801) {
        width = cloudy_3_day_width;
        height = cloudy_3_day_height;
        image = cloudy_3_day_data;
    } else {
        width = cloudy_width;
        height = cloudy_height;
        image = cloudy_data;
    }

    drawIcon(x, y, width, height, image);
}

void ImageRenderer::drawWeatherForecast(
    const OWMForecastResponse &forecast,
    const uint16_t x, const uint16_t y, const uint16_t w, const uint16_t h
) {
    if (forecast.forecast.empty()) return;

    const int padding = 5;
    const int count = std::min((int) forecast.forecast.size(), 7);
    const int itemWidth = (w - 2 * padding) / count;

    // Current time for "Today" check
    time_t now = timeClient.getEpochTime();
    struct tm *nowInfo = localtime(&now);
    int todayYday = nowInfo->tm_yday;
    int todayYear = nowInfo->tm_year;

    for (int i = 0; i < count; i++) {
        const auto &data = forecast.forecast[i];
        int ix = x + padding + i * itemWidth;
        int iy = y + 10;

        // Day of week
        time_t rawtime = data.dt;
        struct tm *timeinfo = localtime(&rawtime);

        char dayBuffer[16];
        if (timeinfo->tm_yday == todayYday && timeinfo->tm_year == todayYear) {
            strcpy(dayBuffer, "Today");
        } else {
            strftime(dayBuffer, sizeof(dayBuffer), "%a", timeinfo); // Short day name
        }

        Paint_DrawString(dayBuffer, ix + itemWidth / 2, iy, &Font24, EPD_13IN3E_WHITE, TEXT_TC);

        // Precipitation %
        String popStr = String((int) (data.pop * 100)) + "%";
        drawIcon(ix  , iy + 22, rainy_1_width, rainy_1_height, rainy_1_data);
        Paint_DrawString(popStr, ix + itemWidth / 2 + 20, iy + 30, &Font16, EPD_13IN3E_WHITE, TEXT_TC);

        constexpr int iconSize = 72;
        const int iconX = ix + (itemWidth - iconSize) / 2;
        const int iconY = iy + 35; // Positioned below the % text

        drawWeather(iconX, iconY, data.weatherId);


        // Min/Max Temp
        String tempStr = String(data.tempMax, 0) + "/" + String(data.tempMin, 0);
        Paint_DrawString(tempStr, ix + itemWidth / 2, y + h - 5, &Font20, EPD_13IN3E_WHITE, TEXT_BC);
    }
}