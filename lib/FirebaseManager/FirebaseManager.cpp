#include "FirebaseManager.h"
#include <logger/task_logger.h>
#include <FirebaseClient.h>
#include "configuration.h"
#include <ArduinoJson.h>

#include "allocator/psram_allocator.h"
#include "webserver/task_webserver.h"

void processSwitch(AsyncResult &aResult);
void processList(AsyncResult &aResult);

FirebaseManager::FirebaseManager(firebase_ns::FirebaseApp &app, AsyncClientClass &aClient,
                                 HueApiClient &hueApi) : _app(app), _aClient(aClient), _hueApi(hueApi) {
}

void FirebaseManager::begin() {
    //_app.getApp<Storage>(_storage);
    _app.getApp<Firestore::Documents>(_fdo);
    _aClient.setSyncReadTimeout(15); // Wait max 15s for data to arrive
    _aClient.setSyncSendTimeout(10); // Wait max 10s to send the query
    _aClient.setSessionTimeout(30000); // Max total session life (30s)
}

void FirebaseManager::loop() const {
    _app.loop(); // Essential for token refresh and async tasks
}

bool FirebaseManager::ready() const {
    return _app.ready();
}

void FirebaseManager::querySwitch(const String &switchUid) {
    FieldFilter fieldFilter;
    Values::StringValue stringValue(switchUid);
    fieldFilter.field(FieldReference("uid"))
            .op(FieldFilterOperator::EQUAL)
            .value(Values::Value(stringValue));

    Filter filter(fieldFilter);
    QueryOptions _queryOptions;
    StructuredQuery query;
    CollectionSelector selector = CollectionSelector("items", true);
    query.where(filter);
    query.from(selector);
    query.limit(30);

    _queryOptions.structuredQuery(query);

    _fdo.runQuery(_aClient, Firestore::Parent(FIREBASE_PROJECT_ID), "", _queryOptions, processSwitch, "runQueryTask Switch");

    LogEvent::post("Fetching switch data from Firestore..\n");
    query.clear();
}

void FirebaseManager::handleSwitchResult(AsyncResult &aResult) {
    PsramAllocator allocator;

    JsonDocument resultDoc(&allocator);

    const DeserializationError err = deserializeJson(resultDoc, aResult.c_str());

    if (err || resultDoc.isNull() || resultDoc.size() == 0) {
        LogEvent::post("Failed to deserialize JSON: %s\n", err.c_str());
        aResult.clear();
        return;
    }

    // 3. Avoid temporary Strings by using const char* or string_view
    const char *fullPath = resultDoc[0]["document"]["name"];
    if (!fullPath) {
        LogEvent::post("No document found\n");
        aResult.clear();
        return;
    }

    // Logic to find the relative path without creating 3 intermediate Strings
    std::string pathStr(fullPath);
    const size_t lastSlash = pathStr.find_last_of('/');
    const size_t docIdx = pathStr.find("/documents/");

    if (docIdx == std::string::npos) {
        LogEvent::post("Invalid path: %s\n", fullPath);
        aResult.clear();
        return;
    }
    const String relativePath = pathStr.substr(docIdx + 11, lastSlash - (docIdx + 11)).c_str();

   _fdo.list(_aClient, Firestore::Parent(FIREBASE_PROJECT_ID), relativePath.c_str(),
                                 ListDocumentsOptions(),  processList, "runQueryTask List");

    aResult.clear();
}



void FirebaseManager::handleListResult(AsyncResult &aResult) const {
    PsramAllocator allocator;

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
            LogEvent::post("Hue ID: %s, State: %s\n", hueId, stateStr);

            if (strlen(hueId) == 0) continue;

            bool targetOn = false;
            if (strcmp(stateStr, "TOGGLE") == 0) {
                if (firstToggle) {
                    auto res = _hueApi.getLight(hueId);
                    if (res.data.size() > 0) {
                        toggleTargetOn = !res.data[0].on.on;
                        firstToggle = false;
                    } else {
                        LogEvent::post("Failed to get light status for %s\n", hueId);
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
