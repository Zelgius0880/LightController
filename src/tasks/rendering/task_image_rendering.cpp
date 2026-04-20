//
// Created by Zelgius on 18-03-26.
//

#include <rendering/task_image_rendering.h>
#include <Arduino.h>
#include <esp_task_wdt.h>

#include <ImageRenderer.h>
#include <WiFiClientSecure.h>

#include <NetatmoClient.h>
#include <OpenMeteoClient.h>
#include "logger/task_logger.h"
#include "configuration.h"

#ifndef NETATMO_UPDATE_INTERVAL_SECONDS
#define NETATMO_UPDATE_INTERVAL_SECONDS 20 * 60
#endif

#ifndef OPEN_METEO_UPDATE_INTERVAL_SECONDS
#define OPEN_METEO_UPDATE_INTERVAL_SECONDS 60 * 60
#endif
#include "esp_sntp.h"

extern QueueHandle_t renderingQueue;

SemaphoreHandle_t ImageRendererEvent::completionSemaphore = nullptr;
ImageRenderer *ImageRenderer::instance = nullptr;

ImageRenderer renderer;

NetatmoClient netatmoClient;
OpenMeteoClient owmClient;

// Storage for Netatmo data
NetatmoMeasureResponse tempMain;
NetatmoMeasureResponse tempModule;
NetatmoMeasureResponse pressureMain;
NetatmoMeasureResponse humidityMain;
OpenMeteoForecastResponse owmForecast;

time_t lastTokenCheck = 0;
time_t lastNetatmoUpdate = 0;
time_t lastOwmUpdate = 0;
time_t lastScreenUpdate = 0;

bool checkToken();

void fetchNetatmo();

void fetchOwm();

uint16_t minutes = 0;

[[noreturn]] void imageRenderingTask(void *) {
    esp_task_wdt_add(nullptr);
    if (ImageRendererEvent::completionSemaphore == nullptr) {
        ImageRendererEvent::completionSemaphore = xSemaphoreCreateBinary();
    }

    while (sntp_get_sync_status() != SNTP_SYNC_STATUS_COMPLETED) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    ImageRendererEvent event{};

    netatmoClient.begin();
    bool needUpdate = false;
    bool tokenValid = false;

    for (;;) {
        esp_task_wdt_reset();

        if (epoch_time() - lastTokenCheck > 5 * 60 * 1000 || lastTokenCheck == 0) {
            tokenValid = checkToken();
        }

        if (tokenValid && (epoch_time() - lastNetatmoUpdate >= NETATMO_UPDATE_INTERVAL_SECONDS || lastNetatmoUpdate ==
                           0)) {
            fetchNetatmo();
            needUpdate = true;
        }

        if (epoch_time() - lastOwmUpdate >= OPEN_METEO_UPDATE_INTERVAL_SECONDS || lastOwmUpdate == 0) {
            fetchOwm();
            needUpdate = true;
        }

        if (xQueueReceive(renderingQueue, &event, pdMS_TO_TICKS(100))) {
            switch (event.type) {
                case ImageRendererEventType::RENDER_IMAGE:
                    LogEvent::post("Rendering image\n");
                    renderer.renderImage(tempMain, tempModule, pressureMain, humidityMain, owmForecast);
                    LogEvent::post("Image rendered\n");
                    if (ImageRendererEvent::completionSemaphore != nullptr) {
                        xSemaphoreGive(ImageRendererEvent::completionSemaphore);
                    }

                    needUpdate = false;

                    break;
                case ImageRendererEventType::NETATMO_CODE:
                    LogEvent::post("Received Netatmo code\n");
                    if (netatmoClient.getToken(event.code)) {
                        lastNetatmoUpdate = 0;
                    }
                    break;
                default:
                    break;
            }
        }

        // The doc advices to let a cooldown of +- 3min between 2 screen updates
        if (needUpdate && (lastScreenUpdate == 0 || epoch_time() - lastScreenUpdate >= 3 * 60 )) {
            LogEvent::post("Rendering image\n");
            renderer.renderImage(tempMain, tempModule, pressureMain, humidityMain, owmForecast);
            LogEvent::post("Image rendered\n");
            needUpdate = false;
            lastScreenUpdate = epoch_time();
        }

        taskYIELD();
    }
}

bool checkToken() {
    lastTokenCheck = epoch_time();
    if (!netatmoClient.getTokenInfo().isValid()) {
        LogEvent::post("Netatmo token expired or about to expire, refreshing...\n");
        if (netatmoClient.refreshToken()) {
            LogEvent::post("Netatmo token refreshed successfully\n");
        } else {
            LogEvent::post("Failed to refresh Netatmo token\n");
            return false;
        }
    }

    return true;
}

void fetchNetatmo() {
    if (netatmoClient.isAuthenticated()) {
        LogEvent::post("Fetching Netatmo data...\n");

        bool status = netatmoClient.getLast24hTemperature("", tempMain);
        if (status)
            LogEvent::post("Fetched main temperature (%d samples)\n", tempMain.size);

        status = netatmoClient.getLast24hTemperature(NETATMO_MODULE_ID, tempModule);
        if (status)
            LogEvent::post("Fetched module temperature (%d samples)\n", tempModule.size);

        status = netatmoClient.getLast24hPressure("", pressureMain);
        if (status)
            LogEvent::post("Fetched main pressure (%d samples)\n", pressureMain.size);

        status = netatmoClient.getLast24hHumidity(humidityMain);
        if (status)
            LogEvent::post("Fetched main humidity (%d samples)\n", humidityMain.size);
    }

    lastNetatmoUpdate = epoch_time();
}

void fetchOwm() {
    LogEvent::post("Fetching OpenMeteo forecast...\n");
    const uint16_t status = owmClient.getForecast(owmForecast);
    if (status) {
        LogEvent::post("Fetched OpenMeteo forecast (%d items)\n", owmForecast.count);
    } else {
        LogEvent::post("Failed to fetch OpenMeteo forecast (status: %d)\n", status);
    }

    lastOwmUpdate = epoch_time();
}
