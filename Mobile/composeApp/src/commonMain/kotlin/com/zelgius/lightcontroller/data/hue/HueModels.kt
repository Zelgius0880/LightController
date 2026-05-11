package com.zelgius.lightcontroller.data.hue

import kotlinx.serialization.SerialName
import kotlinx.serialization.Serializable

@Serializable
data class HueResponse<T>(val data: List<T>)

@Serializable
data class LightResource(
    val id: String,
    val dimming: Dimming? = null,
    val color: ColorInfo? = null,
    @SerialName("color_temperature") val colorTemperature: TemperatureInfo? = null,
    val metadata: Metadata
)

@Serializable
data class Metadata(val name: String)
@Serializable
data class Dimming(val brightness: Float)

@Serializable
data class ColorInfo(
    val xy: XYPoint,
    @SerialName("gamut_type") val gamutType: String
)


@Serializable
data class TemperatureInfo(
    val mirek: Int? = null,
    @SerialName("mirek_schema") val mirekSchema: MirekSchema? = null
)

@Serializable
data class MirekSchema(
    val mirek_minimum: Int,
    val mirek_maximum: Int
)

@Serializable
data class XYPoint(val x: Float, val y: Float)

