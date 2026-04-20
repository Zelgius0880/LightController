package com.zelgius.lightcontroller

import androidx.datastore.core.DataStore
import androidx.datastore.preferences.core.PreferenceDataStoreFactory
import androidx.datastore.preferences.core.Preferences

class JsPlatform: Platform {
    override val name: String = "Web with Kotlin/JS"
}

actual fun getPlatform(): Platform = JsPlatform()

actual fun createDataStore(context: Any?): DataStore<Preferences> =
    PreferenceDataStoreFactory.createWithPath(
        produceFile = { SETTINGS_DATASTORE_FILE_NAME.toPath() }
    )