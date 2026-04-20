package com.zelgius.lightcontroller.domain.settings

import androidx.datastore.core.DataStore
import androidx.datastore.preferences.core.Preferences
import androidx.datastore.preferences.core.edit
import androidx.datastore.preferences.core.intPreferencesKey
import androidx.datastore.preferences.core.stringPreferencesKey
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.map
import org.koin.core.annotation.Singleton

@Singleton
class SettingsRepository(
    private val dataStore: DataStore<Preferences>
) {
    val ipFlow: Flow<String?> = dataStore.data.map { preferences ->
        preferences[KEY_IP]
    }

    val portFlow: Flow<Int?> = dataStore.data.map { preferences ->
        preferences[KEY_PORT]
    }

    suspend fun saveIp(ip: String?) {
        dataStore.edit { preferences ->
            if (ip != null) preferences[KEY_IP] = ip
            else preferences.remove(KEY_IP)
        }
    }

    suspend fun savePort(port: Int?) {
        dataStore.edit { preferences ->
            if (port != null) preferences[KEY_PORT] = port
            else preferences.remove(KEY_PORT)
        }
    }

    companion object {
        private val KEY_IP = stringPreferencesKey("server_ip")
        private val KEY_PORT = intPreferencesKey("server_port")
    }
}
