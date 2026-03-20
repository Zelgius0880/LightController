//
// Created by Zelgius on 18-03-26.
//

#ifndef LIGHTCONTROLLER_TASK_IMAGE_RENDERING_H
#define LIGHTCONTROLLER_TASK_IMAGE_RENDERING_H

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

enum class ImageRendererEventType {
    RENDER_IMAGE,
};

struct ImageRendererEvent {
    ImageRendererEventType type;
    static SemaphoreHandle_t completionSemaphore;
};

[[noreturn]] void imageRenderingTask(void *pvParameters);


#endif //LIGHTCONTROLLER_TASK_IMAGE_RENDERING_H