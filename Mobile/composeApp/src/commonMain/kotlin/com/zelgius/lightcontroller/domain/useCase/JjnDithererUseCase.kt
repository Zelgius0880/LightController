package com.zelgius.lightcontroller.domain.useCase

import androidx.compose.ui.graphics.ImageBitmap
import com.zelgius.lightcontroller.createImageBitmapFromBytes
import com.zelgius.lightcontroller.readPixelsToByteArray
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import org.koin.core.annotation.Factory

@Factory
class JjnDithererUseCase {
    companion object {

        private val matrix = arrayOf(
            floatArrayOf(1f, 0f, 7f / 48f),
            floatArrayOf(2f, 0f, 5f / 48f),
            floatArrayOf(-2f, 1f, 3f / 48f),
            floatArrayOf(-1f, 1f, 5f / 48f),
            floatArrayOf(0f, 1f, 7f / 48f),
            floatArrayOf(1f, 1f, 5f / 48f),
            floatArrayOf(2f, 1f, 3f / 48f),
            floatArrayOf(-2f, 2f, 1f / 48f),
            floatArrayOf(-1f, 2f, 3f / 48f),
            floatArrayOf(0f, 2f, 5f / 48f),
            floatArrayOf(1f, 2f, 3f / 48f),
            floatArrayOf(2f, 2f, 1f / 48f)
        )
    }

    suspend operator fun invoke(
        input: ImageBitmap,
        palette: DitherPalette = DitherPalette.Normalized
    ): ImageBitmap =
        withContext(Dispatchers.Default) {

            val w = input.width
            val h = input.height

            // 1. Platform-specific conversion to ByteArray
            val bytes = input.readPixelsToByteArray()

            // 2. Original Dithering Logic
            val pixels = FloatArray(bytes.size)
            for (i in bytes.indices) {
                pixels[i] = (bytes[i].toInt() and 0xFF).toFloat()
            }

            for (y in 0 until h) {
                for (x in 0 until w) {
                    val i = (y * w + x) * 4
                    val oldR = pixels[i]
                    val oldG = pixels[i + 1]
                    val oldB = pixels[i + 2]

                    val nearest = findNearest(oldR, oldG, oldB, palette)
                    pixels[i] = nearest[0].toFloat()
                    pixels[i + 1] = nearest[1].toFloat()
                    pixels[i + 2] = nearest[2].toFloat()

                    val errR = oldR - pixels[i]
                    val errG = oldG - pixels[i + 1]
                    val errB = oldB - pixels[i + 2]

                    for (row in matrix) {
                        val targetX = x + row[0].toInt()
                        val targetY = y + row[1].toInt()
                        if (targetX in 0 until w && targetY in 0 until h) {
                            val ti = (targetY * w + targetX) * 4
                            val factor = row[2]
                            pixels[ti] += errR * factor
                            pixels[ti + 1] += errG * factor
                            pixels[ti + 2] += errB * factor
                        }
                    }
                }
            }

            val outputBytes = ByteArray(pixels.size)
            for (i in pixels.indices) {
                outputBytes[i] = pixels[i].coerceIn(0f, 255f).toInt().toByte()
            }

            // 3. Platform-specific conversion back to ImageBitmap
            createImageBitmapFromBytes(outputBytes, w, h)
        }

    private fun findNearest(r: Float, g: Float, b: Float, palette: DitherPalette): IntArray {

        var bestColor = palette[0]
        var minDistance = Float.MAX_VALUE
        for (color in palette) {
            val dr = r - color[0]
            val dg = g - color[1]
            val db = b - color[2]
            val distance = (dr * dr) + (dg * dg) + (db * db)
            if (distance < minDistance) {
                minDistance = distance
                bestColor = color
            }
        }
        return bestColor
    }
}

enum class DitherPalette(val colors: Array<IntArray>) {
    Raw(
        arrayOf(
            intArrayOf(0, 0, 0),
            intArrayOf(255, 255, 255),
            intArrayOf(255, 0, 0),
            intArrayOf(0, 255, 0),
            intArrayOf(0, 0, 255),
            intArrayOf(255, 255, 0)
        )
    ),
    Normalized(
        arrayOf(
            intArrayOf(0, 0, 0),
            intArrayOf(255, 255, 255),
            intArrayOf(255, 243, 56),
            intArrayOf(191, 0, 0),
            intArrayOf(100, 64, 255),
            intArrayOf(67, 138, 28)
        )
    );

    operator fun get(index: Int) = colors[index]
    operator fun iterator() = colors.iterator()
}
