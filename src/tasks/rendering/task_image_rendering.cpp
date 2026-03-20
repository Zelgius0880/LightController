//
// Created by Zelgius on 18-03-26.
//

#include <rendering/task_image_rendering.h>
#include <Arduino.h>

#include <ImageRenderer.h>

#include "logger/task_logger.h"

extern QueueHandle_t renderingQueue;
SemaphoreHandle_t ImageRendererEvent::completionSemaphore = nullptr;
ImageRenderer *ImageRenderer::instance = nullptr;

ImageRenderer renderer;

[[noreturn]] void imageRenderingTask(void *pvParameters) {
    if (ImageRendererEvent::completionSemaphore == nullptr) {
        ImageRendererEvent::completionSemaphore = xSemaphoreCreateBinary();
    }
    ImageRendererEvent event{};

    for (;;) {
        if (xQueueReceive(renderingQueue, &event, portMAX_DELAY)) {
            bool success = false;
            switch (event.type) {
                case ImageRendererEventType::RENDER_IMAGE:
                    LogEvent::post("Rendering image\n");
                    success = renderer.renderImage();
                    LogEvent::post("Image rendered\n");
                    if (success && ImageRendererEvent::completionSemaphore != nullptr) {
                        xSemaphoreGive(ImageRendererEvent::completionSemaphore);
                    }

                    break;
                default:
                    break;
            }


        }
    }
}
