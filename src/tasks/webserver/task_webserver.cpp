//
// Created by Zelgius on 18-03-26.
//
#include <Arduino.h>

#include <WebServerHandler.h>
#include <webserver/task_webserver.h>
#include <LightsDatabaseManager.h>
#include <logger/task_logger.h>
#include "configuration.h"
#include <ElegantOTA.h>
#include <esp_task_wdt.h>

extern QueueHandle_t webserverQueue;

AsyncWebServer server(80);
WebServerHandler handler(server);

unsigned long ota_progress_millis = 0;


void onOTAStart() {}

void onOTAProgress(size_t current, size_t final) {
    // Log every 1 second
    if (millis() - ota_progress_millis > 1000) {
        ota_progress_millis = millis();
        Serial.printf("OTA Progress Current: %u bytes, Final: %u bytes\n", current, final);
    }
}

void onOTAEnd(bool success) {
    // Log when OTA has finished
    if (success) {
        Serial.println("OTA update finished successfully!");
    } else {
        Serial.println("There was an error during OTA update!");
    }
}


[[noreturn]] void webserverTask(void *) {
    ElegantOTA.begin(&server); // Start ElegantOTA
    // ElegantOTA callbacks
    ElegantOTA.onStart(onOTAStart);
    ElegantOTA.onProgress(onOTAProgress);
    ElegantOTA.onEnd(onOTAEnd);
    ElegantOTA.setAutoReboot(true);

    server.begin();
    handler.setup();

    LogEvent::post("Webserver task started\n");
    WebServerEvent event{};
    esp_task_wdt_add(nullptr);

    for (;;) {
        esp_task_wdt_reset();

        ElegantOTA.loop();
        if (xQueueReceive(webserverQueue, &event, 0)) {
            switch (event.type) {
                case WebServerEventType::POST_ERROR:
                    handler.setErrorMessage(event.payload);
                    // Update your internal handler state here
                    break;

                case WebServerEventType::UPDATE_USERNAME:
                    handler.setHueUsername(event.payload);
                    break;

                case WebServerEventType::UPDATE_NETATMO_TOKEN: {
                    JsonDocument doc;
                    deserializeJson(doc, event.payload);
                    handler.setNetatmoToken(
                        doc["access_token"] | "",
                        doc["refresh_token"] | "",
                        doc["expires_in"] | 0L,
                        doc["creation_timestamp"] | 0UL
                    );
                    break;
                }

                case WebServerEventType::CLEAR_ERROR:
                    handler.setErrorMessage("");
                    break;

                case WebServerEventType::PRINT_LOG:
                    handler.addLogMessage(event.payload);
                    break;
            }

            // CRITICAL: Clean up memory after processing
            if (event.isPSRAM && event.payload != nullptr) {
                free(event.payload);
            }
        }
        taskYIELD();
    }
}

void WebServerEvent::postError(const char *format, ...) {
    const auto buffer = static_cast<char *>(ps_malloc(256)); // Use 8MB PSRAM
    if (!buffer) return;

    va_list args;
    va_start(args, format);
    vsnprintf(buffer, 256, format, args);
    va_end(args);

    WebServerEvent event = {WebServerEventType::POST_ERROR, buffer, true};
    if (xQueueSend(webserverQueue, &event, 0) != pdPASS) {
        free(buffer); // Prevent leak if queue is full
    }
}

void WebServerEvent::printLog(const char *format, ...) {
    char *buffer = (char *) ps_malloc(256); // Use 8MB PSRAM
    if (!buffer) return;

    va_list args;
    va_start(args, format);
    vsnprintf(buffer, 256, format, args);
    va_end(args);

    WebServerEvent event = {WebServerEventType::PRINT_LOG, buffer, true};
    if (xQueueSend(webserverQueue, &event, 0) != pdPASS) {
        free(buffer); // Prevent leak if queue is full
    }
}

void WebServerEvent::updateUsername(const char *username) {
    char *buffer = (char *) ps_malloc(64);
    if (!buffer) return;

    strncpy(buffer, username, strlen(username) + 1);
    WebServerEvent event = {WebServerEventType::UPDATE_USERNAME, buffer, true};

    if (xQueueSend(webserverQueue, &event, 0) != pdPASS) {
        free(buffer);
    }
}

void WebServerEvent::updateNetatmoToken(const char *accessToken, const char *refreshToken, long expiresIn,
                                        unsigned long creationTimestamp) {
    JsonDocument doc;
    doc["access_token"] = accessToken;
    doc["refresh_token"] = refreshToken;
    doc["expires_in"] = expiresIn;
    doc["creation_timestamp"] = creationTimestamp;

    String serialized;
    serializeJson(doc, serialized);

    char *buffer = (char *) ps_malloc(serialized.length() + 1);
    if (!buffer) return;

    strcpy(buffer, serialized.c_str());
    WebServerEvent event = {WebServerEventType::UPDATE_NETATMO_TOKEN, buffer, true};

    if (xQueueSend(webserverQueue, &event, 0) != pdPASS) {
        free(buffer);
    }
}

void WebServerEvent::clearError() {
    WebServerEvent event = {WebServerEventType::CLEAR_ERROR, nullptr, false};
    xQueueSend(webserverQueue, &event, 0);
}
