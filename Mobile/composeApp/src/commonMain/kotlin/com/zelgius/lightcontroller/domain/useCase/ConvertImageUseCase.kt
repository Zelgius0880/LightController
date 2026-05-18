package com.zelgius.lightcontroller.domain.useCase

import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Canvas
import androidx.compose.ui.graphics.ImageBitmap
import androidx.compose.ui.graphics.Paint
import androidx.compose.ui.graphics.withSave
import com.zelgius.lightcontroller.domain.repository.web.ServerRepository
import com.zelgius.lightcontroller.readPixelsToByteArray
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import org.koin.core.annotation.Factory

@Factory
class ConvertImageUseCase() {
    suspend operator fun invoke(imageBitmap: ImageBitmap, palette: DitherPalette): ByteArray {
        val image = ImageBitmap(imageBitmap.height, imageBitmap.width)
        val canvas = Canvas(image)
        canvas.withSave {
            canvas.translate(image.width.toFloat(), 0f)
            canvas.rotate(90f)
            canvas.drawImage(imageBitmap, topLeftOffset = Offset.Zero, Paint())
        }

        val data = withContext(Dispatchers.Default) {
            canvasTo6ColorBinary(image, palette)
        }

        return data

    }

    private fun canvasTo6ColorBinary(imageBitmap: ImageBitmap, palette: DitherPalette): ByteArray {
        val width = imageBitmap.width
        val height = imageBitmap.height

        // Argb8888 uses 4 bytes per pixel. We'll read them into an IntArray
        val buffer = IntArray(width * height)
        imageBitmap.readPixels(buffer)

        // The output stores two 4-bit color indices per byte (Total: width * height / 2)
        val output = ByteArray((width * height) / 2)
        var outIndex = 0

        for (y in 0 until height) {
            // We step by 2 because we pack two pixels into one byte
            for (x in 0 until width step 2) {
                val pixel1 = buffer[y * width + x]
                val pixel2 = buffer[y * width + x + 1]

                // Extract RGB from Argb8888 (Int)
                val c1 = depalette(
                    (pixel1 shr 16) and 0xFF, // Red
                    (pixel1 shr 8) and 0xFF,  // Green
                    pixel1 and 0xFF ,          // Blue
                    palette
                )

                val c2 = depalette(
                    (pixel2 shr 16) and 0xFF,
                    (pixel2 shr 8) and 0xFF,
                    pixel2 and 0xFF,
                    palette
                )

                // Pack two 4-bit values into one 8-bit byte
                output[outIndex++] = ((c1 shl 4) or (c2 and 0x0F)).toByte()
            }
        }

        return output
    }

    private fun depalette(r: Int, g: Int, b: Int, palette: DitherPalette): Int {
        var mindiff = Int.MAX_VALUE
        var bestc = 0

        for (p in palette.colors.indices) {
            val pr = palette[p][0]
            val pg = palette[p][1]
            val pb = palette[p][2]

            val diffr = r - pr
            val diffg = g - pg
            val diffb = b - pb

            val diff = diffr * diffr + diffg * diffg + diffb * diffb

            if (diff < mindiff) {
                mindiff = diff
                bestc = if (p > 3) (p + 1) else p
            }
        }

        return bestc and 0x0F
    }
}