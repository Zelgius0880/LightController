package com.zelgius.lightcontroller.ui.image

import androidx.compose.runtime.Composable
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.Canvas
import androidx.compose.ui.graphics.ImageBitmap
import androidx.compose.ui.graphics.Paint
import androidx.compose.ui.graphics.drawscope.CanvasDrawScope
import androidx.compose.ui.graphics.drawscope.DrawScope
import androidx.compose.ui.graphics.drawscope.drawIntoCanvas
import androidx.compose.ui.graphics.painter.Painter
import androidx.compose.ui.unit.Density
import androidx.compose.ui.unit.LayoutDirection
import com.zelgius.lightcontroller.domain.useCase.JjnDithererUseCase
import io.github.vinceglb.filekit.FileKit
import io.github.vinceglb.filekit.dialogs.FileKitType
import io.github.vinceglb.filekit.dialogs.compose.util.toImageBitmap
import io.github.vinceglb.filekit.dialogs.openFilePicker
import org.koin.compose.viewmodel.koinViewModel

@Composable
fun ImageScreen(
    viewModel: ImageViewModel = koinViewModel(),
) {

// Pick only image files
    val imageFile = FileKit.openFilePicker(type = FileKitType.Image)?.toImageBitmap()
}

fun Painter.toImageBitmap(width: Int, height: Int): ImageBitmap {
    val imgBitmap = ImageBitmap(width, height)

    Canvas(imgBitmap).apply {
        CanvasDrawScope().drawImageBitmap(
            imgBitmap,
            size = Size(
                width = JjnDithererUseCase.TARGET_WIDTH.toFloat(),
                height = JjnDithererUseCase.TARGET_WIDTH.toFloat()
            )
        )
    }

    return imgBitmap
}

fun DrawScope.drawImageBitmap(imageBitmap: ImageBitmap, offset: Offset = Offset.Zero, size: Size) {
    val imgWidth = imageBitmap.width
    val imgHeight = imageBitmap.height
    val scaleX = size.width / imgWidth
    val scaleY = size.height / imgHeight

    this.drawIntoCanvas { canvas: Canvas ->
        canvas.translate(offset.x, offset.y)
        canvas.scale(scaleX, scaleY)
        canvas.drawImage(
            image = imageBitmap,
            Offset(0f, 0f),
            Paint()
        )
    }
}