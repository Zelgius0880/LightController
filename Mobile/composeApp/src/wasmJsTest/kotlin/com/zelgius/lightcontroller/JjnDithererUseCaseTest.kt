package com.zelgius.lightcontroller

import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.Canvas
import androidx.compose.ui.graphics.ImageBitmap
import androidx.compose.ui.graphics.ImageBitmapConfig
import androidx.compose.ui.graphics.toComposeImageBitmap
import com.zelgius.lightcontroller.Base64Image
import com.zelgius.lightcontroller.data.ImageConfig
import com.zelgius.lightcontroller.domain.useCase.JjnDithererUseCase
import com.zelgius.lightcontroller.ui.image.drawImageBitmap
import io.ktor.client.*
import io.ktor.client.engine.mock.*
import io.ktor.client.plugins.contentnegotiation.*
import io.ktor.http.*
import io.ktor.serialization.kotlinx.json.*
import kotlinx.coroutines.test.runTest
import org.jetbrains.skia.Image
import org.kotlincrypto.hash.sha2.SHA256
import kotlin.io.encoding.Base64
import kotlin.test.Test
import kotlin.test.assertEquals

class JjnDithererUseCaseTest {

    val useCase = JjnDithererUseCase()
    val sha256 = SHA256()


    @Test
    fun `test converted byte array fit to the expected one`() = runTest {
        val bytes = Base64.decode(Base64Image.base64)
        val image = ImageBitmap(
            ImageConfig.TARGET_WIDTH,
            ImageConfig.TARGET_HEIGHT,
            config = ImageBitmapConfig.Argb8888
        )

        Canvas(image).apply {
            drawImageBitmap(
                Image.makeFromEncoded(bytes).toComposeImageBitmap(),
                size = Size(
                    image.width.toFloat(), image.height.toFloat()
                )
            )
        }

        assertEquals(
            expected = "5/NLBtr/9OcFqBZHkhCONnL1CiOqebHMPTo5RG+dbps=",
            actual = Base64.encode(sha256.digest(image.readPixelsToByteArray()))
        )

        val ditheredImage = useCase(image)
    }
}

