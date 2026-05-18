package com.zelgius.lightcontroller.domain.repository

import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.Canvas
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.ImageBitmap
import androidx.compose.ui.graphics.ImageBitmapConfig
import androidx.compose.ui.graphics.Paint
import androidx.compose.ui.graphics.PaintingStyle
import androidx.compose.ui.graphics.Path
import androidx.compose.ui.graphics.drawscope.CanvasDrawScope
import androidx.compose.ui.text.AnnotatedString
import androidx.compose.ui.text.TextLayoutResult
import androidx.compose.ui.text.TextMeasurer
import androidx.compose.ui.text.TextStyle
import androidx.compose.ui.text.drawText
import androidx.compose.ui.unit.Density
import androidx.compose.ui.unit.LayoutDirection
import androidx.compose.ui.unit.sp
import com.tweener.kmpkit.kotlinextensions.isToday
import com.zelgius.lightcontroller.data.ImageConfig
import com.zelgius.lightcontroller.utils.drawImageBitmap
import kotlinx.datetime.LocalDate
import kotlinx.datetime.TimeZone
import kotlinx.datetime.format
import kotlinx.datetime.format.DayOfWeekNames
import kotlinx.datetime.toLocalDateTime
import lightcontroller.composeapp.generated.resources.Res
import org.jetbrains.compose.resources.decodeToImageBitmap
import kotlin.math.PI
import kotlin.math.min
import kotlin.math.sin
import kotlin.time.Clock
import kotlin.time.Duration.Companion.days

