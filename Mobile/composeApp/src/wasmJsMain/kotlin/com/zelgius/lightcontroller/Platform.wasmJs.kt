package com.zelgius.lightcontroller

import androidx.compose.runtime.Composable
import androidx.compose.runtime.mutableStateListOf
import androidx.compose.runtime.remember
import androidx.datastore.core.DataStore
import androidx.datastore.preferences.core.PreferenceDataStoreFactory
import androidx.datastore.preferences.core.Preferences
import androidx.navigation3.runtime.NavKey
import com.github.terrakok.navigation3.browser.ChronologicalBrowserNavigation
import com.github.terrakok.navigation3.browser.buildBrowserHistoryFragment
import com.github.terrakok.navigation3.browser.getBrowserHistoryFragmentName
import com.zelgius.lightcontroller.navigation.Home
import com.zelgius.lightcontroller.navigation.Route
import io.github.vinceglb.filekit.FileKit
import io.github.vinceglb.filekit.download
import io.ktor.client.HttpClient
import io.ktor.client.HttpClientConfig
import io.ktor.client.plugins.contentnegotiation.ContentNegotiation
import io.ktor.client.request.HttpRequestPipeline
import io.ktor.http.URLProtocol
import io.ktor.http.encodedPath
import io.ktor.serialization.kotlinx.json.json
import kotlinx.browser.window
import kotlinx.coroutines.flow.filterNotNull
import kotlinx.coroutines.flow.first
import kotlinx.serialization.json.Json
import okio.Path.Companion.toPath
import org.koin.dsl.module
import kotlin.random.Random

actual val platformModule = module {
    single { Factory() }
}

@Suppress("EXPECT_ACTUAL_CLASSIFIERS_ARE_IN_BETA_WARNING")
actual class Factory {
    actual fun createDataStore(): DataStore<Preferences> =
        PreferenceDataStoreFactory.createWithPath { SETTINGS_DATASTORE_FILE_NAME.toPath() }

    actual suspend fun download(
        bytes: ByteArray, fileName: String,
        mimeType: String
    ): Boolean {
        FileKit.download(bytes, fileName)
        return true
    }

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

actual val selfHosted: Boolean
    get() = false

actual fun openNewTab(url: String) {
    window.open(url = url, target = "_blank")
}

@OptIn(ExperimentalWasmJsInterop::class)
private fun redirectUrl(): String =
    js("encodeURIComponent(\"http://\"+window.LOCAL_IP)+\"/token_result\"")

@OptIn(ExperimentalWasmJsInterop::class)
private fun netatmoClientId(): String = js("window.NETATMO_CLIENT_ID")

@OptIn(ExperimentalWasmJsInterop::class)
actual fun buildNetatmoTokenUrl(): String {
    val clientId = netatmoClientId()
    val scope = "read_station"
    val state = Random.nextLong().toString(36).substring(7)

    return "https://api.netatmo.com/oauth2/authorize?client_id=${clientId}&redirect_uri=${redirectUrl()}&scope=${scope}&state=${state}"
}

@Composable
actual fun isDarkMode(): Boolean = true

actual val PLATFORM: Platform = Platform.Web

actual fun platformHttpClient(
    block: HttpClientConfig<*>.() -> Unit
): HttpClient = HttpClient {
    block()
}