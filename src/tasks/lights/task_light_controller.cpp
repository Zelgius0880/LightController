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
#ifdef DEBUG_FIREBASE
#define DEBUG_FIREBASE(...) LogEvent::post(__VA_ARGS__)
#else
#define DEBUG_FIREBASE(...)
#endif
extern QueueHandle_t lightControllerQueue;
extern WiFiClientSecure sharedClient;

void processAuth(AsyncResult &aResult);

void processSwitch(AsyncResult &aResult);

void processList(AsyncResult &aResult);

firebase_ns::FirebaseApp app;
WiFiClientSecure client;
AsyncClientClass aClient(client);
HueApiClient api(sharedClient, BRIDGE_IP, 443, true);
FirebaseManager firebaseManager(app, aClient, api);
UserAuth user_auth(FIREBASE_WEB_API_KEY, FIREBASE_EMAIL,FIREBASE_PASSWORD);


[[noreturn]] void lightControllerTask(void *pvParameters) {
    esp_task_wdt_add(nullptr);

    client.setInsecure();
    client.setHandshakeTimeout(5);
    client.setTimeout(5);

    initializeApp(aClient, app, getAuth(user_auth), processAuth, "🔐 authTask");

    LedEvent::blink(128, 128, 0, 0, 100);
    while (!app.ready()) {
        vTaskDelay(pdMS_TO_TICKS(500));
        esp_task_wdt_reset();
    }
    LedEvent::off();

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

    WebServerEvent::updateUsername(api.getUsername().c_str());

    BuzzerEvent::melody();

    LightEvent receivedEvent{};

    for (;;) {
        esp_task_wdt_reset();
        firebaseManager.loop();
        if (firebaseManager.ready()) {
            if (xQueueReceive(lightControllerQueue, &receivedEvent, pdMS_TO_TICKS(50))) {
                // Handle outgoing data to Firebase
                if (receivedEvent.type == LightEventType::SWITCH_PRESSED) {
                    static char uidBuffer[13];
                    snprintf(uidBuffer, sizeof(uidBuffer), "%012llx", receivedEvent.data.switchUid);
                    firebaseManager.querySwitch(uidBuffer);
                }
            }
        }
        portYIELD();
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
    }
}


void processSwitch(AsyncResult &aResult) {
    // Exits when no result is available when calling from the loop.
    if (!aResult.isResult())
        return;

    if (aResult.isEvent()) {
        DEBUG_FIREBASE("Event task: %s, msg: %s, code: %d\n", aResult.uid().c_str(),
                       aResult.appEvent().message().c_str(), aResult.appEvent().code());
    }

    if (aResult.isDebug()) {
        DEBUG_FIREBASE("Debug task: %s, msg: %s\n", aResult.uid().c_str(), aResult.debug().c_str());
    }

    if (aResult.isError()) {
        DEBUG_FIREBASE("Error task: %s, msg: %s, code: %d\n", aResult.uid().c_str(),
                       aResult.error().message().c_str(),
                       aResult.error().code());
    }

    if (aResult.available()) {
        DEBUG_FIREBASE("task: %s, payload: %s\n", aResult.uid().c_str(), aResult.c_str());
        firebaseManager.handleSwitchResult(aResult);
    }
}


void processList(AsyncResult &aResult) {
    // Exits when no result is available when calling from the loop.
    if (!aResult.isResult())
        return;

    if (aResult.isEvent()) {
        DEBUG_FIREBASE("Event task: %s, msg: %s, code: %d\n", aResult.uid().c_str(),
                       aResult.appEvent().message().c_str(), aResult.appEvent().code());
    }

    if (aResult.isDebug()) {
        DEBUG_FIREBASE("Debug task: %s, msg: %s\n", aResult.uid().c_str(), aResult.debug().c_str());
    }

    if (aResult.isError()) {
        DEBUG_FIREBASE("Error task: %s, msg: %s, code: %d\n", aResult.uid().c_str(),
                       aResult.error().message().c_str(),
                       aResult.error().code());
    }

    if (aResult.available()) {
        DEBUG_FIREBASE("task: %s, payload: %s\n", aResult.uid().c_str(), aResult.c_str());
        firebaseManager.handleListResult(aResult);
    }
}
