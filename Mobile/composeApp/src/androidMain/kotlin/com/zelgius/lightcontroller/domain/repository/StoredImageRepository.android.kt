package com.zelgius.lightcontroller.domain.repository

import android.content.Context
import android.graphics.Bitmap
import android.graphics.BitmapFactory
import android.graphics.Matrix
import androidx.compose.ui.graphics.Canvas
import androidx.compose.ui.graphics.ImageBitmap
import androidx.compose.ui.graphics.ImageBitmapConfig
import androidx.compose.ui.graphics.asAndroidBitmap
import androidx.compose.ui.graphics.asImageBitmap
import androidx.compose.ui.graphics.withSave
import com.zelgius.lightcontroller.domain.useCase.DitherPalette
import java.io.File
import java.nio.ByteBuffer
import androidx.core.graphics.createBitmap

actual class StoredImageRepository(
    context: Context
) {
    private val rootDir = File(context.filesDir, "carousel").apply { mkdirs() }


    actual fun saveImage(
        data: ByteArray,
        thumbnail: ImageBitmap,
        palette: DitherPalette,
        width: Int,
        height: Int
    ) {
        val id = System.currentTimeMillis().toString()

        // 1. Save Full Binary
        val binFile = File(rootDir, "full_$id.bin")
        binFile.outputStream().use { out ->
            out.write("EINK".toByteArray()) // Magic
            out.write(palette.ordinal)
            out.write(ByteBuffer.allocate(8).putInt(width).putInt(height).array())
            out.write(data)
        }

        // 2. Save Thumbnail
        val thumbFile = File(rootDir, "thumb_$id.png")
        thumbFile.outputStream().use { out ->
            thumbnail.asAndroidBitmap().compress(Bitmap.CompressFormat.PNG, 90, out)
        }
    }

    actual fun getThumbnails(): List<EInkThumbnail> {
        val files = rootDir.listFiles { _, name -> name.startsWith("thumb_") } ?: emptyArray()
        return files.map { file ->
            val id = file.name.removePrefix("thumb_").removeSuffix(".png")
            EInkThumbnail(
                id = id,
                bitmap = BitmapFactory.decodeFile(file.absolutePath).asImageBitmap()
            )
        }.sortedBy { it.id } // Sort by timestamp
    }

    /**
     * Restores the full binary data for sending to the device.
     */
    actual fun restoreFullImage(id: String): LoadedImage? {
        val file = File(rootDir, "full_$id.bin")
        if (!file.exists()) return null

        return file.inputStream().use { input ->
            val magic = ByteArray(4).also { input.read(it) }
            if (String(magic) != "EINK") return null

            val palette = DitherPalette.entries[input.read()]
            val dims = ByteArray(8).also { input.read(it) }
            val buffer = ByteBuffer.wrap(dims)
            val w = buffer.int
            val h = buffer.int

            LoadedImage(input.readBytes(), palette, w, h)
        }
    }

    /**
     * Deletes both the binary and the thumbnail.
     */
    actual fun deleteImage(id: String) {
        File(rootDir, "full_$id.bin").delete()
        File(rootDir, "thumb_$id.png").delete()
    }

    actual fun getDisplayableImage(id: String): ImageBitmap? {
        val image = restoreFullImage(id) ?: return null

        val width = image.width
        val height = image.height
        val palette = image.palette
        val data = image.pixels

        val pixelBuffer = IntArray(width * height)

        var pixelIndex = 0

        for (byte in data) {
            val b = byte.toInt() and 0xFF

            // High nibble
            val c1Encoded = (b shr 4) and 0x0F
            // Low nibble
            val c2Encoded = b and 0x0F

            // APPLY THE UNDO MAPPING HERE
            if (pixelIndex < pixelBuffer.size) {
                val actualIndex1 = undoHardwareMapping(c1Encoded)
                pixelBuffer[pixelIndex++] = paletteColorToArgb(palette[actualIndex1])
            }
            if (pixelIndex < pixelBuffer.size) {
                val actualIndex2 = undoHardwareMapping(c2Encoded)
                pixelBuffer[pixelIndex++] = paletteColorToArgb(palette[actualIndex2])
            }
        }

        val androidBitmap = createBitmap(height, width, Bitmap.Config.ARGB_8888).let {
            it.setPixels(pixelBuffer, 0, height, 0, 0, height, width)
            val matrix = Matrix()
            matrix.postRotate(-90f)
            val rotated = Bitmap.createBitmap(it, 0, 0, it.getWidth(), it.getHeight(), matrix, true)
            it.recycle()
            rotated
        }

        return androidBitmap.asImageBitmap()
    }


    private fun undoHardwareMapping(encodedIndex: Int): Int {
        return when {
            encodedIndex > 4 -> (encodedIndex - 1).coerceIn(0, 5)
            encodedIndex == 4 -> 3 // Safety: if 4 somehow appeared, map to nearest
            else -> encodedIndex.coerceIn(0, 5)
        }
    }

    private fun paletteColorToArgb(rgb: IntArray): Int {
        val r = rgb[0]
        val g = rgb[1]
        val b = rgb[2]
        // 0xFF for Alpha (Fully Opaque) | R | G | B
        return (0xFF shl 24) or (r shl 16) or (g shl 8) or b
    }
}
