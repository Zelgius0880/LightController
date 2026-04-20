#ifndef NETATMOMODELS_H
#define NETATMOMODELS_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <array>
#include <ctime>

#include "time/time_utils.h"

struct NetatmoToken {
    String accessToken;
    String refreshToken;
    long expiresIn;
    unsigned long creationTimestamp; // in seconds

    NetatmoToken() : expiresIn(0), creationTimestamp(0) {}

    bool isValid() const {
        if (accessToken.length() == 0) return false;
        // Current time in seconds. 
        const auto now = epoch_time();
        if (now < 1000000000UL) return true; // NTP not yet synchronized, assume valid for now
        return now < (creationTimestamp + expiresIn - 5*60); // 5 min buffer
    }

    void fromJson(const JsonVariantConst& json) {
        accessToken = json["access_token"].as<String>();
        refreshToken = json["refresh_token"].as<String>();
        expiresIn = json["expires_in"].as<long>();
        if (json["creation_timestamp"].is<unsigned long>()) {
            creationTimestamp = json["creation_timestamp"].as<unsigned long>();
        } else {
            creationTimestamp = epoch_time();
        }
    }

    void toJson(JsonVariant json) const {
        json["access_token"] = accessToken;
        json["refresh_token"] = refreshToken;
        json["expires_in"] = expiresIn;
        json["creation_timestamp"] = creationTimestamp;
    }
};

struct NetatmoMeasureResponse {
    static constexpr size_t kMax = 48; // 24h @ 30min
    bool isModule = false;
    std::array<double, kMax> values{};
    uint8_t size = 0; // actual filled length

    static NetatmoMeasureResponse fromJson(const JsonVariantConst& json, bool module) {
        NetatmoMeasureResponse res;
        res.isModule = module;
        JsonVariantConst body = json["body"];
        
        auto pushVal = [&](double v){
            if (res.size < kMax) {
                res.values[res.size++] = v;
            }
        };

        if (body.is<JsonArrayConst>()) {
            for (JsonVariantConst item : body.as<JsonArrayConst>()) {
                JsonArrayConst valMatrix = item["value"].as<JsonArrayConst>();
                for (JsonArrayConst valArr : valMatrix) {
                    for (JsonVariantConst v : valArr) {
                        pushVal(v.as<double>());
                        if (res.size >= kMax) break;
                    }
                    if (res.size >= kMax) break;
                }
                if (res.size >= kMax) break;
            }
        }
        return res;
    }
};

#endif // NETATMOMODELS_H
