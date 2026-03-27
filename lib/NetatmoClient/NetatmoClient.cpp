#include "NetatmoClient.h"
#include <LittleFS.h>
#include "configuration.h"
#include "logger/task_logger.h"

extern SemaphoreHandle_t fsMutex;
extern PsramAllocator allocator;

NetatmoClient::NetatmoClient(WiFiClientSecure& client) : _secureClient(client) {
    _secureClient.setInsecure();
}

NetatmoClient::~NetatmoClient() {
}

bool NetatmoClient::begin() {
    loadToken();
    return true;
}

bool NetatmoClient::isAuthenticated() {
    if (!_token.isValid()) {
        if (_token.refreshToken.length() > 0) {
            return refreshToken();
        }
        return false;
    }
    return true;
}

bool NetatmoClient::getToken(const String &code) {
    String body = "grant_type=authorization_code";
    body += "&scope=read_station";
    body += "&code=" + code;
    body += "&redirect_uri=http://" + WiFi.localIP().toString() + "/token_result";
    body += "&client_id=" + String(NETATMO_CLIENT_ID);
    body += "&client_secret=" + String(NETATMO_CLIENT_SECRET);

    JsonDocument doc(&allocator);
    int status = sendRequest("POST", "https://api.netatmo.com/oauth2/token", body, doc, false);
    return handleTokenResponse(status, doc);
}

bool NetatmoClient::refreshToken() {
    if (_token.refreshToken.length() == 0) return false;

    String body = "grant_type=refresh_token";
    body += "&refresh_token=" + _token.refreshToken;
    body += "&client_id=" + String(NETATMO_CLIENT_ID);
    body += "&client_secret=" + String(NETATMO_CLIENT_SECRET);

    JsonDocument doc(&allocator);
    int status = sendRequest("POST", "https://api.netatmo.com/oauth2/token", body, doc, false);
    return handleTokenResponse(status, doc);
}

bool NetatmoClient::handleTokenResponse(int status, const JsonDocument &doc) {
    if (status == 200) {
        _token.fromJson(doc);
        saveToken();
        return true;
    }
    return false;
}

int NetatmoClient::getMeasure(const MeasureParams &params, NetatmoMeasureResponse &response) {
    if (!isAuthenticated()) return -1;

    String url = "https://api.netatmo.com/api/getmeasure?";
    url += "device_id=" + (params.deviceId.length() > 0 ? params.deviceId : String(NETATMO_MAC));
    if (params.moduleId.length() > 0) url += "&module_id=" + params.moduleId;
    url += "&scale=" + params.scale;
    if (params.dateEnd > 0) url += "&date_end=" + String(params.dateEnd);
    if (params.dateBegin > 0) url += "&date_begin=" + String(params.dateBegin);
    url += "&optimize=true" ;
    url += "&real_time=" + String(params.realTime ? "true" : "false");
    url += "&type=" + params.type;

    JsonDocument doc(&allocator);
    const int status = sendRequest("GET", url, "", doc, true);
    if (status == 200) {
        response = NetatmoMeasureResponse::fromJson(doc, params.moduleId.length() > 0);
    }
    return status;
}

int NetatmoClient::sendRequest(const String &method, const String &url, const String &body, JsonDocument &responseDoc,
                               const bool authenticated) {
    _http.begin(_secureClient, url);

    if (authenticated) {
        _http.addHeader("Authorization", "Bearer " + _token.accessToken);
    }

    if (method == "POST") {
        _http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    }

    const int httpCode = _http.sendRequest(method.c_str(), body);
    if (httpCode > 0) {
        String responseBody = _http.getString();
        const DeserializationError error = deserializeJson(responseDoc, responseBody);
        if (error != DeserializationError::Ok) {
            LogEvent::post("Failed to deserialize JSON: %s\n", error.c_str());
        }
    }
    _http.end();
    return httpCode;
}

void NetatmoClient::loadToken() {
    if (xSemaphoreTake(fsMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        if (LittleFS.exists(NETATMO_TOKEN_FILE)) {
            File file = LittleFS.open(NETATMO_TOKEN_FILE, "r");
            if (file) {
                JsonDocument doc;
                deserializeJson(doc, file);
                _token.fromJson(doc);
                file.close();
            }
        }
        xSemaphoreGive(fsMutex);
    }
}

void NetatmoClient::saveToken() const {
    LogEvent::post("Saving token: %s", _token.accessToken.c_str());
    if (xSemaphoreTake(fsMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        File file = LittleFS.open(NETATMO_TOKEN_FILE, "w");
        if (file) {
            JsonDocument doc;
            _token.toJson(doc);
            serializeJson(doc, file);
            file.close();
        }
        xSemaphoreGive(fsMutex);
    }
}

// ---- Convenience helpers ----
NetatmoClient::MeasureParams NetatmoClient::makeLast24hParams(const String& type, const String& moduleId) {
    MeasureParams p;
    p.deviceId = String(NETATMO_MAC);
    p.moduleId = moduleId;
    p.scale = "30min";
    const unsigned long now = timeClient.getEpochTime();
    p.dateEnd = now;
    p.dateBegin = now > 86400UL ? static_cast<long>(now - 86400UL) : 0;
    p.optimize = true;   // compact body format and predictable ordering
    p.realTime = true;
    p.type = type;
    return p;
}

int NetatmoClient::getLast24hTemperature(const String& moduleId, NetatmoMeasureResponse& response) {
    const MeasureParams p = makeLast24hParams("temperature", moduleId);
    return getMeasure(p, response);
}

int NetatmoClient::getLast24hPressure(const String& moduleId, NetatmoMeasureResponse& response) {
    const MeasureParams p = makeLast24hParams("pressure", moduleId);
    return getMeasure(p, response);
}

int NetatmoClient::getLast24hHumidity(NetatmoMeasureResponse& response) {
    MeasureParams p;
    p.deviceId = String(NETATMO_MAC);
    p.scale = "30min";      // request the most recent sample
    const unsigned long now = timeClient.getEpochTime();
    p.dateEnd = now;
    p.dateBegin = now > 21600UL ? static_cast<long>(now - 86400UL) : 0; // last 1h window
    p.optimize = true;
    p.realTime = true;
    p.type = "humidity";

    const int status = getMeasure(p, response);
    response.isModule = false;
    // Keep only the latest value if multiple were returned
    if (status == 200 && response.size > 1) {
        const double last = response.values[response.size - 1];
        response.values[0] = last;
        response.size = 1;
    }
    return status;
}
