package com.zelgius.lightcontroller.domain.repository

import androidx.compose.ui.graphics.ImageBitmap
import com.zelgius.lightcontroller.domain.useCase.DitherPalette

actual class StoredImageRepository {

    actual fun saveImage(data: ByteArray, thumbnail: ImageBitmap, palette: DitherPalette, width: Int, height: Int){}
    

    actual fun getThumbnails(): List<EInkThumbnail> = emptyList()


    actual fun restoreFullImage(id: String): LoadedImage? = null


    actual fun deleteImage(id: String){}

    actual fun getDisplayableImage(id: String): ImageBitmap? = null

}