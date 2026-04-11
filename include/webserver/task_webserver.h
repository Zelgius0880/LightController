//
// Created by Zelgius on 18-03-26.
//

#ifndef LIGHTCONTROLLER_TASK_WEB_SERVER_H
#define LIGHTCONTROLLER_TASK_WEB_SERVER_H
enum class WebServerEventType {
};

struct WebServerEvent {
    WebServerEventType type;
    char* payload;    // Pointer to PSRAM buffer
    bool isPSRAM;     // Flag to ensure we free it correctly
};

[[noreturn]] void webserverTask(void *pvParameters);


#endif //LIGHTCONTROLLER_TASK_WEB_SERVER_H