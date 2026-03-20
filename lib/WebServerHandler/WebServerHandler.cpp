#include <LittleFS.h>
#include <WebServerHandler.h>

#include "index_html.h"
#include "logger/task_logger.h"

#include <rendering/task_image_rendering.h>
#include "../../lib/ImageRenderer/ImageRenderer.h"

extern QueueHandle_t renderingQueue;
extern SemaphoreHandle_t fsMutex;

struct PsramAllocator : Allocator {
    virtual ~PsramAllocator() = default;

    void *allocate(size_t size) override { return ps_malloc(size); }
    void deallocate(void *ptr) override { free(ptr); }
    void *reallocate(void *ptr, size_t new_size) override { return ps_realloc(ptr, new_size); }
};

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
    _server.on("/", HTTP_GET, [this](AsyncWebServerRequest *request) {
        handleRoot(request);
    });

    _server.on("/status", HTTP_GET, [this](AsyncWebServerRequest *request) {
        handleStatus(request);
    });


    _server.on("/upload", HTTP_POST, [](AsyncWebServerRequest *request) {
                   request->send(200, "text/plain", "OK");
               }, [this](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len,
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


    _server.on("/image.jpg", HTTP_GET, [this](AsyncWebServerRequest *request) {
        handleImage(request);
    });

    _server.on("/logs", HTTP_GET, [this](AsyncWebServerRequest *request) {
        handleLogs(request);
    });
}


void WebServerHandler::handleRoot(AsyncWebServerRequest *request) {
    request->send(200, "text/html", index_html);
}

void WebServerHandler::handleStatus(AsyncWebServerRequest *request) const {
    JsonDocument doc;
    size_t total = psramInit() ? ESP.getPsramSize() : 0;
    doc["authenticated"] = strlen(_hueUsername) > 0;
    doc["username"] = _hueUsername;
    doc["error"] = _errorBuffer;
    doc["totalBytes"] = total;
    doc["usedBytes"] = psramInit() ? total - ESP.getFreePsram() : 0;

    doc["fsTotal"] = LittleFS.totalBytes();
    doc["fsUsed"] = LittleFS.usedBytes();

    doc["heapTotal"] = ESP.getHeapSize();
    doc["heapFree"] = ESP.getFreeHeap();

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

void WebServerHandler::handleImage(AsyncWebServerRequest *request) {
    if (LittleFS.exists("/image.jpg")) {
        request->send(LittleFS, "/image.jpg", "image/jpeg");
    } else {
        request->send(404, "text/plain", "Image not found");
    }
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
