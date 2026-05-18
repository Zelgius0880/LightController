package com.zelgius.lightcontroller.ui.image

import androidx.compose.animation.AnimatedVisibility
import androidx.compose.foundation.BorderStroke
import androidx.compose.foundation.Image
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.FlowRow
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.aspectRatio
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.heightIn
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.widthIn
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.filled.ArrowBack
import androidx.compose.material.icons.automirrored.filled.Send
import androidx.compose.material.icons.automirrored.twotone.Send
import androidx.compose.material.icons.twotone.Check
import androidx.compose.material.icons.twotone.Save
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.Card
import androidx.compose.material3.CircularWavyProgressIndicator
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.ExperimentalMaterial3ExpressiveApi
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.IconButtonDefaults
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Scaffold
import androidx.compose.material3.SegmentedButton
import androidx.compose.material3.SegmentedButtonDefaults
import androidx.compose.material3.SingleChoiceSegmentedButtonRow
import androidx.compose.material3.Slider
import androidx.compose.material3.SnackbarHost
import androidx.compose.material3.SnackbarHostState
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.material3.TopAppBar
import androidx.compose.runtime.Composable
import androidx.compose.runtime.DisposableEffect
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Canvas
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.ImageBitmap
import androidx.compose.ui.graphics.ImageBitmapConfig
import androidx.compose.ui.graphics.Paint
import androidx.compose.ui.graphics.PaintingStyle
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.text.rememberTextMeasurer
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.max
import com.zelgius.lightcontroller.PLATFORM
import com.zelgius.lightcontroller.Platform
import com.zelgius.lightcontroller.data.ImageConfig.TARGET_HEIGHT
import com.zelgius.lightcontroller.data.ImageConfig.TARGET_WIDTH
import com.zelgius.lightcontroller.domain.useCase.DitherPalette
import com.zelgius.lightcontroller.ui.image.stored.StoredImageDialog
import com.zelgius.lightcontroller.ui.theme.greyShades
import com.zelgius.lightcontroller.utils.getContrastColor
import lightcontroller.composeapp.generated.resources.Res
import lightcontroller.composeapp.generated.resources.alpha
import lightcontroller.composeapp.generated.resources.color_palette
import lightcontroller.composeapp.generated.resources.normalized
import lightcontroller.composeapp.generated.resources.pick_image
import lightcontroller.composeapp.generated.resources.pick_image_meassage
import lightcontroller.composeapp.generated.resources.preview_dither
import lightcontroller.composeapp.generated.resources.raw
import lightcontroller.composeapp.generated.resources.show_queue
import org.jetbrains.compose.resources.stringResource
import org.koin.compose.viewmodel.koinViewModel

@Composable
fun ImageScreen(
    onBack: (() -> Unit)?,
    viewModel: ImageViewModel = koinViewModel(),
) {
    val textMeasurer = rememberTextMeasurer()
    val density = LocalDensity.current
    LaunchedEffect(Unit) {
        viewModel.initialize(textMeasurer, density)
        viewModel.clearMessage()
    }

    DisposableEffect(Unit) {
        onDispose {
            viewModel.reset()
            viewModel.clearMessage()
        }
    }

    val snackbarHostState = remember { SnackbarHostState() }

    val snackbarMessage by viewModel.snackbarMessage.collectAsState()

    // Listen for events from ViewModel
    LaunchedEffect(snackbarMessage) {
        snackbarMessage?.let { message ->
            snackbarHostState.showSnackbar(message)
            viewModel.clearMessage()
        }
    }

    val state by viewModel.state.collectAsState()
    ImageScreen(
        onBack = onBack,
        state = state,
        onPickImage = viewModel::pickImage,
        onDither = viewModel::dither,
        snackbarHostState = snackbarHostState,
        onAlphaChange = {
            viewModel.onOverlayChanged(it, state.color)
        },
        onColorChange = {
            viewModel.onOverlayChanged(state.alpha, it)
        },
        onPaletteChange = viewModel::onPaletteChanged,
        onSend = viewModel::onSend,
        onSaveImage = viewModel::saveImage
    )
}

