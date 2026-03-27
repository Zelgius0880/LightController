#include <LittleFS.h>
#include <WebServerHandler.h>

#include "configuration.h"
#include "index_html.h"
#include "logger/task_logger.h"

#include <rendering/task_image_rendering.h>
#include <ImageRenderer.h>
#include "esp_core_dump.h"

extern QueueHandle_t renderingQueue;
extern SemaphoreHandle_t fsMutex;

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

WebServerHandler::WebServerHandler(AsyncWebServer &server)
    : _server(server), _hueUsername(""), _errorBuffer{}, _logBuffer{} {
}

void WebServerHandler::addLogMessage(const char *msg) {
    if (!msg) return;

    // Copy at most 127 chars to ensure null-termination
    strncpy(_logBuffer[_logIndex], msg, sizeof(_logBuffer[0]) - 1);
    // Ensure null-termination and truncate beyond 127
    _logBuffer[_logIndex][sizeof(_logBuffer[0]) - 1] = '\0';

    // Advance circular index
    _logIndex = (_logIndex + 1) % LOG_BUFFER_SIZE;
    if (_logCount < LOG_BUFFER_SIZE) {
        _logCount++;
    }
}

void WebServerHandler::setup() {
    _server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/html", index_html, processor);
    });

    _server.on("/status", HTTP_GET, [this](AsyncWebServerRequest *request) {
        handleStatus(request);
    });


    _server.on("/upload_image", HTTP_POST, [](AsyncWebServerRequest *request) {
                   request->send(200, "text/plain", "OK");
               }, [this](AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data,
                         size_t len,
                         bool final) {
                   handleUpload(request, filename, index, data, len, final);
               });


    _server.on("/render", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (ImageRendererEvent::completionSemaphore == nullptr) {
            ImageRendererEvent::completionSemaphore = xSemaphoreCreateBinary();
        }

        if (ImageRendererEvent::completionSemaphore == nullptr) {
            request->send(500, "text/plain", "Failed to create semaphore");
            return;
        }

        ImageRendererEvent event{
            .type = ImageRendererEventType::RENDER_IMAGE,
        };

        if (xQueueSend(renderingQueue, &event, pdMS_TO_TICKS(100)) == pdTRUE) {
            if (xSemaphoreTake(ImageRendererEvent::completionSemaphore, pdMS_TO_TICKS(10000)) == pdTRUE) {
                AsyncWebServerResponse *response = request->beginResponse(
                    200, "image/jpeg", ImageRenderer::instance->getJpgOutput(), ImageRenderer::instance->getJpgSize());
                response->addHeader("Cache-Control", "no-cache");
                request->send(response);
            } else {
                request->send(504, "text/plain", "Render Timeout");
            }
        } else {
            request->send(503, "text/plain", "Rendering Queue Full");
        }
    });


    _server.on("/image.jpg", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (LittleFS.exists("/image.jpg")) {
            request->send(LittleFS, "/image.jpg", "image/jpeg");
        } else {
            request->send(404, "text/plain", "Image not found");
        }
    });

    _server.on("/logs", HTTP_GET, [this](AsyncWebServerRequest *request) {
        handleLogs(request);
    });
    _server.on("/crashes", HTTP_GET, [](AsyncWebServerRequest *request) {
        size_t addr = 0;
        size_t size = 0;

        if (esp_core_dump_image_get(&addr, &size) != ESP_OK || size == 0) {
            request->send(200, "text/plain", "No core dump");
            return;
        }

        AsyncWebServerResponse *response = request->beginChunkedResponse(
            "application/octet-stream",
            [addr, size](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
                if (index >= size) return 0;

                size_t toRead = std::min(maxLen, size - index);

                if (esp_flash_read(NULL, buffer, addr + index, toRead) != ESP_OK) {
                    return 0;
                }

                return toRead;
            }
        );

        response->addHeader("Content-Disposition", "attachment; filename=core_dump.bin");
        request->send(response);
    });

    _server.on("/clear_crashes", HTTP_GET, [](AsyncWebServerRequest *request) {
        esp_err_t err = esp_core_dump_image_erase();

        if (err == ESP_OK) {
            request->send(200, "text/plain", "Core dump erased");
        } else {
            request->send(500, "text/plain", "Failed to erase core dump");
        }
    });

    _server.on("/token_result", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (request->hasParam("code")) {
            String code = request->getParam("code")->value();
            ImageRendererEvent event{};
            event.type = ImageRendererEventType::NETATMO_CODE;
            strncpy(event.code, code.c_str(), sizeof(event.code) - 1);
            event.code[sizeof(event.code) - 1] = '\0';

            if (xQueueSend(renderingQueue, &event, pdMS_TO_TICKS(10)) == pdPASS) {
                request->send(200, "text/plain", "Token negotiation started. You can close this tab.");
            } else {
                request->send(500, "text/plain", "Failed to send event to rendering task");
            }
        } else {
            request->send(400, "text/plain", "Missing code parameter");
        }
    });
}

