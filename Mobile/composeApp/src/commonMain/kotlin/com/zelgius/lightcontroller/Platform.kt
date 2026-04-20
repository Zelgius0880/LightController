package com.zelgius.lightcontroller

import androidx.compose.runtime.Composable
import androidx.datastore.core.DataStore
import androidx.datastore.preferences.core.Preferences
import androidx.navigation3.runtime.NavKey
import org.koin.core.module.Module

const val SETTINGS_DATASTORE_FILE_NAME = "settings.preferences_pb"

@Suppress("EXPECT_ACTUAL_CLASSIFIERS_ARE_IN_BETA_WARNING")
expect class Factory {
    fun createDataStore(): DataStore<Preferences>
}

expect val platformModule: Module

@Composable
expect fun createBackStack(): MutableList<NavKey>