//
// Created by Zelgius on 09-03-26.
//

#include "ApiClient.h"

#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

ApiClient::ApiClient(String host, int port, bool https)
{
    _host = host;
    _port = port;
    _https = https;
}

void ApiClient::setBearerToken(const String& token)
{
    _token = token;
}

ApiResponse ApiClient::get(const String& path)
{
    return request("GET", path, "");
}

ApiResponse ApiClient::post(const String& path, const String& jsonBody)
{
    return request("POST", path, jsonBody);
}

ApiResponse ApiClient::request(const String& method, const String& path, const String& body)
{
    ApiResponse result;
    HTTPClient http;

    String url = (_https ? "https://" : "http://") + _host + path;

    WiFiClient client;
    WiFiClientSecure secureClient;

    if (_https)
    {
        secureClient.setInsecure();  // skip cert validation for now
        http.begin(secureClient, url);
    }
    else
    {
        http.begin(client, url);
    }

    http.addHeader("Content-Type", "application/json");

    if (_token.length() > 0)
        http.addHeader("Authorization", "Bearer " + _token);

    int httpCode;

    if (method == "POST")
        httpCode = http.POST(body);
    else
        httpCode = http.GET();

    result.status = httpCode;

    if (httpCode > 0)
        result.body = http.getString();

    http.end();

    return result;
}