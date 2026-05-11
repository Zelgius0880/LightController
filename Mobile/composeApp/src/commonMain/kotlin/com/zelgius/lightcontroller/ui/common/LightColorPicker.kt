package com.zelgius.lightcontroller.ui.common

import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Lightbulb
import androidx.compose.material3.Icon
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.SegmentedButton
import androidx.compose.material3.SegmentedButtonDefaults
import androidx.compose.material3.SingleChoiceSegmentedButtonRow
import androidx.compose.material3.Slider
import androidx.compose.material3.SliderDefaults
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Paint
import androidx.compose.ui.graphics.PaintingStyle
import androidx.compose.ui.graphics.lerp
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import com.github.skydoves.colorpicker.compose.ImageColorPicker
import com.github.skydoves.colorpicker.compose.rememberColorPickerController
import com.zelgius.lightcontroller.utils.xyToColor
import lightcontroller.composeapp.generated.resources.Res
import lightcontroller.composeapp.generated.resources.brightness
import lightcontroller.composeapp.generated.resources.color
import lightcontroller.composeapp.generated.resources.color_palette
import lightcontroller.composeapp.generated.resources.color_temperature
import lightcontroller.composeapp.generated.resources.cool
import lightcontroller.composeapp.generated.resources.gamut_A_picker
import lightcontroller.composeapp.generated.resources.gamut_B_picker
import lightcontroller.composeapp.generated.resources.gamut_C_picker
import lightcontroller.composeapp.generated.resources.mireds
import lightcontroller.composeapp.generated.resources.temperature
import lightcontroller.composeapp.generated.resources.warm
import lightcontroller.composeapp.generated.resources.white_ambiance
import org.jetbrains.compose.resources.imageResource
import org.jetbrains.compose.resources.stringResource
import kotlin.math.pow

data class Light(
    val id: String,
    val dimming: Dimming? = null,
    val color: ColorInfo? = null,
    val colorTemperature: TemperatureInfo? = null
)

data class Dimming(val brightness: Float)

data class ColorInfo(
    val xy: XYPoint,
    val gamutType: String
)

data class TemperatureInfo(
    val mirek: Int? = null,
    val mirekSchema: MirekSchema? = null
)

data class MirekSchema(
    val mirekMinimum: Int,
    val mirekMaximum: Int
)

data class XYPoint(val x: Float, val y: Float, val brightness: Float)

val customWheelPaint = Paint().apply {
    strokeWidth = 4f
    style = PaintingStyle.Stroke
    color = Color.Black
}

@Composable
fun HueGamutPicker(
    gamut: String,
    color: Color,
    onColorSelected: (XYPoint) -> Unit,
    modifier: Modifier = Modifier
) {

    val controller = rememberColorPickerController().apply {
        wheelPaint = customWheelPaint
    }

    var color by remember {
        controller.selectByColor(color, true)
        mutableStateOf(color)
    }


    Box(
        modifier = modifier
            .size(200.dp)
    ) {
        Box(
            modifier = Modifier
                .align(Alignment.Center)
                .clip(CircleShape)
        ) {
            ImageColorPicker(
                modifier = Modifier.fillMaxSize(),
                controller = controller,
                paletteImageBitmap = imageResource(
                    when (gamut.uppercase()) {
                        "C" -> Res.drawable.gamut_C_picker
                        "B" -> Res.drawable.gamut_B_picker
                        else -> Res.drawable.gamut_A_picker
                    }
                ),
                onColorChanged = {
                    if (it.color != Color.Transparent && it.color != Color.Black && it.fromUser) {
                        onColorSelected(it.color.toXy())
                        color = it.color
                    }
                }
            )
        }

        Box(
            modifier = Modifier.align(Alignment.BottomEnd).size(32.dp)
                .clip(RoundedCornerShape(8.dp))
                .border(
                    1.dp,
                    color = MaterialTheme.colorScheme.onSurface,
                    shape = RoundedCornerShape(8.dp)
                )
                .background(color)

        )
    }
}