@OptIn(ExperimentalMaterial3ExpressiveApi::class, ExperimentalMaterial3Api::class)
@Composable
fun ImageScreen(
    onBack: (() -> Unit)?,
    state: ImageState,
    snackbarHostState: SnackbarHostState = remember { SnackbarHostState() },
    onPickImage: () -> Unit = {},
    onDither: () -> Unit = {},
    onAlphaChange: (Float) -> Unit = {},
    onColorChange: (Color) -> Unit = {},
    onPaletteChange: (DitherPalette) -> Unit = {},
    onSend: () -> Unit = {},
    onSaveImage: () -> Unit = {}
) {
    var showSavedImage by remember { mutableStateOf(false) }
    Scaffold(
        snackbarHost = { SnackbarHost(snackbarHostState) },
        topBar = {
            TopAppBar(
                title = {},
                navigationIcon = {
                    if (onBack != null) {
                        IconButton(
                            onClick = onBack,
                        ) {
                            Icon(
                                imageVector = Icons.AutoMirrored.Default.ArrowBack,
                                contentDescription = null
                            )
                        }
                    }
                },
                actions = {
                    IconButton(
                        onClick = onSend,
                        enabled = !state.isLoading
                    ) {
                        Icon(
                            imageVector = Icons.AutoMirrored.TwoTone.Send,
                            contentDescription = null
                        )
                    }
                }
            )
        }
    ) { padding ->
        Box( Modifier.padding(padding).fillMaxSize()) {
            Column(
                modifier = Modifier.widthIn(max = 800.dp).align (Alignment.TopCenter),
                horizontalAlignment = Alignment.CenterHorizontally
            ) {
                val image = state.image

                Box(
                    modifier = Modifier.widthIn(max = 800.dp)
                        .heightIn(max = 500.dp)
                        .aspectRatio(TARGET_WIDTH / TARGET_HEIGHT.toFloat())
                ) {
                    if (image == null) {
                        Card(modifier = Modifier.fillMaxSize().padding(8.dp)) {
                            Column(
                                modifier = Modifier.fillMaxSize(),
                                verticalArrangement = Arrangement.Center
                            ) {
                                Text(
                                    stringResource(Res.string.pick_image_meassage),
                                    modifier = Modifier.align(Alignment.CenterHorizontally)
                                )

                                Button(
                                    onClick = onPickImage,
                                    modifier = Modifier.align(Alignment.CenterHorizontally)
                                ) {
                                    Text(stringResource(Res.string.pick_image))
                                }

                            }
                        }
                    } else {
                        Image(
                            image,
                            contentDescription = null,
                            modifier = Modifier.widthIn(max = 800.dp)
                                .aspectRatio(TARGET_WIDTH / TARGET_HEIGHT.toFloat())
                                .clickable(onClick = onPickImage, enabled = !state.isLoading)
                        )
                    }

                    if (state.isLoading) {
                        Box(
                            Modifier.fillMaxSize()
                                .background(MaterialTheme.colorScheme.surfaceContainer.copy(alpha = 0.5f))
                        ) {
                            CircularWavyProgressIndicator(modifier = Modifier.align(Alignment.Center))
                        }
                    }

                }

                AnimatedVisibility(
                    image != null,
                    modifier = Modifier.fillMaxWidth().padding(8.dp)
                ) {
                    Column {
                        Card(modifier = Modifier.fillMaxWidth().padding(8.dp)) {
                            Column {
                                FlowRow(
                                    horizontalArrangement = Arrangement.SpaceBetween,
                                    modifier = Modifier.fillMaxWidth().padding(8.dp)
                                ) {
                                    greyShades.forEach {
                                        ColorItem(color = it, selected = it == state.color) {
                                            onColorChange(it)
                                        }
                                    }
                                }

                                Text(
                                    stringResource(Res.string.color_palette),
                                    modifier = Modifier.padding(
                                        top = 8.dp,
                                        end = 8.dp,
                                        start = 8.dp
                                    ),
                                    style = MaterialTheme.typography.titleMedium
                                )
                                SingleChoiceSegmentedButtonRow(
                                    Modifier.padding(horizontal = 8.dp).padding(bottom = 8.dp)
                                ) {
                                    DitherPalette.entries.forEachIndexed { index, palette ->
                                        SegmentedButton(
                                            shape = SegmentedButtonDefaults.itemShape(
                                                index = index,
                                                count = DitherPalette.entries.size
                                            ),
                                            onClick = { onPaletteChange(palette) },
                                            selected = palette == state.palette,
                                            label = {
                                                Text(
                                                    when (palette) {
                                                        DitherPalette.Raw -> stringResource(Res.string.raw)
                                                        DitherPalette.Normalized -> stringResource(
                                                            Res.string.normalized
                                                        )
                                                    }
                                                )
                                            }
                                        )

                                    }
                                }

                            }
                        }

                        Card(modifier = Modifier.padding(bottom = 8.dp, end = 8.dp, start = 8.dp)) {
                            Text(
                                stringResource(Res.string.alpha),
                                modifier = Modifier.padding(top = 8.dp, end = 8.dp, start = 8.dp),
                                style = MaterialTheme.typography.titleMedium
                            )
                            Slider(
                                value = state.alpha,
                                modifier = Modifier.padding(
                                    bottom = 8.dp,
                                    end = 8.dp,
                                    start = 8.dp
                                ),
                                onValueChange = onAlphaChange
                            )
                        }

                    }
                }

                Row(
                    modifier = Modifier.fillMaxSize().padding(horizontal = 8.dp),
                    horizontalArrangement = Arrangement.SpaceBetween
                ) {
                    if (PLATFORM == Platform.Android) {
                        Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                            AnimatedVisibility(state.ditheredImage != null) {
                                Button(
                                    onClick = onSaveImage, modifier = Modifier,
                                    colors = ButtonDefaults.buttonColors(containerColor = MaterialTheme.colorScheme.secondary)
                                ) {
                                    Icon(Icons.TwoTone.Save, contentDescription = null)
                                }
                            }
                            Button(
                                onClick = {
                                    showSavedImage = true
                                }, modifier = Modifier,
                                colors = ButtonDefaults.buttonColors(containerColor = MaterialTheme.colorScheme.secondary)
                            ) {
                                Text(stringResource(Res.string.show_queue))
                            }
                        }
                    }

                    if (image != null) {
                        Button(
                            onClick = onDither, modifier = Modifier
                        ) {
                            Text(stringResource(Res.string.preview_dither))
                        }
                    }
                }

            }
        }
    }

    if (showSavedImage) {
        StoredImageDialog {
            showSavedImage = false
        }
    }
}

@Composable
@Preview(showBackground = true)
private fun ColorItem(
    color: Color = Color.Red,
    selected: Boolean = true,
    onClick: () -> Unit = {},
) {
    Surface(
        border = BorderStroke(2.dp, MaterialTheme.colorScheme.onBackground),
        shape = CircleShape,
        color = color,
        onClick = onClick,
        modifier = Modifier
            .size(40.dp)
    ) {
        if (selected) {
            Icon(
                Icons.TwoTone.Check,
                contentDescription = "selected",
                tint = color.getContrastColor(),
                modifier = Modifier.padding(8.dp)
            )
        }
    }
}

@Preview(showBackground = true)
@Composable
private fun ImageScreenEmptyPreview() {
    ImageScreen(onBack = {}, state = ImageState(image = null))
}

@Preview(showBackground = true)
@Composable
private fun ImageScreenPreview() {
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

    ImageScreen(onBack = {}, state = ImageState(image = image))
}

