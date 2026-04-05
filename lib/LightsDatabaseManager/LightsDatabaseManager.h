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
    bool upsertGroup(const Group& group) const;
    bool upsertLight(const Light& light) const;
    bool upsertSwitch(const Switch& sw) const;

    // Retrieval
    std::vector<Group> getAllGroups() const; // Keep this as it's a top-level resource
    Group getGroupBySwitchUid(const String& switchUid) const;
    std::vector<Light> getLightsByGroupId(uint64_t groupId) const; // Already exists
    std::vector<Switch> getSwitchesByGroupId(uint64_t groupId) const; // New method
    Switch getSwitchByUid(const String& switchUid) const; // New method

    // Import (Backup management)
    bool importDatabase(const uint8_t *data, size_t len);

private:
    const char* _dbPath;
    sqlite3* _db;

    bool executeQuery(const char* sql) const;
    bool createTables() const;
};

#endif // LIGHTS_DATABASE_MANAGER_H