class ImageRenderer(
    private val textMeasurer: TextMeasurer,
    private val density: Density,
) {
    var image = ImageBitmap(
        ImageConfig.TARGET_WIDTH,
        ImageConfig.TARGET_HEIGHT,
        config = ImageBitmapConfig.Argb8888
    )
    private val paint = Paint()

    private var canvas: Canvas = Canvas(image)

    private val savedImage = ImageBitmap(
        ImageConfig.TARGET_WIDTH,
        ImageConfig.TARGET_HEIGHT,
        config = ImageBitmapConfig.Argb8888
    )


    fun save() {
        Canvas(savedImage).apply {
            this.drawImage(image, Offset.Zero, paint)
        }
    }

    fun restore() {
        image = ImageBitmap(
            ImageConfig.TARGET_WIDTH,
            ImageConfig.TARGET_HEIGHT,
            config = ImageBitmapConfig.Argb8888
        )
        canvas = Canvas(image).apply {
            drawImage(savedImage, Offset.Zero, paint)
        }

    }

    fun setImage(imageBitmap: ImageBitmap) = canvas.apply {
        image = ImageBitmap(
            ImageConfig.TARGET_WIDTH,
            ImageConfig.TARGET_HEIGHT,
            config = ImageBitmapConfig.Argb8888
        )
        canvas = Canvas(image).apply {
            drawImageBitmap(
                imageBitmap,
                Offset.Zero,
                size = Size(
                    ImageConfig.TARGET_WIDTH.toFloat(),
                    ImageConfig.TARGET_HEIGHT.toFloat(),
                )
            )
        }
    }

    fun drawOverlay(color: Color = Color.Black, alpha: Float = 0.5f) = canvas.apply {
        paint.color = color.copy(alpha = alpha)

        drawRoundRect(
            left = 890f,
            top = 10f,
            right = 890 + 700f,
            bottom = 10 + 150f,
            radiusX = 30f,
            radiusY = 30f,
            paint = paint
        )

        drawRoundRect(
            10f,
            10f,
            10 + 320f,
            10 + 670f,
            radiusX = 30f,
            radiusY = 30f,
            paint = paint
        )
    }

    private val valueStyle = TextStyle(color = Color.White, fontSize = 16.sp)
    private val labelStyle = TextStyle(color = Color.White, fontSize = 12.sp)
    private val daysStyle = TextStyle(color = Color.White, fontSize = 14.sp)
    suspend fun drawPreviews() = canvas.apply {
        val tempMainValues = List(48) { i ->
            21.0 + 1.5 * sin((i - 8) * PI / 12.0)
        }
        drawLineChart(tempMainValues, "Inside", "°C", 20f, 20f, 300f, 150f, Color.Red)

        val tempModuleValues = List(48) { i ->
            12.0 + 6.0 * sin((i - 10) * PI / 12.0)
        }
        drawLineChart(tempModuleValues, "Outside", "°C", 20f, 180f, 300f, 150f, Color.Red)

        val humidityMainValues = List(48) { i ->
            20.0 + (i * 0.2)
        }
        drawBarChart(humidityMainValues, "Humidity", "%", 20f, 340f, 300f, 150f, Color.Blue)

        val pressureMainValues = List(48) { i ->
            1012.0 + (i * 0.2)
        }
        drawBarChart(pressureMainValues, "Pressure", "hPa", 20f, 520f, 300f, 150f, Color.White)


        val now = Clock.System.now()
        val weatherIds = listOf(0, 1, 50, 3, 0, 80, 40) // Simplified to match your renderer logic

        val forecast = List(7) { i ->
            ForecastData(
                dt = (now + i.days).toLocalDateTime(TimeZone.currentSystemDefault()).date,
                tempMin = 5.0 + i,
                tempMax = 15.0 + i,
                pop = if (i == 2) 0.8 else 0.1,
                weatherId = weatherIds[i]
            )
        }
        drawWeatherForecast(forecast, 870f, 20f, 700f, 150f)
    }


    private fun drawLineChart(
        data: List<Double>,
        title: String,
        units: String,
        x: Float, y: Float, w: Float, h: Float,
        lineColor: Color
    ) {
        val minVal = (data.minOrNull() ?: 0.0) - 1.0
        val maxVal = (data.maxOrNull() ?: 0.0) + 1.0

        val chartX = x + 45f
        val chartY = y + 45f
        val chartW = w - 65f
        val chartH = h - 70f

        val paint = Paint().apply {
            this.color = lineColor
            strokeWidth = 3f
            style = PaintingStyle.Stroke
        }

        val path = Path()
        data.forEachIndexed { i, value ->
            val px = chartX + (i * chartW) / (data.size - 1)
            val py = chartY + chartH - ((value - minVal) / (maxVal - minVal) * chartH).toFloat()
            if (i == 0) path.moveTo(px, py) else path.lineTo(px, py)
        }

        canvas.drawPath(path, paint)

        // Draw Axis
        paint.color = Color.White
        canvas.drawLine(
            Offset(
                chartX, chartY
            ), Offset(
                chartX, chartY + chartH
            ), paint
        )

        canvas.drawLine(
            Offset(
                chartX, chartY + chartH
            ), Offset(
                chartX + chartW, chartY + chartH
            ), paint
        )


        val lastVal = data.lastOrNull() ?: 0.0
        textMeasurer.measure(
            text = AnnotatedString("${lastVal.toInt()} $units"),
            style = valueStyle
        ).let {
            canvas.drawText(
                textLayout = it,
                x = x + (w / 2) - (it.size.width / 2),
                y = y + 5f
            )
        }


        val maxLabel = textMeasurer.measure("${maxVal.toInt()}", style = labelStyle)
        val minLabel = textMeasurer.measure("${minVal.toInt()}", style = labelStyle)
        val startLabel = textMeasurer.measure("-24h", style = labelStyle)
        val endLabel = textMeasurer.measure("0h", style = labelStyle)

        canvas.drawText(
            textLayout = maxLabel,
            x = chartX - maxLabel.size.width - 5f,
            y = chartY - (maxLabel.size.height / 2f)
        )

        // Min Label: Bottom of chart, 5px to the left of chartX
        canvas.drawText(
            textLayout = minLabel,
            x = chartX - minLabel.size.width - 5f,
            y = chartY + chartH - (minLabel.size.height / 2f)
        )
        // "-24h": Centered under the start of the X-axis
        canvas.drawText(
            textLayout = startLabel,
            x = chartX - (startLabel.size.width / 2f),
            y = chartY + chartH + 5f
        )


        // "0h": Centered under the end of the X-axis
        canvas.drawText(
            textLayout = endLabel,
            x = chartX + chartW - (endLabel.size.width / 2f),
            y = chartY + chartH + 5f
        )
    }

    private fun drawBarChart(
        data: List<Double>,
        title: String,
        units: String,
        x: Float, y: Float, w: Float, h: Float,
        barColor: Color
    ) {
        val minVal = (data.minOrNull() ?: 0.0) - 2.0
        val maxVal = (data.maxOrNull() ?: 0.0) + 2.0

        val chartX = x + 55f
        val chartY = y + 45f
        val chartW = w - 75f
        val chartH = h - 70f

        val barGap = 2f
        val barWidth = (chartW / data.size) - barGap

        data.forEachIndexed { i, value ->
            val bx = chartX + (i * chartW) / data.size + barGap / 2
            val by = (chartY + chartH - ((value - minVal) / (maxVal - minVal) * chartH)).toFloat()

            canvas.drawRect(
                bx, by, bx + barWidth.coerceAtLeast(1f), chartY + chartH,
                Paint().apply { color = barColor }
            )
        }

        // Draw Axis
        paint.color = Color.White
        canvas.drawLine(
            Offset(
                chartX, chartY
            ), Offset(
                chartX, chartY + chartH
            ), paint
        )

        canvas.drawLine(
            Offset(
                chartX, chartY + chartH
            ), Offset(
                chartX + chartW, chartY + chartH
            ), paint
        )


        val lastVal = data.lastOrNull() ?: 0.0
        textMeasurer.measure(
            text = AnnotatedString("${lastVal.toInt()} $units"),
            style = valueStyle
        ).let {
            canvas.drawText(
                textLayout = it,
                x = x + (w / 2) - (it.size.width / 2),
                y = y + 5f
            )
        }


        val maxLabel = textMeasurer.measure("${maxVal.toInt()}", style = labelStyle)
        val minLabel = textMeasurer.measure("${minVal.toInt()}", style = labelStyle)
        val startLabel = textMeasurer.measure("-24h", style = labelStyle)
        val endLabel = textMeasurer.measure("0h", style = labelStyle)

        canvas.drawText(
            textLayout = maxLabel,
            x = chartX - maxLabel.size.width - 5f,
            y = chartY - (maxLabel.size.height / 2f)
        )

        // Min Label: Bottom of chart, 5px to the left of chartX
        canvas.drawText(
            textLayout = minLabel,
            x = chartX - minLabel.size.width - 5f,
            y = chartY + chartH - (minLabel.size.height / 2f)
        )
        // "-24h": Centered under the start of the X-axis
        canvas.drawText(
            textLayout = startLabel,
            x = chartX - (startLabel.size.width / 2f),
            y = chartY + chartH + 5f
        )


        // "0h": Centered under the end of the X-axis
        canvas.drawText(
            textLayout = endLabel,
            x = chartX + chartW - (endLabel.size.width / 2f),
            y = chartY + chartH + 5f
        )
    }

    private suspend fun drawWeatherForecast(
        forecast: List<ForecastData>,
        x: Float, y: Float, w: Float, h: Float
    ) {

        val count = min(forecast.size, 7)
        val padding = 5f
        val itemWidth = (w - 2 * padding) / count
        val iconSize = 72

        val format = LocalDate.Format {
            dayOfWeek(DayOfWeekNames.ENGLISH_ABBREVIATED)
        }

        forecast.take(count).forEachIndexed { i, item ->
            val ix = x + padding + i * itemWidth
            val iy = y + 10

            val maxMinLabel =
                textMeasurer.measure(
                    "${item.tempMax.toInt()}/${item.tempMin.toInt()}",
                    style = labelStyle
                )
            val dayLabel = textMeasurer.measure(
                if (item.dt.isToday()) "Today" else item.dt.format(format),
                style = daysStyle
            )

            canvas.drawText(
                textLayout = dayLabel,
                x = ix + itemWidth / 2 + 20 - dayLabel.size.width / 2,
                y = iy + 30 - dayLabel.size.height
            )

            canvas.drawText(
                textLayout = maxMinLabel,
                x = ix + itemWidth / 2 - maxMinLabel.size.width / 2,
                y = y + h - 5 - maxMinLabel.size.height
            )

            val iconX = ix + (itemWidth - iconSize) / 2
            val iconY = iy + 35
            canvas.drawImageBitmap(
                getWeatherIcon(item.weatherId),
                Offset(iconX, iconY),
                Size(iconSize.toFloat(), iconSize.toFloat())
            )
        }
    }

    fun Canvas.drawText(x: Float, y: Float, textLayout: TextLayoutResult) {
        CanvasDrawScope().draw(
            density = density,
            layoutDirection = LayoutDirection.Ltr,
            canvas = this,
            size = Size(image.width.toFloat(), image.height.toFloat())
        ) {

            drawText(
                textLayoutResult = textLayout,
                topLeft = Offset(
                    x,
                    y
                )
            )
        }
    }

    private suspend fun getWeatherIcon(iconCode: Int): ImageBitmap {
        val path = if (iconCode == 0) { // Clear sky
            "clear-day"
        } else if (iconCode == 1 || iconCode == 2) { // Mainly clear, partly cloudy
            "cloudy-3-day"
        } else if (iconCode == 3) { // Overcast
            "cloudy"
        } else if (iconCode == 40 || iconCode == 49) { // Fog and depositing rime fog
            "fog-day"
        } else if (iconCode in 50..59) { // Drizzle and Rain
            "rainy-3-day"
        } else if (iconCode in 60..65) { // Drizzle and Rain
            "rainy-3"
        } else if (iconCode in 66..69) { // Drizzle and Rain
            "rain-and-snow-mix"
        } else if (iconCode in 71..79) { // Snow fall
            "snowy-3"
        } else if (iconCode in 80..82 || iconCode == 91 || iconCode == 92) { // Rain showers
            "rainy-3"
        } else if (iconCode == 85 || iconCode == 86) { // Snow showers
            "snowy-3"
        } else if (iconCode in 87..90 || iconCode == 93 || iconCode == 94) { // Snow showers
            "rain-and-snow-mix"
        } else if (iconCode >= 95) { // Thunderstorm
            "thunderstorms"
        } else {
            "cloudy"
        }
        val bytes = Res.readBytes("drawable/$path.png")
        return bytes.decodeToImageBitmap()
    }

}

data class ForecastData(
    val dt: LocalDate,
    val pop: Double,
    val tempMax: Double,
    val tempMin: Double,
    val weatherId: Int
)