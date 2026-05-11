#include <LittleFS.h>
#include <WebServerHandler.h>

#include "configuration.h"
#include "index_html.h"
#include "logger/task_logger.h"

#include <rendering/task_image_rendering.h>
#include <LightsDatabaseManager.h>
#include "esp_core_dump.h"

extern QueueHandle_t renderingQueue;
extern SemaphoreHandle_t fsMutex;
extern LightsDatabaseManager dbManager;

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

WebServerHandler::WebServerHandler(AsyncWebServer &server)
    : _server(server), _importBuffer(nullptr), _importBufferLen(0) {
}

void WebServerHandler::setSwitchAttributionMode(const bool enable) {
    _switchAttributionModeEnabled = enable;
    if (enable) {
        _lastReceivedSwitchData = 0; // Reset last received data when starting attribution
    }
}

bool WebServerHandler::isSwitchAttributionModeEnabled() const {
    return _switchAttributionModeEnabled;
}

void WebServerHandler::setLastReceivedSwitchData(uint64_t data) {
    _lastReceivedSwitchData = data;
}

uint64_t WebServerHandler::getLastReceivedSwitchData() const {
    return _lastReceivedSwitchData;
}

void WebServerHandler::setup() {
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type");

    _secureClient.setInsecure();

    _server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/html", index_html, processor);
    });

    _server.on("/upload_image", HTTP_OPTIONS, [](AsyncWebServerRequest *request) {
        request->send(204); // "No Content" is standard for a preflight success
    });

    // 1. Create the handler
    AsyncCallbackWebHandler *handler = &_server.on("/upload_image", HTTP_POST, [](AsyncWebServerRequest *request) {
        request->send(200, "application/json", R"({"status":"ok"})");
    });

    // 2. Attach the Body handler to it
    handler->onBody([](AsyncWebServerRequest *request, const uint8_t *data, const size_t len, const size_t index,
                       const size_t total) {
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

        constexpr ImageRendererEvent event{
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

    _server.on("/crashes", HTTP_GET, [](AsyncWebServerRequest *request) {
        size_t addr = 0;
        size_t size = 0;

        if (esp_core_dump_image_get(&addr, &size) != ESP_OK || size == 0) {
            request->send(200, "text/plain", "No core dump");
            return;
        }

        AsyncWebServerResponse *response = request->beginChunkedResponse(
            "application/octet-stream",
            [addr, size](uint8_t *buffer, const size_t maxLen, const size_t index) -> size_t {
                if (index >= size) return 0;

                const size_t toRead = std::min(maxLen, size - index);

                if (esp_flash_read(nullptr, buffer, addr + index, toRead) != ESP_OK) {
                    return 0;
                }

                return toRead;
            }
        );

        response->addHeader("Content-Disposition", "attachment; filename=core_dump.bin");
        request->send(response);
    });

    _server.on("/clear_crashes", HTTP_GET, [](AsyncWebServerRequest *request) {
        const esp_err_t err = esp_core_dump_image_erase();

        if (err == ESP_OK) {
            request->send(200, "text/plain", "Core dump erased");
        } else {
            request->send(500, "text/plain", "Failed to erase core dump");
        }
    });

    _server.on("/token_result", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (request->hasParam("code")) {
            const String code = request->getParam("code")->value();
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

    _server.on("/export_db", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(LittleFS, DB_PATH, "application/octet-stream", true);
    });

    _server.on("/import_db", HTTP_POST | HTTP_OPTIONS, [](AsyncWebServerRequest *request) {
                   if (request->method() == HTTP_OPTIONS) {
                       handleOption(request);
                       return;
                   }
                   request->send(200, "text/plain", "Import successful");
               },
               [this](const AsyncWebServerRequest *request, const String &, const size_t index, const uint8_t *data,
                      const size_t len,
                      const bool final) {
                   if (index == 0) {
                       const size_t totalSize = request->contentLength();
                       if (totalSize == 0) {
                           return; // Or handle error: cannot allocate unknown size
                       }

                       // Allocate from PSRAM
                       _importBuffer = static_cast<uint8_t *>(ps_malloc(totalSize));
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

    _server.on("/groups", HTTP_GET, [](AsyncWebServerRequest *request) {
        const std::vector<Group> groups = dbManager.getAllGroups();
        JsonDocument doc(&allocator);
        const JsonArray array = doc.to<JsonArray>();
        for (const auto &group: groups) {
            auto obj = array.add<JsonObject>();
            obj["id"] = group.id;
            obj["name"] = group.name;
            obj["brightness"] = group.brightness;
            obj["x"] = group.x;
            obj["y"] = group.y;
            obj["mirek"] = group.mirek;
        }
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });

    _server.on("/groups", HTTP_POST | HTTP_OPTIONS, [](AsyncWebServerRequest *) {
               }, nullptr,
               [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t, size_t) {
                   if (request->method() == HTTP_OPTIONS) {
                       handleOption(request);
                       return;
                   }

                   JsonDocument doc(&allocator);
                   const DeserializationError error = deserializeJson(doc, data, len);
                   if (error) {
                       request->send(400, "application/json", R"({"error":"Invalid JSON"})");
                       return;
                   }

                   Group group;
                   group.id = doc["id"];
                   group.name = doc["name"].as<String>();
                   group.brightness = doc["brightness"];
                   group.x = doc["x"];
                   group.y = doc["y"];
                   group.mirek = doc["mirek"];

                   if (dbManager.upsertGroup(group)) {
                       request->send(200, "application/json", R"({"status":"ok"})");
                   } else {
                       request->send(500, "application/json", R"({"error":"Failed to upsert group"})");
                   }
               });

    _server.on("/groups", HTTP_DELETE | HTTP_OPTIONS, [](AsyncWebServerRequest *request) {
        if (request->method() == HTTP_OPTIONS) {
            handleOption(request);
            return;
        }

        if (!request->hasParam("id")) {
            request->send(400, "application/json", R"({"error":"Missing id parameter"})");
            return;
        }

        const uint64_t groupId = strtoull(request->getParam("id")->value().c_str(), nullptr, 10);
        if (dbManager.deleteGroup(groupId)) {
            request->send(200, "application/json", R"({"status":"ok"})");
        } else {
            request->send(500, "application/json", R"({"error":"Failed to delete group"})");
        }
    });

    _server.on("/lights", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (!request->hasParam("groupId")) {
            request->send(400, "application/json", R"({"error":"Missing groupId parameter"})");
            return;
        }
        const uint64_t groupId = strtoull(request->getParam("groupId")->value().c_str(), nullptr, 10);
        const std::vector<Light> lights = dbManager.getLightsByGroupId(groupId);
        JsonDocument doc(&allocator);
        const auto array = doc.to<JsonArray>();
        for (const auto &light: lights) {
            auto obj = array.add<JsonObject>();
            obj["uid"] = light.uid;
            obj["name"] = light.name;
            obj["state"] = light.state;
            obj["type"] = light.type;
            obj["brightness"] = light.brightness;
            obj["x"] = light.x;
            obj["y"] = light.y;
            obj["mirek"] = light.mirek;
            obj["groupId"] = light.groupId;
        }
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });

    _server.on("/lights", HTTP_POST | HTTP_OPTIONS, [](AsyncWebServerRequest *) {
               }, nullptr,
               [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t, size_t) {
                   if (request->method() == HTTP_OPTIONS) {
                       handleOption(request);
                       return;
                   }

                   JsonDocument doc(&allocator);
                   const DeserializationError error = deserializeJson(doc, data, len);
                   if (error) {
                       request->send(400, "application/json", R"({"error":"Invalid JSON"})");
                       return;
                   }

                   Light light;
                   light.uid = doc["uid"].as<String>();
                   light.name = doc["name"].as<String>();
                   light.state = doc["state"].as<String>();
                   light.type = doc["type"].as<String>();
                   light.brightness = doc["brightness"];
                   light.x = doc["x"];
                   light.y = doc["y"];
                   light.mirek = doc["mirek"];
                   light.groupId = doc["groupId"];

                   if (dbManager.upsertLight(light)) {
                       request->send(200, "application/json", R"({"status":"ok"})");
                   } else {
                       request->send(500, "application/json", R"({"error":"Failed to upsert light"})");
                   }
               });

    _server.on("/lights", HTTP_DELETE | HTTP_OPTIONS, [](AsyncWebServerRequest *request) {
        if (request->method() == HTTP_OPTIONS) {
            handleOption(request);
            return;
        }

        if (!request->hasParam("uid") || !request->hasParam("groupId")) {
            request->send(400, "application/json", R"({"error":"Missing uid or groupId parameter"})");
            return;
        }

        const String uid = request->getParam("uid")->value();
        const uint64_t groupId = strtoull(request->getParam("groupId")->value().c_str(), nullptr, 10);

        if (dbManager.deleteLight(uid, groupId)) {
            request->send(200, "application/json", R"({"status":"ok"})");
        } else {
            request->send(500, "application/json", R"({"error":"Failed to delete light"})");
        }
    });

    _server.on("/switches", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (!request->hasParam("groupId")) {
            request->send(400, "application/json", R"({"error":"Missing groupId parameter"})");
            return;
        }
        const uint64_t groupId = strtoull(request->getParam("groupId")->value().c_str(), nullptr, 10);
        const std::vector<Switch> switches = dbManager.getSwitchesByGroupId(groupId);
        JsonDocument doc(&allocator);
        const auto array = doc.to<JsonArray>();
        for (const auto &sw: switches) {
            auto obj = array.add<JsonObject>();
            obj["uid"] = sw.uid;
            obj["name"] = sw.name;
            obj["groupId"] = sw.groupId;
        }
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });

    _server.on("/switch_attribution/check", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (!request->hasParam("uid")) {
            request->send(400, "application/json", R"({"error":"Missing uid parameter"})");
            return;
        }
        const String uid = request->getParam("uid")->value();
        const Group group = dbManager.getGroupBySwitchUid(uid);
        JsonDocument doc;
        if (group.id > 0) {
            doc["exists"] = true;
            doc["id"] = group.id;
            doc["name"] = group.name;
        } else {
            doc["exists"] = false;
        }
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });

    _server.on("/switches", HTTP_POST | HTTP_OPTIONS, [](AsyncWebServerRequest *) {
               }, nullptr,
               [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t, size_t) {
                   if (request->method() == HTTP_OPTIONS) {
                       handleOption(request);
                       return;
                   }

                   JsonDocument doc(&allocator);
                   const DeserializationError error = deserializeJson(doc, data, len);
                   if (error) {
                       request->send(400, "application/json", R"({"error":"Invalid JSON"})");
                       return;
                   }

                   Switch sw;
                   sw.uid = doc["uid"].as<String>();
                   sw.name = doc["name"].as<String>();
                   sw.groupId = doc["groupId"];

                   if (dbManager.upsertSwitch(sw)) {
                       request->send(200, "application/json", R"({"status":"ok"})");
                   } else {
                       request->send(500, "application/json", R"({"error":"Failed to upsert switch"})");
                   }
               });

    _server.on("/switches", HTTP_DELETE | HTTP_OPTIONS, [](AsyncWebServerRequest *request) {
        if (request->method() == HTTP_OPTIONS) {
            handleOption(request);
            return;
        }

        if (!request->hasParam("uid")) {
            request->send(400, "application/json", R"({"error":"Missing uid parameter"})");
            return;
        }

        const String uid = request->getParam("uid")->value();
        if (dbManager.deleteSwitch(uid)) {
            request->send(200, "application/json", R"({"status":"ok"})");
        } else {
            request->send(500, "application/json", R"({"error":"Failed to delete switch"})");
        }
    });

    _server.on("/switch_attribution/mode", HTTP_POST | HTTP_OPTIONS, [](AsyncWebServerRequest *) {
               }, nullptr,
               [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t, size_t) {
                   if (request->method() == HTTP_OPTIONS) {
                       handleOption(request);
                       return;
                   }

                   JsonDocument doc(&allocator);
                   const DeserializationError error = deserializeJson(doc, data, len);
                   if (error) {
                       request->send(400, "application/json", R"({"error":"Invalid JSON"})");
                       return;
                   }

                   const bool enabled = doc["enabled"];
                   setSwitchAttributionMode(enabled);
                   request->send(200, "application/json", R"({"status":"ok"})");
               });

    _server.on("/switch_attribution/data", HTTP_GET, [this](AsyncWebServerRequest *request) {
        JsonDocument doc;
        doc["enabled"] = isSwitchAttributionModeEnabled();
        if (_lastReceivedSwitchData != 0) {
            char buf[17];
            sprintf(buf, "%012llx", _lastReceivedSwitchData);
            doc["last_data"] = buf;
        } else {
            doc["last_data"] = static_cast<char *>(nullptr);
        }
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });

    _server.on("/proxy", HTTP_OPTIONS, [](AsyncWebServerRequest *request) {
        handleOption(request);
    });

    _server.on("/proxy", HTTP_ANY, [this](AsyncWebServerRequest *request) {
                   // This part handles GET or the 'finalization' of a POST
                   if (request->method() == HTTP_GET || request->method() == HTTP_OPTIONS) {
                       processProxy(request, "");
                   }
               }, nullptr, [this](AsyncWebServerRequest *request, const uint8_t *data, size_t len, size_t index,
                                  size_t total) {
                   // This part handles the incoming Body (POST/PUT)
                   for (size_t i = 0; i < len; i++) {
                       _globalBodyBuffer += static_cast<char>(data[i]);
                   }

                   if (index + len == total) {
                       processProxy(request, _globalBodyBuffer);
                       _globalBodyBuffer = ""; // Clear for next request
                   }
               });
}

void WebServerHandler::handleOption(AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse(204); // 204 = No Content
    response->addHeader("Access-Control-Allow-Origin", "*");
    response->addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTION");
    response->addHeader("Access-Control-Allow-Headers", "*");
    response->addHeader("Access-Control-Max-Age", "86400"); // Cache preflight for 24h
    request->send(response);
}

void WebServerHandler::processProxy(AsyncWebServerRequest *request, const String &body) {
    const String targetUrl = request->getParam("url")->value();
    _http.begin(_secureClient, targetUrl);

    // Transparently forward the method and body

    const size_t headersCount = request->headers();
    for (int i = 0; i < headersCount; i++) {
        const AsyncWebHeader *h = request->getHeader(i);
        String name = h->name();
        // Skip headers that should be handled by the ESP32 itself
        if (!name.equalsIgnoreCase("Host") &&
            !name.equalsIgnoreCase("Content-Length") &&
            !name.equalsIgnoreCase("Connection")) {
            _http.addHeader(name, h->value());
        }
    }

    const int httpCode = _http.sendRequest(request->methodToString(), body);
    if (httpCode > 0) {
        AsyncWebServerResponse *response = request->beginResponse(httpCode, request->header("accept"),
                                                                  _http.getString());
        // The essential CORS headers for the KMP App
        response->addHeader("Access-Control-Allow-Origin", "*");
        response->addHeader("Access-Control-Allow-Methods", "GET, POST, PATCH, PUT, DELETE, OPTIONS");
        response->addHeader("Access-Control-Allow-Headers", "*");
        request->send(response);
    } else {
        request->send(502, "text/plain", "Proxy Error");
    }
    _http.end();
}

String WebServerHandler::processor(const String &var) {
    if (var == "NETATMO_CLIENT_ID") {
        return NETATMO_CLIENT_ID;
    } else if (var == "LOCAL_IP") {
        return WiFi.localIP().toString();
    }
    return {};
}
