package com.zelgius.lightcontroller.domain.repository

import androidx.compose.ui.graphics.ImageBitmap
import com.zelgius.lightcontroller.domain.useCase.DitherPalette


expect class StoredImageRepository {
    /**
     * Saves the binary data and a thumbnail.
     * @param data The packed nibble ByteArray (1600x1200 / 2)
     * @param thumbnail The ImageBitmap to save as a preview
     * @param palette The palette used (stored in the binary header)
     */
    fun saveImage(data: ByteArray, thumbnail: ImageBitmap, palette: DitherPalette, width: Int, height: Int)

    /**
     * Lists all thumbnails for the carousel.
     */
    fun getThumbnails(): List<EInkThumbnail>

    /**
     * Restores the full binary data for sending to the device.
     */
    fun restoreFullImage(id: String): LoadedImage?

    /**
     * Deletes both the binary and the thumbnail.
     */
    fun deleteImage(id: String)

    fun getDisplayableImage(id: String): ImageBitmap?
}

data class EInkThumbnail(val id: String, val bitmap:ImageBitmap)
data class LoadedImage(val pixels: ByteArray, val palette: DitherPalette, val width: Int, val height: Int)