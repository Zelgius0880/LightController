//
// Created by Zelgius on 18-03-26.
//
#include <Arduino.h>

#include <WebServerHandler.h>
#include <webserver/task_webserver.h>
#include <logger/task_logger.h>
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
        taskYIELD();
    }
}
