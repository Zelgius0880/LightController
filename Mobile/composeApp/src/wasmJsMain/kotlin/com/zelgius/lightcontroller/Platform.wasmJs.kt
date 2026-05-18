package com.zelgius.lightcontroller

import androidx.compose.runtime.Composable
import androidx.compose.runtime.mutableStateListOf
import androidx.compose.runtime.remember
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.ImageBitmap
import androidx.compose.ui.graphics.asComposeImageBitmap
import androidx.compose.ui.graphics.asSkiaBitmap
import androidx.compose.ui.graphics.colorspace.ColorSpace
import androidx.compose.ui.graphics.colorspace.ColorSpaces
import androidx.compose.ui.graphics.toComposeImageBitmap
import androidx.compose.ui.graphics.toPixelMap
import androidx.datastore.core.DataStore
import androidx.datastore.preferences.core.PreferenceDataStoreFactory
import androidx.datastore.preferences.core.Preferences
import androidx.navigation3.runtime.NavKey
import com.github.terrakok.navigation3.browser.ChronologicalBrowserNavigation
import com.github.terrakok.navigation3.browser.buildBrowserHistoryFragment
import com.github.terrakok.navigation3.browser.getBrowserHistoryFragmentName
import com.zelgius.lightcontroller.domain.repository.StoredImageRepository
import com.zelgius.lightcontroller.navigation.Home
import com.zelgius.lightcontroller.navigation.Route
import io.github.vinceglb.filekit.FileKit
import io.github.vinceglb.filekit.PlatformFile
import io.github.vinceglb.filekit.download
import io.ktor.client.HttpClient
import io.ktor.client.HttpClientConfig
import io.ktor.client.plugins.contentnegotiation.ContentNegotiation
import io.ktor.client.request.HttpRequestPipeline
import io.ktor.http.URLProtocol
import io.ktor.http.encodedPath
import io.ktor.serialization.kotlinx.json.json
import kotlinx.browser.document
import kotlinx.browser.window
import kotlinx.coroutines.flow.filterNotNull
import kotlinx.coroutines.flow.first
import kotlinx.serialization.json.Json
import okio.Path.Companion.toPath
import org.jetbrains.compose.resources.decodeToImageBitmap
import org.jetbrains.skia.Bitmap
import org.jetbrains.skia.ColorAlphaType
import org.jetbrains.skia.ColorType
import org.jetbrains.skia.EncodedImageFormat
import org.jetbrains.skia.Image
import org.jetbrains.skia.ImageInfo
import org.jetbrains.skia.impl.use
import org.koin.dsl.module
import org.w3c.dom.CanvasRenderingContext2D
import org.w3c.dom.HTMLCanvasElement
import org.w3c.dom.get
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

    actual fun createStoredImageRepository(): StoredImageRepository = StoredImageRepository()

}

@Composable
internal actual fun createBackStack(): MutableList<NavKey> {
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

@OptIn(ExperimentalWasmJsInterop::class)
internal actual fun selfHostedIp(): String? = getLocalIp()

external fun getLocalIp(): String
external fun getNetatmoClientId(): String

internal actual fun openNewTab(url: String) {
    window.open(url = url, target = "_blank")
}

@OptIn(ExperimentalWasmJsInterop::class)
internal fun redirectUrl(): String =
    js("encodeURIComponent(\"http://\"+window.LOCAL_IP)+\"/token_result\"")

@OptIn(ExperimentalWasmJsInterop::class)
internal fun netatmoClientId(): String = getNetatmoClientId()

@OptIn(ExperimentalWasmJsInterop::class)
actual fun buildNetatmoTokenUrl(): String {
    val clientId = netatmoClientId()
    val scope = "read_station"
    val state = Random.nextLong().toString(36).substring(7)

    return "https://api.netatmo.com/oauth2/authorize?client_id=${clientId}&redirect_uri=${redirectUrl()}&scope=${scope}&state=${state}"
}

@Composable
internal actual fun isDarkMode(): Boolean = true

actual val PLATFORM: Platform = Platform.Web

internal actual fun platformHttpClient(
    block: HttpClientConfig<*>.() -> Unit
): HttpClient = HttpClient {
    block()
}

internal actual fun ImageBitmap.readPixelsToByteArray(): ByteArray {
    val bitmap = asSkiaBitmap()
    val pixelMap = toPixelMap()


    val pixels = IntArray(pixelMap.width * pixelMap.height)
    this.readPixels(pixels)


    val width = pixelMap.width
    val height = pixelMap.height

    val output = ByteArray(width * height * 4)

    var i = 0
    var p = 0
    while (i < output.size) {

        val a = (pixels[p] ushr 24) and 0xFF
        val r = (pixels[p] ushr 16) and 0xFF
        val g = (pixels[p] ushr 8) and 0xFF
        val b = pixels[p] and 0xFF

        output[i] = r.toByte()      // R
        output[i + 1] = g.toByte()  // G
        output[i + 2] = b.toByte()  // B
        output[i + 3] = a.toByte()  // A

        i += 4
        p++
    }

    return output

}

internal actual suspend fun createImageBitmapFromBytes(
    bytes: ByteArray,
    width: Int,
    height: Int
): ImageBitmap {
    val bitmap = Bitmap()

    bitmap.allocPixels(
        ImageInfo(
            width = width,
            height = height,
            colorType = ColorType.RGBA_8888,
            colorSpace = org.jetbrains.skia.ColorSpace.sRGB,
            alphaType = ColorAlphaType.OPAQUE
        )
    )

    bitmap.installPixels(bytes)

    return bitmap.asComposeImageBitmap()
}
