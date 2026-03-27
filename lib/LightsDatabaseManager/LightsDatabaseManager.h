#ifndef LIGHTS_DATABASE_MANAGER_H
#define LIGHTS_DATABASE_MANAGER_H

#include <Arduino.h>
#include <sqlite3.h>
#include <vector>

struct Group {
    uint64_t id;
    String name;
    float brightness;
    float x;
    float y;
};

struct Light {
    String uid;
    String name;
    String state;
    String type;
    float brightness;
    float x;
    float y;
    uint64_t groupId;
};

struct Switch {
    String uid;
    String name;
    uint64_t groupId;
};

class LightsDatabaseManager {
public:
    LightsDatabaseManager(const char* dbPath);
    ~LightsDatabaseManager();

    bool begin();
    void close();

    // Upsert methods
    bool upsertGroup(const Group& group);
    bool upsertLight(const Light& light) const;
    bool upsertSwitch(const Switch& sw);

    // Retrieval
    Group getGroupBySwitchUid(const String& switchUid);
    std::vector<Light> getLightsByGroupId(uint64_t groupId);

    // Import (Backup management)
    bool importDatabase(const uint8_t *data, size_t len);

    static int db_open(const char* filename, sqlite3** db);

private:
    const char* _dbPath;
    sqlite3* _db;

    bool executeQuery(const char* sql) const;
    bool createTables();
};

#endif // LIGHTS_DATABASE_MANAGER_H
