package com.zelgius.lightcontroller.ui.image.stored

import androidx.compose.ui.graphics.ImageBitmap
import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import androidx.lifecycle.viewmodel.compose.viewModel
import com.zelgius.lightcontroller.domain.repository.EInkThumbnail
import com.zelgius.lightcontroller.domain.repository.StoredImageRepository
import com.zelgius.lightcontroller.domain.repository.web.ServerRepository
import com.zelgius.lightcontroller.domain.useCase.SendImageUseCase
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.update
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import org.koin.core.annotation.KoinViewModel

@KoinViewModel
class StoredImagesDialogViewModel(
    private val storedImageRepository: StoredImageRepository,
    private val serverRepository: ServerRepository
): ViewModel() {

    private val _state = MutableStateFlow(State())
    val state = _state.asStateFlow()

    fun initialize() {
        _state.update {
            State().copy(images = storedImageRepository.getThumbnails())
        }
    }

    fun displayImage(id: String) = viewModelScope.launch {
        withContext(Dispatchers.Default) {
            storedImageRepository.getDisplayableImage(id)?.let {
                _state.update { state ->
                    state.copy(displayedImage = it, selectedThumbnail = state.images.find {i ->  i.id == id })
                }
            }
        }
    }

    fun deleteSelectedImage() {
        val id = state.value.selectedThumbnail?.id?: return

        storedImageRepository.deleteImage(id)
        initialize()
    }

    fun send() = viewModelScope.launch{
        _state.update {
            it.copy(loading = true)
        }
        val id = state.value.selectedThumbnail?.id?: return@launch

        val image = storedImageRepository.restoreFullImage(id)
        if(image != null) {
            serverRepository.uploadImage(image.pixels)
            serverRepository.triggerRender()
        }
        _state.update {
            it.copy(loading = false)
        }
    }
}

data class State(
    val images: List<EInkThumbnail> = emptyList(),
    val displayedImage: ImageBitmap? = null,
    val loading: Boolean = false,
    val selectedThumbnail: EInkThumbnail? = null
)


