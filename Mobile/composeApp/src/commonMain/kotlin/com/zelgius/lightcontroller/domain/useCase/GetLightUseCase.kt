package com.zelgius.lightcontroller.domain.useCase

import com.zelgius.lightcontroller.domain.repository.hue.HueRepository
import com.zelgius.lightcontroller.ui.common.ColorInfo
import com.zelgius.lightcontroller.ui.common.Dimming
import com.zelgius.lightcontroller.ui.common.Light
import com.zelgius.lightcontroller.ui.common.MirekSchema
import com.zelgius.lightcontroller.ui.common.TemperatureInfo
import com.zelgius.lightcontroller.ui.common.XYPoint
import org.koin.core.annotation.Factory

@Factory
class GetLightUseCase(
    private val hueRepository: HueRepository
) {

    suspend operator fun invoke(lightId: String): Light? {
        val light = hueRepository.getLight(lightId) ?: return null

        return Light(
            id = light.id,
            color = light.color?.let {
                ColorInfo(
                    xy = XYPoint(it.xy.x, it.xy.y, (light.dimming?.brightness?: 100f) / 100),
                    gamutType = light.color.gamutType
                )
            },
            dimming = light.dimming?.let {
                Dimming(it.brightness)
            },
            colorTemperature = light.colorTemperature?.let {
                TemperatureInfo(
                    mirek = it.mirek,
                    mirekSchema = it.mirekSchema?.let { schema ->
                        MirekSchema(schema.mirek_minimum, schema.mirek_maximum)
                    }
                )
            }
        )
    }
}