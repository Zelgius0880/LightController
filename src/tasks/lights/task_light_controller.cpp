//
// Created by Zelgius on 18-03-26.
//
#include <esp_task_wdt.h>
#include <lights/task_light_controller.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "configuration.h"
#include <HueApiClient.h>
#include <LightsDatabaseManager.h>
#include <ArduinoJson.h>
#include <allocator/psram_allocator.h>
#include <logger/task_logger.h>
#include <webserver/task_webserver.h>

#include "buzzer/task_buzzer.h"
#include "leds/task_leds.h"

extern QueueHandle_t lightControllerQueue;
extern WiFiClientSecure sharedClient;
HueApiClient api(sharedClient, BRIDGE_IP, 443, true);

LightsDatabaseManager dbManager(DB_PATH);

void handleSwitch(const LightEvent &receivedEvent);

[[noreturn]] void lightControllerTask(void *) {
    esp_task_wdt_add(nullptr);

    LedEvent::blink(128, 128, 0, 0, 100);
    dbManager.begin();
    LedEvent::off();

    LogEvent::post("Database Manager Initialized\n");

    LedEvent::blink(0, 0, 128, 0, 100);

    api.loadCredentials();
    while (!api.isAuthenticated()) {
        api.handleAuthentication();
        vTaskDelay(pdMS_TO_TICKS(500));
        esp_task_wdt_reset();
    }

    LedEvent::plain(0, 128, 0, 128);

    BuzzerEvent::melody();

    LightEvent receivedEvent{};

    for (;;) {
        esp_task_wdt_reset();
        if (xQueueReceive(lightControllerQueue, &receivedEvent, pdMS_TO_TICKS(50))) {
            if (receivedEvent.type == LightEventType::SWITCH_PRESSED) {
                handleSwitch(receivedEvent);
            }
        }
        portYIELD();
    }
}

void handleSwitch(const LightEvent &receivedEvent) {
    char uidBuffer[13];
    snprintf(uidBuffer, sizeof(uidBuffer), "%012llx", receivedEvent.data.switchUid);

    Group group = dbManager.getGroupBySwitchUid(uidBuffer);
    if (group.id != 0) {
        std::vector<Light> lights = dbManager.getLightsByGroupId(group.id);

        bool firstToggle = true;
        bool toggleTargetOn = false;

        for (const auto &light: lights) {
            bool targetOn = false;
            if (light.state == "TOGGLE") {
                if (firstToggle) {
                    auto res = api.getLight(light.uid.c_str());
                    if (!res.data.empty()) {
                        toggleTargetOn = !res.data[0].on.on;
                        firstToggle = false;
                    } else {
                        LogEvent::post("Failed to get light status for %s\n", light.uid.c_str());
                        continue;
                    }
                }
                targetOn = toggleTargetOn;
            } else {
                targetOn = (light.state == "ON");
            }

            JsonDocument updateDoc(&allocator);
            JsonObject on = updateDoc["on"].to<JsonObject>();
            on["on"] = targetOn;

            if (targetOn) {
                if (light.brightness > 0) {
                    updateDoc["dimming"]["brightness"] = light.brightness;
                }
                if (light.x != 0 || light.y != 0) {
                    JsonObject color = updateDoc["color"].to<JsonObject>();
                    JsonObject xy = color["xy"].to<JsonObject>();
                    xy["x"] = light.x;
                    xy["y"] = light.y;
                }
            }

            const auto response = api.updateLight(light.uid.c_str(), updateDoc.as<JsonVariantConst>());
            if (response.status != 200) {
                if (response.errors.description != nullptr)
                    LogEvent::post("Failed to update light %s (code %d)\n",
                                             response.errors.description.c_str(), response.status);
                else
                    LogEvent::post("Failed to update light: Unknow error (code %d)\n", response.status);
            }
        }
        BuzzerEvent::bip2();
    } else {
        LogEvent::post("No group found for switch %s\n", uidBuffer);
    }
}


bool LightEvent::switchPressed(const uint64_t switchUid) {
    if (lightControllerQueue == nullptr) return false;

    LightEvent event{};
    event.type = LightEventType::SWITCH_PRESSED;
    event.data.switchUid = switchUid;

    if (xQueueSend(lightControllerQueue, &event, pdMS_TO_TICKS(10)) != pdPASS) {
        LogEvent::post("[ERROR] Light Controller Queue Full!\n");
        return false;
    }
    return true;
}
