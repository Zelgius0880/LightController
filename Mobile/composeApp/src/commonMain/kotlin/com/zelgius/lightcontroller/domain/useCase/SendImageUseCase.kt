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
class SendImageUseCase(
    private val serverRepository: ServerRepository
) {
    suspend operator fun invoke(data: ByteArray) {
        serverRepository.uploadImage(data)
        serverRepository.triggerRender()
    }
}