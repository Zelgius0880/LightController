package com.zelgius.lightcontroller.data

import com.zelgius.lightcontroller.domain.repository.web.AttributionDataResponse
import com.zelgius.lightcontroller.domain.repository.web.Group
import com.zelgius.lightcontroller.domain.repository.web.Light
import com.zelgius.lightcontroller.domain.repository.web.StatusResponse
import com.zelgius.lightcontroller.domain.repository.web.Switch
import com.zelgius.lightcontroller.domain.repository.web.SwitchCheckResponse
import io.ktor.client.HttpClient
import io.ktor.client.call.body
import io.ktor.client.request.delete
import io.ktor.client.request.forms.formData
import io.ktor.client.request.forms.submitFormWithBinaryData
import io.ktor.client.request.get
import io.ktor.client.request.parameter
import io.ktor.client.request.post
import io.ktor.client.request.setBody
import io.ktor.client.statement.HttpResponse
import io.ktor.http.ContentType
import io.ktor.http.Headers
import io.ktor.http.HttpHeaders
import io.ktor.http.contentType
import org.koin.core.annotation.Named
import org.koin.core.annotation.Singleton

@Singleton
class ServerService(
    @Named("REST") private val client: HttpClient,
) {

    suspend fun getStatus(baseUrl: String): StatusResponse = client.get("$baseUrl/status").body()

    suspend fun triggerRender(baseUrl: String) = client.get("$baseUrl/render")

    // --- Database & Files ---

    suspend fun exportDatabase(baseUrl: String): ByteArray =
        client.get("$baseUrl/export_db").body()

    suspend fun importDatabase(baseUrl: String, bytes: ByteArray): HttpResponse {
        client.submitFormWithBinaryData(
            url = "$baseUrl/import_db",
            formData = formData{
                append("db", bytes, Headers.build {
                    append(HttpHeaders.ContentType, "application/octet-stream")
                })
            }
        )

        return client.post("$baseUrl/import_db") {
            setBody(bytes)
            contentType(ContentType.Application.OctetStream)
        }
    }

    suspend fun uploadImage(baseUrl: String, bytes: ByteArray): HttpResponse {
        return client.post("$baseUrl/upload_image") {
            setBody(bytes)
            contentType(ContentType.Application.OctetStream)
        }
    }

    // --- Group Management ---

    suspend fun getAllGroups(baseUrl: String): List<Group> = client.get("$baseUrl/groups").body()

    suspend fun upsertGroup(baseUrl: String, group: Group): HttpResponse {
        return client.post("$baseUrl/groups") {
            contentType(ContentType.Application.Json)
            setBody(group)
        }
    }

    suspend fun deleteGroup(baseUrl: String, id: Long): HttpResponse {
        return client.delete("$baseUrl/groups") {
            parameter("id", id)
        }
    }

    // --- Light Management ---

    suspend fun getLightsByGroup(baseUrl: String, groupId: Long): List<Light> =
        client.get("$baseUrl/lights") {
            parameter("groupId", groupId)
        }.body()

    suspend fun upsertLight(baseUrl: String, light: Light): HttpResponse {
        return client.post("$baseUrl/lights") {
            contentType(ContentType.Application.Json)
            setBody(light)
        }
    }

    suspend fun deleteLight(baseUrl: String, uid: String, groupId: Long): HttpResponse {
        return client.delete("$baseUrl/lights") {
            parameter("uid", uid)
            parameter("groupId", groupId)
        }
    }

    // --- Switch Management ---

    suspend fun getSwitchesByGroup(baseUrl: String, groupId: Long): List<Switch> =
        client.get("$baseUrl/switches") {
            parameter("groupId", groupId)
        }.body()

    suspend fun upsertSwitch(baseUrl: String, switch: Switch): HttpResponse {
        return client.post("$baseUrl/switches") {
            contentType(ContentType.Application.Json)
            setBody(switch)
        }
    }

    suspend fun deleteSwitch(baseUrl: String, uid: String): HttpResponse {
        return client.delete("$baseUrl/switches") {
            parameter("uid", uid)
        }
    }

    suspend fun checkSwitch(baseUrl: String, uid: String): SwitchCheckResponse =
        client.get("$baseUrl/switch_attribution/check") {
            parameter("uid", uid)
        }.body()

    // --- Switch Attribution ---

    suspend fun toggleAttributionMode(baseUrl: String, enabled: Boolean): HttpResponse {
        return client.post("$baseUrl/switch_attribution/mode") {
            contentType(ContentType.Application.Json)
            setBody(mapOf("enabled" to enabled))
        }
    }

    suspend fun getAttributionData(baseUrl: String): AttributionDataResponse =
        client.get("$baseUrl/switch_attribution/data").body()
}