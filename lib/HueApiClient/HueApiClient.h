//
// Created by Zelgius on 09-03-26.
//

#ifndef LIGHTCONTROLLER_HUEAPICLIENT_H
#define LIGHTCONTROLLER_HUEAPICLIENT_H

//#define DEBUG_HUE
//#define DEBUG 1

#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

#include "HueModels.h"

#include "allocator/psram_allocator.h"

#ifdef DEBUG_HUE
#define HUE_LOG(...) LogEvent::post(__VA_ARGS__)
#else
#define HUE_LOG(...)
#endif

extern PsramAllocator allocator;

struct ApiResponse {
    int status;

    JsonDocument body{&allocator};

    template<typename T>
    Hue::Response<T> as() const {
        return Hue::Response<T>::fromJson(body, status);
    }

    Hue::AuthResponse asAuth() const {
        return Hue::AuthResponse::fromJson(body, status);
    }
};

class HueApiClient {
public:
    HueApiClient(WiFiClientSecure &client, const String &host, int port = 443, bool https = true);

    void setApiKey(const String &apiKey);

    // Auth
    Hue::AuthResponse authenticate(const String &deviceType, bool generateClientKey = true);

    // Resources
    Hue::Response<Hue::Resource> getResources();

    // Devices
    Hue::Response<Hue::Device> getDevices();

    Hue::Response<Hue::Device> getDevice(const String &id);

    // Lights
    Hue::Response<Hue::Light> getLights();

    Hue::Response<Hue::Light> getLight(const String &id);

    Hue::Response<Hue::ResourceIdentifier> updateLight(const String &id, const JsonVariantConst &body);

    // Generic methods
    ApiResponse get(const String &path);

    ApiResponse put(const String &path, const JsonVariantConst &body);

    ApiResponse post(const String &path, const JsonVariantConst &body);

    bool loadCredentials();

    void handleAuthentication();

    bool isAuthenticated() const { return _authenticated; };
    String getUsername() const { return _username; };

private:
    String _host;
    int _port;
    bool _https;
    String _apiKey;
    String _clientKey;
    String _username;
    bool _authenticated = false;
    unsigned long _lastAuthAttempt = 0;

    ApiResponse request(const String &method, const String &path, const String &body);

    static bool saveCredentials(const String &username, const String &clientKey);

    HTTPClient _http;
    WiFiClientSecure &_secureClient;
    WiFiClient _client;
};

#endif //LIGHTCONTROLLER_HUEAPICLIENT_H
