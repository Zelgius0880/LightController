//
// Created by Zelgius on 09-03-26.
//

#ifndef LIGHTCONTROLLER_APICLIENT_H
#define LIGHTCONTROLLER_APICLIENT_H


#include <Arduino.h>

struct ApiResponse
{
    int status;
    String body;
};

class ApiClient
{
public:
    ApiClient(String host, int port, bool https = true);

    void setBearerToken(const String& token);

    ApiResponse get(const String& path);
    ApiResponse post(const String& path, const String& jsonBody);

private:
    String _host;
    int _port;
    bool _https;
    String _token;

    ApiResponse request(const String& method, const String& path, const String& body);
};

#endif //LIGHTCONTROLLER_APICLIENT_H