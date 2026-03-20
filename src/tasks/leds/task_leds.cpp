//
// Created by Zelgius on 18-03-26.
//
#include <leds/task_leds.h>
#include <logger/task_logger.h>
#include <Arduino.h>

extern QueueHandle_t ledQueue;

[[noreturn]] void ledsTask(void *pvParameters) {
    // Initial state: Off
    LedEvent currentCmd = {0, 0, 0, 0, false, false, 500};
    bool isOn = false;
    uint16_t repeatCounter = 0;

    LogEvent::post("Led task started\n");

    for (;;) {
        LedEvent receivedEvent{};

        // 1. Determine wait time based on whether we are blinking
        TickType_t xWait = (currentCmd.effectActive)
                               ? pdMS_TO_TICKS(currentCmd.effectSpeed)
                               : portMAX_DELAY;

        // 2. Wait for a new command
        if (xQueueReceive(ledQueue, &receivedEvent, xWait)) {
            currentCmd = receivedEvent;
            repeatCounter = currentCmd.repeat;
            isOn = true; // Start the blink cycle immediately
        }

        // 3. Execution Logic
        if (currentCmd.effectActive) {
            if (isOn) {
                // Apply global brightness scaling to the colors
                uint8_t r = (currentCmd.r * currentCmd.brightness) / 255;
                uint8_t g = (currentCmd.g * currentCmd.brightness) / 255;
                uint8_t b = (currentCmd.b * currentCmd.brightness) / 255;
                neopixelWrite(RGB_BUILTIN, r, g, b);
            } else {
                neopixelWrite(RGB_BUILTIN, 0, 0, 0);
            }

            isOn = !isOn; // Toggle for next blink

            // 4. Handle Repeat Count
            if (currentCmd.repeat > 0) {
                if (isOn) {
                    // One full blink (On -> Off) completed
                    repeatCounter--;
                    if (repeatCounter == 0) {
                        currentCmd.effectActive = false;
                        neopixelWrite(RGB_BUILTIN, 0, 0, 0);
                    }
                }
            }
        } else {
            // Static Light (Not blinking)
            uint8_t r = (currentCmd.r * currentCmd.brightness) / 255;
            uint8_t g = (currentCmd.g * currentCmd.brightness) / 255;
            uint8_t b = (currentCmd.b * currentCmd.brightness) / 255;
            neopixelWrite(RGB_BUILTIN, r, g, b);
        }
    }
}


void LedEvent::plain(const uint8_t r, const uint8_t g, const uint8_t b, const uint8_t brightness) {
    post(r, g, b, brightness, false, 0, 0);
}

void LedEvent::blink(const uint8_t r, const uint8_t g, const uint8_t b, const uint16_t repeat, const uint16_t speed,
                    const uint8_t brightness) {
    post(r, g, b, brightness, true, repeat, speed);
}

void LedEvent::off() {
    post(0, 0, 0, 0, false, 0, 0);
}

bool LedEvent::post(const uint8_t r, uint8_t g, const uint8_t b, const uint8_t brightness, const bool blink,
                   const uint16_t repeat, const uint16_t speed) {
    if (ledQueue == nullptr) return false;

    LedEvent event{};
    event.r = r;
    event.g = g;
    event.b = b;
    event.brightness = brightness;
    event.effectActive = blink;
    event.repeat = repeat;
    event.effectSpeed = speed;

    // Send to queue. We use a 10ms timeout just in case the queue is momentarily blocked.
    if (xQueueSend(ledQueue, &event, pdMS_TO_TICKS(10)) != pdPASS) {
        LogEvent::post("[ERROR] Lighting Queue Full!\n");
        return false;
    }
    return true;
}
