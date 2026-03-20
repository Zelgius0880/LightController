#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <SPI.h>

#include "configuration.h"
#include "RCSwitch.h"
#include "leds/task_leds.h"
#include "lights/task_light_controller.h"
#include "logger/task_logger.h"
#include "rendering/task_image_rendering.h"
#include "webserver/task_webserver.h"
#include "buzzer/task_buzzer.h"

#define RECEIVER_PIN 2

auto receiver = RCSwitch();

QueueHandle_t lightControllerQueue;
QueueHandle_t ledQueue;
QueueHandle_t logQueue;
QueueHandle_t webserverQueue;
QueueHandle_t renderingQueue;
QueueHandle_t buzzerQueue;

SemaphoreHandle_t fsMutex;

void setup() {
    Serial.begin(115200);
    neopixelWrite(RGB_BUILTIN, 0, 0, 0); // Green for success
    fsMutex = xSemaphoreCreateMutex();

    if (!LittleFS.begin(true)) {
        Serial.println("LittleFS Mount Failed");
        return;
    }

#if defined(INITIAL_USER_NAME) && defined(INITIAL_CLIENT_KEY)
    LittleFS.format();
    File file = LittleFS.open(CREDENTIALS_FILE, FILE_WRITE);

    JsonDocument doc;
    doc["username"] = INITIAL_USER_NAME;
    doc["clientkey"] = INITIAL_CLIENT_KEY;
    if (serializeJson(doc, file) == 0) {
        file.close();
    }
    file.close();
#endif // INITIAL_USER_NAME && INITIAL_CLIENT_KEY

    if (psramInit()) {
        Serial.println("\nPSRAM is correctly initialized");
    } else {
        Serial.println("PSRAM not available");
    }

    WiFi.begin(WIFI_SSID, PASSWORD);
    Serial.print("Connecting");

    while (WiFiClass::status() != WL_CONNECTED) {
        neopixelWrite(RGB_BUILTIN, 0, 0, 128);
        delay(500);
        Serial.print(".");
        neopixelWrite(RGB_BUILTIN, 0, 0, 0);
    }

    Serial.println();
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());


    lightControllerQueue = xQueueCreate(5, sizeof(LightEvent));
    logQueue = xQueueCreate(10, sizeof(LogEvent));
    ledQueue = xQueueCreate(5, sizeof(LedEvent));
    webserverQueue = xQueueCreate(5, sizeof(WebServerEvent));
    renderingQueue = xQueueCreate(5, sizeof(ImageRendererEvent));
    buzzerQueue = xQueueCreate(5, sizeof(BuzzerEvent));

    if (lightControllerQueue != nullptr && ledQueue != nullptr && logQueue != nullptr && webserverQueue != nullptr &&
        renderingQueue != nullptr && buzzerQueue != nullptr) {
        // System Core
        xTaskCreatePinnedToCore(ledsTask, "Leds", 4096, nullptr, 1, nullptr, 0);
        xTaskCreatePinnedToCore(loggerTask, "Logger", 4096, nullptr, 1, nullptr, 0);
        xTaskCreatePinnedToCore(buzzerTask, "Buzzer", 4096, nullptr, 1, nullptr, 0);

        // App Core
        xTaskCreatePinnedToCore(webserverTask, "WebServer", 4096, nullptr, 1, nullptr, 1);
        xTaskCreatePinnedToCore(lightControllerTask, "LightController", 16384, nullptr, 1, nullptr, 1);
        xTaskCreatePinnedToCore(imageRenderingTask, "ImageRendering", 8192, nullptr, 1, nullptr, 1);
    }


    receiver.enableReceive(digitalPinToInterrupt(RECEIVER_PIN));
}


uint32_t lastValue = 0;
uint32_t lastTime = 0;

void loop() {
    if (receiver.available()) {
        const uint64_t protocol = receiver.getReceivedProtocol();
        const uint32_t value = receiver.getReceivedValue();
        const uint32_t now = millis();

        if (value != lastValue || (now - lastTime) >= 500) {
            lastValue = value;
            lastTime = now;

            uint64_t data = protocol;
            data = data << 40;
            data = data | 0x000400000000 | value;

            LogEvent::post("Signal received: %012llx\n", data);
            receiver.resetAvailable();
            
            LightEvent::switchPressed(data);
            BuzzerEvent::bip();
        } else {
            receiver.resetAvailable();
        }
    }

    vTaskDelay(pdMS_TO_TICKS(100));
}
