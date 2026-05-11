package com.zelgius.lightcontroller.domain.repository.web

import com.zelgius.lightcontroller.data.ServerService
import com.zelgius.lightcontroller.domain.repository.settings.SettingsRepository
import com.zelgius.lightcontroller.selfHosted
import io.ktor.client.statement.HttpResponse
import io.ktor.http.HttpStatusCode
import kotlinx.coroutines.flow.first
import org.koin.core.annotation.Provided
import org.koin.core.annotation.Single

@Single
class ServerRepository (
    @Provided private val serverService: ServerService,
    @Provided private val settingsRepository: SettingsRepository
) {
    private suspend fun ip(): String {
        if(selfHosted) return "http://127.0.0.1"
        return settingsRepository.ipFlow.first()?.let {
            if(!it.startsWith("http")) "http://$it"
            else it
        } ?: throw IpNotSetException()
    }


    // --- System Endpoints ---

    suspend fun getStatus(): StatusResponse = serverService.getStatus(ip())


    suspend fun triggerRender(): Boolean {
        val response = serverService.triggerRender(ip())
        return response.status == HttpStatusCode.OK
    }

    // --- Database & Files ---

    suspend fun exportDatabase(): ByteArray = serverService.exportDatabase(ip())

    suspend fun importDatabase(bytes: ByteArray): HttpResponse {
        return serverService.importDatabase(ip(), bytes)
    }

    suspend fun uploadImage(bytes: ByteArray): HttpResponse {
        return serverService.uploadImage(ip(), bytes)
    }

    // --- Group Management ---

    suspend fun getAllGroups(): List<Group> = serverService.getAllGroups(ip())

    suspend fun upsertGroup(group: Group): HttpResponse {
        return serverService.upsertGroup(ip(), group)
    }

    suspend fun deleteGroup(id: Long): HttpResponse {
        return serverService.deleteGroup(ip(), id)
    }

    // --- Light Management ---

    suspend fun getLightsByGroup(groupId: Long): List<Light> =
       serverService.getLightsByGroup(ip(), groupId)

    suspend fun upsertLight(light: Light): HttpResponse {
        return serverService.upsertLight(ip(), light)
    }

    suspend fun deleteLight(uid: String, groupId: Long): HttpResponse {
        return serverService.deleteLight(ip(), uid, groupId)
    }

    // --- Switch Management ---

    suspend fun getSwitchesByGroup(groupId: Long): List<Switch> =
       serverService.getSwitchesByGroup(ip(), groupId)

    suspend fun upsertSwitch(switch: Switch): HttpResponse {
        return serverService.upsertSwitch(ip(), switch)
    }

    suspend fun deleteSwitch(uid: String): HttpResponse {
        return serverService.deleteSwitch(ip(), uid)
    }

    suspend fun checkSwitch(uid: String): SwitchCheckResponse =
       serverService.checkSwitch(ip(), uid)

    // --- Switch Attribution ---

    suspend fun toggleAttributionMode(enabled: Boolean): HttpResponse {
       return serverService.toggleAttributionMode(ip(), enabled)
    }

    suspend fun getAttributionData(): AttributionDataResponse =
       serverService.getAttributionData(ip())
}

class IpNotSetException : IllegalStateException()