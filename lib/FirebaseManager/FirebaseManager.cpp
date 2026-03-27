#include "FirebaseManager.h"
#include <FirebaseClient.h>
#include "configuration.h"
#include <ArduinoJson.h>

#include "allocator/psram_allocator.h"
#include "buzzer/task_buzzer.h"
#include "../HueApiClient/HueApiClient.h"

void processSwitch(AsyncResult &aResult);
void processList(AsyncResult &aResult);

FirebaseManager::FirebaseManager(firebase_ns::FirebaseApp &app, AsyncClientClass &aClient,
                                 HueApiClient &hueApi) : _app(app), _aClient(aClient), _hueApi(hueApi) {
}

void FirebaseManager::begin() {
    //_app.getApp<Storage>(_storage);
    _app.getApp<Firestore::Documents>(_fdo);
    _aClient.setSyncReadTimeout(5);
    _aClient.setSyncSendTimeout(5);
    _aClient.setSessionTimeout(5);
}

void FirebaseManager::loop() const {
    _app.loop(); // Essential for token refresh and async tasks
}

bool FirebaseManager::ready() const {
    return _app.ready();
}

void FirebaseManager::querySwitch(const String &switchUid) {
    FieldFilter fieldFilter;
    const Values::StringValue stringValue(switchUid);
    fieldFilter.field(FieldReference("uid"))
            .op(FieldFilterOperator::EQUAL)
            .value(Values::Value(stringValue));

    const Filter filter(fieldFilter);
    QueryOptions queryOptions;
    StructuredQuery query;
    const auto selector = CollectionSelector("items", true);
    query.where(filter);
    query.from(selector);
    query.limit(1);

    queryOptions.structuredQuery(query);

    _fdo.runQuery(_aClient, Firestore::Parent(FIREBASE_PROJECT_ID), "", queryOptions, processSwitch, "runQueryTask Switch");

    HUE_LOG("Fetching switch data from Firestore..\n");
    query.clear();
}

void FirebaseManager::handleSwitchResult(AsyncResult &aResult) {
    JsonDocument resultDoc(&allocator);

    const DeserializationError err = deserializeJson(resultDoc, aResult.c_str());

    if (err || resultDoc.isNull() || resultDoc.size() == 0) {
        HUE_LOG("Failed to deserialize JSON: %s\n", err.c_str());
        aResult.clear();
        return;
    }

    // 3. Avoid temporary Strings by using const char* or string_view
    const char *fullPath = resultDoc[0]["document"]["name"];
    if (!fullPath) {
        HUE_LOG("No document found\n");
        aResult.clear();
        return;
    }

    // Logic to find the relative path without creating 3 intermediate Strings
    std::string pathStr(fullPath);
    const size_t lastSlash = pathStr.find_last_of('/');
    const size_t docIdx = pathStr.find("/documents/");

    if (docIdx == std::string::npos) {
        HUE_LOG("Invalid path: %s\n", fullPath);
        aResult.clear();
        return;
    }
    _lastRelativePath = String(pathStr.substr(docIdx + 11, lastSlash - (docIdx + 11)).c_str());

    _fdo.list(_aClient, Firestore::Parent(FIREBASE_PROJECT_ID), _lastRelativePath,
              getBaseListOptions(), processList, "runQueryTask List");

    BuzzerEvent::bip2();
    //aResult.clear();
}

ListDocumentsOptions FirebaseManager::getBaseListOptions() {
    ListDocumentsOptions options;
    options.orderBy("state desc");
    return options;
}

void FirebaseManager::handleListResult(AsyncResult &aResult) const {
    JsonDocument itemsDoc(&allocator); // Use PSRAM here too!
    deserializeJson(itemsDoc, aResult.c_str());

    // 5. Use the pipe operator | for safe defaults (prevents crashes on missing fields)

    bool firstToggle = true;
    bool toggleTargetOn = false;


    for (JsonObject item: itemsDoc["documents"].as<JsonArray>()) {
        JsonObject f = item["fields"];

        const char *type = f["itemType"]["stringValue"] | "";

        if (strcmp(type, "LIGHT") == 0) {
            const char *hueId = f["uid"]["stringValue"] | "";
            const char *stateStr = f["state"]["stringValue"] | "";
            HUE_LOG("Hue ID: %s, State: %s\n", hueId, stateStr);

            if (strlen(hueId) == 0) continue;

            bool targetOn = false;
            if (strcmp(stateStr, "TOGGLE") == 0) {
                if (firstToggle) {
                    auto res = _hueApi.getLight(hueId);
                    if (!res.data.empty()) {
                        toggleTargetOn = !res.data[0].on.on;
                        firstToggle = false;
                    } else {
                        HUE_LOG("Failed to get light status for %s\n", hueId);
                        continue;
                    }
                }
                targetOn = toggleTargetOn;
            } else {
                targetOn = (strcmp(stateStr, "ON") == 0);
            }

            JsonDocument updateDoc(&allocator);
            JsonObject on = updateDoc["on"].to<JsonObject>();
            on["on"] = targetOn;

            if (targetOn) {
                if (f["brightness"]) {
                    updateDoc["dimming"]["brightness"] = f["brightness"]["integerValue"].as<float>();
                }
                if (f["x"] && f["y"]) {
                    JsonObject color = updateDoc["color"].to<JsonObject>();
                    JsonObject xy = color["xy"].to<JsonObject>();
                    xy["x"] = f["x"]["doubleValue"].as<float>();
                    xy["y"] = f["y"]["doubleValue"].as<float>();
                }
            }

            _hueApi.updateLight(hueId, updateDoc.as<JsonVariantConst>());
        }
    }

    aResult.clear();
}
