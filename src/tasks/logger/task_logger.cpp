//
// Created by Zelgius on 18-03-26.
//

#include <logger/task_logger.h>
#include <Arduino.h>
#include <esp_task_wdt.h>
#include <LittleFS.h>

#include <WebSocketsServer.h>
#include <ArduinoJson.h>

#include "configuration.h"
#include "NetatmoModels.h"

extern QueueHandle_t logQueue;
extern SemaphoreHandle_t fsMutex;

auto webSocket = WebSocketsServer(81);

volatile int clientNum = -1;

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

bool loadHueCredentials();

void loadNetatmoToken();

String getStatusJson();

char hueUsername[64];
NetatmoToken netatmoToken;

void webSocketEvent(const uint8_t num, const WStype_t type, const uint8_t *, size_t) {
    switch (type) {
        case WStype_DISCONNECTED:
            clientNum = -1;
            break;
        case WStype_CONNECTED: {
            clientNum = num;
            webSocket.sendTXT(num, "Connected");
        }
        break;
        default: break;
    }
}

[[noreturn]] void loggerTask(void *) {
    webSocket.begin();
    webSocket.onEvent(webSocketEvent);

    LogEvent packet{};

    Serial.println("Logger task started");

    esp_task_wdt_add(nullptr);

    unsigned long lastStatusUpdate = 0;

    for (;;) {
        webSocket.loop();
        esp_task_wdt_reset();

        if (millis() - lastStatusUpdate >= 10000) {
            lastStatusUpdate = millis();
            String status = getStatusJson();
            webSocket.broadcastTXT(status);
        }

        if (xQueueReceive(logQueue, &packet, 10)) {
            if (packet.payload != nullptr) {
                webSocket.broadcastTXT(timeClient.getFormattedTime() + ": " + packet.payload);

                if (packet.isPSRAM) {
                    free(packet.payload);
                }
            }
        }

        taskYIELD();
    }
}

// Helper for "Fire and Forget" logging of large strings
void LogEvent::post(const char *format, ...) {
    // Allocate space in the 8MB PSRAM so we don't fragment Internal RAM
    // 1. Allocate a buffer in the 8MB PSRAM (not Internal RAM!)
    // 256 bytes is usually plenty for a single log line.
    const size_t BUF_SIZE = 256;
    const auto buffer = static_cast<char *>(ps_malloc(BUF_SIZE));

    if (buffer == nullptr) {
        // Fallback: If PSRAM is full, we can't log formatted data safely.
        return;
    }

    // 2. Format the string into the PSRAM buffer
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, BUF_SIZE, format, args);
    va_end(args);

    // 3. Prepare the packet for the Logger Task
    LogEvent packet{};
    packet.payload = buffer;
    packet.isPSRAM = true; // Tells the Logger Task to 'free()' this later

    // 4. Send to Queue (0 wait time).
    // If the queue is full, we must free the buffer immediately to prevent a leak.
    if (xQueueSend(logQueue, &packet, 0) != pdPASS) {
        free(buffer);
    }
}

bool loadHueCredentials() {
    JsonDocument doc;
    DeserializationError error;
    if (xSemaphoreTake(fsMutex, pdMS_TO_TICKS(1000))) {
        if (!LittleFS.exists(CREDENTIALS_FILE)) {
            xSemaphoreGive(fsMutex);
            return false;
        }

        File file = LittleFS.open(CREDENTIALS_FILE, FILE_READ);
        if (!file) {
            xSemaphoreGive(fsMutex);
            return false;
        }

        error = deserializeJson(doc, file);
        file.close();
        xSemaphoreGive(fsMutex);
    }
    if (error) {
        return false;
    }

    strcpy(hueUsername, doc["username"].as<String>().c_str());

    return true;
}


String getStatusJson() {
    loadHueCredentials();
    loadNetatmoToken();

    JsonDocument doc;
    const size_t total = psramInit() ? ESP.getPsramSize() : 0;
    doc["authenticated"] = strlen(hueUsername) > 0;
    doc["username"] = hueUsername;
    doc["totalBytes"] = total;
    doc["usedBytes"] = psramInit() ? total - ESP.getFreePsram() : 0;

    const auto netatmo = doc["netatmo"].to<JsonObject>();
    netatmo["authenticated"] = netatmoToken.accessToken.length() > 0;
    netatmo["expires_in"] = netatmoToken.expiresIn;
    netatmo["creation_timestamp"] = netatmoToken.creationTimestamp;
    netatmo["valid"] = netatmoToken.isValid();

    doc["fsTotal"] = LittleFS.totalBytes();
    doc["fsUsed"] = LittleFS.usedBytes();
    doc["firmware"] = TOSTRING(FIRWARE_VERSION);

    doc["heapTotal"] = ESP.getHeapSize();
    doc["heapFree"] = ESP.getFreeHeap();

    String response;
    serializeJson(doc, response);
    return response;
}

void loadNetatmoToken() {
    if (xSemaphoreTake(fsMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        if (LittleFS.exists(NETATMO_TOKEN_FILE)) {
            File file = LittleFS.open(NETATMO_TOKEN_FILE, "r");
            if (file) {
                JsonDocument doc;
                deserializeJson(doc, file);
                netatmoToken.fromJson(doc);
                file.close();
            }
        }
        xSemaphoreGive(fsMutex);
    }
}