fun Color.toXy(): XYPoint {
    val red: Float =
        if (red > 0.04045f) ((red + 0.055f) / (1.0f + 0.055f)).pow(2.4f) else (red / 12.92f)
    val green: Float =
        if (green > 0.04045f) ((green + 0.055f) / (1.0f + 0.055f)).pow(2.4f) else (green / 12.92f)
    val blue: Float =
        if (blue > 0.04045f) ((blue + 0.055f) / (1.0f + 0.055f)).pow(2.4f) else (blue / 12.92f)

    val X = (red * 0.4124 + green * 0.3576 + blue * 0.1805).toFloat()
    val Y = (red * 0.2126 + green * 0.7152 + blue * 0.0722).toFloat()
    val Z = (red * 0.0193 + green * 0.1192 + blue * 0.9505).toFloat()

    val x = X / (X + Y + Z)
    val y = Y / (X + Y + Z)
    val brightness = Y * 100f

    return XYPoint(x, y, brightness)
}

val mirekGradient = listOf(
    Color(0xFFFDF4FF),
    Color(0xFFFFF4EA),
    Color(0xFFFFD2A0),
    Color(0xFFFFBD73),
    Color(0xFFFF9721),
    Color(0xFFFF8912),
)
val mirekRange = (153..500)

/**
 * Get the color from Mirek
 */
fun Int.toColor(): Color {
    // 1. Normalize the value to a 0.0 - 1.0 range
    val fraction =
        ((this.toFloat() - mirekRange.first) / (mirekRange.last - mirekRange.first)).coerceIn(
            0f,
            1f
        )

    // 2. Determine where in the list the fraction falls
    val lastIndex = mirekGradient.lastIndex
    val scaledFraction = fraction * lastIndex

    val startIndex = scaledFraction.toInt().coerceAtMost(lastIndex)
    val endIndex = (startIndex + 1).coerceAtMost(lastIndex)

    // 3. Calculate how far we are between these two specific colors
    val remainder = scaledFraction - startIndex

    // 4. Linearly interpolate between the two colors
    return lerp(mirekGradient[startIndex], mirekGradient[endIndex], remainder)
}

fun XYPoint.toRgb(): Color {
    val x = x // the given x value
    val y = y // the given y value
    val z = 1.0f - x - y
    val Y: Float = brightness / 100f // The given brightness value
    val X = (Y / y) * x
    val Z = (Y / y) * z

    var r = X * 1.656492f - Y * 0.354851f - Z * 0.255038f
    var g = -X * 0.707196f + Y * 1.655397f + Z * 0.036152f
    var b = X * 0.051713f - Y * 0.121364f + Z * 1.011530f

    r = if (r <= 0.0031308f) 12.92f * r else (1.0f + 0.055f) * r.pow(1.0f / 2.4f) - 0.055f
    g = if (g <= 0.0031308f) 12.92f * g else (1.0f + 0.055f) * g.pow(1.0f / 2.4f) - 0.055f
    b = if (b <= 0.0031308f) 12.92f * b else (1.0f + 0.055f) * b.pow(1.0f / 2.4f) - 0.055f

    return Color(r, g, b)
}

enum class PickerMode { COLOR, TEMPERATURE }

@Composable
fun MirekPicker(
    schema: MirekSchema,
    currentMirek: Int,
    onMirekChanged: (Int) -> Unit,
    modifier: Modifier = Modifier
) {
    Column(modifier = modifier.fillMaxWidth()) {
        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.SpaceBetween,
            verticalAlignment = Alignment.CenterVertically
        ) {
            Text(
                stringResource(Res.string.color_temperature),
                style = MaterialTheme.typography.labelLarge
            )
            Text(
                text = "$currentMirek ${stringResource(Res.string.mireds)}",
                style = MaterialTheme.typography.bodySmall,
                color = MaterialTheme.colorScheme.onSurfaceVariant
            )
        }

        Spacer(modifier = Modifier.height(12.dp))

        Box(contentAlignment = Alignment.Center) {
            // Visual Gradient Background
            Box(
                modifier = Modifier
                    .fillMaxWidth()
                    .height(12.dp)
                    .clip(RoundedCornerShape(4.dp))
                    .background(
                        brush = Brush.linearGradient(
                            colors = buildList {
                                add(schema.mirekMinimum.toColor())
                                addAll(mirekGradient.slice(1 until mirekGradient.lastIndex))
                                add(schema.mirekMaximum.toColor())
                            }
                        )
                    )
            )

            Slider(
                value = currentMirek.toFloat(),
                onValueChange = { onMirekChanged(it.toInt()) },
                valueRange = schema.mirekMinimum.toFloat()..schema.mirekMaximum.toFloat(),
                colors = SliderDefaults.colors(
                    activeTrackColor = Color.Transparent, // Let the gradient show through
                    inactiveTrackColor = Color.Transparent
                ),
                modifier = Modifier.fillMaxWidth()
            )
        }

        Row(modifier = Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.SpaceBetween) {
            Text(stringResource(Res.string.cool), style = MaterialTheme.typography.bodySmall)
            Text(stringResource(Res.string.warm), style = MaterialTheme.typography.bodySmall)
        }
    }
}

