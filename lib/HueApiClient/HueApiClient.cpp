//
// Created by Zelgius on 09-03-26.
//

#include "HueApiClient.h"

#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <webserver/task_webserver.h>
#include <logger/task_logger.h>
#include <LittleFS.h>

#include "configuration.h"
#include "allocator/psram_allocator.h"
#include "leds/task_leds.h"

extern SemaphoreHandle_t fsMutex;

HueApiClient::HueApiClient(const String &host, const int port, const bool https) {
    _host = host;
    _port = port;
    _https = https;
}

void HueApiClient::setApiKey(const String &apiKey) {
    _apiKey = apiKey;
}

Hue::AuthResponse HueApiClient::authenticate(const String &deviceType, bool generateClientKey) {
    JsonDocument doc;
    doc["devicetype"] = deviceType;
    if (generateClientKey) {
        doc["generateclientkey"] = true;
    }

    return post("/api", doc).asAuth();
}

Hue::Response<Hue::Resource> HueApiClient::getResources() {
    return get("/clip/v2/resource").as<Hue::Resource>();
}

Hue::Response<Hue::Device> HueApiClient::getDevices() {
    return get("/clip/v2/resource/device").as<Hue::Device>();
}

Hue::Response<Hue::Device> HueApiClient::getDevice(const String &id) {
    return get("/clip/v2/resource/device/" + id).as<Hue::Device>();
}

Hue::Response<Hue::Light> HueApiClient::getLights() {
    return get("/clip/v2/resource/lights").as<Hue::Light>();
}

Hue::Response<Hue::Light> HueApiClient::getLight(const String &id) {
    return get("/clip/v2/resource/light/" + id).as<Hue::Light>();
}

Hue::Response<Hue::ResourceIdentifier> HueApiClient::updateLight(const String &id, const JsonVariantConst &body) {
    return put("/clip/v2/resource/light/" + id, body).as<Hue::ResourceIdentifier>();
}

ApiResponse HueApiClient::get(const String &path) {
    return request("GET", path, "");
}

ApiResponse HueApiClient::post(const String &path, const JsonVariantConst &body) {
    String bodyStr;
    serializeJson(body, bodyStr);
    return request("POST", path, bodyStr);
}

ApiResponse HueApiClient::put(const String &path, const JsonVariantConst &body) {
    String bodyStr;
    serializeJson(body, bodyStr);
    return request("PUT", path, bodyStr);
}

ApiResponse HueApiClient::request(const String &method, const String &path, const String &body) {
    ApiResponse result;

    String url = (_https ? "https://" : "http://") + _host + ":" + String(_port) + path;

    if (_https) {
        _secureClient.setInsecure();
        _http.begin(_secureClient, url);
    } else {
        _http.begin(_client, url);
    }

    _http.addHeader("Content-Type", "application/json");
    if (_apiKey.length() > 0) _http.addHeader("hue-application-key", _apiKey);

    const int httpCode = _http.sendRequest(method.c_str(), body);
    result.status = httpCode;

    if (httpCode > 0) {
        String responseBody = _http.getString();
# ifdef DEBUG
        Serial.printf("[HUE] Response Body: %s\n", responseBody.c_str());
#endif
        deserializeJson(result.body, responseBody);
    }

    _http.end();
    return result;
}

void HueApiClient::handleAuthentication() {
    if (_username.length() == 0) {
        if (millis() - _lastAuthAttempt > 20000) {
            _lastAuthAttempt = millis();

            const Hue::AuthResponse authResponse = authenticate("LightController#controller");

            if (!authResponse.successes.empty()) {
                _username = authResponse.successes[0].username;
                _clientKey = authResponse.successes[0].clientKey;
                setApiKey(_username);
                WebServerEvent::clearError();

                if (saveCredentials(_username, _clientKey)) {
                    LogEvent::post("Credentials saved successfully\n");
                }

                LedEvent::plain(0, 128, 0); // Green for success
                LogEvent::post("Authenticated: %s\n", _username.c_str());
            } else if (!authResponse.errors.empty()) {
                if (authResponse.errors[0].type == 101) {
                    // Link button not pressed
                    LedEvent::plain(128, 128, 0); // Orange/Yellow for alert
                    LogEvent::post("Press the link button...\n");
                    WebServerEvent::postError("Press the link button on the bridge");
                } else {
                    // Other error
                    LedEvent::plain(128, 0, 0); // Red for error
                    LogEvent::post("Auth Error: %s\n", authResponse.errors[0].description.c_str());
                    WebServerEvent::postError(authResponse.errors[0].description.c_str());
                }
            } else if (authResponse.status != 200) {
                // HTTP error or something went wrong
                LedEvent::plain(128, 0, 0); // Red for error
                LogEvent::post("Auth HTTP Status: %d\n", authResponse.status);
                WebServerEvent::postError("HTTP Error: %d", authResponse.status);
            }
        }
    } else {
        if (!_authenticated) {
            _authenticated = true;
            Serial.println("Using existing credentials");
            LedEvent::plain(0, 128, 0); // Green for everything is alright
        }
    }
}

bool HueApiClient::loadCredentials() {
    JsonDocument doc;
    DeserializationError error;
    if (xSemaphoreTake(fsMutex, pdMS_TO_TICKS(1000))) {
        if (!LittleFS.exists(CREDENTIALS_FILE)) {
            xSemaphoreGive(fsMutex);
            return false;
        }

        File file = LittleFS.open(CREDENTIALS_FILE, FILE_READ);
        if (!file) {
            xSemaphoreGive(fsMutex);
            return false;
        }

        error = deserializeJson(doc, file);
        file.close();
        xSemaphoreGive(fsMutex);
    }
    if (error) {
        LogEvent::post("deserializeJson error\n");
        return false;
    }

    _username = doc["username"].as<String>();
    _clientKey = doc["clientkey"].as<String>();

    LogEvent::post("User name %s\n", _username.c_str());

    if (_username.length() > 0) {
        setApiKey(_username);
        return true;
    }


    return false;
}

bool HueApiClient::saveCredentials(const String &username, const String &clientKey) {
    if (xSemaphoreTake(fsMutex, pdMS_TO_TICKS(1000))) {
        File file = LittleFS.open(CREDENTIALS_FILE, FILE_WRITE);
        if (!file) {
            return false;
        }

        JsonDocument doc;
        doc["username"] = username;
        doc["clientkey"] = clientKey;

        if (serializeJson(doc, file) == 0) {
            file.close();
            return false;
        }

        file.close();
        xSemaphoreGive(fsMutex);
    }
    return true;
}
