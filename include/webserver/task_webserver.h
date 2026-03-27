//
// Created by Zelgius on 18-03-26.
//

#ifndef LIGHTCONTROLLER_TASK_WEB_SERVER_H
#define LIGHTCONTROLLER_TASK_WEB_SERVER_H
enum class WebServerEventType {
    POST_ERROR,
    CLEAR_ERROR,
    UPDATE_USERNAME,
    PRINT_LOG,
    UPDATE_NETATMO_TOKEN
};

struct WebServerEvent {
    WebServerEventType type;
    char* payload;    // Pointer to PSRAM buffer
    bool isPSRAM;     // Flag to ensure we free it correctly

    static void postError(const char* format, ...);
    static void printLog(const char* format, ...);
    static void updateUsername(const char* username);
    static void updateNetatmoToken(const char* accessToken, const char* refreshToken, long expiresIn, unsigned long creationTimestamp);
    static void clearError();
};

[[noreturn]] void webserverTask(void *pvParameters);


#endif //LIGHTCONTROLLER_TASK_WEB_SERVER_H