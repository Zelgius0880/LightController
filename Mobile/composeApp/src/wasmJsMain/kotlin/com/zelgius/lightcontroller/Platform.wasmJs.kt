package com.zelgius.lightcontroller

import androidx.compose.runtime.Composable
import androidx.compose.runtime.derivedStateOf
import androidx.compose.runtime.mutableStateListOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.snapshots.SnapshotStateList
import androidx.datastore.core.DataStore
import androidx.datastore.preferences.core.PreferenceDataStoreFactory
import androidx.datastore.preferences.core.Preferences
import androidx.navigation3.runtime.NavBackStack
import androidx.navigation3.runtime.NavEntry
import androidx.navigation3.runtime.NavKey
import androidx.navigation3.ui.NavDisplay
import androidx.savedstate.serialization.SavedStateConfiguration
import com.github.terrakok.navigation3.browser.ChronologicalBrowserNavigation
import com.github.terrakok.navigation3.browser.HierarchicalBrowserNavigation
import com.github.terrakok.navigation3.browser.buildBrowserHistoryFragment
import com.github.terrakok.navigation3.browser.getBrowserHistoryFragmentName
import com.github.terrakok.navigation3.browser.getBrowserHistoryFragmentParameters
import com.zelgius.lightcontroller.navigation.Home
import com.zelgius.lightcontroller.navigation.Route
import okio.Path.Companion.toPath
import org.koin.dsl.module

actual val platformModule = module {
    single { Factory() }
}

@Suppress("EXPECT_ACTUAL_CLASSIFIERS_ARE_IN_BETA_WARNING")
actual class Factory {
    actual fun createDataStore(): DataStore<Preferences> =
        PreferenceDataStoreFactory.createWithPath { SETTINGS_DATASTORE_FILE_NAME.toPath() }
}

@Composable
actual fun createBackStack(): MutableList<NavKey> {
    val backStack = remember { mutableStateListOf<NavKey>(Home) }
    ChronologicalBrowserNavigation(
        backStack = backStack,
        saveKey = { key ->
            if (key is Route) buildBrowserHistoryFragment(key.name) else null
        },
        restoreKey = { fragment ->
            getBrowserHistoryFragmentName(fragment)?.let {
                Route.fromName(it)
            }
        }
    )
    return backStack
}