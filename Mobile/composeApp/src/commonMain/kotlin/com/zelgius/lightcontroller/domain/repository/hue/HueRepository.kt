package com.zelgius.lightcontroller.domain.repository.hue

import com.zelgius.lightcontroller.HUE_BRIDGE_IP
import com.zelgius.lightcontroller.data.hue.HueLightService
import com.zelgius.lightcontroller.data.hue.LightResource
import com.zelgius.lightcontroller.domain.repository.settings.SettingsRepository
import kotlinx.coroutines.flow.filterNotNull
import kotlinx.coroutines.flow.first
import org.koin.core.annotation.Singleton

@Singleton
class HueRepository(
    private val settingsRepository: SettingsRepository,
    private val hueLightService: HueLightService,
) {
    suspend fun getLight(lightId: String): LightResource? {
        val apiKey = settingsRepository.hueApiKey.filterNotNull().first()
        return  hueLightService.getLight(HUE_BRIDGE_IP, apiKey, lightId)
    }

    suspend fun getLights(): List<LightResource>? {
        val apiKey = settingsRepository.hueApiKey.filterNotNull().first()
        return  hueLightService.getLights(HUE_BRIDGE_IP, apiKey)
    }
}