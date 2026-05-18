package com.zelgius.lightcontroller.ui.image

import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.Canvas
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.ImageBitmap
import androidx.compose.ui.text.TextMeasurer
import androidx.compose.ui.unit.Density
import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.zelgius.lightcontroller.domain.repository.ImageRenderer
import com.zelgius.lightcontroller.domain.repository.StoredImageRepository
import com.zelgius.lightcontroller.domain.useCase.ConvertImageUseCase
import com.zelgius.lightcontroller.domain.useCase.DitherPalette
import com.zelgius.lightcontroller.domain.useCase.JjnDithererUseCase
import com.zelgius.lightcontroller.domain.useCase.SendImageUseCase
import com.zelgius.lightcontroller.utils.drawImageBitmap
import io.github.vinceglb.filekit.FileKit
import io.github.vinceglb.filekit.dialogs.FileKitType
import io.github.vinceglb.filekit.dialogs.compose.util.toImageBitmap
import io.github.vinceglb.filekit.dialogs.openFilePicker
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.update
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import lightcontroller.composeapp.generated.resources.Res
import lightcontroller.composeapp.generated.resources.image_queued
import lightcontroller.composeapp.generated.resources.image_render_failed
import lightcontroller.composeapp.generated.resources.image_rendered
import org.jetbrains.compose.resources.getString
import org.koin.core.annotation.KoinViewModel

@KoinViewModel
class ImageViewModel(
    val jjnDithererUseCase: JjnDithererUseCase,
    val sendImageUseCase: SendImageUseCase,
    val convertImageUseCase: ConvertImageUseCase,
    val storedImageRepository: StoredImageRepository
) : ViewModel() {

    private val _snackbarMessage = MutableStateFlow<String?>(null)
    val snackbarMessage = _snackbarMessage.asStateFlow()
    fun clearMessage() {
        _snackbarMessage.value = null
    }


    private val _state = MutableStateFlow(ImageState())
    val state = _state.asStateFlow()

    private lateinit var imageRenderer: ImageRenderer

    fun initialize(
        textMeasurer: TextMeasurer,
        density: Density,
    ) {
        imageRenderer = ImageRenderer(textMeasurer, density)
    }


    fun pickImage() = viewModelScope.launch {
        val image = FileKit.openFilePicker(type = FileKitType.Image)?.toImageBitmap()
        if (image != null) {
            imageRenderer.apply {
                setImage(image)
                save()
                drawOverlay()
                drawPreviews()
            }
            _state.update {
                it.copy(image = imageRenderer.image)
            }
        }
    }

    fun onOverlayChanged(alpha: Float, color: Color) = viewModelScope.launch {
        imageRenderer.apply {

            restore()
            drawOverlay(alpha = alpha, color = color)
            drawPreviews()
            _state.update {
                it.copy(alpha = alpha, color = color, image = image)
            }
        }
    }

    fun onPaletteChanged(palette: DitherPalette) {
        _state.update {
            it.copy(palette = palette)
        }

    }

    fun dither() = viewModelScope.launch {

        _state.update {
            it.copy(isLoading = true)
        }

        imageRenderer.restore()
        imageRenderer.drawOverlay(state.value.color, state.value.alpha)

        val ditheredImage = jjnDithererUseCase(imageRenderer.image, state.value.palette)
        imageRenderer.setImage(ditheredImage)
        imageRenderer.drawPreviews()

        _state.update {
            it.copy(image = imageRenderer.image, ditheredImage = ditheredImage, isLoading = false)
        }
    }

    fun reset() {
        _state.value = ImageState()
    }

    fun onSend() = viewModelScope.launch {
        _state.update {
            it.copy(isLoading = true)
        }

        try {
            imageRenderer.restore()
            imageRenderer.drawOverlay(state.value.color, state.value.alpha)

            val ditheredImage = jjnDithererUseCase(imageRenderer.image, state.value.palette)
            sendImageUseCase(convertImageUseCase(ditheredImage, state.value.palette))

            _snackbarMessage.value = getString(Res.string.image_rendered)
            _state.update {
                it.copy(isLoading = false)
            }
        } catch (e: Exception) {
            e.printStackTrace()
            _snackbarMessage.value = getString(Res.string.image_render_failed)
            _state.update {
                it.copy(isLoading = false)
            }
        }

    }

    fun saveImage() = viewModelScope.launch {
        val image = state.value.ditheredImage ?: return@launch
        val palette = state.value.palette

        val ratio = 512 / image.width.toFloat()
        val thumbnail =
            ImageBitmap((image.width * ratio).toInt(), (image.height * ratio).toInt())

        Canvas(thumbnail).apply {
            drawImageBitmap(
                image,
                Offset.Zero,
                Size(thumbnail.width.toFloat(), thumbnail.height.toFloat())
            )
        }

        withContext(Dispatchers.Default) {
            storedImageRepository.saveImage(
                convertImageUseCase(image, palette),
                thumbnail,
                palette,
                imageRenderer.image.width,
                imageRenderer.image.height
            )
        }

        _snackbarMessage.value = getString(Res.string.image_queued)

    }
}

data class ImageState(
    val image: ImageBitmap? = null,
    val ditheredImage: ImageBitmap? = null,
    val isLoading: Boolean = false,
    val alpha: Float = 0.6f,
    val color: Color = Color.Black,
    val palette: DitherPalette = DitherPalette.Normalized
)
