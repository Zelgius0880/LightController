package com.zelgius.lightcontroller

import androidx.compose.runtime.Composable
import androidx.compose.ui.graphics.ImageBitmap
import androidx.datastore.core.DataStore
import androidx.datastore.preferences.core.Preferences
import androidx.navigation3.runtime.NavKey
import com.zelgius.lightcontroller.domain.repository.StoredImageRepository
import io.ktor.client.HttpClient
import io.ktor.client.HttpClientConfig
import org.koin.core.module.Module

const val SETTINGS_DATASTORE_FILE_NAME = "settings.preferences_pb"
const val HUE_BRIDGE_IP = "192.168.1.252"


enum class Platform {
    Android, Web
}
expect  val PLATFORM: Platform

@Suppress("EXPECT_ACTUAL_CLASSIFIERS_ARE_IN_BETA_WARNING")
expect class Factory {
    fun createDataStore(): DataStore<Preferences>
    suspend fun download(bytes: ByteArray, fileName: String,
                         mimeType: String
                         ): Boolean

    fun createStoredImageRepository(): StoredImageRepository
}

expect val platformModule: Module

@Composable
expect fun createBackStack(): MutableList<NavKey>

expect val selfHosted: Boolean

expect fun openNewTab(url: String)

expect fun buildNetatmoTokenUrl(): String

@Composable
expect fun isDarkMode(): Boolean

expect fun platformHttpClient(
    block: HttpClientConfig<*>.() -> Unit = {}
): HttpClient


internal expect fun ImageBitmap.readPixelsToByteArray(): ByteArray
internal expect suspend fun createImageBitmapFromBytes(bytes: ByteArray, width: Int, height: Int): ImageBitmap