#include <LittleFS.h>
#include <WebServerHandler.h>

#include "configuration.h"
#include "index_html.h"
#include "logger/task_logger.h"

#include <rendering/task_image_rendering.h>
#include <ImageRenderer.h>
#include <LightsDatabaseManager.h>
#include "esp_core_dump.h"

extern QueueHandle_t renderingQueue;
extern SemaphoreHandle_t fsMutex;
extern LightsDatabaseManager dbManager;

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

WebServerHandler::WebServerHandler(AsyncWebServer &server)
    : _server(server), _hueUsername(""), _errorBuffer{}, _logBuffer{}, _importBuffer(nullptr), _importBufferLen(0) {
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
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type");
    _server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/html", index_html, processor);
    });

    _server.on("/status", HTTP_GET, [this](AsyncWebServerRequest *request) {
        handleStatus(request);
    });

    _server.on("/upload_image", HTTP_OPTIONS, [](AsyncWebServerRequest *request) {
        request->send(204); // "No Content" is standard for a preflight success
    });

    // 1. Create the handler
    AsyncCallbackWebHandler *handler = &_server.on("/upload_image", HTTP_POST, [](AsyncWebServerRequest *request) {
        request->send(200, "application/json", R"({"status":"ok"})");
    });

    // 2. Attach the Body handler to it
    handler->onBody([this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        if (xSemaphoreTake(fsMutex, pdMS_TO_TICKS(5000))) {
            if (!index) {
                // Open file for writing
                request->_tempFile = LittleFS.open("/image.bin", "w");
            }

            if (request->_tempFile) {
                request->_tempFile.write(data, len);
            }

            if (index + len == total) {
                request->_tempFile.close();
            }
            xSemaphoreGive(fsMutex);
        }
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
            if (xSemaphoreTake(ImageRendererEvent::completionSemaphore, pdMS_TO_TICKS(10*60000)) == pdTRUE) {
                request->send(200, "application/json", R"({"status":"ok"})");
            } else {
                request->send(504, "text/plain", "Render Timeout");
            }
        } else {
            request->send(503, "text/plain", "Rendering Queue Full");
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

    _server.on("/export_db", HTTP_GET, [this](AsyncWebServerRequest *request) {
        request->send(LittleFS, DB_PATH, "application/octet-stream", true);
    });

    _server.on("/import_db", HTTP_POST, [this](AsyncWebServerRequest *request) {
                   request->send(200, "text/plain", "Import successful");
               },
               [this](AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len,
                      bool final) {
                   if (index == 0) {
                       size_t totalSize = request->contentLength();
                       if (totalSize == 0) {
                           return; // Or handle error: cannot allocate unknown size
                       }

                       // Allocate from PSRAM
                       _importBuffer = (uint8_t *) ps_malloc(totalSize);
                       if (_importBuffer == nullptr) {
                           LogEvent::post("Failed to allocate PSRAM for DB import\n");
                           return;
                       }
                       _importBufferLen = totalSize;
                   }

                   if (_importBuffer) {
                       memcpy(_importBuffer + index, data, len);
                   }

                   if (final && _importBuffer) {
                       bool success = false;

                       // Take FS mutex to prevent concurrent access
                       if (xSemaphoreTake(fsMutex, pdMS_TO_TICKS(5000))) {
                           success = dbManager.importDatabase(_importBuffer, _importBufferLen);
                           xSemaphoreGive(fsMutex);
                       }

                       // CRITICAL: Always free PSRAM after the import attempt
                       free(_importBuffer);
                       _importBuffer = nullptr;
                       _importBufferLen = 0;

                       if (success) {
                           LogEvent::post("Database imported from PSRAM successfully\n");
                       } else {
                           LogEvent::post("Failed to import database from PSRAM\n");
                       }
                   }
               });
}

void WebServerHandler::handleStatus(AsyncWebServerRequest *request) const {
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
    LogEvent::post("Uploading image chunk %d\n", index);
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
        } else {
            LogEvent::post("Get MUTEX failed");
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
        } else {
            LogEvent::post("Get MUTEX failed");
        }
    }

    if (final) {
        LogEvent::post("Upload complete\n");
    }
}
