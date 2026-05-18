package com.zelgius.lightcontroller.domain.repository.settings

import androidx.datastore.core.DataStore
import androidx.datastore.preferences.core.Preferences
import androidx.datastore.preferences.core.edit
import androidx.datastore.preferences.core.intPreferencesKey
import androidx.datastore.preferences.core.stringPreferencesKey
import com.zelgius.lightcontroller.selfHostedIp
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.firstOrNull
import kotlinx.coroutines.flow.flowOf
import kotlinx.coroutines.flow.map
import org.koin.core.annotation.Singleton

interface SettingsRepository {
    val ipFlow: Flow<String?>
    val portFlow: Flow<Int?>
    val hueApiKey: Flow<String?>
    suspend fun saveIp(ip: String?)
    suspend fun savePort(port: Int?)
    suspend fun saveHueApiKey(hueApiKey: String?)
}

class SettingsRepositoryImpl(
    private val dataStore: DataStore<Preferences>
) : SettingsRepository{
    override val ipFlow: Flow<String?> = if(selfHostedIp() == null )dataStore.data.map { preferences ->
        preferences[KEY_IP]
    } else flowOf(selfHostedIp())

    override val portFlow: Flow<Int?> =  if(selfHostedIp() == null ) dataStore.data.map { preferences ->
        preferences[KEY_PORT]
    } else flowOf(81)

    override  val hueApiKey: Flow<String?> = dataStore.data.map { preferences ->
        preferences[KEY_HUE_API_KEY]
    }

    override suspend fun saveIp(ip: String?) {
        dataStore.edit { preferences ->
            if (ip != null) preferences[KEY_IP] = ip
            else preferences.remove(KEY_IP)
        }
    }

    override suspend fun savePort(port: Int?) {
        dataStore.edit { preferences ->
            if (port != null) preferences[KEY_PORT] = port
            else preferences.remove(KEY_PORT)
        }
    }

    override suspend fun saveHueApiKey(hueApiKey: String?) {
        val oldKey = this.hueApiKey.firstOrNull()

        if(oldKey != hueApiKey) {
            dataStore.edit { preferences ->
                if (hueApiKey != null) preferences[KEY_HUE_API_KEY] = hueApiKey
                else preferences.remove(KEY_HUE_API_KEY)
            }
        }
    }

    companion object {
        private val KEY_IP = stringPreferencesKey("server_ip")
        private val KEY_HUE_API_KEY = stringPreferencesKey("hue_api_key")
        private val KEY_PORT = intPreferencesKey("server_port")
    }
}
