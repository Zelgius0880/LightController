package com.zelgius.lightcontroller.domain.repository.web

import kotlinx.serialization.Serializable

@Serializable
data class Group(
    val id: Long,
    val name: String,
    val brightness: Float,
    val x: Float,
    val y: Float,
    val mirek: Int? = null,
)

@Serializable
data class Light(
    val uid: String,
    val name: String,
    val state: StateMode,
    val type: LightType,
    val brightness: Float,
    val x: Float,
    val y: Float,
    val mirek: Int? = null,
    val groupId: Long,
): WebItem

@Serializable
enum class LightType {
    HUE
}

@Serializable
enum class StateMode {
    TOGGLE, ON, OFF;


}

@Serializable
data class Switch(
    val uid: String,
    val name: String,
    val groupId: Long
): WebItem

@Serializable
data class StatusResponse(
    val authenticated: Boolean? = null,
    val username: String? = null,
    val error: String? = null,
    val totalBytes: Int? = null,
    val usedBytes: Int? = null,
    val netatmo: NetatmoStatus? = null,
    val fsTotal: Int? = null,
    val fsUsed: Int? = null,
    val firmware: String? = null,
    val heapTotal: Int? = null,
    val heapFree: Int? = null
)

@Serializable
data class NetatmoStatus(
    val authenticated: Boolean,
    val expires_in: Int,
    val creation_timestamp: Int,
    val valid: Boolean
)

@Serializable
data class ErrorResponse(val error: String)

@Serializable
data class SwitchCheckResponse(
    val exists: Boolean = false,
    val id: Int? = null,
    val name: String? = null,
)

@Serializable
data class AttributionDataResponse(
    val enabled: Boolean,
    val last_data: String? = null
)

sealed interface WebItem