//
// Created by Zelgius on 18-03-26.
//

#ifndef LIGHTCONTROLLER_LIGHTS_EVENTS_H
#define LIGHTCONTROLLER_LIGHTS_EVENTS_H

#include <Arduino.h>

enum class LightEventType {
    SWITCH_PRESSED,
};

struct LightEvent {
    LightEventType type;
    struct {
        uint64_t switchUid;
    } data;

    static bool switchPressed(uint64_t switchUid);
};

[[noreturn]]  void lightControllerTask(void *pvParameters);

#endif //LIGHTCONTROLLER_LIGHTS_EVENTS_H