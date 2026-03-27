#include <LightsDatabaseManager.h>
#include <LittleFS.h>
#define DB_PATH "/lights.db"

LightsDatabaseManager::LightsDatabaseManager(const char *dbPath) : _dbPath(dbPath), _db(nullptr) {
}

LightsDatabaseManager::~LightsDatabaseManager() {
    close();
}

bool LightsDatabaseManager::begin() {
    const auto path = String(_dbPath);
    if (!LittleFS.exists(path)) {
        File file = LittleFS.open(path, FILE_WRITE);
        file.close();
    }

    /*if (!LittleFS.exists(String(path)+"-journal")){
        File file = LittleFS.open(String(path)+"-journal", FILE_WRITE);
        file.close();
    }*/

    sqlite3_initialize();

    if (sqlite3_open(("/littlefs/" + path).c_str(), &_db) != SQLITE_OK) {
        Serial.printf("Can't open database: %s\n", sqlite3_errmsg(_db));
        return false;
    }

    // Enable foreign keys
    executeQuery("PRAGMA foreign_keys = ON;");

    return createTables();
}

void LightsDatabaseManager::close() {
    if (_db) {
        sqlite3_close(_db);
        _db = nullptr;
    }
}

bool LightsDatabaseManager::createTables() {
    const auto sqlGroups = "CREATE TABLE IF NOT EXISTS groups ("
            "id INTEGER PRIMARY KEY, "
            "name TEXT, "
            "brightness REAL, "
            "x REAL, "
            "y REAL);";

    const auto sqlLights = "CREATE TABLE IF NOT EXISTS lights ("
            "uid TEXT PRIMARY KEY, "
            "name TEXT, "
            "type TEXT);";

    const auto sqlGroupLights = "CREATE TABLE IF NOT EXISTS group_lights ("
            "groupId INTEGER, "
            "lightUid TEXT, "
            "brightness REAL, "
            "x REAL, "
            "y REAL, "
            "state TEXT, "
            "PRIMARY KEY(groupId, lightUid), "
            "FOREIGN KEY(groupId) REFERENCES groups(id), "
            "FOREIGN KEY(lightUid) REFERENCES lights(uid));";

    const auto sqlSwitches = "CREATE TABLE IF NOT EXISTS switches ("
            "uid TEXT PRIMARY KEY, "
            "name TEXT, "
            "groupId INTEGER, "
            "FOREIGN KEY(groupId) REFERENCES groups(id));";

    return executeQuery(sqlGroups) && executeQuery(sqlLights) && executeQuery(sqlGroupLights) &&
           executeQuery(sqlSwitches);
}

