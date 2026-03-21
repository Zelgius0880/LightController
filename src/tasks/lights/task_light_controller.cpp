//
// Created by Zelgius on 18-03-26.
//

#include <esp_task_wdt.h>
#include <lights/task_light_controller.h>
#include <projdefs.h>
#include <task.h>
#include <queue.h>
#include <FirebaseClient.h>

#include "configuration.h"
#include "FirebaseManager.h"
#include "HueApiClient.h"
#include <logger/task_logger.h>
#include <webserver/task_webserver.h>

#include "buzzer/task_buzzer.h"
#include "leds/task_leds.h"

extern QueueHandle_t lightControllerQueue;

void processAuth(AsyncResult &aResult);

void processSwitch(AsyncResult &aResult);

void processList(AsyncResult &aResult);

firebase_ns::FirebaseApp app;
WiFiClientSecure client;
AsyncClientClass aClient(client);
HueApiClient api(BRIDGE_IP, 443, true);
FirebaseManager firebaseManager(app, aClient, api);
UserAuth user_auth(FIREBASE_WEB_API_KEY, FIREBASE_EMAIL,FIREBASE_PASSWORD, 3000);

AsyncResult _firestoreResult;

[[noreturn]] void lightControllerTask(void *pvParameters) {
    esp_task_wdt_add(nullptr);

    LightEvent receivedEvent;

    client.setInsecure();

    LogEvent::post("Light controller task started\n");

    initializeApp(aClient, app, getAuth(user_auth), processAuth, "🔐 authTask");
    LogEvent::post("Initializing Firebase app..");

    LedEvent::blink(128, 128, 0, 0, 100);
    while (!app.ready()) {
        vTaskDelay(pdMS_TO_TICKS(500));
        LogEvent::post(".");
        esp_task_wdt_reset();
    }
    LedEvent::off();

    LogEvent::post("\n");
    LogEvent::post("Firebase app initialized successfully\n");
    firebaseManager.begin();

    WebServerEvent::printLog("Firebase Manager Initialized\n");

    LedEvent::blink(0, 0, 128, 0, 100);
    api.loadCredentials();
    while (!api.isAuthenticated()) {
        api.handleAuthentication();
        vTaskDelay(pdMS_TO_TICKS(500));
        esp_task_wdt_reset();
    }
    LedEvent::plain(0, 128, 0, 128);

    WebServerEvent::printLog("Hue API Client Initialized\n");
    WebServerEvent::updateUsername(api.getUsername().c_str());

    BuzzerEvent::melody();

    for (;;) {
        esp_task_wdt_reset();
        if (xQueueReceive(lightControllerQueue, &receivedEvent, pdMS_TO_TICKS(50))) {
            // Handle outgoing data to Firebase
            if (receivedEvent.type == LightEventType::SWITCH_PRESSED) {
                static char uidBuffer[13];
                snprintf(uidBuffer, sizeof(uidBuffer), "%012llx", receivedEvent.data.switchUid);
                firebaseManager.querySwitch(uidBuffer);
            }
        }

        firebaseManager.loop();

        if (aClient.lastError().code() != 0) {
            LogEvent::post("Firebase Error: %s (Code: %d)\n",
                           aClient.lastError().message().c_str(),
                           aClient.lastError().code());

            aClient.stopAsync(true);
            WebServerEvent::printLog("Network lost after SSL error. Waiting for reconnect...");
            vTaskDelay(pdMS_TO_TICKS(5000));
        } else {
            portYIELD();
        }
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


void processAuth(AsyncResult &aResult) {
    // Exits when no result is available when calling from the loop.
    if (!aResult.isResult())
        return;

    if (aResult.isEvent()) {
        WebServerEvent::printLog("Event task: %s, msg: %s, code: %d\n", aResult.uid().c_str(),
                                 aResult.appEvent().message().c_str(), aResult.appEvent().code());
    }

    if (aResult.isDebug()) {
        WebServerEvent::printLog("Debug task: %s, msg: %s\n", aResult.uid().c_str(), aResult.debug().c_str());
    }

    if (aResult.isError()) {
        WebServerEvent::printLog("Error task: %s, msg: %s, code: %d\n", aResult.uid().c_str(),
                                 aResult.error().message().c_str(),
                                 aResult.error().code());
    }

    if (aResult.available()) {
        WebServerEvent::printLog("task: %s, payload: %s\n", aResult.uid().c_str(), aResult.c_str());

        // The information printed from Firebase.printf may be truncated because of limited buffer memory to reduce the stack usage,
        // use Serial.println(aResult.c_str()) to print entire content.
    }
}


void processSwitch(AsyncResult &aResult) {
    // Exits when no result is available when calling from the loop.
    if (!aResult.isResult())
        return;

    if (aResult.isEvent()) {
        WebServerEvent::printLog("Event task: %s, msg: %s, code: %d\n", aResult.uid().c_str(),
                                 aResult.appEvent().message().c_str(), aResult.appEvent().code());
    }

    if (aResult.isDebug()) {
        WebServerEvent::printLog("Debug task: %s, msg: %s\n", aResult.uid().c_str(), aResult.debug().c_str());
    }

    if (aResult.isError()) {
        WebServerEvent::printLog("Error task: %s, msg: %s, code: %d\n", aResult.uid().c_str(),
                                 aResult.error().message().c_str(),
                                 aResult.error().code());
    }

    if (aResult.available()) {
        WebServerEvent::printLog("task: %s, payload: %s\n", aResult.uid().c_str(), aResult.c_str());
        firebaseManager.handleSwitchResult(aResult);

        // The information printed from Firebase.printf may be truncated because of limited buffer memory to reduce the stack usage,
        // use Serial.println(aResult.c_str()) to print entire content.
    }
}


void processList(AsyncResult &aResult) {
    // Exits when no result is available when calling from the loop.
    if (!aResult.isResult())
        return;

    if (aResult.isEvent()) {
        WebServerEvent::printLog("Event task: %s, msg: %s, code: %d\n", aResult.uid().c_str(),
                                 aResult.appEvent().message().c_str(), aResult.appEvent().code());
    }

    if (aResult.isDebug()) {
        WebServerEvent::printLog("Debug task: %s, msg: %s\n", aResult.uid().c_str(), aResult.debug().c_str());
    }

    if (aResult.isError()) {
        WebServerEvent::printLog("Error task: %s, msg: %s, code: %d\n", aResult.uid().c_str(),
                                 aResult.error().message().c_str(),
                                 aResult.error().code());
    }

    if (aResult.available()) {
        WebServerEvent::printLog("task: %s, payload: %s\n", aResult.uid().c_str(), aResult.c_str());
        firebaseManager.handleListResult(aResult);

        // The information printed from Firebase.printf may be truncated because of limited buffer memory to reduce the stack usage,
        // use Serial.println(aResult.c_str()) to print entire content.
    }
}
