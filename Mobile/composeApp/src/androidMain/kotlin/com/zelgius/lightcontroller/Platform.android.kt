package com.zelgius.lightcontroller

import android.content.Context
import androidx.compose.runtime.Composable
import androidx.datastore.core.DataStore
import androidx.datastore.preferences.core.PreferenceDataStoreFactory
import androidx.datastore.preferences.core.Preferences
import androidx.navigation3.runtime.NavKey
import androidx.navigation3.runtime.rememberNavBackStack
import androidx.savedstate.serialization.SavedStateConfiguration
import com.zelgius.lightcontroller.navigation.Home
import com.zelgius.lightcontroller.navigation.Placeholder
import com.zelgius.lightcontroller.navigation.Route
import com.zelgius.lightcontroller.navigation.Settings
import kotlinx.serialization.modules.SerializersModule
import kotlinx.serialization.modules.polymorphic
import okio.Path.Companion.toPath
import org.koin.core.module.dsl.singleOf
import org.koin.dsl.module

fun getPreferencesDataStore(path: String) = PreferenceDataStoreFactory.createWithPath {
    path.toPath()
}

actual val platformModule = module {

    singleOf(::provideFactory)
}

fun provideFactory(context: Context) = Factory(context)

@Suppress("EXPECT_ACTUAL_CLASSIFIERS_ARE_IN_BETA_WARNING")
actual class Factory(
    private val context: Context,
) {
    actual fun createDataStore(): DataStore<Preferences> {
        val path = context.filesDir.resolve(SETTINGS_DATASTORE_FILE_NAME).absolutePath
        return getPreferencesDataStore(path)
    }
}


val config = SavedStateConfiguration {
    serializersModule = SerializersModule {
        polymorphic(NavKey::class) {
            subclass(Home::class, Home.serializer())
            subclass(Settings::class, Settings.serializer())
            subclass(Placeholder::class, Placeholder.serializer())

        }
    }
}
@Composable
actual fun createBackStack(): MutableList<NavKey> = rememberNavBackStack(config, Home)