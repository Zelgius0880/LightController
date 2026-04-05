//
// Created by Zelgius on 18-03-26.
//

#include <logger/task_logger.h>
#include <Arduino.h>
#include <esp_task_wdt.h>

// We send a pointer to the string, and a flag telling us if it's in PSRAM


extern QueueHandle_t logQueue;

[[noreturn]] void loggerTask(void *) {
#ifdef ENABLE_LOGS
    LogEvent packet{};

    Serial.println("Logger task started");

    esp_task_wdt_add(nullptr);

    for (;;) {
        esp_task_wdt_add(nullptr);
        if (xQueueReceive(logQueue, &packet, 10)) {
            // 1. Print the message
            if (packet.payload != nullptr) {
                Serial.print(packet.payload);
                Serial.flush();

                // 2. CRITICAL: Free the memory after printing to prevent leaks
                if (packet.isPSRAM) {
                    free(packet.payload);
                }
            }
        }

        taskYIELD();
    }
#endif
}

// Helper for "Fire and Forget" logging of large strings
void LogEvent::post(const char *format, ...) {
#ifdef ENABLE_LOGS
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
#endif
}
