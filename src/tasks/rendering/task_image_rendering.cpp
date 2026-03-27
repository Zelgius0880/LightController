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

extern QueueHandle_t renderingQueue;
extern WiFiClientSecure sharedClient;
SemaphoreHandle_t ImageRendererEvent::completionSemaphore = nullptr;
ImageRenderer *ImageRenderer::instance = nullptr;

ImageRenderer renderer;
NetatmoClient netatmoClient(sharedClient);
OpenWeatherMapClient owmClient(sharedClient);

// Storage for Netatmo data
NetatmoMeasureResponse tempMain;
NetatmoMeasureResponse tempModule;
NetatmoMeasureResponse pressureMain;
NetatmoMeasureResponse humidityMain;
OWMForecastResponse owmForecast;


[[noreturn]] void imageRenderingTask(void *pvParameters) {
    esp_task_wdt_add(nullptr);
    if (ImageRendererEvent::completionSemaphore == nullptr) {
        ImageRendererEvent::completionSemaphore = xSemaphoreCreateBinary();
    }

    while (!timeClient.isTimeSet()) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    ImageRendererEvent event{};

    netatmoClient.begin();

    const NetatmoToken& token = netatmoClient.getTokenInfo();
    WebServerEvent::updateNetatmoToken(token.accessToken.c_str(), token.refreshToken.c_str(), token.expiresIn, token.creationTimestamp);

    uint32_t lastTokenCheck = 0;
    uint32_t lastNetatmoUpdate = 0;
    const uint32_t netatmoUpdateInterval = 15 * 60 * 1000; // 15 minutes

    for (;;) {
        esp_task_wdt_reset();

        // Check token validity every 60 seconds
        if (millis() - lastTokenCheck > 60000) {
            lastTokenCheck = millis();
            if (!netatmoClient.getTokenInfo().isValid()) {
                LogEvent::post("Netatmo token expired or about to expire, refreshing...\n");
                if (netatmoClient.refreshToken()) {
                    LogEvent::post("Netatmo token refreshed successfully\n");
                    const NetatmoToken& t = netatmoClient.getTokenInfo();
                    WebServerEvent::updateNetatmoToken(t.accessToken.c_str(), t.refreshToken.c_str(), t.expiresIn, t.creationTimestamp);
                } else {
                    LogEvent::post("Failed to refresh Netatmo token\n");
                }
            }
        }

        // Fetch Netatmo data every 15 minutes
        if (millis() - lastNetatmoUpdate > netatmoUpdateInterval || lastNetatmoUpdate == 0 ) {
            if (netatmoClient.isAuthenticated()) {
                LogEvent::post("Fetching Netatmo data...\n");
                
                int status = netatmoClient.getLast24hTemperature("", tempMain);
                if (status == 200) LogEvent::post("Fetched main temperature (%d samples)\n", tempMain.size);
                
                status = netatmoClient.getLast24hTemperature(NETATMO_MODULE_ID, tempModule);
                if (status == 200) LogEvent::post("Fetched module temperature (%d samples)\n", tempModule.size);

                status = netatmoClient.getLast24hPressure("", pressureMain);
                if (status == 200) LogEvent::post("Fetched main pressure (%d samples)\n", pressureMain.size);

                status = netatmoClient.getCurrentHumidity(humidityMain);
                if (status == 200) LogEvent::post("Fetched main humidity (%.1f %%)\n", humidityMain.values[0]);

                LogEvent::post("Fetching OpenWeatherMap forecast...\n");
                status = owmClient.getForecast(owmForecast);
                if (status == 200) {
                    LogEvent::post("Fetched OWM forecast (%d items)\n", owmForecast.forecast.size());
                } else {
                    LogEvent::post("Failed to fetch OWM forecast (status: %d)\n", status);
                }

                lastNetatmoUpdate = millis();
            }
        }

        if (xQueueReceive(renderingQueue, &event, pdMS_TO_TICKS(100))) {
            bool success = false;
            switch (event.type) {
                case ImageRendererEventType::RENDER_IMAGE:
                    LogEvent::post("Rendering image\n");
                    success = renderer.renderImage(tempMain, tempModule);
                    LogEvent::post("Image rendered\n");
                    if (success && ImageRendererEvent::completionSemaphore != nullptr) {
                        xSemaphoreGive(ImageRendererEvent::completionSemaphore);
                    }

                    break;
                case ImageRendererEventType::NETATMO_CODE:
                    LogEvent::post("Received Netatmo code\n");
                    if (netatmoClient.getToken(event.code)) {
                        const NetatmoToken& t = netatmoClient.getTokenInfo();
                        WebServerEvent::updateNetatmoToken(t.accessToken.c_str(), t.refreshToken.c_str(), t.expiresIn, t.creationTimestamp);
                    }
                    break;
                default:
                    break;
            }
        }

        taskYIELD();
    }
}
