package com.zelgius.lightcontroller.domain.useCase

import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import org.koin.core.annotation.Factory

@Factory
class JjnDithererUseCase {
    companion object {
        const val TARGET_WIDTH = 1600
        const val TARGET_HEIGHT = 1200
        /* private val palette = arrayOf(
             intArrayOf(0, 0, 0),
             intArrayOf(255, 255, 255),
             intArrayOf(255, 0, 0),
             intArrayOf(0, 255, 0),
             intArrayOf(0, 0, 255),
             intArrayOf(255, 255, 0)
         )*/
        private val palette = arrayOf(
            intArrayOf(0, 0, 0),
            intArrayOf(255, 255, 255),
            intArrayOf(255, 243, 56),
            intArrayOf(191, 0, 0),
            intArrayOf(100, 64, 255),
            intArrayOf(67, 138, 28)
        )

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

    suspend operator fun invoke(input: ByteArray, w: Int = TARGET_WIDTH, h: Int = TARGET_HEIGHT): ByteArray =
        withContext(Dispatchers.Default) {
            // Convert signed Bytes to a mutable FloatArray for precision math
            val pixels = FloatArray(input.size)
            for (i in input.indices) {
                pixels[i] = (input[i].toInt() and 0xFF).toFloat()
            }

            for (y in 0 until h) {
                for (x in 0 until w) {
                    val i = (y * w + x) * 4

                    // Get current pixel color
                    val oldR = pixels[i]
                    val oldG = pixels[i + 1]
                    val oldB = pixels[i + 2]

                    // Find nearest palette match
                    val nearest = findNearest(oldR, oldG, oldB)
                    pixels[i] = nearest[0].toFloat()
                    pixels[i + 1] = nearest[1].toFloat()
                    pixels[i + 2] = nearest[2].toFloat()

                    // Calculate error (Difference between original and palette)
                    val errR = oldR - pixels[i]
                    val errG = oldG - pixels[i + 1]
                    val errB = oldB - pixels[i + 2]

                    // Distribute error to neighbors using Jarvis, Judice, and Ninke matrix
                    for (row in matrix) {
                        val targetX = x + row[0].toInt()
                        val targetY = y + row[1].toInt()
                        val factor = row[2]

                        if (targetX in 0 until w && targetY in 0 until h) {
                            val ti = (targetY * w + targetX) * 4
                            pixels[ti] += errR * factor
                            pixels[ti + 1] += errG * factor
                            pixels[ti + 2] += errB * factor
                        }
                    }
                }
            }

            // Convert back to ByteArray, clamping values to 0-255
            val output = ByteArray(pixels.size)
            for (i in pixels.indices) {
                output[i] = pixels[i].coerceIn(0f, 255f).toInt().toByte()
            }

            output
        }

    private fun findNearest(r: Float, g: Float, b: Float): IntArray {
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