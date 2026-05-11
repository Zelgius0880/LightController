package com.zelgius.lightcontroller.utils

import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.luminance
import kotlinx.datetime.LocalDateTime
import kotlin.math.pow

val dateFormat = LocalDateTime.Format {
    year()
    chars("-")
    monthNumber()
    chars("-")
    day()
}

val timeFormat = LocalDateTime.Format {
    hour()
    chars(":")
    minute()
    chars(":")
    second()
}

fun xyToColor(x: Float, y: Float, brightness: Float): Color {
    // 1. Calculate XYZ values
    // If y is 0, the color is essentially black to avoid division by zero
    if (y == 0f) return Color.Black

    val z = 1.0f - x - y
    val X = (brightness / y) * x
    val Z = (brightness / y) * z

    // 2. Convert XYZ to Linear RGB (using sRGB D65 matrix)
    val r = X * 3.2406f - brightness * 1.5372f - Z * 0.4986f
    val g = -X * 0.9689f + brightness * 1.8758f + Z * 0.0415f
    val b = X * 0.0557f - brightness * 0.2040f + Z * 1.0570f

    // 3. Apply Reverse Gamma Correction (Linear RGB to sRGB)
    // Also clamp values between 0 and 1
    fun gammaCorrect(value: Float): Float {
        val clamped = value.coerceIn(0f, 1f)
        return if (clamped <= 0.0031308f) {
            12.92f * clamped
        } else {
            (1.055f * clamped.pow(1.0f / 2.4f) - 0.055f)
        }
    }

    return Color(
        red = gammaCorrect(r),
        green = gammaCorrect(g),
        blue = gammaCorrect(b),
        alpha = 1.0f
    )
}

fun Color.getContrastColor(): Color {
    // 0.5 is the middle ground, but some designers prefer 0.45
    // to give white text a bit more "room" on slightly dimmed colors.
    return if (this.luminance() > 0.5f) Color.Black else Color.White
}

data class ColorComponents(val x: Float, val y: Float, val brightness: Float)
fun Color.colorToXy(): ColorComponents {
    // 1. Get sRGB values (0.0 to 1.0)
    val rSrgb = red
    val gSrgb = green
    val bSrgb = blue

    // 2. Remove Gamma Correction (sRGB to Linear RGB)
    fun toLinear(value: Float): Float {
        return if (value <= 0.04045f) {
            value / 12.92f
        } else {
            ((value + 0.055f) / 1.055f).pow(2.4f)
        }
    }

    val rLine = toLinear(rSrgb)
    val gLine = toLinear(gSrgb)
    val bLine = toLinear(bSrgb)

    // 3. Convert Linear RGB to XYZ (using sRGB D65 matrix)
    val X = rLine * 0.4124f + gLine * 0.3576f + bLine * 0.1805f
    val Y = rLine * 0.2126f + gLine * 0.7152f + bLine * 0.0722f
    val Z = rLine * 0.0193f + gLine * 0.1192f + bLine * 0.9505f

    // 4. Calculate x, y and brightness
    val sum = X + Y + Z

    return if (sum == 0f) {
        // Handle black/dark colors to avoid division by zero
        ColorComponents(0f, 0f, 0f)
    } else {
        ColorComponents(
            x = X / sum,
            y = Y / sum,
            brightness = Y // Y in XYZ space represents luminance/brightness
        )
    }
}