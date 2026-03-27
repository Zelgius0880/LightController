//
// Created by Zelgius on 20-03-26.
//

#ifndef LIGHTCONTROLLER_TASK_BUZZER_H
#define LIGHTCONTROLLER_TASK_BUZZER_H

#include <Arduino.h>

#define BUZZER_PIN 1

enum class BuzzerType {
    BIP,
    BIP2,
    MELODY
};

struct BuzzerEvent {
    BuzzerType type;
    uint32_t frequency;
    uint32_t duration;

    static bool post(BuzzerType type, uint32_t frequency = 1000, uint32_t duration = 100);
    static void bip(uint32_t frequency = 1000, uint32_t duration = 100);
    static void bip2(uint32_t frequency = 1000, uint32_t duration = 100);
    static void melody();
};

[[noreturn]] void buzzerTask(void *pvParameters);

#endif //LIGHTCONTROLLER_TASK_BUZZER_H