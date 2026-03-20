//
// Created by Zelgius on 18-03-26.
//

#ifndef LIGHTCONTROLLER_TASK_LEDS_H
#define LIGHTCONTROLLER_TASK_LEDS_H

#include <Arduino.h>

struct LedEvent {
    uint8_t r, g, b;
    uint8_t brightness;
    bool effectActive;
    bool repeat;
    uint16_t effectSpeed;

    static bool post(uint8_t r, uint8_t g, uint8_t b, uint8_t brightness, bool blink, uint16_t repeat, uint16_t speed);
    static void off();
    static void blink(uint8_t r, uint8_t g, uint8_t b, uint16_t repeat = 0, uint16_t speed = 500, uint8_t brightness = 64);
    static void plain(uint8_t r, uint8_t g, uint8_t b, uint8_t brightness = 64);
};

[[noreturn]] void ledsTask(void *pvParameters);

#endif //LIGHTCONTROLLER_TASK_LEDS_H