bool LightsDatabaseManager::executeQuery(const char *sql) const {
    char *zErrMsg = nullptr;
    const int rc = sqlite3_exec(_db, sql, nullptr, nullptr, &zErrMsg);
    if (rc != SQLITE_OK) {
        Serial.printf("SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        return false;
    }
    return true;
}

bool LightsDatabaseManager::upsertGroup(const Group &group) {
    sqlite3_stmt *res;
    const char *sql = "INSERT INTO groups (id, name, brightness, x, y) VALUES (?, ?, ?, ?, ?) "
            "ON CONFLICT(id) DO UPDATE SET name=excluded.name, brightness=excluded.brightness, x=excluded.x, y=excluded.y;";

    if (sqlite3_prepare_v2(_db, sql, -1, &res, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_int64(res, 1, group.id);
    sqlite3_bind_text(res, 2, group.name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(res, 3, group.brightness);
    sqlite3_bind_double(res, 4, group.x);
    sqlite3_bind_double(res, 5, group.y);

    int rc = sqlite3_step(res);
    sqlite3_finalize(res);
    return rc == SQLITE_DONE;
}

bool LightsDatabaseManager::upsertLight(const Light &light) const {
    sqlite3_stmt *res;
    // 1. Upsert into lights base table
    const char *sql1 = "INSERT INTO lights (uid, name, type) VALUES (?, ?, ?) "
            "ON CONFLICT(uid) DO UPDATE SET name=excluded.name, type=excluded.type;";

    if (sqlite3_prepare_v2(_db, sql1, -1, &res, 0) != SQLITE_OK) return false;

    sqlite3_bind_text(res, 1, light.uid.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(res, 2, light.name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(res, 4, light.type.c_str(), -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(res);
    sqlite3_finalize(res);
    if (rc != SQLITE_DONE) return false;

    // 2. Upsert into group_lights join table
    const char *sql2 = "INSERT INTO group_lights (groupId, lightUid, brightness, x, y, state) VALUES (?, ?, ?, ?, ?, ?) "
            "ON CONFLICT(groupId, lightUid) DO UPDATE SET brightness=excluded.brightness, x=excluded.x, y=excluded.y, state=excluded.state;";

    if (sqlite3_prepare_v2(_db, sql2, -1, &res, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_int64(res, 1, light.groupId);
    sqlite3_bind_text(res, 2, light.uid.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(res, 3, light.brightness);
    sqlite3_bind_double(res, 4, light.x);
    sqlite3_bind_double(res, 5, light.y);
    sqlite3_bind_text(res, 6, light.state.c_str(), -1, SQLITE_TRANSIENT);

    rc = sqlite3_step(res);
    sqlite3_finalize(res);
    return rc == SQLITE_DONE;
}

bool LightsDatabaseManager::upsertSwitch(const Switch &sw) {
    sqlite3_stmt *res;
    const char *sql = "INSERT INTO switches (uid, name, groupId) VALUES (?, ?, ?) "
            "ON CONFLICT(uid) DO UPDATE SET name=excluded.name, groupId=excluded.groupId;";

    if (sqlite3_prepare_v2(_db, sql, -1, &res, 0) != SQLITE_OK) return false;

    sqlite3_bind_text(res, 1, sw.uid.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(res, 2, sw.name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(res, 3, sw.groupId);

    int rc = sqlite3_step(res);
    sqlite3_finalize(res);
    return rc == SQLITE_DONE;
}

Group LightsDatabaseManager::getGroupBySwitchUid(const String &switchUid) {
    Group group = {0, "", 0.0f, 0.0f, 0.0f};
    sqlite3_stmt *res;
    const char *sql = "SELECT g.id, g.name, g.brightness, g.x, g.y FROM groups g "
            "JOIN switches s ON g.id = s.groupId WHERE s.uid = ?;";

    if (sqlite3_prepare_v2(_db, sql, -1, &res, 0) == SQLITE_OK) {
        sqlite3_bind_text(res, 1, switchUid.c_str(), -1, SQLITE_TRANSIENT);
        if (sqlite3_step(res) == SQLITE_ROW) {
            group.id = sqlite3_column_int64(res, 0);
            group.name = (const char *) sqlite3_column_text(res, 1);
            group.brightness = (float) sqlite3_column_double(res, 2);
            group.x = (float) sqlite3_column_double(res, 3);
            group.y = (float) sqlite3_column_double(res, 4);
        }
        sqlite3_finalize(res);
    }
    return group;
}

std::vector<Light> LightsDatabaseManager::getLightsByGroupId(uint64_t groupId) {
    std::vector<Light> lights;
    sqlite3_stmt *res;
    const char *sql = "SELECT l.uid, l.name, gl.state, l.type, gl.brightness, gl.x, gl.y, gl.groupId "
            "FROM lights l "
            "JOIN group_lights gl ON l.uid = gl.lightUid "
            "WHERE gl.groupId = ?;";

    if (sqlite3_prepare_v2(_db, sql, -1, &res, 0) == SQLITE_OK) {
        sqlite3_bind_int64(res, 1, groupId);
        while (sqlite3_step(res) == SQLITE_ROW) {
            Light light;
            light.uid = (const char *) sqlite3_column_text(res, 0);
            light.name = (const char *) sqlite3_column_text(res, 1);
            light.state = (const char *) sqlite3_column_text(res, 2);
            light.type = (const char *) sqlite3_column_text(res, 3);
            light.brightness = (float) sqlite3_column_double(res, 4);
            light.x = (float) sqlite3_column_double(res, 5);
            light.y = (float) sqlite3_column_double(res, 6);
            light.groupId = sqlite3_column_int64(res, 7);
            lights.push_back(light);
        }
        sqlite3_finalize(res);
    }
    return lights;
}

bool LightsDatabaseManager::importDatabase(const uint8_t *data, const size_t len) {
    close(); // Close existing SQLite handle

    // 1. Clean up old files to prevent rollback/conflicts
    LittleFS.remove(_dbPath);
    LittleFS.remove(String(_dbPath) + "-journal");
    LittleFS.remove(String(_dbPath) + "-wal");

    // 2. Write the buffer directly to the new database file
    File dbFile = LittleFS.open(_dbPath, "w");
    if (dbFile) {
        size_t written = dbFile.write(data, len);
        dbFile.flush();
        dbFile.close();

        if (written != len) {
            Serial.println("Failed to write full buffer to FS");
            return false;
        }

        // 3. Re-initialize the database engine
        return begin();
    }

    return false;
}
