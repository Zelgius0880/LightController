//
// Created by Zelgius on 18-03-26.
//
#include <Arduino.h>

#include <WebServerHandler.h>
#include <webserver/task_webserver.h>
#include <logger/task_logger.h>


extern QueueHandle_t webserverQueue;

AsyncWebServer server(80);
WebServerHandler handler(server);

[[noreturn]] void webserverTask(void *) {
    server.begin();
    handler.setup();

    LogEvent::post("Webserver task started\n");
    WebServerEvent event{};

    server.begin();

    for (;;) {
        if (xQueueReceive(webserverQueue, &event, portMAX_DELAY)) {
            switch(event.type) {
                case WebServerEventType::POST_ERROR:
                    handler.setErrorMessage(event.payload);
                    // Update your internal handler state here
                    break;

                case WebServerEventType::UPDATE_USERNAME:
                    handler.setHueUsername(event.payload);
                    break;

                case WebServerEventType::CLEAR_ERROR:
                    handler.setErrorMessage("");
                    break;

                case WebServerEventType::PRINT_LOG:
                    handler.addLogMessage(event.payload);
                    break;
            }

            // CRITICAL: Clean up memory after processing
            if (event.isPSRAM && event.payload != nullptr) {
                free(event.payload);
            }
        }
    }
}

void WebServerEvent::postError(const char* format, ...) {
    const auto buffer = static_cast<char *>(ps_malloc(256)); // Use 8MB PSRAM
    if (!buffer) return;

    va_list args;
    va_start(args, format);
    vsnprintf(buffer, 256, format, args);
    va_end(args);

    WebServerEvent event = { WebServerEventType::POST_ERROR, buffer, true };
    if (xQueueSend(webserverQueue, &event, 0) != pdPASS) {
        free(buffer); // Prevent leak if queue is full
    }
}

void WebServerEvent::printLog(const char* format, ...) {
    char* buffer = (char*)ps_malloc(256); // Use 8MB PSRAM
    if (!buffer) return;

    va_list args;
    va_start(args, format);
    vsnprintf(buffer, 256, format, args);
    va_end(args);

    WebServerEvent event = { WebServerEventType::PRINT_LOG, buffer, true };
    if (xQueueSend(webserverQueue, &event, 0) != pdPASS) {
        free(buffer); // Prevent leak if queue is full
    }
}

void WebServerEvent::updateUsername(const char* username) {
    char* buffer = (char*)ps_malloc(64);
    if (!buffer) return;

    strncpy(buffer, username, strlen(username) +1);
    WebServerEvent event = { WebServerEventType::UPDATE_USERNAME, buffer, true };

    if (xQueueSend(webserverQueue, &event, 0) != pdPASS) {
        free(buffer);
    }
}

void WebServerEvent::clearError() {
    WebServerEvent event = { WebServerEventType::CLEAR_ERROR, nullptr, false };
    xQueueSend(webserverQueue, &event, 0);
}