@Composable
fun BrightnessSlider(
    value: Float,
    onValueChange: (Float) -> Unit,
    modifier: Modifier = Modifier
) {
    Column(modifier = modifier.fillMaxWidth()) {
        Text(stringResource(Res.string.brightness), style = MaterialTheme.typography.labelLarge)

        Row(
            modifier = Modifier.fillMaxWidth(),
            verticalAlignment = Alignment.CenterVertically
        ) {
            // Low brightness icon
            Icon(
                imageVector = Icons.Default.Lightbulb,
                contentDescription = null,
                modifier = Modifier.size(18.dp),
                tint = MaterialTheme.colorScheme.onSurfaceVariant.copy(alpha = 0.6f)
            )

            Slider(
                value = value,
                onValueChange = onValueChange,
                valueRange = 0f..100f,
                modifier = Modifier.weight(1f).padding(horizontal = 8.dp)
            )

            // High brightness icon
            Icon(
                imageVector = Icons.Default.Lightbulb,
                contentDescription = null,
                modifier = Modifier.size(24.dp),
                tint = MaterialTheme.colorScheme.primary
            )
        }

        Text(
            text = "${value.toInt()}%",
            modifier = Modifier.align(Alignment.CenterHorizontally),
            style = MaterialTheme.typography.bodySmall
        )
    }
}

@Composable
fun HueLightPicker(
    light: Light,
    modifier: Modifier = Modifier,
    onXYUpdate: (XYPoint) -> Unit = {},
    onMirekUpdate: (mirek: Int) -> Unit = {},
    onBrightnessUpdate: (brightness: Float) -> Unit = {},
) {
    // Determine capabilities
    val supportsColor = light.color?.gamutType != null
    val supportsMirek = light.colorTemperature?.mirekSchema != null


    // Internal UI state for the toggle
    var activeMode by remember {
        mutableStateOf(if (supportsColor && light.colorTemperature?.mirek == null) PickerMode.COLOR else PickerMode.TEMPERATURE)
    }

    // Working state initialized from the light parameter
    var currentBrightness by remember { mutableStateOf(light.dimming?.brightness ?: 100f) }
    var currentXy by remember {
        mutableStateOf(
            light.color?.xy?.copy(brightness = currentBrightness) ?: XYPoint(0.4f, 0.4f, 40f)
        )
    }
    var currentMirek by remember { mutableStateOf(light.colorTemperature?.mirek ?: 153) }

    LaunchedEffect(light) {
        activeMode =
            if (supportsColor && light.colorTemperature?.mirek == null) PickerMode.COLOR else PickerMode.TEMPERATURE

        currentBrightness = light.dimming?.brightness ?: 100f
        currentXy =
            light.color?.xy?.copy(brightness = currentBrightness) ?: XYPoint(0.4f, 0.4f, 40f)
        currentMirek = light.colorTemperature?.mirek ?: 153
    }

    Column(modifier = modifier.padding(16.dp), horizontalAlignment = Alignment.CenterHorizontally) {
        // 1. Mode Toggle (Disabled if only one mode is managed)
        if (supportsColor && supportsMirek) {
            SingleChoiceSegmentedButtonRow(modifier = Modifier.fillMaxWidth()) {
                SegmentedButton(
                    selected = activeMode == PickerMode.COLOR,
                    onClick = { activeMode = PickerMode.COLOR },
                    shape = SegmentedButtonDefaults.itemShape(index = 0, count = 2)
                ) { Text(stringResource(Res.string.color)) }
                SegmentedButton(
                    selected = activeMode == PickerMode.TEMPERATURE,
                    onClick = { activeMode = PickerMode.TEMPERATURE },
                    shape = SegmentedButtonDefaults.itemShape(index = 1, count = 2)
                ) { Text(stringResource(Res.string.temperature)) }
            }
        } else {
            // Header for single-mode lights
            Text(
                text = if (supportsColor) stringResource(Res.string.color_palette) else stringResource(
                    Res.string.white_ambiance
                ),
                style = MaterialTheme.typography.titleMedium
            )
        }

        Spacer(modifier = Modifier.height(24.dp))

        // 2. Active Picker
        when (activeMode) {
            PickerMode.COLOR -> {
                light.color?.gamutType?.let { g ->
                    HueGamutPicker(
                        gamut = g,
                        color = currentXy.toRgb(),
                        onColorSelected = {
                            if (activeMode != PickerMode.COLOR) return@HueGamutPicker
                            currentXy = it.copy(brightness = currentBrightness)
                            onXYUpdate(currentXy)
                        },
                        modifier = Modifier.size(300.dp)
                    )
                }
            }

            PickerMode.TEMPERATURE -> {
                light.colorTemperature?.mirekSchema?.let { schema ->
                    MirekPicker(
                        schema = schema,
                        currentMirek = currentMirek,
                        onMirekChanged = {
                            if (activeMode != PickerMode.TEMPERATURE) return@MirekPicker
                            currentMirek = it
                            onMirekUpdate(it)
                        }
                    )
                }
            }
        }

        Spacer(modifier = Modifier.height(32.dp))

        // 3. Brightness (Always available for dimmable lights)
        BrightnessSlider(
            value = currentBrightness,
            onValueChange = {
                currentBrightness = it
                onBrightnessUpdate(it)
            }
        )
    }
}

