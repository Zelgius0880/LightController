//
// Created by Zelgius on 18-03-26.
//

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

extern QueueHandle_t lightControllerQueue;

void processData(AsyncResult &aResult);

firebase_ns::FirebaseApp app;
WiFiClientSecure client;
AsyncClientClass aClient(client);
HueApiClient api(BRIDGE_IP, 443, true, true);
FirebaseManager firebaseManager(app, aClient, api);
UserAuth user_auth(FIREBASE_WEB_API_KEY, FIREBASE_EMAIL,FIREBASE_PASSWORD, 3000);


[[noreturn]] void lightControllerTask(void *pvParameters) {
    LightEvent receivedEvent;

    client.setInsecure();

    LogEvent::post("Light controller task started\n");

    initializeApp(aClient, app, getAuth(user_auth), processData, "🔐 authTask");
    LogEvent::post("Initializing Firebase app..");

    LedEvent::blink(128,128,0, 0, 100);
    while (!app.ready()) {
        vTaskDelay(pdMS_TO_TICKS(500));
        LogEvent::post(".");
    }
    LedEvent::off();

    LogEvent::post("\n");
    LogEvent::post("Firebase app initialized successfully\n");
    firebaseManager.begin();

    WebServerEvent::printLog("Firebase Manager Initialized\n");

    LedEvent::blink(0,0,128, 0, 100);
    api.loadCredentials();
    while (!api.isAuthenticated()) {
        api.handleAuthentication();
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    LedEvent::plain(0,128,0,128);

    WebServerEvent::printLog("Hue API Client Initialized\n");
    WebServerEvent::updateUsername(api.getUsername().c_str());

    BuzzerEvent::melody();

    for (;;) {
        if (xQueueReceive(lightControllerQueue, &receivedEvent, pdMS_TO_TICKS(50))) {
            // Handle outgoing data to Firebase
            if (receivedEvent.type == LightEventType::SWITCH_PRESSED) {
                static char uidBuffer[13];
                snprintf(uidBuffer, sizeof(uidBuffer), "%012llx", receivedEvent.data.switchUid);
                firebaseManager.handleSwitch(uidBuffer);
            }
        }

       // if (firebaseManager.ready()) {
            // Depending on your SDK version, it might be Firebase.loop()
            // or it might handle itself inside ready().
            firebaseManager.loop();
        //}
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


void processData(AsyncResult &aResult) {
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
        WebServerEvent::printLog("Error task: %s, msg: %s, code: %d\n", aResult.uid().c_str(), aResult.error().message().c_str(),
                 aResult.error().code());
    }

    if (aResult.downloadProgress()) {
        WebServerEvent::printLog("Downloaded, task: %s, %d%s (%d of %d)\n", aResult.uid().c_str(),
                 aResult.downloadInfo().progress,
                 "%", aResult.downloadInfo().downloaded, aResult.downloadInfo().total);
        if (aResult.downloadInfo().total == aResult.downloadInfo().downloaded) {
            WebServerEvent::printLog("Download task: %s, complete!✅️\n", aResult.uid().c_str());
        }
    }

    if (aResult.uploadProgress()) {
        WebServerEvent::printLog("Uploaded, task: %s, %d%s (%d of %d)\n", aResult.uid().c_str(), aResult.uploadInfo().progress,
                 "%", aResult.uploadInfo().uploaded, aResult.uploadInfo().total);
        if (aResult.uploadInfo().total == aResult.uploadInfo().uploaded) {
            WebServerEvent::printLog("Upload task: %s, complete!✅️\n", aResult.uid().c_str());
            LogEvent::post("Download URL: ");
            LogEvent::post(aResult.uploadInfo().downloadUrl.c_str());
            LogEvent::post("\n");
        }
    }
}
