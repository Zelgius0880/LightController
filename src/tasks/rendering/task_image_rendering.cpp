//
// Created by Zelgius on 18-03-26.
//

#include <rendering/task_image_rendering.h>
#include <Arduino.h>
#include <esp_task_wdt.h>

#include <ImageRenderer.h>
#include <WiFiClientSecure.h>

#include <NetatmoClient.h>
#include <OpenWeatherMapClient.h>
#include <webserver/task_webserver.h>
#include "logger/task_logger.h"
#include "configuration.h"

#if !defined(ENABLE_NETATMO) || !defined(ENABLE_OWM)
#include "mock_data.h"
#endif

#ifndef NETATMO_UPDATE_INTERVAL_MINUTES
#define NETATMO_UPDATE_INTERVAL_MINUTES 20
#endif

#ifndef OWM_UPDATE_INTERVAL_MINUTES
#define OWM_UPDATE_INTERVAL_MINUTES 60
#endif

extern QueueHandle_t renderingQueue;
extern WiFiClientSecure sharedClient;
SemaphoreHandle_t ImageRendererEvent::completionSemaphore = nullptr;
ImageRenderer *ImageRenderer::instance = nullptr;

ImageRenderer renderer;

#ifdef ENABLE_NETATMO
NetatmoClient netatmoClient(sharedClient);
#endif

#ifdef ENABLE_OWM
OpenWeatherMapClient owmClient(sharedClient);
#endif

// Storage for Netatmo data
NetatmoMeasureResponse tempMain;
NetatmoMeasureResponse tempModule;
NetatmoMeasureResponse pressureMain;
NetatmoMeasureResponse humidityMain;
OWMForecastResponse owmForecast;

uint32_t lastTokenCheck = 0;
uint32_t lastNetatmoUpdate = 0;
uint32_t lastOwmUpdate = 0;
uint32_t lastScreenUpdate = 0;

bool checkToken();

void fetchNetatmo();

void fetchOwm();

uint16_t minutes = 0;

uint16_t elapsedMinutes(const uint32_t from) {
    const auto elapsedMillis = millis() - from;
    return elapsedMillis / (60 * 1000);
}

[[noreturn]] void imageRenderingTask(void *) {
    esp_task_wdt_add(nullptr);
    if (ImageRendererEvent::completionSemaphore == nullptr) {
        ImageRendererEvent::completionSemaphore = xSemaphoreCreateBinary();
    }

    while (!timeClient.isTimeSet()) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

#if !defined(ENABLE_NETATMO) || !defined(ENABLE_OWM)
    populateMocks(tempMain, tempModule, pressureMain, humidityMain, owmForecast);
#endif

    ImageRendererEvent event{};

#ifdef ENABLE_NETATMO
    netatmoClient.begin();

    const NetatmoToken &token = netatmoClient.getTokenInfo();
    WebServerEvent::updateNetatmoToken(token.accessToken.c_str(), token.refreshToken.c_str(), token.expiresIn,
                                       token.creationTimestamp);
#endif


    bool needUpdate = false;

    for (;;) {
        esp_task_wdt_reset();

        bool tokenValid = false;
        if (millis() - lastTokenCheck > 60000 || lastTokenCheck == 0) {
            tokenValid = checkToken();
        }

        if (tokenValid && (elapsedMinutes(lastNetatmoUpdate) >= NETATMO_UPDATE_INTERVAL_MINUTES || lastNetatmoUpdate ==
                           0)) {
            fetchNetatmo();
            needUpdate = true;
        }

        if (elapsedMinutes(lastOwmUpdate) >= OWM_UPDATE_INTERVAL_MINUTES || lastOwmUpdate == 0) {
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
#ifdef ENABLE_NETATMO

                    LogEvent::post("Received Netatmo code\n");
                    if (netatmoClient.getToken(event.code)) {
                        const NetatmoToken &t = netatmoClient.getTokenInfo();
                        WebServerEvent::updateNetatmoToken(t.accessToken.c_str(), t.refreshToken.c_str(), t.expiresIn,
                                                           t.creationTimestamp);
                    }
#endif
                    break;
                default:
                    break;
            }
        }

        // The doc advices to let a cooldown of +- 3min between 2 screen updates
        if (needUpdate && (lastScreenUpdate == 0 || millis() - lastScreenUpdate >= 3 * 60 * 1000)) {
            LogEvent::post("Rendering image\n");
            renderer.renderImage(tempMain, tempModule, pressureMain, humidityMain, owmForecast);
            LogEvent::post("Image rendered\n");
            needUpdate = false;
            lastScreenUpdate = millis();
        }

        taskYIELD();
    }
}

bool checkToken() {
#ifdef ENABLE_NETATMO

    // Check token validity every 60 seconds
    if (millis() - lastTokenCheck > 60000) {
        lastTokenCheck = millis();
        if (!netatmoClient.getTokenInfo().isValid()) {
            LogEvent::post("Netatmo token expired or about to expire, refreshing...\n");
            if (netatmoClient.refreshToken()) {
                LogEvent::post("Netatmo token refreshed successfully\n");
                const NetatmoToken &t = netatmoClient.getTokenInfo();
                WebServerEvent::updateNetatmoToken(t.accessToken.c_str(), t.refreshToken.c_str(), t.expiresIn,
                                                   t.creationTimestamp);
            } else {
                LogEvent::post("Failed to refresh Netatmo token\n");
                return false;
            }
        }
    }

    return true;
#else
    return true;
#endif
}

void fetchNetatmo() {
#ifdef ENABLE_NETATMO
    if (netatmoClient.isAuthenticated()) {
        WebServerEvent::printLog("Fetching Netatmo data...\n");

        int status = netatmoClient.getLast24hTemperature("", tempMain);
        if (status == 200)
            LogEvent::post("Fetched main temperature (%d samples)\n", tempMain.size);

        status = netatmoClient.getLast24hTemperature(NETATMO_MODULE_ID, tempModule);
        if (status == 200)
            LogEvent::post("Fetched module temperature (%d samples)\n", tempModule.size);

        status = netatmoClient.getLast24hPressure("", pressureMain);
        if (status == 200)
            LogEvent::post("Fetched main pressure (%d samples)\n", pressureMain.size);

        status = netatmoClient.getLast24hHumidity(humidityMain);
        if (status == 200)
            LogEvent::post("Fetched main humidity (%d samples)\n", humidityMain.size);
    }
    lastNetatmoUpdate = millis();
#else
    lastNetatmoUpdate = millis();
#endif
}

void fetchOwm() {
#ifdef ENABLE_OWM

    LogEvent::post("Fetching OpenWeatherMap forecast...\n");
    const uint16_t status = owmClient.getForecast(owmForecast);
    if (status == 200) {
        LogEvent::post("Fetched OWM forecast (%d items)\n", owmForecast.count);
    } else {
        LogEvent::post("Failed to fetch OWM forecast (status: %d)\n", status);
    }
#endif
    lastOwmUpdate = millis();
}
