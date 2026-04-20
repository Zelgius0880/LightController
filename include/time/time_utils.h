//
// Created by Zelgius on 15-04-26.
//

#ifndef LIGHTCONTROLLER_TIME_UTILS_H
#define LIGHTCONTROLLER_TIME_UTILS_H
#include <ctime>

#include "logger/task_logger.h"

inline time_t epoch_time(const bool local = false) {
    time_t now;
    if (!local) {
        time(&now);
        return now;
    }

    tm timeInfo;
    getLocalTime(&timeInfo);

    const auto time =  mktime(&timeInfo);
    return time;
}

inline void time_string(char *timeStringBuff) {
    tm timeInfo{};
    getLocalTime(&timeInfo);
    strftime(timeStringBuff, 50, "%Y-%m-%d %H:%M:%S", &timeInfo);
}

#endif //LIGHTCONTROLLER_TIME_UTILS_H