void WebServerHandler::handleStatus(AsyncWebServerRequest *request) const {
    PsramAllocator allocator;
    JsonDocument doc(&allocator);
    size_t total = psramInit() ? ESP.getPsramSize() : 0;
    doc["authenticated"] = strlen(_hueUsername) > 0;
    doc["username"] = _hueUsername;
    doc["error"] = _errorBuffer;
    doc["totalBytes"] = total;
    doc["usedBytes"] = psramInit() ? total - ESP.getFreePsram() : 0;

    JsonObject netatmo = doc["netatmo"].to<JsonObject>();
    netatmo["authenticated"] = _netatmoToken.accessToken.length() > 0;
    netatmo["expires_in"] = _netatmoToken.expiresIn;
    netatmo["creation_timestamp"] = _netatmoToken.creationTimestamp;
    netatmo["valid"] = _netatmoToken.isValid();

    doc["fsTotal"] = LittleFS.totalBytes();
    doc["fsUsed"] = LittleFS.usedBytes();
    doc["firmware"] = TOSTRING(FIRWARE_VERSION);

    doc["heapTotal"] = ESP.getHeapSize();
    doc["heapFree"] = ESP.getFreeHeap();

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

String WebServerHandler::processor(const String &var) {
    if (var == "NETATMO_CLIENT_ID") {
        return NETATMO_CLIENT_ID;
    }
    return String();
}

void WebServerHandler::handleLogs(AsyncWebServerRequest *request) const {
    PsramAllocator allocator;
    JsonDocument doc(&allocator);

    // Emit oldest -> newest
    const size_t total = _logCount;
    const size_t start = (_logIndex + (LOG_BUFFER_SIZE - total)) % LOG_BUFFER_SIZE;
    for (size_t i = 0; i < total; ++i) {
        const size_t idx = (start + i) % LOG_BUFFER_SIZE;
        doc.add(_logBuffer[idx]);
    }

    AsyncResponseStream *response = request->beginResponseStream("application/json");
    serializeJson(doc, *response);
    request->send(response);
}

void WebServerHandler::handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data,
                                    size_t len, bool final) {
    if (index == 0) {
        // First chunk, open file for WRITE to truncate existing
        if (xSemaphoreTake(fsMutex, pdMS_TO_TICKS(1000))) {
            File file = LittleFS.open("/image.jpg", FILE_WRITE);
            if (!file) {
                LogEvent::post("Failed to open file for writing at index 0\n");
                request->send(500, "text/plain", "Failed to open file for writing");
                return;
            }
            file.write(data, len);
            file.close();
            xSemaphoreGive(fsMutex);
        }
    } else {
        // Subsequent chunks, open file for APPEND
        if (xSemaphoreTake(fsMutex, pdMS_TO_TICKS(1000))) {
            File file = LittleFS.open("/image.jpg", FILE_APPEND);
            if (!file) {
                LogEvent::post("Failed to open file for appending at index %d\n", index);
                // We don't send 500 here because the request is already being handled,
                // but we should probably find a way to signal failure if final=true
                return;
            }
            file.write(data, len);
            file.close();
            xSemaphoreGive(fsMutex);
        }
    }

    if (final) {
        LogEvent::post("Upload complete\n");
    }
}