@Preview(showBackground = true, name = "Mirek - Cool State")
@Composable
fun PreviewMirekCool() {
    MaterialTheme {
        MirekPicker(
            schema = MirekSchema(153, 500),
            currentMirek = 153,
            onMirekChanged = {},
            modifier = Modifier.padding(16.dp)
        )
    }
}

@Preview(showBackground = true, name = "Mirek - Warm State")
@Composable
fun PreviewMirekWarm() {
    MaterialTheme {
        MirekPicker(
            schema = MirekSchema(153, 500),
            currentMirek = 450,
            onMirekChanged = {},
            modifier = Modifier.padding(16.dp)
        )
    }
}

@Preview(showBackground = true, name = "Brightness - Mid Level")
@Composable
fun PreviewBrightness() {
    MaterialTheme {
        BrightnessSlider(
            value = 50f,
            onValueChange = {},
            modifier = Modifier.padding(16.dp)
        )
    }
}

@Preview(showBackground = true, name = "Full Controller - Interactive")
@Composable
fun PreviewFullHuePicker() {
    // Mocking a high-end bulb (like Hue Play or Color Ambiance)
    val mockLight = Light(
        id = "mock-123",
        dimming = Dimming(brightness = 75f),
        color = ColorInfo(
            xy = Color.Red.toXy(),
            gamutType = "C",
        ),
        colorTemperature = TemperatureInfo(
            mirek = 300,
            mirekSchema = MirekSchema(153, 500)
        )
    )

    MaterialTheme {
        Surface(
            modifier = Modifier.fillMaxWidth(),
            color = MaterialTheme.colorScheme.surface
        ) {
            // We wrap it in a Column to simulate the Dialog content area
            Column(modifier = Modifier.padding(8.dp)) {
                HueLightPicker(
                    light = mockLight,
                )
            }
        }
    }
}

@Preview(showBackground = true, name = "White Ambiance Only (No Toggle)")
@Composable
fun PreviewWhiteAmbianceOnly() {
    val mockLight = Light(
        id = "mock-white",
        dimming = Dimming(brightness = 100f),
        colorTemperature = TemperatureInfo(
            mirek = 250,
            mirekSchema = MirekSchema(153, 454)
        )
        // Note: No color/gamut property here
    )

    MaterialTheme {
        HueLightPicker(
            light = mockLight,
        )
    }
}