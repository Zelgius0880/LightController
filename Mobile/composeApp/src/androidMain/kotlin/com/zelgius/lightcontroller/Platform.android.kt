package com.zelgius.lightcontroller

import android.content.ContentValues
import android.content.Context
import android.graphics.Bitmap
import android.net.Uri
import android.os.Build
import android.os.Environment
import android.provider.MediaStore
import androidx.compose.foundation.isSystemInDarkTheme
import androidx.compose.runtime.Composable
import androidx.compose.ui.graphics.ImageBitmap
import androidx.compose.ui.graphics.asAndroidBitmap
import androidx.compose.ui.graphics.asImageBitmap
import androidx.compose.ui.graphics.colorspace.ColorSpaces
import androidx.compose.ui.graphics.toPixelMap
import androidx.datastore.core.DataStore
import androidx.datastore.preferences.core.PreferenceDataStoreFactory
import androidx.datastore.preferences.core.Preferences
import androidx.navigation3.runtime.NavKey
import androidx.navigation3.runtime.rememberNavBackStack
import androidx.savedstate.serialization.SavedStateConfiguration
import com.zelgius.lightcontroller.domain.repository.StoredImageRepository
import com.zelgius.lightcontroller.navigation.Home
import com.zelgius.lightcontroller.navigation.Image
import com.zelgius.lightcontroller.navigation.Lights
import com.zelgius.lightcontroller.navigation.Placeholder
import com.zelgius.lightcontroller.navigation.Route
import com.zelgius.lightcontroller.navigation.Settings
import io.ktor.client.HttpClient
import io.ktor.client.HttpClientConfig
import io.ktor.client.engine.okhttp.OkHttp
import io.ktor.utils.io.ByteReadChannel
import kotlinx.serialization.modules.SerializersModule
import kotlinx.serialization.modules.polymorphic
import okhttp3.OkHttpClient
import okhttp3.logging.HttpLoggingInterceptor
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

    companion object {
        var dataStore: DataStore<Preferences>? = null
    }

    actual fun createDataStore(): DataStore<Preferences> {
        return dataStore ?: run {
            val path = context.filesDir.resolve(SETTINGS_DATASTORE_FILE_NAME).absolutePath
            getPreferencesDataStore(path).apply {
                dataStore = this
            }
        }
    }

    actual fun createStoredImageRepository(): StoredImageRepository = StoredImageRepository(context)


    actual suspend fun download(
        bytes: ByteArray, fileName: String,
        mimeType: String
    ): Boolean {
        val contentResolver = context.contentResolver

        // 1. Prepare file metadata
        val contentValues = ContentValues().apply {
            put(MediaStore.MediaColumns.DISPLAY_NAME, fileName)
            put(MediaStore.MediaColumns.MIME_TYPE, mimeType)
            put(MediaStore.MediaColumns.RELATIVE_PATH, Environment.DIRECTORY_DOWNLOADS)
            put(MediaStore.MediaColumns.IS_PENDING, 1)
        }

        val uri: Uri? = contentResolver.insert(
            MediaStore.Downloads.EXTERNAL_CONTENT_URI,
            contentValues
        )

        uri?.let { targetUri ->
            try {
                contentResolver.openOutputStream(targetUri)?.use { outputStream ->
                    outputStream.write(bytes)
                }

                contentValues.clear()
                contentValues.put(MediaStore.MediaColumns.IS_PENDING, 0)
                contentResolver.update(targetUri, contentValues, null, null)
                return true
            } catch (e: Exception) {
                e.printStackTrace()
                // Clean up the failed entry if necessary
                contentResolver.delete(targetUri, null, null)
                return false
            }
        }
        return false
    }
}

actual fun openNewTab(url: String) {
    // New tab is not supported
}


val config = SavedStateConfiguration {
    serializersModule = SerializersModule {
        polymorphic(NavKey::class) {
            subclass(Home::class, Home.serializer())
            subclass(Settings::class, Settings.serializer())
            subclass(Lights::class, Lights.serializer())
            subclass(Image::class, Image.serializer())
            subclass(Placeholder::class, Placeholder.serializer())

        }
    }
}

@Composable
actual fun createBackStack(): MutableList<NavKey> = rememberNavBackStack(config, Home)

actual fun selfHostedIp(): String? = null

actual fun buildNetatmoTokenUrl(): String {
    throw IllegalStateException("Netatmo token is not supported for Android")
}

@Composable
actual fun isDarkMode(): Boolean = isSystemInDarkTheme()


actual val PLATFORM: Platform = Platform.Android

actual fun platformHttpClient(
    block: HttpClientConfig<*>.() -> Unit
): HttpClient = HttpClient(OkHttp) {
    engine {
        config {
            addInterceptor(HttpLoggingInterceptor().apply {
                level = HttpLoggingInterceptor.Level.BODY
            })
            hostnameVerifier { hostname, _ ->
                hostname == HUE_BRIDGE_IP
            }
        }
    }
    block()
}

internal actual fun ImageBitmap.readPixelsToByteArray(): ByteArray {
    val bitmap = this.asAndroidBitmap()
    val pixels = IntArray(width * height)
    bitmap.getPixels(pixels, 0, width, 0, 0, width, height)

    val bytes = ByteArray(width * height * 4)
    for (i in pixels.indices) {
        val color = pixels[i]
        bytes[i * 4] = ((color shr 16) and 0xFF).toByte()     // R
        bytes[i * 4 + 1] = ((color shr 8) and 0xFF).toByte()  // G
        bytes[i * 4 + 2] = (color and 0xFF).toByte()          // B
        bytes[i * 4 + 3] = ((color shr 24) and 0xFF).toByte() // A
    }
    return bytes

}

internal actual suspend fun createImageBitmapFromBytes(
    bytes: ByteArray,
    width: Int,
    height: Int
): ImageBitmap {
    val pixels = IntArray(width * height)
    for (i in pixels.indices) {
        val r = bytes[i * 4].toInt() and 0xFF
        val g = bytes[i * 4 + 1].toInt() and 0xFF
        val b = bytes[i * 4 + 2].toInt() and 0xFF
        val a = bytes[i * 4 + 3].toInt() and 0xFF
        pixels[i] = (a shl 24) or (r shl 16) or (g shl 8) or b
    }
    return Bitmap.createBitmap(pixels, width, height, Bitmap.Config.ARGB_8888).asImageBitmap()
}