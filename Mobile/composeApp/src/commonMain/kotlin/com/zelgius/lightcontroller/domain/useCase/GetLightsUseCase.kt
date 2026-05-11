package com.zelgius.lightcontroller.domain.useCase

import com.zelgius.lightcontroller.data.hue.LightResource
import com.zelgius.lightcontroller.domain.repository.hue.HueRepository
import com.zelgius.lightcontroller.domain.repository.web.Group
import com.zelgius.lightcontroller.domain.repository.web.Light
import com.zelgius.lightcontroller.domain.repository.web.ServerRepository
import org.koin.core.annotation.Factory

@Factory
class GetLightsUseCase(
    private val hueRepository: HueRepository,
) {

    suspend operator fun invoke(group: Group, groupLights: List<Light>): List<LightResource> {
        val lights = hueRepository.getLights() ?: return emptyList()
        val addedLights = groupLights.map { it.uid }

        return lights.filter { it.id !in addedLights }.sortedBy { it.metadata.name }
    }
}