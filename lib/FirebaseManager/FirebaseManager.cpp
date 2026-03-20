#include "FirebaseManager.h"
#include <logger/task_logger.h>
#include <FirebaseClient.h>
#include "configuration.h"
#include <ArduinoJson.h>

struct PsramAllocator : Allocator {
    virtual ~PsramAllocator() = default;

    void *allocate(size_t size) override { return ps_malloc(size); }
    void deallocate(void *ptr) override { free(ptr); }
    void *reallocate(void *ptr, size_t new_size) override { return ps_realloc(ptr, new_size); }
};

FirebaseManager::FirebaseManager(firebase_ns::FirebaseApp &app, AsyncClientClass &aClient,
                                 HueApiClient &hueApi) : _app(app), _aClient(aClient), _hueApi(hueApi) {
}

void FirebaseManager::begin() {
    //_app.getApp<Storage>(_storage);
    _app.getApp<Firestore::Documents>(_fdo);
}

void FirebaseManager::loop() const {
    _app.loop(); // Essential for token refresh and async tasks
}

bool FirebaseManager::ready() const {
    return _app.ready();
}

void FirebaseManager::handleSwitch(const String &switchUid) {
    PsramAllocator allocator;

    FieldFilter fieldFilter;
    Values::StringValue stringValue(switchUid);
    fieldFilter.field(FieldReference("uid"))
            .op(FieldFilterOperator::EQUAL)
            .value(Values::Value(stringValue));

    Filter filter(fieldFilter);

    _query.where(filter);
    _query.from(_selector);
    _query.limit(30);

    _defaultQueryOptions.structuredQuery(_query);
    _query.clear();

    String response = _fdo.runQuery(_aClient, Firestore::Parent(FIREBASE_PROJECT_ID), "", _defaultQueryOptions);

    JsonDocument resultDoc(&allocator);
    DeserializationError err = deserializeJson(resultDoc, response);
    response = ""; // Manually free the large string memory immediately!

    if (err || resultDoc.isNull() || resultDoc.size() == 0) return;

    // 3. Avoid temporary Strings by using const char* or string_view
    const char *fullPath = resultDoc[0]["document"]["name"];
    if (!fullPath) return;

    // Logic to find the relative path without creating 3 intermediate Strings
    std::string pathStr(fullPath);
    size_t lastSlash = pathStr.find_last_of('/');
    size_t docIdx = pathStr.find("/documents/");

    if (docIdx == std::string::npos) return;
    std::string relativePath = pathStr.substr(docIdx + 11, lastSlash - (docIdx + 11));

    // 4. Second fetch
    String itemsList = _fdo.list(_aClient, Firestore::Parent(FIREBASE_PROJECT_ID), relativePath.c_str(),
                                 ListDocumentsOptions());

    JsonDocument itemsDoc(&allocator); // Use PSRAM here too!
    deserializeJson(itemsDoc, itemsList);
    itemsList = ""; // Free the string memory

    // 5. Use the pipe operator | for safe defaults (prevents crashes on missing fields)

    for (JsonObject item: itemsDoc["documents"].as<JsonArray>()) {
        JsonObject f = item["fields"];

        const char *type = f["itemType"]["stringValue"] | "";

        if (strcmp(type, "LIGHT") == 0) {
            LogEvent::post("Found light: %s (%s)\n",
                           f["name"]["stringValue"] | "Unknown",
                           f["uid"]["stringValue"] | "");

            const char* hueId = f["uid"]["stringValue"] | "";
            const char* stateStr = f["state"]["stringValue"] | "";

            if (strlen(hueId) == 0) continue;

            LogEvent::post("Processing light: %s (Hue ID: %s, State: %s)\n",
                f["name"]["stringValue"] | "Unknown", hueId, stateStr);

            bool targetOn = false;
            bool doToggle = (strcmp(stateStr, "TOGGLE") == 0);

            if (doToggle) {
                auto res = _hueApi.getLight(hueId);
                if (res.data.size() > 0) {
                    targetOn = !res.data[0].on.on;
                } else {
                    LogEvent::post("Failed to get light status for %s\n", hueId);
                    continue;
                }
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
}
