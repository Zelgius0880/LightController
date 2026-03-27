#ifndef FIREBASE_MANAGER_H
#define FIREBASE_MANAGER_H

#include <FirebaseClient.h>

#include <HueApiClient.h>

class AsyncClientClass;

namespace firebase_ns {
    class FirebaseApp;
}

struct LightDevice {
    String uid;
    String name;
    String state;
    String id;
};

class FirebaseManager {
public:
    FirebaseManager(firebase_ns::FirebaseApp &app, AsyncClientClass &aClient, HueApiClient &hueApi);

    void begin();

    void loop() const;

    // Firestore logic
    void querySwitch(const String &switchUid);

    void handleSwitchResult(AsyncResult &aResult);


    void handleListResult(AsyncResult &aResult) const;

    bool ready() const;

private:
    firebase_ns::FirebaseApp &_app;
    AsyncClientClass &_aClient;
    HueApiClient &_hueApi;
    Firestore::Documents _fdo;

    String _lastRelativePath;

    static ListDocumentsOptions getBaseListOptions();
};

#endif
