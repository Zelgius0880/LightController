package com.zelgius.lightcontroller.ui.image.stored

import androidx.compose.animation.AnimatedContent
import androidx.compose.foundation.Image
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.aspectRatio
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.lazy.LazyItemScope
import androidx.compose.foundation.lazy.LazyRow
import androidx.compose.foundation.lazy.items
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.twotone.Send
import androidx.compose.material.icons.twotone.Check
import androidx.compose.material.icons.twotone.Delete
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Button
import androidx.compose.material3.Card
import androidx.compose.material3.CircularWavyProgressIndicator
import androidx.compose.material3.ExperimentalMaterial3ExpressiveApi
import androidx.compose.material3.FilledIconButton
import androidx.compose.material3.Icon
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedIconButton
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.ComposableOpenTarget
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.remember
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Canvas
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.ImageBitmap
import androidx.compose.ui.graphics.ImageBitmapConfig
import androidx.compose.ui.graphics.Paint
import androidx.compose.ui.graphics.PaintingStyle
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import com.zelgius.lightcontroller.data.ImageConfig
import com.zelgius.lightcontroller.data.ImageConfig.TARGET_HEIGHT
import com.zelgius.lightcontroller.data.ImageConfig.TARGET_WIDTH
import com.zelgius.lightcontroller.domain.repository.EInkThumbnail
import lightcontroller.composeapp.generated.resources.Res
import lightcontroller.composeapp.generated.resources.close
import lightcontroller.composeapp.generated.resources.select_image
import lightcontroller.composeapp.generated.resources.stored_image
import org.jetbrains.compose.resources.stringResource
import org.koin.compose.viewmodel.koinViewModel

@Composable
fun StoredImageDialog(
    viewModel: StoredImagesDialogViewModel = koinViewModel(),
    onDismissRequest: () -> Unit
) {
    LaunchedEffect(Unit) {
        viewModel.initialize()
    }

    val state by viewModel.state.collectAsState()
    StoredImageDialog(
        state = state,
        onDismissRequest = onDismissRequest,
        onRemoveImage = viewModel::deleteSelectedImage,
        onSend = viewModel::send,
        onThumbnailClicked = { viewModel.displayImage(it.id) }
    )
}

@OptIn(ExperimentalMaterial3ExpressiveApi::class)
@Composable
private fun StoredImageDialog(
    state: State,
    onDismissRequest: () -> Unit,
    onRemoveImage: () -> Unit,
    onSend: () -> Unit,
    onThumbnailClicked: (EInkThumbnail) -> Unit
) {
    AlertDialog(
        onDismissRequest = onDismissRequest,
        title = {
            Text(stringResource(Res.string.stored_image))
        },
        text = {
            Column(verticalArrangement = Arrangement.spacedBy(8.dp)) {
                LazyRow(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                    items(state.images, key = { it.id }) { item ->
                        ThumbnailsItem(
                            item = item,
                            selected = item.id == state.selectedThumbnail?.id,
                            onThumbnailClicked = onThumbnailClicked
                        )
                    }
                }

                val image = state.displayedImage
                if (image == null) {
                    Card(
                        modifier = Modifier.fillMaxWidth()
                            .aspectRatio(TARGET_WIDTH / TARGET_HEIGHT.toFloat())
                    ) {
                        Box(Modifier.fillMaxSize()) {
                            Text(
                                stringResource(Res.string.select_image), modifier = Modifier.align(
                                    Alignment.Center
                                )
                            )
                        }
                    }
                } else {
                    Box(
                        Modifier.fillMaxWidth()
                            .aspectRatio(TARGET_WIDTH / TARGET_HEIGHT.toFloat())
                    ) {
                        Image(
                            image,
                            contentDescription = null,
                            modifier = Modifier.fillMaxSize()
                        )

                        Column(
                            modifier = Modifier.align(
                                Alignment.TopEnd
                            ).padding(8.dp)
                        ) {

                            FilledIconButton(
                                onClick = onSend,
                                enabled = !state.loading
                            ) {
                                AnimatedContent(state.loading) {
                                    if(!it) {
                                        Icon(
                                            Icons.AutoMirrored.TwoTone.Send,
                                            contentDescription = null
                                        )
                                    } else {
                                        CircularWavyProgressIndicator(modifier = Modifier.size(32.dp))
                                    }
                                }
                            }

                            OutlinedIconButton(
                                onClick = onRemoveImage,
                            ) {
                                Icon(Icons.TwoTone.Delete, contentDescription = null)
                            }
                        }
                    }
                }
            }
        },
        confirmButton = {
            Button(onClick = onDismissRequest) {
                Text(stringResource(Res.string.close))
            }
        }
    )
}

