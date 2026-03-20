//
// Created by Zelgius on 09-03-26.
//

#ifndef LIGHTCONTROLLER_HUEAPICLIENT_H
#define LIGHTCONTROLLER_HUEAPICLIENT_H


#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>
#include <leds/task_leds.h>

#include "HueModels.h"

struct ApiResponse
{
    int status;
    JsonDocument body;

    template<typename T>
    Hue::Response<T> as() const {
        return Hue::Response<T>::fromJson(body, status);
    }

    Hue::AuthResponse asAuth() const {
        return Hue::AuthResponse::fromJson(body, status);
    }
};

class HueApiClient
{
public:
    HueApiClient(const String &host, int port = 443, bool https = true, bool debug = false);

    void setApiKey(const String& apiKey);
    void setDebug(bool debug);

    // Auth
    Hue::AuthResponse authenticate(const String& deviceType, bool generateClientKey = true);

    // Resources
    Hue::Response<Hue::Resource> getResources();

    // Devices
    Hue::Response<Hue::Device> getDevices();
    Hue::Response<Hue::Device> getDevice(const String& id);

    // Lights
    Hue::Response<Hue::Light> getLights();
    Hue::Response<Hue::Light> getLight(const String& id);
    Hue::Response<Hue::ResourceIdentifier> updateLight(const String& id, const JsonVariantConst& body);

    // Generic methods
    ApiResponse get(const String& path);
    ApiResponse put(const String& path, const JsonVariantConst& body);
    ApiResponse post(const String& path, const JsonVariantConst& body);
    bool loadCredentials();
    void handleAuthentication();
    bool isAuthenticated() const {return _authenticated;};
    String getUsername() const {return _username;};


private:
    String _host;
    int _port;
    bool _https;
    bool _debug;
    String _apiKey;
    String _clientKey;
    String _username;
    bool _authenticated = false;
    unsigned long _lastAuthAttempt = 0;
    ApiResponse request(const String& method, const String& path, const String& body) const;

    static bool saveCredentials(const String &username, const String &clientKey);
};

#endif //LIGHTCONTROLLER_HUEAPICLIENT_H