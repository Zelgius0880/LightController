//
// Created by Zelgius on 18-03-26.
//

#ifndef LIGHTCONTROLLER_TASK_LOGGER_H
#define LIGHTCONTROLLER_TASK_LOGGER_H
struct LogEvent {
    char* payload;
    bool isPSRAM;

    static void post(const char* format, ...);
};
[[noreturn]] void loggerTask(void *pvParameters);

#endif //LIGHTCONTROLLER_TASK_LOGGER_H