@Composable
private fun LazyItemScope.ThumbnailsItem(
    item: EInkThumbnail,
    selected: Boolean,
    onThumbnailClicked: (EInkThumbnail) -> Unit
) {
    Card(
        modifier = Modifier.height(72.dp)
            .aspectRatio(TARGET_WIDTH / TARGET_HEIGHT.toFloat())
            .animateItem()
            .clickable(onClick = { onThumbnailClicked(item) })
    ) {
        Box {
            Image(
                item.bitmap,
                contentDescription = null,
                modifier = Modifier.fillMaxSize()
            )

            if (selected) {
                Box(
                    modifier = Modifier.fillMaxSize()
                        .background(MaterialTheme.colorScheme.background.copy(alpha = 0.1f))
                ) {
                    Icon(
                        Icons.TwoTone.Check,
                        contentDescription = null,
                        modifier = Modifier.fillMaxSize().padding(8.dp)
                    )
                }
            }
        }
    }
}


@Composable
@Preview
private fun ThumbnailsItemPreview() {
    val image = remember {
        val image = ImageBitmap(
            TARGET_WIDTH,
            TARGET_HEIGHT,
            config = ImageBitmapConfig.Argb8888
        )

        Canvas(image).apply {
            // 1. Fill the background with a neutral dark gray
            val backgroundPaint = Paint().apply { color = Color(0xFF1A1A1B) }
            drawRect(0f, 0f, TARGET_WIDTH.toFloat(), TARGET_HEIGHT.toFloat(), backgroundPaint)

            // 2. Draw a centered calibration circle
            val circlePaint = Paint().apply {
                color = Color.Cyan
                style = PaintingStyle.Stroke
                strokeWidth = 5f
                isAntiAlias = true
            }
            drawCircle(Offset(TARGET_WIDTH / 2f, TARGET_HEIGHT / 2f), 400f, circlePaint)

            // 3. Draw a semi-transparent overlay to test Alpha
            val rectPaint = Paint().apply {
                color = Color.Magenta.copy(alpha = 0.5f)
            }
            drawRect(400f, 300f, 1200f, 900f, rectPaint)

            // 4. Add a "Horizon Line" to test coordinate alignment
            val linePaint = Paint().apply {
                color = Color.White
                strokeWidth = 2f
            }
            drawLine(
                Offset(0f, TARGET_HEIGHT / 2f),
                Offset(TARGET_WIDTH.toFloat(), TARGET_HEIGHT / 2f),
                paint = linePaint
            )

            // 5. Diagonal cross to check corners
            drawLine(
                Offset(0f, 0f),
                Offset(TARGET_WIDTH.toFloat(), TARGET_HEIGHT.toFloat()),
                linePaint
            )
        }

        image
    }

    LazyRow() {
        item {
            ThumbnailsItem(
                item = EInkThumbnail(
                    "1",
                    image
                ),
                selected = false,
            ) {}

        }

        item {
            ThumbnailsItem(
                item = EInkThumbnail(
                    "1",
                    image
                ),
                selected = true,
            ) {}
        }
    }
}