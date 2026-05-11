package com.zelgius.lightcontroller.data.hue

import io.ktor.client.HttpClient
import io.ktor.client.call.body
import io.ktor.client.request.get
import io.ktor.client.request.headers
import org.koin.core.annotation.Named
import org.koin.core.annotation.Singleton

@Singleton
class HueLightService(
    @Named("Proxy") private val client: HttpClient,
) {
    suspend fun getLight(bridgeIp: String, appKey: String, lightId: String): LightResource? {
        val baseUrl = "https://$bridgeIp/clip/v2/resource/light"
        val response: HueResponse<LightResource> = client.get("$baseUrl/$lightId") {
            headers {
                append("hue-application-key", appKey)
            }
        }.body()
        return response.data.firstOrNull()
    }

    suspend fun getLights(bridgeIp: String, appKey: String): List<LightResource> {
        val baseUrl = "https://$bridgeIp/clip/v2/resource/light"
        val response: HueResponse<LightResource> = client.get(baseUrl) {
            headers {
                append("hue-application-key", appKey)
            }
        }.body()
        return response.data
    }
}